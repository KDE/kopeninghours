%{
/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "openinghours_p.h"
#include "openinghoursparser_p.h"
#include "openinghoursscanner_p.h"
#include "logging.h"

using namespace KOpeningHours;

void yyerror(YYLTYPE *loc, OpeningHoursPrivate *parser, yyscan_t scanner, char const* msg)
{
    Q_UNUSED(scanner);
    qCDebug(Log) << "PARSER ERROR:" << msg << "at position" << loc->first_column;
    parser->m_error = OpeningHours::SyntaxError;
}

static void initSelectors(Selectors &sels)
{
    sels.timeSelector = nullptr;
    sels.weekdaySelector = nullptr;
    sels.weekSelector = nullptr;
    sels.monthdaySelector = nullptr;
    sels.yearSelector = nullptr;
    sels.wideRangeSelectorComment.str = nullptr;
    sels.wideRangeSelectorComment.len = 0;
    sels.seen_24_7 = false;
    sels.colonAfterWideRangeSelector = false;
}

static void applySelectors(const Selectors &sels, Rule *rule)
{
    rule->m_timeSelector.reset(sels.timeSelector);
    rule->m_weekdaySelector.reset(sels.weekdaySelector);
    rule->m_weekSelector.reset(sels.weekSelector);
    rule->m_monthdaySelector.reset(sels.monthdaySelector);
    rule->m_yearSelector.reset(sels.yearSelector);
    rule->m_seen_24_7 = sels.seen_24_7;
    rule->m_colonAfterWideRangeSelector = sels.colonAfterWideRangeSelector;
    rule->m_wideRangeSelectorComment = QString::fromUtf8(sels.wideRangeSelectorComment.str, sels.wideRangeSelectorComment.len);
}

static bool extendMonthdaySelector(MonthdayRange *monthdaySelector, int day)
{
    const auto prevSelector = lastSelector(monthdaySelector);
    if (prevSelector->begin.year == prevSelector->end.year
     && prevSelector->begin.month == prevSelector->end.month
     && prevSelector->begin.day < day
     && prevSelector->end.day < day)
    {
        auto sel = new MonthdayRange;
        sel->begin = sel->end = prevSelector->end;
        sel->begin.day = sel->end.day = day;
        appendSelector(prevSelector, sel);
        return true;
    }
    return false;
}

%}

%code requires {

#include "openinghours_p.h"
#include "interval.h"

using namespace KOpeningHours;

struct StringRef {
    const char *str;
    int len;
};

struct Selectors {
    Timespan *timeSelector;
    WeekdayRange *weekdaySelector;
    Week *weekSelector;
    MonthdayRange *monthdaySelector;
    YearRange *yearSelector;
    StringRef wideRangeSelectorComment;
    bool seen_24_7;
    bool colonAfterWideRangeSelector;
};

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

}

%define api.pure
%define parse.error verbose

%locations
%lex-param { yyscan_t scanner }
%parse-param { KOpeningHours::OpeningHoursPrivate *parser }
%parse-param { yyscan_t scanner }

%glr-parser
%expect 18
// (1) is for "T_YEAR T_COMMA T_YEAR T_MONTH", which is syntactically invalid anyway
// (2) and (3) are for overlaps between weekday/holiday sequences and holiday-on-weekday selectors, which are also invalid
%expect-rr 3

%union {
    int num;
    StringRef strRef;
    State state;
    Rule *rule;
    Time time;
    Selectors selectors;
    Timespan *timespan;
    WeekdayRange *weekdayRange;
    Week *week;
    Date date;
    MonthdayRange *monthdayRange;
    DateOffset dateOffset;
    YearRange *yearRange;
}

%token T_NORMAL_RULE_SEPARATOR
%token T_ADDITIONAL_RULE_SEPARATOR
%token T_FALLBACK_SEPARATOR

%token <state> T_STATE

%token T_24_7
%token <num> T_YEAR

%token T_PLUS
%token T_MINUS
%token T_SLASH
%token T_COLON
%token T_COMMA

%token T_ALT_TIME_SEP
%token T_ALT_TIME_SEP_OR_SUFFIX
%token <num> T_ALT_TIME_AM
%token <num> T_ALT_TIME_PM

%token T_ALT_RANGE_SEP

%token <time> T_EVENT

%token T_LBRACKET
%token T_RBRACKET
%token T_LPAREN
%token T_RPAREN

%token T_PH
%token T_SH

%token T_KEYWORD_DAY
%token T_KEYWORD_WEEK

%token T_EASTER

%token <num> T_WEEKDAY
%token <num> T_MONTH
%token <num> T_INTEGER

%token <strRef> T_COMMENT

%token T_INVALID

%type <rule> Rule
%type <selectors> SelectorSequence
%type <selectors> WideRangeSelector
%type <selectors> SmallRangeSelector
%type <selectors> TimeSelector
%type <selectors> WeekdaySelector
%type <selectors> WeekSelector
%type <selectors> MonthdaySelector
%type <selectors> YearSelector
%type <selectors> YearSelectorCombined
%type <timespan> Timespan
%type <time> Time
%type <time> VariableTime
%type <time> ExtendedHourMinute
%type <weekdayRange> HolidayAndWeekdaySequence
%type <weekdayRange> HolidayAndWeekday
%type <weekdayRange> HolidayOrWeekdaySequence
%type <weekdayRange> WeekdaySequence
%type <weekdayRange> WeekdayRange
%type <weekdayRange> HolidaySequence
%type <weekdayRange> Holiday
%type <num> NthSequence
%type <num> NthEntry
%type <num> DayOffset
%type <dateOffset> DateOffset
%type <week> Week
%type <date> DateFrom
%type <date> DateTo
%type <date> VariableDate
%type <monthdayRange> MonthdayRange
%type <monthdayRange> MonthdayRangeAdditional
%type <dateOffset> AltMonthdayOffset
%type <yearRange> YearRange
%type <yearRange> YearRangeStandalone

%destructor { delete $$; } <rule>
%destructor {
    delete $$.timeSelector;
    delete $$.weekdaySelector;
    delete $$.weekSelector;
    delete $$.monthdaySelector;
    delete $$.yearSelector;
} <selectors>
%destructor { delete $$; } <timespan>
%destructor { delete $$; } <weekdayRange>
%destructor { delete $$; } <week>
%destructor { delete $$; } <monthdayRange>
%destructor { delete $$; } <yearRange>

// resolve SR conflict between the YearSelector and MonthdaySelector on T_YEAR T_MONTH
%nonassoc T_YEAR
%nonassoc T_MONTH

%verbose

%%
// see https://wiki.openstreetmap.org/wiki/Key:opening_hours/specification

Ruleset:
  Rule[R] { parser->addRule($R); }
| Ruleset T_NORMAL_RULE_SEPARATOR Rule[R] { parser->addRule($R); }
| Ruleset T_ADDITIONAL_RULE_SEPARATOR Rule[R] {
    $R->m_ruleType = Rule::AdditionalRule;
    parser->addRule($R);
  }
| Ruleset T_FALLBACK_SEPARATOR Rule[R] {
    $R->m_ruleType = Rule::FallbackRule;
    parser->addRule($R);
  }
| Ruleset T_SLASH error {
    if (!parser->isRecovering()) {
        parser->restartFrom(@3.first_column, Rule::NormalRule);
        parser->m_ruleSeparatorRecovery = true;
        yyerrok;
    } else {
        YYERROR;
    }
  }
| Ruleset error {
    if (!parser->isRecovering()) {
        parser->restartFrom(@2.first_column, Rule::GuessRuleType);
        parser->m_ruleSeparatorRecovery = true;
        yyerrok;
    } else {
        YYERROR;
    }
  }
;

Rule:
  SelectorSequence[S] {
    $$ = new Rule;
    applySelectors($S, $$);
  }
| SelectorSequence[S] T_COMMENT[C] {
    $$ = new Rule;
    $$->setComment($C.str, $C.len);
    applySelectors($S, $$);
  }
| SelectorSequence[S] T_STATE[T] {
    $$ = new Rule;
    $$->setState($T);
    applySelectors($S, $$);
  }
| SelectorSequence[S] T_STATE[T] T_COMMENT[C] {
    $$ = new Rule;
    $$->setComment($C.str, $C.len);
    $$->setState($T);
    applySelectors($S, $$);
  }
| T_COMMENT[C] {
    $$ = new Rule;
    $$->setComment($C.str, $C.len);
  }
| T_STATE[T] {
    $$ = new Rule;
    $$->setState($T);
  }
| T_STATE[T] T_COMMENT[C] {
    $$ = new Rule;
    $$->setComment($C.str, $C.len);
    $$->setState($T);
  }
;

SelectorSequence:
  T_24_7 { initSelectors($$); $$.seen_24_7 = true; }
| SmallRangeSelector[S] { $$ = $S; }
| WideRangeSelector[W] { $$ = $W; }
| WideRangeSelector[W] T_COLON {
    $$ = $W;
    $$.colonAfterWideRangeSelector = true;
  }
| WideRangeSelector[W] SmallRangeSelector[S] {
    $$.timeSelector = $S.timeSelector;
    $$.weekdaySelector = $S.weekdaySelector;
    $$.weekSelector = $W.weekSelector;
  }
| WideRangeSelector[W] T_COLON SmallRangeSelector[S] {
    $$.timeSelector = $S.timeSelector;
    $$.weekdaySelector = $S.weekdaySelector;
    $$.weekSelector = $W.weekSelector;
    $$.colonAfterWideRangeSelector = true;
  }
;

WideRangeSelector:
  YearSelector[Y] { $$ = $Y; }
| MonthdaySelector[M] { $$ = $M; }
| WeekSelector[W] { $$ = $W; }
| YearSelector[Y] MonthdaySelector[M] {
    $$.yearSelector = $Y.yearSelector;
    $$.monthdaySelector = $M.monthdaySelector;
}
| YearSelector[Y] WeekSelector[W] {
    $$.yearSelector = $Y.yearSelector;
    $$.weekSelector = $W.weekSelector;
  }
| MonthdaySelector[M] WeekSelector[W] {
    $$.monthdaySelector = $M.monthdaySelector;
    $$.weekSelector = $W.weekSelector;
  }
| YearSelector[Y] MonthdaySelector[M] WeekSelector[W] {
    $$.yearSelector = $Y.yearSelector;
    $$.monthdaySelector = $M.monthdaySelector;
    $$.weekSelector = $W.weekSelector;
  }
| T_COMMENT[C] T_COLON {
    initSelectors($$);
    $$.wideRangeSelectorComment = $C;
    $$.colonAfterWideRangeSelector = true;
  }
;

SmallRangeSelector:
  TimeSelector[T] { $$ = $T; }
| WeekdaySelector[W] { $$ = $W; }
| WeekdaySelector[W] T_COLON { $$ = $W; }
| WeekdaySelector[W] TimeSelector[T] {
    $$.timeSelector = $T.timeSelector;
    $$.weekdaySelector = $W.weekdaySelector;
  }
| WeekdaySelector[W] T_COLON TimeSelector[T] {
    $$.timeSelector = $T.timeSelector;
    $$.weekdaySelector = $W.weekdaySelector;
  }
;

// Time selector
TimeSelector:
  Timespan[T] {
    initSelectors($$);
    $$.timeSelector = $T;
  }
| TimeSelector[T1] T_COMMA Timespan[T2] {
    $$ = $T1;
    appendSelector($$.timeSelector, $T2);
  }
| TimeSelector[T] T_COMMA error {
    $$ = $T;
    parser->restartFrom(@3.first_column, Rule::AdditionalRule);
    yyerrok;
  }
| TimeSelector[T] T_SLASH Time[E] error { /* wrong use of slash as a timespan separator */
    $$ = $T;
    parser->restartFrom(@E.first_column, Rule::AdditionalRule);
    yyerrok;
  }
;

Timespan:
  Time[T] {
    $$ = new Timespan;
    $$->begin = $$->end = $T;
  }
| Time[T] T_PLUS {
    $$ = new Timespan;
    $$->begin = $$->end = $T;
    $$->openEnd = true;
  }
| Time[T1] RangeSeparator Time[T2] {
    $$ = new Timespan;
    $$->begin = $T1;
    $$->end = $T2;
    if ($$->begin == $$->end) {
        $$->end.hour += 24;
    }
  }
| Time[T1] RangeSeparator Time[T2] T_PLUS {
    $$ = new Timespan;
    $$->begin = $T1;
    $$->end = $T2;
    if ($$->begin == $$->end) {
        $$->end.hour += 24;
    }
    $$->openEnd = true;
  }
| Time[T1] RangeSeparator Time[T2] T_SLASH T_INTEGER[I] {
    $$ = new Timespan;
    $$->begin = $T1;
    $$->end = $T2;
    $$->interval = $I;
  }
| Time[T1] RangeSeparator Time[T2] T_SLASH ExtendedHourMinute[I] {
    $$ = new Timespan;
    $$->begin = $T1;
    $$->end = $T2;
    $$->interval = $I.hour * 60 + $I.minute;
  }
;

Time:
  ExtendedHourMinute[T] { $$ = $T; }
| VariableTime[T] { $$ = $T; }
;

VariableTime:
  T_EVENT[E] { $$ = $E; }
| T_LPAREN T_EVENT[E] T_PLUS ExtendedHourMinute[O] T_RPAREN {
    $$ = $E;
    $$.hour = $O.hour;
    $$.minute = $O.minute;
  }
| T_LPAREN T_EVENT[E] T_MINUS ExtendedHourMinute[O] T_RPAREN {
    $$ = $E;
    $$.hour = -$O.hour;
    $$.minute = -$O.minute;
  }
;

// Weekday selector
WeekdaySelector:
  HolidayOrWeekdaySequence[S] {
    initSelectors($$);
    $$.weekdaySelector = $S;
  }
| HolidayAndWeekdaySequence[S] {
    initSelectors($$);
    $$.weekdaySelector = $S;
  }
;

HolidayAndWeekdaySequence:
  HolidayAndWeekday[HW] { $$ = $HW; }
| HolidayAndWeekdaySequence[HW1] T_COMMA HolidayAndWeekday[HW2] {
    $$ = $HW1;
    appendSelector($$, $HW2);
  }
;

HolidayAndWeekday:
  HolidaySequence[H] WeekdaySequence[W] {
    $$ = new WeekdayRange;
    $$->lhsAndSelector.reset($H);
    $$->rhsAndSelector.reset($W);
  }
| WeekdaySequence[W] HolidaySequence[H] { // wrong order according to the specification
    $$ = new WeekdayRange;
    $$->lhsAndSelector.reset($H);
    $$->rhsAndSelector.reset($W);
  }
;

HolidayOrWeekdaySequence:
  WeekdayRange[W] { $$ = $W; }
| Holiday[H] { $$ = $H; }
| HolidayOrWeekdaySequence[S] T_COMMA WeekdayRange[W] {
    $$ = $S;
    appendSelector($$, $W);
  }
| HolidayOrWeekdaySequence[S] T_COMMA Holiday[H] {
    $$ = $S;
    appendSelector($$, $H);
  }
;

WeekdaySequence:
  WeekdayRange[W] { $$ = $W; }
| WeekdaySequence[S] T_COMMA WeekdayRange[W] {
    $$ = $S;
    appendSelector($$, $W);
  }
;

WeekdayRange:
  T_WEEKDAY[D] {
    $$ = new WeekdayRange;
    $$->beginDay = $D;
    $$->endDay = $D;
  }
| T_WEEKDAY[D1] RangeSeparator T_WEEKDAY[D2] {
    $$ = new WeekdayRange;
    $$->beginDay = $D1;
    $$->endDay = $D2;
  }
| T_WEEKDAY[D] T_LBRACKET NthSequence[N] T_RBRACKET {
    $$ = new WeekdayRange;
    $$->beginDay = $$->endDay = $D;
    $$->nthMask = $N;
  }
| T_WEEKDAY[D] T_LBRACKET NthSequence[N] T_RBRACKET DayOffset[O] {
    $$ = new WeekdayRange;
    $$->beginDay = $$->endDay = $D;
    $$->nthMask = $N;
    $$->offset = $O;
  }
;

HolidaySequence:
  Holiday[H] { $$ = $H; }
| HolidaySequence[S] T_COMMA Holiday[H] { $$ = $S; appendSelector($$, $H); }
;

Holiday:
  T_PH {
    $$ = new WeekdayRange;
    $$->holiday = WeekdayRange::PublicHoliday;
  }
| T_PH DayOffset[O] {
    $$ = new WeekdayRange;
    $$->holiday = WeekdayRange::PublicHoliday;
    $$->offset = $O;
  }
| T_SH {
    $$ = new WeekdayRange;
    $$->holiday = WeekdayRange::SchoolHoliday;
  }
;

NthSequence:
  NthEntry[N] { $$ = $N; }
| NthSequence[N1] T_COMMA NthEntry[N2] { $$ = $N1 | $N2; }

NthEntry:
  T_INTEGER[N] {
    if ($N < 1 || $N > 5) { YYABORT; }
    $$ = (1 << (2 * $N));
  }
| T_INTEGER[N1] T_MINUS T_INTEGER[N2] {
    if ($N1 < 1 || $N1 > 5 || $N2 < 1 || $N2 > 5 || $N2 <= $N1) { YYABORT; }
    $$ = 0;
    for (int i = $N1; i <= $N2; ++i) {
        $$ |= (1 << (2 * i));
    }
  }
| T_MINUS T_INTEGER[N] {
    if ($N < 1 || $N > 5) { YYABORT; }
    $$ = (1 << ((2 * (6 - $N)) - 1));
  }
;

DayOffset:
  T_PLUS T_INTEGER[N] T_KEYWORD_DAY { $$ = $N; }
| T_MINUS T_INTEGER[N] T_KEYWORD_DAY { $$ = -$N; }
;

// Week selector
WeekSelector:
  T_KEYWORD_WEEK Week[W] {
    initSelectors($$);
    $$.weekSelector = $W;
  }
| WeekSelector[W1] T_COMMA Week[W2] {
    $$ = $W1;
    appendSelector($$.weekSelector, $W2);
  }
;

Week:
  T_INTEGER[N] {
    $$ = new Week;
    $$->beginWeek = $$->endWeek = $N;
  }
| T_INTEGER[N1] T_MINUS T_INTEGER[N2] {
    $$ = new Week;
    $$->beginWeek = $N1;
    $$->endWeek = $N2;
  }
| T_INTEGER[N1] T_MINUS T_INTEGER[N2] T_SLASH T_INTEGER[I] {
    $$ = new Week;
    $$->beginWeek = $N1;
    $$->endWeek = $N2;
    $$->interval = $I;
  }
;

// Month selector
// the split between MonthdayRange and MonthdayRangeAdditional is to avoid matching a list of years
// to avoid an ambiguity with YearRange below
MonthdaySelector:
  MonthdayRange[M] {
    initSelectors($$);
    $$.monthdaySelector = $M;
  }
| MonthdaySelector[S] T_COMMA MonthdayRangeAdditional[M] {
    $$ = $S;
    appendSelector($$.monthdaySelector, $M);
  }
| MonthdaySelector[S] T_COMMA T_INTEGER[D] {
    // month day sets, not covered the official grammar but in the
    // description in https://wiki.openstreetmap.org/wiki/Key:opening_hours#Summary_syntax
    $$ = $S;
    if (!extendMonthdaySelector($$.monthdaySelector, $D)) {
        delete $$.monthdaySelector;
        YYABORT;
    }
  }
| MonthdaySelector[S] T_ADDITIONAL_RULE_SEPARATOR T_INTEGER[D] {
    // same as the above, just with the wrong ", " separator
    $$ = $S;
    if (!extendMonthdaySelector($$.monthdaySelector, $D)) {
        delete $$.monthdaySelector;
        YYABORT;
    }
  }
;

MonthdayRange:
  T_YEAR[Y] {
    $$ = new MonthdayRange;
    $$->begin = $$->end = { $Y, 0, 0, Date::FixedDate, { 0, 0, 0 } };
  }
| MonthdayRangeAdditional[M] { $$ = $M; }

MonthdayRangeAdditional:
  T_MONTH[M] {
    $$ = new MonthdayRange;
    $$->begin = $$->end = { 0, $M, 0, Date::FixedDate, { 0, 0, 0 } };
  }
| T_YEAR[Y] T_MONTH[M] {
    $$ = new MonthdayRange;
    $$->begin = $$->end = { $Y, $M, 0, Date::FixedDate, { 0, 0, 0 } };
  }
| T_MONTH[M1] RangeSeparator T_MONTH[M2] {
    $$ = new MonthdayRange;
    $$->begin = { 0, $M1, 0, Date::FixedDate, { 0, 0, 0 } };
    $$->end = { 0, $M2, 0, Date::FixedDate, { 0, 0, 0 } };
  }
| T_YEAR[Y] T_MONTH[M1] RangeSeparator T_MONTH[M2] {
    $$ = new MonthdayRange;
    $$->begin = { $Y, $M1, 0, Date::FixedDate, { 0, 0, 0 } };
    $$->end = { $Y, $M2, 0, Date::FixedDate, { 0, 0, 0 } };
  }
| T_YEAR[Y1] T_MONTH[M1] RangeSeparator T_YEAR[Y2] T_MONTH[M2] {
    $$ = new MonthdayRange;
    $$->begin = { $Y1, $M1, 0, Date::FixedDate, { 0, 0, 0 } };
    $$->end = { $Y2, $M2, 0, Date::FixedDate, { 0, 0, 0 } };
  }
| T_MONTH[M1] AltMonthdayOffset[O1] RangeSeparator T_MONTH[M2] AltMonthdayOffset[O2] {
    $$ = new MonthdayRange;
    $$->begin = { 0, $M1, 0, Date::FixedDate, $O1 };
    $$->end = { 0, $M2, 0, Date::FixedDate, $O2 };
  }
| DateFrom[D] {
    $$ = new MonthdayRange;
    $$->begin = $$->end = $D;
  }
| DateFrom[D] DateOffset[O] {
    $$ = new MonthdayRange;
    $$->begin = $D;
    $$->begin.offset = $O;
    $$->end = $$->begin;
  }
| DateFrom[F] RangeSeparator DateTo[T] {
    $$ = new MonthdayRange;
    $$->begin = $F;
    $$->end = $T;
    if ($$->end.month == 0) { $$->end.month = $$->begin.month; }
  }
| DateFrom[F] DateOffset[OF] RangeSeparator DateTo[T] {
    $$ = new MonthdayRange;
    $$->begin = $F;
    $$->begin.offset = $OF;
    $$->end = $T;
    if ($$->end.month == 0) { $$->end.month = $$->begin.month; }
  }
| DateFrom[F] RangeSeparator DateTo[T] DateOffset[OT] {
    $$ = new MonthdayRange;
    $$->begin = $F;
    $$->end = $T;
    if ($$->end.month == 0) { $$->end.month = $$->begin.month; }
    $$->end.offset = $OT;
  }
| DateFrom[F] RangeSeparator T_MONTH[M] AltMonthdayOffset[O] {
    $$ = new MonthdayRange;
    $$->begin = $F;
    $$->end = { 0, $M, 0, Date::FixedDate, $O };
  }
| DateFrom[F] DateOffset[OF] RangeSeparator DateTo[T] DateOffset[OT] {
    $$ = new MonthdayRange;
    $$->begin = $F;
    $$->begin.offset = $OF;
    $$->end = $T;
    if ($$->end.month == 0) { $$->end.month = $$->begin.month; }
    $$->end.offset = $OT;
  }
;

DateOffset:
  T_PLUS T_WEEKDAY[D] { $$ = { 0, (int8_t)$D, 1 }; }
| T_MINUS T_WEEKDAY[D] { $$ = { 0, (int8_t)$D, -1 }; }
| DayOffset[O] { $$ = { (int16_t)$O, 0, 0 }; }
;

DateFrom:
  T_MONTH[M] T_INTEGER[D] { $$ = { 0, $M, $D, Date::FixedDate, { 0, 0, 0 } }; }
| T_YEAR[Y] T_MONTH[M] T_INTEGER[D] { $$ = { $Y, $M, $D, Date::FixedDate, { 0, 0, 0 } }; }
| VariableDate[D] { $$ = $D; }
| T_YEAR[Y] VariableDate[D] {
    $$ = $D;
    $$.year = $Y;
  }
;

DateTo:
  DateFrom[D] { $$ = $D; }
| T_INTEGER[N] { $$ = { 0, 0, $N, Date::FixedDate, { 0, 0, 0 } }; }
;

VariableDate:
  T_EASTER { $$ = { 0, 0, 0, Date::Easter, { 0, 0, 0 } }; }
;

AltMonthdayOffset:
  T_WEEKDAY[D] T_LBRACKET T_INTEGER[N] T_RBRACKET { $$ = { 0, (int8_t)$D, (int8_t)$N }; }
| T_WEEKDAY[D] T_LBRACKET T_MINUS T_INTEGER[N] T_RBRACKET { $$ = { 0, (int8_t)$D, (int8_t)-$N }; }
| T_WEEKDAY[D] T_LBRACKET T_INTEGER[N] T_RBRACKET DayOffset[O] { $$ = { (int16_t)$O, (int8_t)$D, (int8_t)$N }; }
| T_WEEKDAY[D] T_LBRACKET T_MINUS T_INTEGER[N] T_RBRACKET DayOffset[O] { $$ = { (int16_t)$O, (int8_t)$D, (int8_t)-$N }; }

// Year selector
// the split between the standalone and combined rules here is to avoid matching a single year
// to avoid a practically relevant ambiguity with MonthdayRange
YearSelector:
  YearRangeStandalone[Y] {
    initSelectors($$);
    $$.yearSelector = $Y;
  }
| YearSelectorCombined[S] T_COMMA YearRange[Y] {
    $$ = $S;
    appendSelector($$.yearSelector, $Y);
  }
;
YearSelectorCombined:
  YearRange[Y] {
    initSelectors($$);
    $$.yearSelector = $Y;
  }
| YearSelectorCombined[S] T_COMMA YearRange[Y] {
    $$ = $S;
    appendSelector($$.yearSelector, $Y);
  }
;

YearRange:
  T_YEAR[Y] {
    $$ = new YearRange;
    $$->begin = $$->end = $Y;
  }
| YearRangeStandalone[Y] { $$ = $Y; }
;

YearRangeStandalone:
  T_YEAR[Y1] RangeSeparator T_YEAR[Y2] {
    $$ = new YearRange;
    $$->begin = $Y1;
    $$->end = $Y2;
    if ($$->end < $$->begin) {
        delete $$;
        YYABORT;
    }
  }
| T_YEAR[Y] T_SLASH T_INTEGER[I] {
    $$ = new YearRange;
    $$->begin = $Y;
    $$->interval = $I;
  }
| T_YEAR[Y1] RangeSeparator T_YEAR[Y2] T_SLASH T_INTEGER[I] {
    $$ = new YearRange;
    $$->begin = $Y1;
    $$->end = $Y2;
    if ($$->end < $$->begin) {
        delete $$;
        YYABORT;
    }
    $$->interval = $I;
  }
| T_YEAR[Y] T_PLUS {
    $$ = new YearRange;
    $$->begin = $Y;
  }

// basic building blocks
ExtendedHourMinute:
  T_INTEGER[H] T_COLON T_INTEGER[M] {
    $$ = { Time::NoEvent, $H, $M };
    if (!Time::isValid($$)) { YYABORT; }
  }
| T_INTEGER[H] T_ALT_TIME_SEP T_INTEGER[M] {
    $$ = { Time::NoEvent, $H, $M };
    if (!Time::isValid($$)) { YYABORT; }
  }
| T_INTEGER[H] T_COLON T_INTEGER[M] T_ALT_TIME_SEP_OR_SUFFIX {
    $$ = { Time::NoEvent, $H, $M };
    if (!Time::isValid($$)) { YYABORT; }
  }
| T_INTEGER[H] T_ALT_TIME_SEP_OR_SUFFIX T_INTEGER[M] {
    $$ = { Time::NoEvent, $H, $M };
    if (!Time::isValid($$)) { YYABORT; }
  }
| T_INTEGER[H] T_ALT_TIME_SEP_OR_SUFFIX {
    $$ = { Time::NoEvent, $H, 0 };
    if (!Time::isValid($$)) { YYABORT; }
  }
| T_INTEGER[H] T_COLON T_ALT_TIME_AM[M] {
    $$ = { Time::NoEvent, $H, $M };
    Time::convertFromAm($$);
    if (!Time::isValid($$)) { YYABORT; }
  }
| T_INTEGER[H] T_ALT_TIME_SEP T_ALT_TIME_AM[M] {
    $$ = { Time::NoEvent, $H, $M };
    Time::convertFromAm($$);
    if (!Time::isValid($$)) { YYABORT; }
  }
| T_INTEGER[H] T_COLON T_ALT_TIME_PM[M] {
    $$ = { Time::NoEvent, $H, $M };
    Time::convertFromPm($$);
    if (!Time::isValid($$)) { YYABORT; }
  }
| T_INTEGER[H] T_ALT_TIME_SEP T_ALT_TIME_PM[M] {
    $$ = { Time::NoEvent, $H, $M };
    Time::convertFromPm($$);
    if (!Time::isValid($$)) { YYABORT; }
  }
| T_ALT_TIME_AM[H] {
    $$ = { Time::NoEvent, $H, 0 };
    Time::convertFromAm($$);
    if (!Time::isValid($$)) { YYABORT; }
  }
| T_ALT_TIME_PM[H] {
    $$ = { Time::NoEvent, $H, 0 };
    Time::convertFromPm($$);
    if (!Time::isValid($$)) { YYABORT; }
  }
;

RangeSeparator:
  T_MINUS
| T_ALT_RANGE_SEP
%%
