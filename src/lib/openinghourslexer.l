%{
/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "openinghoursparser_p.h"

%}

%option warn nodefault
/* technically the case of all tokens is clearly defined, but reality ignores that in parts, so we do the same */
%option caseless
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

/* technically two hour digits are mandatory, but that isn't always followed in OSM data */
/* 'h' instead of ':', and possibly missing minutes, is also non-standard tolerance */
EXTENDED_HOUR_MINUTE [0-4]?[0-9](:[0-5][0-9]|h([0-5][0-9])?)
YEAR        [1-2][019][0-9][0-9]

%%

{SPACE} {}

{EXTENDED_HOUR_MINUTE} {
    yylval->time.event = Time::NoEvent;
    yylval->time.hour = strtol(yytext, nullptr, 10);
    const char *separator = nullptr;
    for (const auto sep : {':', 'h', 'H'}) {
        separator = strchr(yytext, sep);
        if (separator) {
            break;
        }
    }
    Q_ASSERT(separator);
    yylval->time.minute = strtol(separator + 1, nullptr, 10);
    return T_EXTENDED_HOUR_MINUTE;
}
{YEAR} { yylval->num = std::strtol(yytext, nullptr, 10); return T_YEAR; }

;/. { return T_NORMAL_RULE_SEPARATOR; } // technically this should have space after the semicolon, but that is not always followed in OSM data
", " { return T_ADDITIONAL_RULE_SEPARATOR; }
"||" { return T_FALLBACK_SEPARATOR; } // technically this should have a space on either side, but that is not always followed in OSM data

"open"    { yylval->state = Interval::Open;    return T_STATE; }
"closed"  { yylval->state = Interval::Closed;  return T_STATE; }
"off"     { yylval->state = Interval::Closed;  return T_STATE; }
"unknown" { yylval->state = Interval::Unknown; return T_STATE; }

"24/7" { return T_24_7; }

"+" { return T_PLUS; }
-|‒|–|—|― { return T_MINUS; }
"/" { return T_SLASH; }
":" { return T_COLON; }
,/. { return T_COMMA; }

[,;] {} // eat trailing commas/semicolons, while invalid those occur commonly in OSM data. Practically this is done indirectly in combination with the above rules as we cannot lookahead to EOF

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

 /* technically weekday names should be two letter English abbreviations, but reality is more creative */
"Monday"    { yylval->num = 1; return T_WEEKDAY; }
"Tuesday"   { yylval->num = 2; return T_WEEKDAY; }
"Wednesday" { yylval->num = 3; return T_WEEKDAY; }
"Thursday"  { yylval->num = 4; return T_WEEKDAY; }
"Friday"    { yylval->num = 5; return T_WEEKDAY; }
"Saturday"  { yylval->num = 6; return T_WEEKDAY; }
"Sunday"    { yylval->num = 7; return T_WEEKDAY; }

Mon? { yylval->num = 1; return T_WEEKDAY; }
Tue? { yylval->num = 2; return T_WEEKDAY; }
Wed? { yylval->num = 3; return T_WEEKDAY; }
Thu? { yylval->num = 4; return T_WEEKDAY; }
Fri? { yylval->num = 5; return T_WEEKDAY; }
Sat? { yylval->num = 6; return T_WEEKDAY; }
Sun? { yylval->num = 7; return T_WEEKDAY; }

 /* localized day names (technically not allowed, but present in reality) */
So { yylval->num = 7; return T_WEEKDAY; }

 /* same for month names, technically those should be three letter English abbreviations */
"January" { yylval->num = 1; return T_MONTH; }
"February" { yylval->num = 2; return T_MONTH; }
"March" { yylval->num = 3; return T_MONTH; }
"April" { yylval->num = 4; return T_MONTH; }
"June" { yylval->num = 6; return T_MONTH; }
"July" { yylval->num = 7; return T_MONTH; }
"August" { yylval->num = 8; return T_MONTH; }
"September" { yylval->num = 9; return T_MONTH; }
"October" { yylval->num = 10; return T_MONTH; }
"November" { yylval->num = 11; return T_MONTH; }
"December" { yylval->num = 12; return T_MONTH; }

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
