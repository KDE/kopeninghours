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

%}

%code requires {

#include "openinghours_p.h"
#include "interval.h"

using namespace KOpeningHours;

struct StringRef {
    const char *str;
    int len;
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
    StringRef strRef;
    Interval::State state;
}

%token T_NORMAL_RULE_SEPARATOR
%token T_ADDITIONAL_RULE_SEPARATOR
%token T_FALLBACK_SEPARATOR

%token <state> T_STATE

%token T_24_7
%token T_EXTENDED_HOUR_MINUTE

%token T_PLUS
%token T_MINUS
%token T_SLASH
%token T_COLON
%token T_COMMA

%token T_EVENT

%token T_LBRACKET
%token T_RBRACKET
%token T_LPAREN
%token T_RPAREN

%token T_PH
%token T_SH

%token T_KEYWORD_DAY
%token T_KEYWORD_WEEK

%token T_EASTER

%token T_WEEKDAY
%token T_MONTH
%token <num> T_INTEGER

%token <strRef> T_COMMENT

%token T_INVALID

%type Rule

%destructor { free($$); } <str>

%verbose

%%
// see https://wiki.openstreetmap.org/wiki/Key:opening_hours/specification

Ruleset:
  Rule
| Ruleset T_NORMAL_RULE_SEPARATOR Rule
| Ruleset T_ADDITIONAL_RULE_SEPARATOR Rule
| Ruleset T_FALLBACK_SEPARATOR T_COMMENT
;

Rule: SelectorSequence RuleModifier
;

RuleModifier:
  %empty
| T_COMMENT
| T_STATE
| T_STATE T_COMMENT
;

SelectorSequence:
  T_24_7
| SmallRangeSelector
| WideRangeSelector
| WideRangeSelector SmallRangeSelector
;

WideRangeSelector:
  YearSelector
| MonthdaySelector
| WeekSelector
| YearSelector MonthdaySelector
| YearSelector WeekSelector
| MonthdaySelector WeekSelector
| YearSelector MonthdaySelector WeekSelector
| T_COMMENT T_COLON
;

SmallRangeSelector:
  TimeSelector
| WeekdaySelector
| WeekdaySelector TimeSelector
;

// Time selector
TimeSelector:
  Timespan
| TimeSelector T_COMMA Timespan
;

Timespan:
  Time
| Time T_PLUS
| Time T_MINUS Time
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
  WeekdaySequence
| HolidySequence
| HolidySequence T_COMMA WeekdaySequence
| WeekdaySequence T_COMMA HolidySequence
| HolidySequence " " WeekdaySequence // TODO
;

WeekdaySequence:
  WeekdayRange
| WeekdaySequence T_COMMA WeekdayRange
;

WeekdayRange:
  T_WEEKDAY
| T_WEEKDAY T_MINUS T_WEEKDAY
  // TODO
;

HolidySequence:
  Holiday
| HolidySequence T_COMMA Holiday
;

Holiday:
  T_PH
| T_PH DayOffset
| T_SH
;

DayOffset:
  T_PLUS T_INTEGER T_KEYWORD_DAY
| T_MINUS T_INTEGER T_KEYWORD_DAY
;

// Week selector
WeekSelector:
  T_KEYWORD_WEEK Week
| WeekSelector T_COMMA Week
;

Week:
  T_INTEGER
| T_INTEGER T_MINUS T_INTEGER
| T_INTEGER T_MINUS T_INTEGER T_SLASH T_INTEGER

// Month selector
MonthdaySelector:
  MonthdayRange
| MonthdaySelector T_COMMA MonthdayRange
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
  T_INTEGER // TODO
;

%%
