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
}

static void applySelectors(const Selectors &sels, Rule *rule)
{
    rule->m_timeSelector.reset(sels.timeSelector);
    rule->m_weekdaySelector.reset(sels.weekdaySelector);
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
}

%token T_NORMAL_RULE_SEPARATOR
%token T_ADDITIONAL_RULE_SEPARATOR
%token T_FALLBACK_SEPARATOR

%token <state> T_STATE

%token T_24_7
%token <time> T_EXTENDED_HOUR_MINUTE

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
%type <offset> DayOffset

%destructor { delete $$; } <rule>
%destructor {
    delete $$.timeSelector;
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
  }
;

WideRangeSelector:
  YearSelector
| MonthdaySelector
| WeekSelector
| YearSelector MonthdaySelector
| YearSelector WeekSelector
| MonthdaySelector WeekSelector
| YearSelector MonthdaySelector WeekSelector
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
    $$.timeSelector->next.reset($T2);
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
// TODO
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
    $$.weekdaySelector->next.reset($W);
  }
| WeekdaySequence[W] T_COMMA HolidySequence[H] {
    initSelectors($$);
    $$.weekdaySelector = $W;
    $$.weekdaySelector->next.reset($H);
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
| WeekdaySequence[S] T_COMMA WeekdayRange[W] { $$->next.reset($W); }
;

WeekdayRange:
  T_WEEKDAY[D] {
    $$ = new WeekdayRange;
    $$->beginDay = $D;
  }
| T_WEEKDAY[D1] T_MINUS T_WEEKDAY[D2] {
    $$ = new WeekdayRange;
    $$->beginDay = $D1;
    $$->endDay = $D2;
  }
  // TODO
;

HolidySequence:
  Holiday[H] { $$ = $H; }
| HolidySequence[S] T_COMMA Holiday[H] { $$ = $S; $$->next.reset($H); }
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

DayOffset:
  T_PLUS T_INTEGER[N] T_KEYWORD_DAY { $$ = $N; }
| T_MINUS T_INTEGER[N] T_KEYWORD_DAY { $$ = -$N; }
;

// Week selector
WeekSelector:
  T_KEYWORD_WEEK Week {
    initSelectors($$);
  }
| WeekSelector T_COMMA Week {
    initSelectors($$);
  }
;

Week:
  T_INTEGER
| T_INTEGER T_MINUS T_INTEGER
| T_INTEGER T_MINUS T_INTEGER T_SLASH T_INTEGER

// Month selector
MonthdaySelector:
  MonthdayRange {
    initSelectors($$);
  }
| MonthdaySelector T_COMMA MonthdayRange {
    initSelectors($$);
  }
;

MonthdayRange:
  T_MONTH // TODO
| DateFrom
| DateFrom DateOffset
| DateFrom T_MINUS DateTo
;

DateOffset:
  T_PLUS T_WEEKDAY
| T_MINUS T_WEEKDAY
| DayOffset
;

DateFrom:
  T_MONTH T_INTEGER
| T_INTEGER T_MONTH T_INTEGER
| VariableDate
| T_INTEGER VariableDate
;

DateTo:
  DateFrom
| T_INTEGER
;

VariableDate:
  T_EASTER
;

// Year selector
YearSelector:
  T_INTEGER {// TODO
    initSelectors($$);
  }
;

%%
