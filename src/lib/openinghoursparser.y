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
    (void)scanner;
    qCDebug(Log) << "PARSER ERROR:" << msg; // << "in" << parser->fileName() << "line:" << loc->first_line << "column:" << loc->first_column;
    parser->m_error = OpeningHours::SyntaxError;
}

static void initSelectors(Selectors &sels)
{
    sels.timeSelector = nullptr;
    sels.weekdaySelector = nullptr;
    sels.weekSelector = nullptr;
    sels.monthdaySelector = nullptr;
    sels.yearSelector = nullptr;
}

static void applySelectors(const Selectors &sels, Rule *rule)
{
    rule->m_timeSelector.reset(sels.timeSelector);
    rule->m_weekdaySelector.reset(sels.weekdaySelector);
    rule->m_weekSelector.reset(sels.weekSelector);
    rule->m_monthdaySelector.reset(sels.monthdaySelector);
    rule->m_yearSelector.reset(sels.yearSelector);
}

template <typename T> void appendSelector(T* firstSelector, T* selector)
{
    while(firstSelector->next) {
        firstSelector = firstSelector->next.get();
    }
    firstSelector->next.reset(selector);
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

%union {
    uint32_t num;
    int32_t offset;
    StringRef strRef;
    Interval::State state;
    Rule *rule;
    Time time;
    Selectors selectors;
    Timespan *timespan;
    WeekdayRange *weekdayRange;
    Week *week;
    Date date;
    MonthdayRange *monthdayRange;
    YearRange *yearRange;
}

%token T_NORMAL_RULE_SEPARATOR
%token T_ADDITIONAL_RULE_SEPARATOR
%token T_FALLBACK_SEPARATOR

%token <state> T_STATE

%token T_24_7
%token <time> T_EXTENDED_HOUR_MINUTE
%token <num> T_YEAR

%token T_PLUS
%token T_MINUS
%token T_SLASH
%token T_COLON
%token T_COMMA

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
%type <timespan> Timespan
%type <time> Time
%type <weekdayRange> WeekdaySequence
%type <weekdayRange> WeekdayRange
%type <weekdayRange> HolidySequence
%type <weekdayRange> Holiday
%type <num> NthSequence
%type <num> NthEntry
%type <offset> DayOffset
%type <offset> DateOffset
%type <week> Week
%type <date> DateFrom
%type <date> DateTo
%type <date> VariableDate
%type <monthdayRange> MonthdayRange
%type <yearRange> YearRange

%destructor { delete $$; } <rule>
%destructor {
    delete $$.timeSelector;
    delete $$.weekdaySelector;
    delete $$.weekSelector;
    delete $$.monthdaySelector;
    delete $$.yearSelector;
} <selectors>

%verbose

%%
// see https://wiki.openstreetmap.org/wiki/Key:opening_hours/specification

Ruleset:
  Rule[R] { parser->addRule($R); }
| Ruleset T_NORMAL_RULE_SEPARATOR Rule[R] { parser->addRule($R); }
| Ruleset T_ADDITIONAL_RULE_SEPARATOR Rule[R] { parser->addRule($R); }
| Ruleset T_FALLBACK_SEPARATOR T_COMMENT { /* TODO */ }
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
    $$->m_state = $T;
    applySelectors($S, $$);
  }
| SelectorSequence[S] T_STATE[T] T_COMMENT[C] {
    $$ = new Rule;
    $$->setComment($C.str, $C.len);
    $$->m_state = $T;
    applySelectors($S, $$);
  }
;

SelectorSequence:
  T_24_7 { initSelectors($$); }
| SmallRangeSelector[S] { $$ = $S; }
| WideRangeSelector[W] { $$ = $W; }
| WideRangeSelector[W] SmallRangeSelector[S] {
    $$.timeSelector = $S.timeSelector;
    $$.weekdaySelector = $S.weekdaySelector;
    $$.weekSelector = $W.weekSelector;
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
| T_COMMENT T_COLON { initSelectors($$); }
;

SmallRangeSelector:
  TimeSelector[T] { $$ = $T; }
| WeekdaySelector[W] { $$ = $W; }
| WeekdaySelector[W] TimeSelector[T] {
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
;

Timespan:
  Time[T] {
    $$ = new Timespan;
    $$->begin = $$->end = $T;
  }
| Time[T] T_PLUS {
    $$ = new Timespan;
    $$->begin = $T;
  }
| Time[T1] T_MINUS Time[T2] {
    $$ = new Timespan;
    $$->begin = $T1;
    $$->end = $T2;
  }
| Time[T1] T_MINUS Time[T2] T_PLUS {
    $$ = new Timespan;
    $$->begin = $T1;
    $$->end = $T2;
    // TODO open end flag
  }
| Time[T1] T_MINUS Time[T2] T_SLASH T_INTEGER[I] {
    $$ = new Timespan;
    $$->begin = $T1;
    $$->end = $T2;
    $$->interval = $I;
  }
// TODO interval in hour:min format
;

Time:
  T_EXTENDED_HOUR_MINUTE
| VariableTime
;

VariableTime:
  T_EVENT
| T_LPAREN T_EVENT T_PLUS T_INTEGER T_RPAREN
| T_LPAREN T_EVENT T_MINUS T_INTEGER T_RPAREN
;

// Weekday selector
WeekdaySelector:
  WeekdaySequence[W] {
    initSelectors($$);
    $$.weekdaySelector = $W;
  }
| HolidySequence[H] {
    initSelectors($$);
    $$.weekdaySelector = $H;
  }
| HolidySequence[H] T_COMMA WeekdaySequence[W] {
    initSelectors($$);
    $$.weekdaySelector = $H;
    appendSelector($$.weekdaySelector, $W);
  }
| WeekdaySequence[W] T_COMMA HolidySequence[H] {
    initSelectors($$);
    $$.weekdaySelector = $W;
    appendSelector($$.weekdaySelector, $H);
  }
| HolidySequence[H] " " WeekdaySequence[W] { // TODO
    initSelectors($$);
    $$.weekdaySelector = new WeekdayRange;
    $$.weekdaySelector->next.reset($H);
    $$.weekdaySelector->next2.reset($W);
  }
;

WeekdaySequence:
  WeekdayRange[W] { $$ = $W; }
| WeekdaySequence[S] T_COMMA WeekdayRange[W] { appendSelector($$, $W); }
;

WeekdayRange:
  T_WEEKDAY[D] {
    $$ = new WeekdayRange;
    $$->beginDay = $D;
    $$->endDay = $D;
  }
| T_WEEKDAY[D1] T_MINUS T_WEEKDAY[D2] {
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

HolidySequence:
  Holiday[H] { $$ = $H; }
| HolidySequence[S] T_COMMA Holiday[H] { $$ = $S; appendSelector($$, $H); }
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
  T_INTEGER[N] { $$ = (1 << ($N - 1)); }
| T_INTEGER[N1] T_MINUS T_INTEGER[N2] {
    $$ = 0;
    for (int i = $N1; i < $N2; ++i) {
        $$ |= (1 << (i - 1));
    }
  }
| T_MINUS T_INTEGER[N] {
    $$ = (1 << ($N - 1)) | (1 << 7);
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
MonthdaySelector:
  MonthdayRange[M] {
    initSelectors($$);
    $$.monthdaySelector = $M;
  }
| MonthdaySelector[S] T_COMMA MonthdayRange[M] {
    $$ = $S;
    appendSelector($$.monthdaySelector, $M);
  }
;

MonthdayRange:
  T_MONTH[M] {
    $$ = new MonthdayRange;
    $$->begin = $$->end = { 0, $M, 0, Date::FixedDate };
  }
| T_MONTH[M1] T_MINUS T_MONTH[M2] {
    $$ = new MonthdayRange;
    $$->begin = { 0, $M1, 0, Date::FixedDate };
    $$->end = { 0, $M2, 0, Date::FixedDate };
  }
| DateFrom[D] {
    $$ = new MonthdayRange;
    $$->begin = $$->end = $D;
  }
| DateFrom[D] DateOffset[O] {
    $$ = new MonthdayRange;
    $$->begin = $D;
    $$->offset = $O;
  }
| DateFrom[F] T_MINUS DateTo[T] {
    $$ = new MonthdayRange;
    $$->begin = $F;
    $$->end = $T;
  }
;

DateOffset:
  T_PLUS T_WEEKDAY[D] { $$ = $D; }
| T_MINUS T_WEEKDAY[D] { $$ = -$D; }
| DayOffset[O] { $$ = $O; }
;

DateFrom:
  T_MONTH[M] T_INTEGER[D] { $$ = { 0, $M, $D, Date::FixedDate }; }
| T_YEAR[Y] T_MONTH[M] T_INTEGER[D] { $$ = { $Y, $M, $D, Date::FixedDate }; }
| VariableDate[D] { $$ = $D; }
| T_INTEGER[Y] VariableDate[D] {
    $$ = $D;
    $$.year = $Y;
  }
;

DateTo:
  DateFrom[D] { $$ = $D; }
| T_INTEGER[N] { $$ = { 0, 0, $N, Date::FixedDate }; }
;

VariableDate:
  T_EASTER { $$ = { 0, 0, 0, Date::Easter }; }
;

// Year selector
YearSelector:
  YearRange[Y] {
    initSelectors($$);
    $$.yearSelector = $Y;
  }
| YearSelector[S] T_COMMA YearRange[Y] {
    $$ = $S;
    appendSelector($$.yearSelector, $Y);
  }
;

YearRange:
  T_YEAR[Y] {
    $$ = new YearRange;
    $$->begin = $$->end = $Y;
  }
| T_YEAR[Y1] T_MINUS T_YEAR[Y2] {
    $$ = new YearRange;
    $$->begin = $Y1;
    $$->end = $Y2;
    if ($$->end < $$->begin) {
        YYABORT;
    }
  }
| T_YEAR[Y1] T_MINUS T_YEAR[Y2] T_SLASH T_INTEGER[I] {
    $$ = new YearRange;
    $$->begin = $Y1;
    $$->end = $Y2;
    if ($$->end < $$->begin) {
        YYABORT;
    }
    $$->interval = $I;
  }
| T_YEAR[Y] T_PLUS {
    $$ = new YearRange;
    $$->begin = $Y;
  }
%%
