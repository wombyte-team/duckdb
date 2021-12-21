#include "duckdb/function/scalar/date_functions.hpp"
#include "duckdb/common/case_insensitive_map.hpp"
#include "duckdb/common/enums/date_part_specifier.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/types/date.hpp"
#include "duckdb/common/types/timestamp.hpp"
#include "duckdb/common/vector_operations/vector_operations.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/execution/expression_executor.hpp"
#include "duckdb/function/scalar/nested_functions.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "duckdb/storage/statistics/numeric_statistics.hpp"

namespace duckdb {

bool TryGetDatePartSpecifier(const string &specifier_p, DatePartSpecifier &result) {
	auto specifier = StringUtil::Lower(specifier_p);
	if (specifier == "year" || specifier == "y" || specifier == "years") {
		result = DatePartSpecifier::YEAR;
	} else if (specifier == "month" || specifier == "mon" || specifier == "months" || specifier == "mons") {
		result = DatePartSpecifier::MONTH;
	} else if (specifier == "day" || specifier == "days" || specifier == "d" || specifier == "dayofmonth") {
		result = DatePartSpecifier::DAY;
	} else if (specifier == "decade" || specifier == "decades") {
		result = DatePartSpecifier::DECADE;
	} else if (specifier == "century" || specifier == "centuries") {
		result = DatePartSpecifier::CENTURY;
	} else if (specifier == "millennium" || specifier == "millennia") {
		result = DatePartSpecifier::MILLENNIUM;
	} else if (specifier == "microseconds" || specifier == "microsecond") {
		result = DatePartSpecifier::MICROSECONDS;
	} else if (specifier == "milliseconds" || specifier == "millisecond" || specifier == "ms" || specifier == "msec" ||
	           specifier == "msecs") {
		result = DatePartSpecifier::MILLISECONDS;
	} else if (specifier == "second" || specifier == "seconds" || specifier == "s") {
		result = DatePartSpecifier::SECOND;
	} else if (specifier == "minute" || specifier == "minutes" || specifier == "m") {
		result = DatePartSpecifier::MINUTE;
	} else if (specifier == "hour" || specifier == "hours" || specifier == "h") {
		result = DatePartSpecifier::HOUR;
	} else if (specifier == "epoch") {
		// seconds since 1970-01-01
		result = DatePartSpecifier::EPOCH;
	} else if (specifier == "dow" || specifier == "dayofweek" || specifier == "weekday") {
		// day of the week (Sunday = 0, Saturday = 6)
		result = DatePartSpecifier::DOW;
	} else if (specifier == "isodow") {
		// isodow (Monday = 1, Sunday = 7)
		result = DatePartSpecifier::ISODOW;
	} else if (specifier == "week" || specifier == "weeks" || specifier == "w" || specifier == "weekofyear") {
		// week number
		result = DatePartSpecifier::WEEK;
	} else if (specifier == "doy" || specifier == "dayofyear") {
		// day of the year (1-365/366)
		result = DatePartSpecifier::DOY;
	} else if (specifier == "quarter" || specifier == "quarters") {
		// quarter of the year (1-4)
		result = DatePartSpecifier::QUARTER;
	} else if (specifier == "yearweek") {
		// Combined year and week YYYYWW
		result = DatePartSpecifier::YEARWEEK;
	} else if (specifier == "era") {
		result = DatePartSpecifier::ERA;
	} else if (specifier == "offset") {
		result = DatePartSpecifier::OFFSET;
	} else {
		return false;
	}
	return true;
}

DatePartSpecifier GetDatePartSpecifier(const string &specifier) {
	DatePartSpecifier result;
	if (!TryGetDatePartSpecifier(specifier, result)) {
		throw ConversionException("extract specifier \"%s\" not recognized", specifier);
	}
	return result;
}

template <class T>
static void LastYearFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	int32_t last_year = 0;
	UnaryExecutor::Execute<T, int64_t>(args.data[0], result, args.size(),
	                                   [&](T input) { return Date::ExtractYear(input, &last_year); });
}

template <class T, class OP>
static unique_ptr<BaseStatistics> PropagateDatePartStatistics(vector<unique_ptr<BaseStatistics>> &child_stats) {
	// we can only propagate complex date part stats if the child has stats
	if (!child_stats[0]) {
		return nullptr;
	}
	auto &nstats = (NumericStatistics &)*child_stats[0];
	if (nstats.min.is_null || nstats.max.is_null) {
		return nullptr;
	}
	// run the operator on both the min and the max, this gives us the [min, max] bound
	auto min = nstats.min.GetValueUnsafe<T>();
	auto max = nstats.max.GetValueUnsafe<T>();
	if (min > max) {
		return nullptr;
	}
	auto min_part = OP::template Operation<T, int64_t>(min);
	auto max_part = OP::template Operation<T, int64_t>(max);
	auto result = make_unique<NumericStatistics>(LogicalType::BIGINT, Value::BIGINT(min_part), Value::BIGINT(max_part));
	if (child_stats[0]->validity_stats) {
		result->validity_stats = child_stats[0]->validity_stats->Copy();
	}
	return move(result);
}

template <int64_t MIN, int64_t MAX>
static unique_ptr<BaseStatistics> PropagateSimpleDatePartStatistics(vector<unique_ptr<BaseStatistics>> &child_stats) {
	// we can always propagate simple date part statistics
	// since the min and max can never exceed these bounds
	auto result = make_unique<NumericStatistics>(LogicalType::BIGINT, Value::BIGINT(MIN), Value::BIGINT(MAX));
	if (!child_stats[0]) {
		// if there are no child stats, we don't know
		result->validity_stats = make_unique<ValidityStatistics>(true);
	} else if (child_stats[0]->validity_stats) {
		result->validity_stats = child_stats[0]->validity_stats->Copy();
	}
	return move(result);
}

struct DatePart {
	struct YearOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return Date::ExtractYear(input);
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateDatePartStatistics<T, YearOperator>(child_stats);
		}
	};

	struct MonthOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return Date::ExtractMonth(input);
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			// min/max of month operator is [1, 12]
			return PropagateSimpleDatePartStatistics<1, 12>(child_stats);
		}
	};

	struct DayOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return Date::ExtractDay(input);
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			// min/max of day operator is [1, 31]
			return PropagateSimpleDatePartStatistics<1, 31>(child_stats);
		}
	};

	struct DecadeOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return YearOperator::Operation<TA, TR>(input) / 10;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateDatePartStatistics<T, DecadeOperator>(child_stats);
		}
	};

	struct CenturyOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return ((YearOperator::Operation<TA, TR>(input) - 1) / 100) + 1;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateDatePartStatistics<T, CenturyOperator>(child_stats);
		}
	};

	struct MilleniumOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return ((YearOperator::Operation<TA, TR>(input) - 1) / 1000) + 1;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateDatePartStatistics<T, MilleniumOperator>(child_stats);
		}
	};

	struct QuarterOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return (Date::ExtractMonth(input) - 1) / Interval::MONTHS_PER_QUARTER + 1;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			// min/max of quarter operator is [1, 4]
			return PropagateSimpleDatePartStatistics<1, 4>(child_stats);
		}
	};

	struct DayOfWeekOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			// day of the week (Sunday = 0, Saturday = 6)
			// turn sunday into 0 by doing mod 7
			return Date::ExtractISODayOfTheWeek(input) % 7;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<0, 6>(child_stats);
		}
	};

	struct ISODayOfWeekOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			// isodow (Monday = 1, Sunday = 7)
			return Date::ExtractISODayOfTheWeek(input);
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<1, 7>(child_stats);
		}
	};

	struct DayOfYearOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return Date::ExtractDayOfTheYear(input);
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<1, 366>(child_stats);
		}
	};

	struct WeekOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return Date::ExtractISOWeekNumber(input);
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<1, 54>(child_stats);
		}
	};

	struct YearWeekOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return YearOperator::Operation<TA, TR>(input) * 100 + WeekOperator::Operation<TA, TR>(input);
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateDatePartStatistics<T, YearWeekOperator>(child_stats);
		}
	};

	struct MicrosecondsOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return 0;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<0, 60000000>(child_stats);
		}
	};

	struct MillisecondsOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return 0;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<0, 60000>(child_stats);
		}
	};

	struct SecondsOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return 0;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<0, 60>(child_stats);
		}
	};

	struct MinutesOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return 0;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<0, 60>(child_stats);
		}
	};

	struct HoursOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return 0;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<0, 24>(child_stats);
		}
	};

	struct EpochOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return Date::Epoch(input);
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateDatePartStatistics<T, EpochOperator>(child_stats);
		}
	};

	struct EraOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			return Date::ExtractYear(input) > 0 ? 1 : 0;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<0, 1>(child_stats);
		}
	};

	struct OffsetOperator {
		template <class TA, class TR>
		static inline TR Operation(TA input) {
			// Regular timestamps are UTC.
			return 0;
		}

		template <class T>
		static unique_ptr<BaseStatistics> PropagateStatistics(ClientContext &context, BoundFunctionExpression &expr,
		                                                      FunctionData *bind_data,
		                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
			return PropagateSimpleDatePartStatistics<0, 0>(child_stats);
		}
	};

	struct StructOperator {
		template <class TA, class TR>
		static inline void Operation(TR *part_values, const TA &input) {
			int32_t yyyy;
			int32_t mm;
			int32_t dd;
			Date::Convert(input, yyyy, mm, dd);

			part_values[int(DatePartSpecifier::YEAR)] = yyyy;
			part_values[int(DatePartSpecifier::MONTH)] = mm;
			part_values[int(DatePartSpecifier::DAY)] = dd;
			part_values[int(DatePartSpecifier::DECADE)] = yyyy / 10;
			part_values[int(DatePartSpecifier::CENTURY)] = (yyyy - 1) / 100 + 1;
			part_values[int(DatePartSpecifier::MILLENNIUM)] = (yyyy - 1) / 1000 + 1;
			part_values[int(DatePartSpecifier::EPOCH)] = Date::Epoch(input);

			const auto isodow = Date::ExtractISODayOfTheWeek(input);
			const auto ww = isodow % 7;
			part_values[int(DatePartSpecifier::DOW)] = ww;
			part_values[int(DatePartSpecifier::ISODOW)] = isodow;
			part_values[int(DatePartSpecifier::WEEK)] = Date::ExtractISOWeekNumber(input);
			part_values[int(DatePartSpecifier::QUARTER)] = (mm - 1) / Interval::MONTHS_PER_QUARTER + 1;
			part_values[int(DatePartSpecifier::DOY)] = Date::ExtractDayOfTheYear(input);
			part_values[int(DatePartSpecifier::YEARWEEK)] = yyyy * 100 + ww;

			part_values[int(DatePartSpecifier::ERA)] = yyyy > 0 ? 1 : 0;
			part_values[int(DatePartSpecifier::OFFSET)] = 0;
		}
	};
};

template <>
int64_t DatePart::YearOperator::Operation(timestamp_t input) {
	return YearOperator::Operation<date_t, int64_t>(Timestamp::GetDate(input));
}

template <>
int64_t DatePart::YearOperator::Operation(interval_t input) {
	return input.months / Interval::MONTHS_PER_YEAR;
}

template <>
int64_t DatePart::YearOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"year\" not recognized");
}

template <>
int64_t DatePart::MonthOperator::Operation(timestamp_t input) {
	return MonthOperator::Operation<date_t, int64_t>(Timestamp::GetDate(input));
}

template <>
int64_t DatePart::MonthOperator::Operation(interval_t input) {
	return input.months % Interval::MONTHS_PER_YEAR;
}

template <>
int64_t DatePart::MonthOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"month\" not recognized");
}

template <>
int64_t DatePart::DayOperator::Operation(timestamp_t input) {
	return DayOperator::Operation<date_t, int64_t>(Timestamp::GetDate(input));
}

template <>
int64_t DatePart::DayOperator::Operation(interval_t input) {
	return input.days;
}

template <>
int64_t DatePart::DayOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"day\" not recognized");
}

template <>
int64_t DatePart::DecadeOperator::Operation(interval_t input) {
	return input.months / Interval::MONTHS_PER_DECADE;
}

template <>
int64_t DatePart::DecadeOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"decade\" not recognized");
}

template <>
int64_t DatePart::CenturyOperator::Operation(interval_t input) {
	return input.months / Interval::MONTHS_PER_CENTURY;
}

template <>
int64_t DatePart::CenturyOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"century\" not recognized");
}

template <>
int64_t DatePart::MilleniumOperator::Operation(interval_t input) {
	return input.months / Interval::MONTHS_PER_MILLENIUM;
}

template <>
int64_t DatePart::MilleniumOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"millennium\" not recognized");
}

template <>
int64_t DatePart::QuarterOperator::Operation(timestamp_t input) {
	return QuarterOperator::Operation<date_t, int64_t>(Timestamp::GetDate(input));
}

template <>
int64_t DatePart::QuarterOperator::Operation(interval_t input) {
	return MonthOperator::Operation<interval_t, int64_t>(input) / Interval::MONTHS_PER_QUARTER + 1;
}

template <>
int64_t DatePart::QuarterOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"quarter\" not recognized");
}

template <>
int64_t DatePart::DayOfWeekOperator::Operation(timestamp_t input) {
	return DayOfWeekOperator::Operation<date_t, int64_t>(Timestamp::GetDate(input));
}

template <>
int64_t DatePart::DayOfWeekOperator::Operation(interval_t input) {
	throw NotImplementedException("interval units \"dow\" not recognized");
}

template <>
int64_t DatePart::DayOfWeekOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"dow\" not recognized");
}

template <>
int64_t DatePart::ISODayOfWeekOperator::Operation(timestamp_t input) {
	return ISODayOfWeekOperator::Operation<date_t, int64_t>(Timestamp::GetDate(input));
}

template <>
int64_t DatePart::ISODayOfWeekOperator::Operation(interval_t input) {
	throw NotImplementedException("interval units \"isodow\" not recognized");
}

template <>
int64_t DatePart::ISODayOfWeekOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"isodow\" not recognized");
}

template <>
int64_t DatePart::DayOfYearOperator::Operation(timestamp_t input) {
	return DayOfYearOperator::Operation<date_t, int64_t>(Timestamp::GetDate(input));
}

template <>
int64_t DatePart::DayOfYearOperator::Operation(interval_t input) {
	throw NotImplementedException("interval units \"doy\" not recognized");
}

template <>
int64_t DatePart::DayOfYearOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"doy\" not recognized");
}

template <>
int64_t DatePart::WeekOperator::Operation(timestamp_t input) {
	return WeekOperator::Operation<date_t, int64_t>(Timestamp::GetDate(input));
}

template <>
int64_t DatePart::WeekOperator::Operation(interval_t input) {
	throw NotImplementedException("interval units \"week\" not recognized");
}

template <>
int64_t DatePart::WeekOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"week\" not recognized");
}

template <>
int64_t DatePart::YearWeekOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"yearweek\" not recognized");
}

template <>
int64_t DatePart::MicrosecondsOperator::Operation(timestamp_t input) {
	auto time = Timestamp::GetTime(input);
	// remove everything but the second & microsecond part
	return time.micros % Interval::MICROS_PER_MINUTE;
}

template <>
int64_t DatePart::MicrosecondsOperator::Operation(interval_t input) {
	// remove everything but the second & microsecond part
	return input.micros % Interval::MICROS_PER_MINUTE;
}

template <>
int64_t DatePart::MicrosecondsOperator::Operation(dtime_t input) {
	// remove everything but the second & microsecond part
	return input.micros % Interval::MICROS_PER_MINUTE;
}

template <>
int64_t DatePart::MillisecondsOperator::Operation(timestamp_t input) {
	return MicrosecondsOperator::Operation<timestamp_t, int64_t>(input) / Interval::MICROS_PER_MSEC;
}

template <>
int64_t DatePart::MillisecondsOperator::Operation(interval_t input) {
	return MicrosecondsOperator::Operation<interval_t, int64_t>(input) / Interval::MICROS_PER_MSEC;
}

template <>
int64_t DatePart::MillisecondsOperator::Operation(dtime_t input) {
	return MicrosecondsOperator::Operation<dtime_t, int64_t>(input) / Interval::MICROS_PER_MSEC;
}

template <>
int64_t DatePart::SecondsOperator::Operation(timestamp_t input) {
	return MicrosecondsOperator::Operation<timestamp_t, int64_t>(input) / Interval::MICROS_PER_SEC;
}

template <>
int64_t DatePart::SecondsOperator::Operation(interval_t input) {
	return MicrosecondsOperator::Operation<interval_t, int64_t>(input) / Interval::MICROS_PER_SEC;
}

template <>
int64_t DatePart::SecondsOperator::Operation(dtime_t input) {
	return MicrosecondsOperator::Operation<dtime_t, int64_t>(input) / Interval::MICROS_PER_SEC;
}

template <>
int64_t DatePart::MinutesOperator::Operation(timestamp_t input) {
	auto time = Timestamp::GetTime(input);
	// remove the hour part, and truncate to minutes
	return (time.micros % Interval::MICROS_PER_HOUR) / Interval::MICROS_PER_MINUTE;
}

template <>
int64_t DatePart::MinutesOperator::Operation(interval_t input) {
	// remove the hour part, and truncate to minutes
	return (input.micros % Interval::MICROS_PER_HOUR) / Interval::MICROS_PER_MINUTE;
}

template <>
int64_t DatePart::MinutesOperator::Operation(dtime_t input) {
	// remove the hour part, and truncate to minutes
	return (input.micros % Interval::MICROS_PER_HOUR) / Interval::MICROS_PER_MINUTE;
}

template <>
int64_t DatePart::HoursOperator::Operation(timestamp_t input) {
	return Timestamp::GetTime(input).micros / Interval::MICROS_PER_HOUR;
}

template <>
int64_t DatePart::HoursOperator::Operation(interval_t input) {
	return input.micros / Interval::MICROS_PER_HOUR;
}

template <>
int64_t DatePart::HoursOperator::Operation(dtime_t input) {
	return input.micros / Interval::MICROS_PER_HOUR;
}

template <>
int64_t DatePart::EpochOperator::Operation(timestamp_t input) {
	return Timestamp::GetEpochSeconds(input);
}

template <>
int64_t DatePart::EpochOperator::Operation(interval_t input) {
	int64_t interval_years = input.months / Interval::MONTHS_PER_YEAR;
	int64_t interval_days;
	interval_days = Interval::DAYS_PER_YEAR * interval_years;
	interval_days += Interval::DAYS_PER_MONTH * (input.months % Interval::MONTHS_PER_YEAR);
	interval_days += input.days;
	int64_t interval_epoch;
	interval_epoch = interval_days * Interval::SECS_PER_DAY;
	// we add 0.25 days per year to sort of account for leap days
	interval_epoch += interval_years * (Interval::SECS_PER_DAY / 4);
	interval_epoch += input.micros / Interval::MICROS_PER_SEC;
	return interval_epoch;
}

template <>
int64_t DatePart::EpochOperator::Operation(dtime_t input) {
	return input.micros / Interval::MICROS_PER_SEC;
}

template <>
unique_ptr<BaseStatistics>
DatePart::EpochOperator::PropagateStatistics<dtime_t>(ClientContext &context, BoundFunctionExpression &expr,
                                                      FunctionData *bind_data,
                                                      vector<unique_ptr<BaseStatistics>> &child_stats) {
	// time seconds range over a single day
	return PropagateSimpleDatePartStatistics<0, 86400>(child_stats);
}

template <>
int64_t DatePart::EraOperator::Operation(timestamp_t input) {
	return EraOperator::Operation<date_t, int64_t>(Timestamp::GetDate(input));
}

template <>
int64_t DatePart::EraOperator::Operation(interval_t input) {
	throw NotImplementedException("interval units \"era\" not recognized");
}

template <>
int64_t DatePart::EraOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"era\" not recognized");
}

template <>
int64_t DatePart::OffsetOperator::Operation(timestamp_t input) {
	return 0;
}

template <>
int64_t DatePart::OffsetOperator::Operation(interval_t input) {
	throw NotImplementedException("\"interval\" units \"offset\" not recognized");
}

template <>
int64_t DatePart::OffsetOperator::Operation(dtime_t input) {
	throw NotImplementedException("\"time\" units \"offset\" not recognized");
}

template <>
void DatePart::StructOperator::Operation(int64_t *part_values, const dtime_t &input) {
	const auto micros = MicrosecondsOperator::Operation<dtime_t, int64_t>(input);
	part_values[int(DatePartSpecifier::MICROSECONDS)] = micros;
	part_values[int(DatePartSpecifier::MILLISECONDS)] = micros / Interval::MICROS_PER_MSEC;
	part_values[int(DatePartSpecifier::SECOND)] = micros / Interval::MICROS_PER_SEC;
	part_values[int(DatePartSpecifier::MINUTE)] = MinutesOperator::Operation<dtime_t, int64_t>(input);
	part_values[int(DatePartSpecifier::HOUR)] = HoursOperator::Operation<dtime_t, int64_t>(input);
}

template <>
void DatePart::StructOperator::Operation(int64_t *part_values, const timestamp_t &input) {
	date_t d;
	dtime_t t;
	Timestamp::Convert(input, d, t);
	Operation(part_values, d);
	Operation(part_values, t);
}

template <>
void DatePart::StructOperator::Operation(int64_t *part_values, const interval_t &input) {
	const auto mm = input.months % Interval::MONTHS_PER_YEAR;
	part_values[int(DatePartSpecifier::YEAR)] = input.months / Interval::MONTHS_PER_YEAR;
	part_values[int(DatePartSpecifier::MONTH)] = mm;
	part_values[int(DatePartSpecifier::DAY)] = input.days;
	part_values[int(DatePartSpecifier::DECADE)] = input.months / Interval::MONTHS_PER_DECADE;
	part_values[int(DatePartSpecifier::CENTURY)] = input.months / Interval::MONTHS_PER_CENTURY;
	part_values[int(DatePartSpecifier::MILLENNIUM)] = input.months / Interval::MONTHS_PER_MILLENIUM;
	part_values[int(DatePartSpecifier::QUARTER)] = mm / Interval::MONTHS_PER_QUARTER + 1;

	const auto micros = MicrosecondsOperator::Operation<interval_t, int64_t>(input);
	part_values[int(DatePartSpecifier::MICROSECONDS)] = micros;
	part_values[int(DatePartSpecifier::MILLISECONDS)] = micros / Interval::MICROS_PER_MSEC;
	part_values[int(DatePartSpecifier::SECOND)] = micros / Interval::MICROS_PER_SEC;
	part_values[int(DatePartSpecifier::MINUTE)] = MinutesOperator::Operation<interval_t, int64_t>(input);
	part_values[int(DatePartSpecifier::HOUR)] = HoursOperator::Operation<interval_t, int64_t>(input);

	part_values[int(DatePartSpecifier::EPOCH)] = EpochOperator::Operation<interval_t, int64_t>(input);
}

template <typename T>
static int64_t ExtractElement(DatePartSpecifier type, T element) {
	switch (type) {
	case DatePartSpecifier::YEAR:
		return DatePart::YearOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::MONTH:
		return DatePart::MonthOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::DAY:
		return DatePart::DayOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::DECADE:
		return DatePart::DecadeOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::CENTURY:
		return DatePart::CenturyOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::MILLENNIUM:
		return DatePart::MilleniumOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::QUARTER:
		return DatePart::QuarterOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::DOW:
		return DatePart::DayOfWeekOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::ISODOW:
		return DatePart::ISODayOfWeekOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::DOY:
		return DatePart::DayOfYearOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::WEEK:
		return DatePart::WeekOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::YEARWEEK:
		return DatePart::YearWeekOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::EPOCH:
		return DatePart::EpochOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::MICROSECONDS:
		return DatePart::MicrosecondsOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::MILLISECONDS:
		return DatePart::MillisecondsOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::SECOND:
		return DatePart::SecondsOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::MINUTE:
		return DatePart::MinutesOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::HOUR:
		return DatePart::HoursOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::ERA:
		return DatePart::EraOperator::template Operation<T, int64_t>(element);
	case DatePartSpecifier::OFFSET:
		return DatePart::OffsetOperator::template Operation<T, int64_t>(element);
	default:
		throw NotImplementedException("Specifier type not implemented for DATEPART");
	}
}

struct DatePartBinaryOperator {
	template <class TA, class TB, class TR>
	static inline TR Operation(TA specifier, TB date) {
		return ExtractElement<TB>(GetDatePartSpecifier(specifier.GetString()), date);
	}
};

template <typename T>
static void DatePartFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	D_ASSERT(args.ColumnCount() == 2);
	auto &part_arg = args.data[0];
	auto &date_arg = args.data[1];

	BinaryExecutor::ExecuteStandard<string_t, T, int64_t, DatePartBinaryOperator>(part_arg, date_arg, result,
	                                                                              args.size());
}

void AddGenericDatePartOperator(BuiltinFunctions &set, const string &name, scalar_function_t date_func,
                                scalar_function_t ts_func, scalar_function_t interval_func,
                                function_statistics_t date_stats, function_statistics_t ts_stats) {
	ScalarFunctionSet operator_set(name);
	operator_set.AddFunction(
	    ScalarFunction({LogicalType::DATE}, LogicalType::BIGINT, move(date_func), false, nullptr, nullptr, date_stats));
	operator_set.AddFunction(ScalarFunction({LogicalType::TIMESTAMP}, LogicalType::BIGINT, move(ts_func), false,
	                                        nullptr, nullptr, ts_stats));
	operator_set.AddFunction(ScalarFunction({LogicalType::INTERVAL}, LogicalType::BIGINT, move(interval_func)));
	set.AddFunction(operator_set);
}

template <class OP>
static void AddDatePartOperator(BuiltinFunctions &set, string name) {
	AddGenericDatePartOperator(set, name, ScalarFunction::UnaryFunction<date_t, int64_t, OP>,
	                           ScalarFunction::UnaryFunction<timestamp_t, int64_t, OP>,
	                           ScalarFunction::UnaryFunction<interval_t, int64_t, OP>,
	                           OP::template PropagateStatistics<date_t>, OP::template PropagateStatistics<timestamp_t>);
}

void AddGenericTimePartOperator(BuiltinFunctions &set, const string &name, scalar_function_t date_func,
                                scalar_function_t ts_func, scalar_function_t interval_func, scalar_function_t time_func,
                                function_statistics_t date_stats, function_statistics_t ts_stats,
                                function_statistics_t time_stats) {
	ScalarFunctionSet operator_set(name);
	operator_set.AddFunction(
	    ScalarFunction({LogicalType::DATE}, LogicalType::BIGINT, move(date_func), false, nullptr, nullptr, date_stats));
	operator_set.AddFunction(ScalarFunction({LogicalType::TIMESTAMP}, LogicalType::BIGINT, move(ts_func), false,
	                                        nullptr, nullptr, ts_stats));
	operator_set.AddFunction(ScalarFunction({LogicalType::INTERVAL}, LogicalType::BIGINT, move(interval_func)));
	operator_set.AddFunction(
	    ScalarFunction({LogicalType::TIME}, LogicalType::BIGINT, move(time_func), false, nullptr, nullptr, time_stats));
	set.AddFunction(operator_set);
}

template <class OP>
static void AddTimePartOperator(BuiltinFunctions &set, string name) {
	AddGenericTimePartOperator(
	    set, name, ScalarFunction::UnaryFunction<date_t, int64_t, OP>,
	    ScalarFunction::UnaryFunction<timestamp_t, int64_t, OP>, ScalarFunction::UnaryFunction<interval_t, int64_t, OP>,
	    ScalarFunction::UnaryFunction<dtime_t, int64_t, OP>, OP::template PropagateStatistics<date_t>,
	    OP::template PropagateStatistics<timestamp_t>, OP::template PropagateStatistics<dtime_t>);
}

struct LastDayOperator {
	template <class TA, class TR>
	static inline TR Operation(TA input) {
		int32_t yyyy, mm, dd;
		Date::Convert(input, yyyy, mm, dd);
		yyyy += (mm / 12);
		mm %= 12;
		++mm;
		return Date::FromDate(yyyy, mm, 1) - 1;
	}
};

template <>
date_t LastDayOperator::Operation(timestamp_t input) {
	return LastDayOperator::Operation<date_t, date_t>(Timestamp::GetDate(input));
}

struct MonthNameOperator {
	template <class TA, class TR>
	static inline TR Operation(TA input) {
		return Date::MONTH_NAMES[DatePart::MonthOperator::Operation<TA, int64_t>(input) - 1];
	}
};

struct DayNameOperator {
	template <class TA, class TR>
	static inline TR Operation(TA input) {
		return Date::DAY_NAMES[DatePart::DayOfWeekOperator::Operation<TA, int64_t>(input)];
	}
};

struct StructDatePart {
	using part_codes_t = vector<DatePartSpecifier>;

	struct BindData : public VariableReturnBindData {
		part_codes_t part_codes;

		explicit BindData(const LogicalType &stype, const part_codes_t &part_codes_p)
		    : VariableReturnBindData(stype), part_codes(part_codes_p) {
		}

		unique_ptr<FunctionData> Copy() override {
			return make_unique<BindData>(stype, part_codes);
		}
	};

	static unique_ptr<FunctionData> Bind(ClientContext &context, ScalarFunction &bound_function,
	                                     vector<unique_ptr<Expression>> &arguments) {
		// collect names and deconflict, construct return type
		if (!arguments[0]->IsFoldable()) {
			throw BinderException("%s can only take constant lists of part names", bound_function.name);
		}

		case_insensitive_set_t name_collision_set;
		child_list_t<LogicalType> struct_children;
		part_codes_t part_codes;

		Value parts_list = ExpressionExecutor::EvaluateScalar(*arguments[0]);
		if (parts_list.type().id() == LogicalTypeId::LIST) {
			if (parts_list.list_value.empty()) {
				throw BinderException("%s requires non-empty lists of part names", bound_function.name);
			}
			for (const auto &part_value : parts_list.list_value) {
				if (part_value.is_null) {
					throw BinderException("NULL struct entry name in %s", bound_function.name);
				}
				const auto part_name = part_value.ToString();
				const auto part_code = GetDatePartSpecifier(part_name);
				if (name_collision_set.find(part_name) != name_collision_set.end()) {
					throw BinderException("Duplicate struct entry name \"%s\" in %s", part_name, bound_function.name);
				}
				name_collision_set.insert(part_name);
				part_codes.emplace_back(part_code);
				struct_children.emplace_back(make_pair(part_name, LogicalType::BIGINT));
			}
		} else {
			throw BinderException("%s can only take constant lists of part names", bound_function.name);
		}

		arguments.erase(arguments.begin());
		bound_function.arguments.erase(bound_function.arguments.begin());
		bound_function.return_type = LogicalType::STRUCT(move(struct_children));
		return make_unique<BindData>(bound_function.return_type, part_codes);
	}

	template <typename INPUT_TYPE>
	static void Function(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &func_expr = (BoundFunctionExpression &)state.expr;
		auto &info = (BindData &)*func_expr.bind_info;
		D_ASSERT(args.ColumnCount() == 1);

		const auto count = args.size();
		Vector &input = args.data[0];
		vector<int64_t> part_values(int(DatePartSpecifier::OFFSET) + 1, 0);

		if (input.GetVectorType() == VectorType::CONSTANT_VECTOR) {
			result.SetVectorType(VectorType::CONSTANT_VECTOR);

			if (ConstantVector::IsNull(input)) {
				ConstantVector::SetNull(result, true);
			} else {
				ConstantVector::SetNull(result, false);
				auto tdata = ConstantVector::GetData<INPUT_TYPE>(input);
				DatePart::StructOperator::Operation(part_values.data(), tdata[0]);
				auto &child_entries = StructVector::GetEntries(result);
				for (size_t col = 0; col < child_entries.size(); ++col) {
					auto &child_entry = child_entries[col];
					ConstantVector::SetNull(*child_entry, false);
					auto pdata = ConstantVector::GetData<int64_t>(*child_entry);
					pdata[0] = part_values[int(info.part_codes[col])];
				}
			}
		} else {
			VectorData rdata;
			input.Orrify(count, rdata);

			const auto &arg_valid = rdata.validity;
			auto tdata = (const INPUT_TYPE *)rdata.data;

			result.SetVectorType(VectorType::FLAT_VECTOR);
			auto &child_entries = StructVector::GetEntries(result);
			for (auto &child_entry : child_entries) {
				child_entry->SetVectorType(VectorType::FLAT_VECTOR);
			}

			auto &res_valid = FlatVector::Validity(result);
			for (idx_t i = 0; i < count; ++i) {
				const auto idx = rdata.sel->get_index(i);
				if (arg_valid.RowIsValid(idx)) {
					res_valid.SetValid(idx);
					DatePart::StructOperator::Operation(part_values.data(), tdata[idx]);
					for (size_t col = 0; col < child_entries.size(); ++col) {
						auto &child_entry = child_entries[col];
						FlatVector::Validity(*child_entry).SetValid(idx);
						auto pdata = FlatVector::GetData<int64_t>(*child_entry);
						pdata[idx] = part_values[int(info.part_codes[col])];
					}
				} else {
					res_valid.SetInvalid(idx);
					for (auto &child_entry : child_entries) {
						FlatVector::Validity(*child_entry).SetInvalid(idx);
					}
				}
			}
		}

		result.Verify(count);
	}

	template <typename INPUT_TYPE>
	static ScalarFunction GetFunction(const LogicalType &temporal_type) {
		auto part_type = LogicalType::LIST(LogicalType::VARCHAR);
		auto result_type = LogicalType::STRUCT({});
		return ScalarFunction({part_type, temporal_type}, result_type, Function<INPUT_TYPE>, false, Bind);
	}
};

void DatePartFun::RegisterFunction(BuiltinFunctions &set) {
	// register the individual operators
	AddGenericDatePartOperator(set, "year", LastYearFunction<date_t>, LastYearFunction<timestamp_t>,
	                           ScalarFunction::UnaryFunction<interval_t, int64_t, DatePart::YearOperator>,
	                           DatePart::YearOperator::PropagateStatistics<date_t>,
	                           DatePart::YearOperator::PropagateStatistics<timestamp_t>);
	AddDatePartOperator<DatePart::MonthOperator>(set, "month");
	AddDatePartOperator<DatePart::DayOperator>(set, "day");
	AddDatePartOperator<DatePart::DecadeOperator>(set, "decade");
	AddDatePartOperator<DatePart::CenturyOperator>(set, "century");
	AddDatePartOperator<DatePart::MilleniumOperator>(set, "millennium");
	AddDatePartOperator<DatePart::QuarterOperator>(set, "quarter");
	AddDatePartOperator<DatePart::DayOfWeekOperator>(set, "dayofweek");
	AddDatePartOperator<DatePart::ISODayOfWeekOperator>(set, "isodow");
	AddDatePartOperator<DatePart::DayOfYearOperator>(set, "dayofyear");
	AddDatePartOperator<DatePart::WeekOperator>(set, "week");
	AddDatePartOperator<DatePart::EraOperator>(set, "era");
	AddTimePartOperator<DatePart::EpochOperator>(set, "epoch");
	AddTimePartOperator<DatePart::MicrosecondsOperator>(set, "microsecond");
	AddTimePartOperator<DatePart::MillisecondsOperator>(set, "millisecond");
	AddTimePartOperator<DatePart::SecondsOperator>(set, "second");
	AddTimePartOperator<DatePart::MinutesOperator>(set, "minute");
	AddTimePartOperator<DatePart::HoursOperator>(set, "hour");

	//  register combinations
	AddDatePartOperator<DatePart::YearWeekOperator>(set, "yearweek");

	//  register various aliases
	AddDatePartOperator<DatePart::DayOperator>(set, "dayofmonth");
	AddDatePartOperator<DatePart::DayOfWeekOperator>(set, "weekday");
	AddDatePartOperator<DatePart::WeekOperator>(set, "weekofyear"); //  Note that WeekOperator is ISO-8601, not US

	//  register the last_day function
	ScalarFunctionSet last_day("last_day");
	last_day.AddFunction(ScalarFunction({LogicalType::DATE}, LogicalType::DATE,
	                                    ScalarFunction::UnaryFunction<date_t, date_t, LastDayOperator>));
	last_day.AddFunction(ScalarFunction({LogicalType::TIMESTAMP}, LogicalType::DATE,
	                                    ScalarFunction::UnaryFunction<timestamp_t, date_t, LastDayOperator>));
	set.AddFunction(last_day);

	//  register the monthname function
	ScalarFunctionSet monthname("monthname");
	monthname.AddFunction(ScalarFunction({LogicalType::DATE}, LogicalType::VARCHAR,
	                                     ScalarFunction::UnaryFunction<date_t, string_t, MonthNameOperator>));
	monthname.AddFunction(ScalarFunction({LogicalType::TIMESTAMP}, LogicalType::VARCHAR,
	                                     ScalarFunction::UnaryFunction<timestamp_t, string_t, MonthNameOperator>));
	set.AddFunction(monthname);

	//  register the dayname function
	ScalarFunctionSet dayname("dayname");
	dayname.AddFunction(ScalarFunction({LogicalType::DATE}, LogicalType::VARCHAR,
	                                   ScalarFunction::UnaryFunction<date_t, string_t, DayNameOperator>));
	dayname.AddFunction(ScalarFunction({LogicalType::TIMESTAMP}, LogicalType::VARCHAR,
	                                   ScalarFunction::UnaryFunction<timestamp_t, string_t, DayNameOperator>));
	set.AddFunction(dayname);

	// finally the actual date_part function
	ScalarFunctionSet date_part("date_part");
	date_part.AddFunction(
	    ScalarFunction({LogicalType::VARCHAR, LogicalType::DATE}, LogicalType::BIGINT, DatePartFunction<date_t>));
	date_part.AddFunction(ScalarFunction({LogicalType::VARCHAR, LogicalType::TIMESTAMP}, LogicalType::BIGINT,
	                                     DatePartFunction<timestamp_t>));
	date_part.AddFunction(
	    ScalarFunction({LogicalType::VARCHAR, LogicalType::TIME}, LogicalType::BIGINT, DatePartFunction<dtime_t>));
	date_part.AddFunction(ScalarFunction({LogicalType::VARCHAR, LogicalType::INTERVAL}, LogicalType::BIGINT,
	                                     DatePartFunction<interval_t>));

	// struct variants
	date_part.AddFunction(StructDatePart::GetFunction<date_t>(LogicalType::DATE));
	date_part.AddFunction(StructDatePart::GetFunction<timestamp_t>(LogicalType::TIMESTAMP));
	date_part.AddFunction(StructDatePart::GetFunction<dtime_t>(LogicalType::TIME));
	date_part.AddFunction(StructDatePart::GetFunction<interval_t>(LogicalType::INTERVAL));

	set.AddFunction(date_part);
	date_part.name = "datepart";
	set.AddFunction(date_part);
}

} // namespace duckdb
