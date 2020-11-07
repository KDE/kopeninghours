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

%%

{SPACE} {}

{EXTENDED_HOUR_MINUTE} { return T_EXTENDED_HOUR_MINUTE; }

"; " { return T_NORMAL_RULE_SEPARATOR; }
", " { return T_ADDITIONAL_RULE_SEPARATOR; }
"||" { return T_FALLBACK_SEPARATOR; }

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

"dawn" { return T_EVENT; }
"sunrise" { return T_EVENT; }
"sunset" { return T_EVENT; }
"dusk" { return T_EVENT; }

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

{INTEGER} { return T_INTEGER; }

"Su" { return T_WEEKDAY; }
"Mo" { return T_WEEKDAY; }
"Tu" { return T_WEEKDAY; }
"We" { return T_WEEKDAY; }
"Th" { return T_WEEKDAY; }
"Fr" { return T_WEEKDAY; }
"Sa" { return T_WEEKDAY; }

"Jan" { return T_MONTH; }
"Feb" { return T_MONTH; }
"Mar" { return T_MONTH; }
"Apr" { return T_MONTH; }
"May" { return T_MONTH; }
"Jun" { return T_MONTH; }
"Jul" { return T_MONTH; }
"Aug" { return T_MONTH; }
"Sep" { return T_MONTH; }
"Oct" { return T_MONTH; }
"Nov" { return T_MONTH; }
"Dec" { return T_MONTH; }

{COMMENT} {
    yylval->strRef.str = yytext;
    yylval->strRef.len = yyleng;
    return T_COMMENT;
}

. {
    printf("unexpected character: %s at %d:%d\n", yytext, yylloc->first_line, yylloc->first_column);
    return T_INVALID;
}

%%
