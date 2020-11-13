%{
/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "openinghoursparser_p.h"

%}

%option warn nodefault

%option reentrant
%option noyywrap
%option nounput
%option never-interactive
%option bison-bridge
%option bison-locations
%option yylineno

SPACE       [ \t\r\n]+

INTEGER     [0-9]+
COMMENT     ["][^"]*["]

EXTENDED_HOUR_MINUTE [0-4][0-9]:[0-5][0-9]
YEAR        [1-2][019][0-9][0-9]

%%

{SPACE} {}

{EXTENDED_HOUR_MINUTE} {
    yylval->time.event = Time::NoEvent;
    yylval->time.hour = strtol(yytext, nullptr, 10);
    yylval->time.minute = strtol(yytext + 3, nullptr, 10);
    return T_EXTENDED_HOUR_MINUTE;
}
{YEAR} { yylval->num = std::strtol(yytext, nullptr, 10); return T_YEAR; }

"; " { return T_NORMAL_RULE_SEPARATOR; }
", " { return T_ADDITIONAL_RULE_SEPARATOR; }
" || " { return T_FALLBACK_SEPARATOR; }

"open"    { yylval->state = Interval::Open;    return T_STATE; }
"closed"  { yylval->state = Interval::Closed;  return T_STATE; }
"off"     { yylval->state = Interval::Closed;  return T_STATE; }
"unknown" { yylval->state = Interval::Unknown; return T_STATE; }

"24/7" { return T_24_7; }

"+" { return T_PLUS; }
"-" { return T_MINUS; }
"/" { return T_SLASH; }
":" { return T_COLON; }
"," { return T_COMMA; }

"dawn"    { yylval->time = { Time::Dawn,    0, 0 }; return T_EVENT; }
"sunrise" { yylval->time = { Time::Sunrise, 0, 0 }; return T_EVENT; }
"sunset"  { yylval->time = { Time::Sunset , 0, 0 }; return T_EVENT; }
"dusk"    { yylval->time = { Time::Dusk,    0, 0 }; return T_EVENT; }

"[" { return T_LBRACKET; }
"]" { return T_RBRACKET; }
"(" { return T_LPAREN; }
")" { return T_RPAREN; }

"PH" { return T_PH; }
"SH" { return T_SH; }

" day" { return T_KEYWORD_DAY; }
" days" { return T_KEYWORD_DAY; }
"week" { return T_KEYWORD_WEEK; }
"easter" { return T_EASTER; }

{INTEGER} { yylval->num = std::strtol(yytext, nullptr, 10); return T_INTEGER; }

"Mo" { yylval->num = 1; return T_WEEKDAY; }
"Tu" { yylval->num = 2; return T_WEEKDAY; }
"We" { yylval->num = 3; return T_WEEKDAY; }
"Th" { yylval->num = 4; return T_WEEKDAY; }
"Fr" { yylval->num = 5; return T_WEEKDAY; }
"Sa" { yylval->num = 6; return T_WEEKDAY; }
"Su" { yylval->num = 7; return T_WEEKDAY; }

"Jan" { yylval->num = 1; return T_MONTH; }
"Feb" { yylval->num = 2; return T_MONTH; }
"Mar" { yylval->num = 3; return T_MONTH; }
"Apr" { yylval->num = 4; return T_MONTH; }
"May" { yylval->num = 5; return T_MONTH; }
"Jun" { yylval->num = 6; return T_MONTH; }
"Jul" { yylval->num = 7; return T_MONTH; }
"Aug" { yylval->num = 8; return T_MONTH; }
"Sep" { yylval->num = 9; return T_MONTH; }
"Oct" { yylval->num = 10; return T_MONTH; }
"Nov" { yylval->num = 11; return T_MONTH; }
"Dec" { yylval->num = 12; return T_MONTH; }

{COMMENT} {
    yylval->strRef.str = yytext + 1;
    yylval->strRef.len = yyleng - 2;
    return T_COMMENT;
}

. {
    printf("unexpected character: %s at %d:%d\n", yytext, yylloc->first_line, yylloc->first_column);
    return T_INVALID;
}

%%
