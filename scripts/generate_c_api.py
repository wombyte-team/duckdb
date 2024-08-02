import os
import json
import re
import glob
import copy
from packaging.version import Version
from functools import reduce
from pathlib import Path

###
# DuckDB C API header generation
###
# This script generates the DuckDB C API headers.
#
# The script works by parsing the definition files in `src/include/duckdb/main/capi/header_generation`, which are then
# used to generate the 3 header files:
#
# The main C header. This is what to include when linking with DuckDB through the C API.
DUCKDB_HEADER_OUT_FILE = 'src/include/duckdb.h'
# The header to be included by DuckDB C extensions
DUCKDB_HEADER_EXT_OUT_FILE = 'src/include/duckdb_extension.h'
# An internal header for DuckDB extension C API
DUCKDB_HEADER_EXT_INTERNAL_OUT_FILE = 'src/include/duckdb/main/capi/extension_api.hpp'

# Whether the script allows functions with parameters without a comment explaining them
ALLOW_UNCOMMENTED_PARAMS = True

DUCKDB_EXT_API_PTR_NAME = 'duckdb_ext_api'
DUCKDB_EXT_API_STRUCT_NAME = 'duckdb_ext_api_v0'

# Define the extension struct
EXT_API_DEFINITION_PATTERN = 'src/include/duckdb/main/capi/header_generation/apis/v0/*/*.json'
EXT_API_EXCLUSION_FILE = 'src/include/duckdb/main/capi/header_generation/apis/v0/exclusion_list.json'

# The JSON files that define all available CAPI functions
CAPI_FUNCTION_DEFINITION_FILES = 'src/include/duckdb/main/capi/header_generation/functions/**/*.json'

# The original order of the function groups in the duckdb.h files. We maintain this for easier PR reviews.
# TODO: replace this with alphabetical ordering in a separate PR
ORIGINAL_FUNCTION_GROUP_ORDER = [
    'open_connect',
    'configuration',
    'query_execution',
    'result_functions',
    'safe_fetch_functions',
    'helpers',
    'date_time_timestamp_helpers',
    'hugeint_helpers',
    'unsigned_hugeint_helpers',
    'decimal_helpers',
    'prepared_statements',
    'bind_values_to_prepared_statements',
    'execute_prepared_statements',
    'extract_statements',
    'pending_result_interface',
    'value_interface',
    'logical_type_interface',
    'data_chunk_interface',
    'vector_interface',
    'validity_mask_functions',
    'scalar_functions',
    'aggregate_functions',
    'table_functions',
    'table_function_bind',
    'table_function_init',
    'table_function',
    'replacement_scans',
    'profiling_info',
    'appender',
    'table_description',
    'arrow_interface',
    'threading_information',
    'streaming_result_interface',
]

# The file that forms the base for the header generation
BASE_HEADER_TEMPLATE = 'src/include/duckdb/main/capi/header_generation/header_base.hpp'
# The comment marking where this script will inject its contents
BASE_HEADER_CONTENT_MARK = '// DUCKDB_FUNCTIONS_ARE_GENERATED_HERE\n'


def HEADER(file):
    return f'''//===----------------------------------------------------------------------===//
//
//                         DuckDB
//
// {file}
//
//
//===----------------------------------------------------------------------===//
//
// !!!!!!!
// WARNING: this file is autogenerated by scripts/generate_c_api.py, manual changes will be overwritten
// !!!!!!!

'''


DUCKDB_H_HEADER = HEADER('duckdb.h')
DUCKDB_EXT_H_HEADER = HEADER('duckdb_extension.h')
DUCKDB_EXT_INTERNAL_H_HEADER = HEADER('extension_api.hpp')


# Loads the template for the header files to be generated
def fetch_header_template_main():
    # Read the base header
    with open(BASE_HEADER_TEMPLATE, 'r') as f:
        result = f.read()

    # Trim the base header
    header_mark = '// DUCKDB_START_OF_HEADER\n'
    if header_mark not in result:
        print(f"Could not find the header start mark: {header_mark}")
        exit(1)

    return result[result.find(header_mark) + len(header_mark) :]


def fetch_header_template_ext():
    return """#pragma once
        
#include "duckdb.h"

//===--------------------------------------------------------------------===//
// Functions
//===--------------------------------------------------------------------===//

// DUCKDB_FUNCTIONS_ARE_GENERATED_HERE
"""


# Parse the CAPI_FUNCTION_DEFINITION_FILES to get the full list of functions
def parse_capi_function_definitions():
    # Collect all functions
    function_files = glob.glob(CAPI_FUNCTION_DEFINITION_FILES, recursive=True)

    function_groups = []
    function_map = {}

    # Read functions
    for file in function_files:
        with open(file, 'r') as f:
            try:
                json_data = json.loads(f.read())
            except json.decoder.JSONDecodeError as err:
                print(f"Invalid JSON found in {file}: {err}")
                exit(1)

            function_groups.append(json_data)
            for function in json_data['entries']:
                if function['name'] in function_map:
                    print(f"Duplicate symbol found when parsing C API file {file}: {function['name']}")
                    exit(1)

                function['group'] = json_data['group']
                if 'deprecated' in json_data:
                    function['group_deprecated'] = json_data['deprecated']

                function_map[function['name']] = function

    # Reorder to match original order: purely intended to keep the PR review sane
    function_groups_ordered = []

    if len(function_groups) != len(ORIGINAL_FUNCTION_GROUP_ORDER):
        print(
            "The list used to match the original order of function groups in the original the duckdb.h file does not match the new one. Did you add a new function group? please also add it to ORIGINAL_FUNCTION_GROUP_ORDER for now."
        )

    for order_group in ORIGINAL_FUNCTION_GROUP_ORDER:
        curr_group = next(group for group in function_groups if group['group'] == order_group)
        function_groups.remove(curr_group)
        function_groups_ordered.append(curr_group)

    return (function_groups_ordered, function_map)


# Read extension API
def parse_ext_api_definitions():
    api_definitions = {}
    versions = []
    for file in list(glob.glob(EXT_API_DEFINITION_PATTERN)):
        with open(file, 'r') as f:
            try:
                obj = json.loads(f.read())
                api_definitions[obj['version']] = obj
                versions.append(obj['version'])
                if Path(file).stem != obj['version']:
                    print(f"\nMismatch between filename and version in file for {file}")
                    exit(1)
            except json.decoder.JSONDecodeError as err:
                print(f"\nInvalid JSON found in {file}: {err}")
                exit(1)

    versions.sort(key=Version)
    return [api_definitions[x] for x in versions]

def parse_exclusion_list(function_map):
    exclusion_set = set()
    with open(EXT_API_EXCLUSION_FILE, 'r') as f:
        try:
            data = json.loads(f.read())
        except json.decoder.JSONDecodeError as err:
            print(f"\nInvalid JSON found in {EXT_API_EXCLUSION_FILE}: {err}")
            exit(1)

        for group in data['exclusion_list']:
            for entry in group['entries']:
                if entry not in function_map:
                    print(f"\nInvalid item found in exclusion list: {entry}. This entry does not occur in the API!")
                    exit(1)
                exclusion_set.add(entry)
    return exclusion_set


# Creates the comment that accompanies describing a C api function
def create_function_comment(function_obj):
    result = ''

    function_name = function_obj['name']
    # Construct comment
    if 'comment' in function_obj:
        comment = function_obj['comment']
        result += '/*!\n'
        result += comment['description']
        # result += '\n\n'
        if 'params' in function_obj:
            for param in function_obj['params']:
                param_name = param['name']
                if not 'param_comments' in comment:
                    if not ALLOW_UNCOMMENTED_PARAMS:
                        print(comment)
                        print(f'\nMissing param comments for function {function_name}')
                        exit(1)
                    continue
                if param['name'] in comment['param_comments']:
                    param_comment = comment['param_comments'][param['name']]
                    result += f'* @param {param_name} {param_comment}\n'
                elif not ALLOW_UNCOMMENTED_PARAMS:
                    print(f'\nUncommented parameter found: {param_name} of function {function_name}')
                    exit(1)
        if 'return_value' in comment:
            comment_return_value = comment['return_value']
            result += f'* @return {comment_return_value}\n'
        result += '*/\n'
    return result


# Creates the function declaration for the regular C header file
def create_function_declaration(function_obj):
    result = ''
    function_name = function_obj['name']
    function_return_type = function_obj['return_type']

    # Construct function declaration
    result += f'DUCKDB_API {function_return_type}'
    if result[-1] != '*':
        result += ' '
    result += f'{function_name}('

    if 'params' in function_obj:
        if len(function_obj['params']) > 0:
            for param in function_obj['params']:
                param_type = param['type']
                param_name = param['name']
                result += f'{param_type}'
                if result[-1] != '*':
                    result += ' '
                result += f'{param_name}, '
            result = result[:-2]  # Trailing comma
    result += ');\n'

    return result


# Creates the function declaration for extension api struct
def create_struct_member(function_obj):
    result = ''

    function_name = function_obj['name']
    function_return_type = function_obj['return_type']
    result += f'    {function_return_type} (*{function_name})('
    if 'params' in function_obj:
        if len(function_obj['params']) > 0:
            for param in function_obj['params']:
                param_type = param['type']
                param_name = param['name']
                result += f'{param_type} {param_name},'
            result = result[:-1]  # Trailing comma
    result += ');'

    return result


# Creates the function declaration for extension api struct
def create_function_typedef(function_obj):
    function_name = function_obj['name']
    return f'#define {function_name} {DUCKDB_EXT_API_PTR_NAME}->{function_name}\n'


def to_camel_case(snake_str):
    return " ".join(x.capitalize() for x in snake_str.lower().split("_"))


def parse_semver(version):
    if version[0] != 'v':
        print(f"\nVersion string {version} does not start with a v")
        exit(1)

    versions = version[1:].split(".")

    if len(versions) != 3:
        print(f"\nVersion string {version} is invalid, only vx.y.z is supported")
        exit(1)

    return int(versions[0]), int(versions[1]), int(versions[2])


def create_version_defines(version):
    major, minor, patch = parse_semver(EXT_API_VERSION)
    version_string = f'v{major}.{minor}.{patch}'

    result = ""
    result += f"#define DUCKDB_EXTENSION_API_VERSION_MAJOR {major}\n"
    result += f"#define DUCKDB_EXTENSION_API_VERSION_MINOR {minor}\n"
    result += f"#define DUCKDB_EXTENSION_API_VERSION_PATCH {patch}\n"

    return result


# Create duckdb.h
def create_duckdb_h(ext_api_version):
    function_declarations_finished = ''

    for curr_group in FUNCTION_GROUPS:
        function_declarations_finished += f'''//===--------------------------------------------------------------------===//
// {to_camel_case(curr_group['group'])}
//===--------------------------------------------------------------------===//\n\n'''

        if 'description' in curr_group:
            function_declarations_finished += curr_group['description'] + '\n'

        if 'deprecated' in curr_group and curr_group['deprecated']:
            function_declarations_finished += f'#ifndef DUCKDB_API_NO_DEPRECATED\n'

        for function in curr_group['entries']:
            if 'deprecated' in function and function['deprecated']:
                function_declarations_finished += '#ifndef DUCKDB_API_NO_DEPRECATED\n'

            function_declarations_finished += create_function_comment(function)
            function_declarations_finished += create_function_declaration(function)

            if 'deprecated' in function and function['deprecated']:
                function_declarations_finished += '#endif\n'

            function_declarations_finished += '\n'

        if 'deprecated' in curr_group and curr_group['deprecated']:
            function_declarations_finished += '#endif\n'

    header_template = fetch_header_template_main()
    duckdb_h = DUCKDB_H_HEADER + header_template.replace(BASE_HEADER_CONTENT_MARK, function_declarations_finished)
    with open(DUCKDB_HEADER_OUT_FILE, 'w+') as f:
        f.write(duckdb_h)


def write_struct_member_definitions(version_entries, initialize=False):
    result = ""
    for function_name in version_entries:
        function_lookup = FUNCTION_MAP[function_name]
        function_lookup_name = function_lookup['name']

        if initialize:
            result += f'        result.{function_lookup_name} = {function_lookup_name};\n'
        else:
            result += f'        result.{function_lookup_name} = nullptr;\n'

    return result


def create_extension_api_struct(ext_api_version, with_create_method=False, validate_exclusion_list=True):
    functions_in_struct = set()

    # Generate the struct
    extension_struct_finished = 'typedef struct {\n'
    for api_version_entry in EXT_API_DEFINITIONS:
        version = api_version_entry['version']
        extension_struct_finished += f'    // Version {version}\n'
        for function_name in api_version_entry['entries']:
            function_lookup = FUNCTION_MAP[function_name]
            functions_in_struct.add(function_lookup['name'])
            extension_struct_finished += create_struct_member(function_lookup)
            extension_struct_finished += '\n'
    extension_struct_finished += '} ' + f'{DUCKDB_EXT_API_STRUCT_NAME};\n\n'

    if validate_exclusion_list:

        # Check for missing entries
        missing_entries = []
        for group in FUNCTION_GROUPS:
            for function in group['entries']:
                if function['name'] not in functions_in_struct and function['name'] not in EXT_API_EXCLUSION_SET:
                    missing_entries.append(function['name'])
        if missing_entries:
            print(
                "\nExclusion list validation failed! This means a C API function has been defined but not added to the API struct nor the exclusion list"
            )
            print(f" * Missing functions: {missing_entries}")
            exit(1)
        # Check for entries both in the API definition and the exclusion list
        double_entries = []
        for api_version_entry in EXT_API_DEFINITIONS:
            for function_name in api_version_entry['entries']:
                if function_name in EXT_API_EXCLUSION_SET:
                    double_entries.append(function_name)
        if double_entries:
            print(
                "\nExclusion list is invalid, there are entries in the extension api that are also in the exclusion list!"
            )
            print(f" * Missing functions: {double_entries}")
            exit(1)

    if with_create_method:
        extension_struct_finished += "inline duckdb_ext_api_v0 CreateApi(idx_t minor_version, idx_t patch_version) {\n"
        extension_struct_finished += "    duckdb_ext_api_v0 result;\n"
        for api_version_entry in EXT_API_DEFINITIONS:

            if len(api_version_entry['entries']) == 0:
                continue

            major, minor, patch = parse_semver(api_version_entry['version'])
            if minor == 0:
                extension_struct_finished += f'if (patch_version >= {patch})' + '{'
            else:
                extension_struct_finished += f'if (minor_version >= {minor} && patch_version >= {patch})' + '{'
            extension_struct_finished += write_struct_member_definitions(api_version_entry['entries'], initialize=True)
            extension_struct_finished += '} else {'
            extension_struct_finished += write_struct_member_definitions(api_version_entry['entries'], initialize=False)
            extension_struct_finished += '}'

        extension_struct_finished += "    return result;\n"
        extension_struct_finished += "}\n\n"

    extension_struct_finished += create_version_defines(ext_api_version)
    extension_struct_finished += "\n"

    return extension_struct_finished


# Create duckdb_extension_api.h
def create_duckdb_ext_h(ext_api_version, ext_struct_api_function_set):

    # Generate the typedefs
    typedefs = ""
    for group in FUNCTION_GROUPS:
        functions_to_add = []
        for function in group['entries']:
            if function['name'] not in ext_struct_api_function_set:
                continue
            functions_to_add.append(function)

        if functions_to_add:
            group_name = group['group']
            typedefs += f'//! {group_name}\n'
            for fun_to_add in functions_to_add:
                typedefs += create_function_typedef(fun_to_add)

            typedefs += '\n'

    extension_header_body = create_extension_api_struct(ext_api_version) + '\n\n' + typedefs

    # Add the Macros
    extension_header_body += """
//! Internal MACROS
#ifdef __cplusplus
#define DUCKDB_EXTENSION_EXTERN_C_GUARD_OPEN  extern "C" {
#define DUCKDB_EXTENSION_EXTERN_C_GUARD_CLOSE }
#else
#define DUCKDB_EXTENSION_EXTERN_C_GUARD_OPEN
#define DUCKDB_EXTENSION_EXTERN_C_GUARD_CLOSE
#endif

#define DUCKDB_EXTENSION_GLUE_HELPER(x, y) x##y
#define DUCKDB_EXTENSION_GLUE(x, y) DUCKDB_EXTENSION_GLUE_HELPER(x, y)
#define DUCKDB_EXTENSION_STR_HELPER(x) #x
#define DUCKDB_EXTENSION_STR(x) DUCKDB_EXTENSION_STR_HELPER(x)

// This goes in the c/c++ file containing the entrypoint (handle
#define DUCKDB_EXTENSION_GLOBAL const duckdb_ext_api_v0 *duckdb_ext_api = 0;
// Initializes the C Extension API: First thing to call in the extension entrypoint
#define DUCKDB_EXTENSION_API_INIT(info, access, minimum_api_version)\
	duckdb_ext_api = (duckdb_ext_api_v0 *)access->get_api(info, minimum_api_version);\
	if (!duckdb_ext_api) {\
		return;\
	};

// Place in global scope of any C/C++ file that needs to access the extension API
#define DUCKDB_EXTENSION_EXTERN extern const duckdb_ext_api_v0 *duckdb_ext_api;

//===--------------------------------------------------------------------===//
// ENTRYPOINT Macros
//===--------------------------------------------------------------------===//
// Note: the DUCKDB_EXTENSION_ENTRYPOINT macro requires the DUCKDB_EXTENSION_NAME and DUCKDB_EXTENSION_CAPI_VERSION to be set

#ifdef DUCKDB_EXTENSION_NAME
#ifdef DUCKDB_EXTENSION_CAPI_VERSION

// Main entrypoint: opens (and closes) a connection automatically for the extension to register its functionality through 
#define DUCKDB_EXTENSION_ENTRYPOINT\
	DUCKDB_EXTENSION_GLOBAL static void DUCKDB_EXTENSION_GLUE(DUCKDB_EXTENSION_NAME,_init_c_api_internal)(duckdb_connection connection, duckdb_extension_info info, duckdb_extension_access *access);\
	    DUCKDB_EXTENSION_EXTERN_C_GUARD_OPEN\
	    DUCKDB_EXTENSION_API void DUCKDB_EXTENSION_GLUE(DUCKDB_EXTENSION_NAME,_init_c_api)(\
	    duckdb_extension_info info, duckdb_extension_access *access) {\
		DUCKDB_EXTENSION_API_INIT(info, access, DUCKDB_EXTENSION_STR(DUCKDB_EXTENSION_CAPI_VERSION));\
		duckdb_database *db = access->get_database(info);\
		duckdb_connection conn;\
		if (duckdb_connect(*db, &conn) == DuckDBError) {\
			access->set_error(info, "Failed to open connection to database");\
			return;\
		}\
		DUCKDB_EXTENSION_GLUE(DUCKDB_EXTENSION_NAME,_init_c_api_internal)(conn, info, access);\
		duckdb_disconnect(&conn);\
	}\
	DUCKDB_EXTENSION_EXTERN_C_GUARD_CLOSE static void DUCKDB_EXTENSION_GLUE(DUCKDB_EXTENSION_NAME,_init_c_api_internal)

// Custom entrypoint: just forwards the info and access
#define DUCKDB_EXTENSION_ENTRYPOINT_CUSTOM\
	DUCKDB_EXTENSION_GLOBAL static void DUCKDB_EXTENSION_GLUE(DUCKDB_EXTENSION_NAME,_init_c_api_internal)(\
	    duckdb_extension_info info, duckdb_extension_access *access);\
	    DUCKDB_EXTENSION_EXTERN_C_GUARD_OPEN\
	    DUCKDB_EXTENSION_API void DUCKDB_EXTENSION_GLUE(DUCKDB_EXTENSION_NAME,_init_c_api)(\
	    duckdb_extension_info info, duckdb_extension_access *access) {\
		DUCKDB_EXTENSION_API_INIT(info, access, DUCKDB_EXTENSION_STR(DUCKDB_EXTENSION_CAPI_VERSION));\
		DUCKDB_EXTENSION_GLUE(DUCKDB_EXTENSION_NAME,_init_c_api_internal)(info, access);\
	}\
	DUCKDB_EXTENSION_EXTERN_C_GUARD_CLOSE static void DUCKDB_EXTENSION_GLUE(DUCKDB_EXTENSION_NAME,_init_c_api_internal)
#endif
#endif
    """

    header_template = fetch_header_template_ext()
    duckdb_ext_h = DUCKDB_EXT_H_HEADER + header_template.replace(BASE_HEADER_CONTENT_MARK, extension_header_body)
    with open(DUCKDB_HEADER_EXT_OUT_FILE, 'w+') as f:
        f.write(duckdb_ext_h)


# Create duckdb_extension_internal.hpp
def create_duckdb_ext_internal_h(ext_api_version):
    extension_header_body = create_extension_api_struct(ext_api_version, with_create_method=True)
    header_template = fetch_header_template_ext()
    duckdb_ext_h = DUCKDB_EXT_INTERNAL_H_HEADER + header_template.replace(
        BASE_HEADER_CONTENT_MARK, extension_header_body
    )
    with open(DUCKDB_HEADER_EXT_INTERNAL_OUT_FILE, 'w+') as f:
        f.write(duckdb_ext_h)


def get_extension_api_version():
    versions = []

    for version_entry in EXT_API_DEFINITIONS:
        versions.append(version_entry['version'])

    versions_copy = copy.deepcopy(versions)

    versions.sort(key=Version)

    if versions != versions_copy:
        print("\nFailed to parse extension api: versions are not ordered correctly")
        exit(1)

    return versions[-1]

def create_struct_function_set():
    result = set()
    for api in EXT_API_DEFINITIONS:
        for entry in api['entries']:
            result.add(entry)
    return result

# TODO make this code less spaghetti
if __name__ == "__main__":
    EXT_API_DEFINITIONS = parse_ext_api_definitions()
    EXT_API_SET = create_struct_function_set()
    EXT_API_VERSION = get_extension_api_version()
    FUNCTION_GROUPS, FUNCTION_MAP = parse_capi_function_definitions()
    FUNCTION_MAP_SIZE = len(FUNCTION_MAP)

    API_STRUCT_FUNCTION_COUNT = reduce(lambda x, y: len(x['entries']) + len(y['entries']), EXT_API_DEFINITIONS)
    EXT_API_EXCLUSION_SET = parse_exclusion_list(FUNCTION_MAP)
    EXT_API_EXCLUSION_SET_SIZE = len(EXT_API_EXCLUSION_SET)

    print("Information")
    print(f" * Current Extension C API Version: {EXT_API_VERSION}")
    print(f" * Total functions: {FUNCTION_MAP_SIZE}")
    print(f" * Functions in C API struct: {API_STRUCT_FUNCTION_COUNT}")
    print(f" * Functions in C API but excluded from struct: {EXT_API_EXCLUSION_SET_SIZE}")

    print()

    print("Generating headers")
    print(f" * {DUCKDB_HEADER_OUT_FILE}")
    create_duckdb_h(EXT_API_VERSION)
    print(f" * {DUCKDB_HEADER_EXT_OUT_FILE}")
    create_duckdb_ext_h(DUCKDB_HEADER_EXT_INTERNAL_OUT_FILE, EXT_API_SET)
    print(f" * {DUCKDB_HEADER_EXT_INTERNAL_OUT_FILE}")

    print()

    os.system(f"python3 scripts/format.py {DUCKDB_HEADER_OUT_FILE} --fix --noconfirm")
    os.system(f"python3 scripts/format.py {DUCKDB_HEADER_EXT_OUT_FILE} --fix --noconfirm")
    os.system(f"python3 scripts/format.py {DUCKDB_HEADER_EXT_INTERNAL_OUT_FILE} --fix --noconfirm")

    print()
    print("C API headers generated successfully!")
