%{
/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "openinghoursparser_p.h"

#define YY_USER_ACTION yylloc->first_column = yylloc->last_column; yylloc->last_column += yyleng;

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

YEAR        [1-2][019][0-9][0-9]

%%

{SPACE} {}

{YEAR} { yylval->num = std::strtol(yytext, nullptr, 10); return T_YEAR; }

;/. { return T_NORMAL_RULE_SEPARATOR; } // technically this should have space after the semicolon, but that is not always followed in OSM data
", " { return T_ADDITIONAL_RULE_SEPARATOR; }
"||" { return T_FALLBACK_SEPARATOR; } // technically this should have a space on either side, but that is not always followed in OSM data

"open"    { yylval->state = State::Open;    return T_STATE; }
"closed"  { yylval->state = State::Closed;  return T_STATE; }
"off"     { yylval->state = State::Off;  return T_STATE; }
"unknown" { yylval->state = State::Unknown; return T_STATE; }

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

  /* am/pm time format support, non-standard and has to appear before the generic number token. */
[0-5]?[0-9](\ ?a\.?m\.?|a) { yylval->num = std::strtol(yytext, nullptr, 10); return T_ALT_TIME_AM; }
[0-5]?[0-9](\ ?p\.?m\.?|p) { yylval->num = std::strtol(yytext, nullptr, 10); return T_ALT_TIME_PM; }

{INTEGER} { yylval->num = std::strtol(yytext, nullptr, 10); return T_INTEGER; }

 /* technically weekday names should be two letter English abbreviations, but reality is more creative */
Mondays?    { yylval->num = 1; return T_WEEKDAY; }
Tuesdays?   { yylval->num = 2; return T_WEEKDAY; }
Wednesdays? { yylval->num = 3; return T_WEEKDAY; }
Thursdays?  { yylval->num = 4; return T_WEEKDAY; }
Fridays?    { yylval->num = 5; return T_WEEKDAY; }
Saturdays?  { yylval->num = 6; return T_WEEKDAY; }
Sundays?    { yylval->num = 7; return T_WEEKDAY; }

Tues  { yylval->num = 2; return T_WEEKDAY; }
Thurs? { yylval->num = 4; return T_WEEKDAY; }

Mon? { yylval->num = 1; return T_WEEKDAY; }
Tue? { yylval->num = 2; return T_WEEKDAY; }
Wed? { yylval->num = 3; return T_WEEKDAY; }
Thu? { yylval->num = 4; return T_WEEKDAY; }
Fri? { yylval->num = 5; return T_WEEKDAY; }
Sat? { yylval->num = 6; return T_WEEKDAY; }
Sun? { yylval->num = 7; return T_WEEKDAY; }

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

  /* various alternative formats, none of this is remotely compliant with the specification, but appears in reality nevertheless */

  /* alternative time formats */
：|︓|ː { return T_ALT_TIME_SEP; }
h { return T_ALT_TIME_SEP_OR_SUFFIX; }

  /* alternative range separators */
~|～|to|à|bis|a|ás|às|as|au { return T_ALT_RANGE_SEP; }

  /* localized state names */
ferm(e|é)|geschlossen { yylval->state = State::Closed;  return T_STATE; }

 /* German localized day names. */
Montags?     { yylval->num = 1; return T_WEEKDAY; }
Dienstags?   { yylval->num = 2; return T_WEEKDAY; }
Mittwochs?   { yylval->num = 3; return T_WEEKDAY; }
Donnerstags? { yylval->num = 4; return T_WEEKDAY; }
Freitags?    { yylval->num = 5; return T_WEEKDAY; }
Samstags?    { yylval->num = 6; return T_WEEKDAY; }
Sonntags?    { yylval->num = 7; return T_WEEKDAY; }

 /* "Di" conflicts with French... */
So { yylval->num = 7; return T_WEEKDAY; }

  /* French localized day names. */
Lundi    { yylval->num = 1; return T_WEEKDAY; }
Mardi    { yylval->num = 2; return T_WEEKDAY; }
Mercredi { yylval->num = 3; return T_WEEKDAY; }
Jeudi    { yylval->num = 4; return T_WEEKDAY; }
Vendredi { yylval->num = 5; return T_WEEKDAY; }
Samedi   { yylval->num = 6; return T_WEEKDAY; }
Dimanche { yylval->num = 7; return T_WEEKDAY; }

  /* Spanish localized day names */
Lunes        { yylval->num = 1; return T_WEEKDAY; }
Martes       { yylval->num = 2; return T_WEEKDAY; }
Mi(é|e)rcoles { yylval->num = 3; return T_WEEKDAY; }
Jueves       { yylval->num = 4; return T_WEEKDAY; }
Viernes      { yylval->num = 5; return T_WEEKDAY; }
S(á|a)bado    { yylval->num = 6; return T_WEEKDAY; }
Domingo      { yylval->num = 7; return T_WEEKDAY; }

  /* Portuguese localized day names */
feira   { yylval->num = 1; return T_WEEKDAY; }
segunda { yylval->num = 1; return T_WEEKDAY; }
ter(ç|c)a   { yylval->num = 2; return T_WEEKDAY; }
quarta  { yylval->num = 3; return T_WEEKDAY; }
quinta  { yylval->num = 4; return T_WEEKDAY; }
sexta   { yylval->num = 5; return T_WEEKDAY; }

  /* skip filler words */
from|von|du|de {}

. {
    printf("unexpected character: %s at %d:%d\n", yytext, yylloc->first_line, yylloc->first_column);
    return T_INVALID;
}

%%
