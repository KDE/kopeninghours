%{
/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "openinghoursparser_p.h"

#include <cstring>

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

SPACE       ([ \t\r\n]| | |　)+

INTEGER     [0-9]+

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
-|‒|–|‑|—|―|－|−|ー { return T_MINUS; }
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
"whitsun" { return T_WHITSUN; } // non-standard, will be turned into "easter +49 days"

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

Mo\. { yylval->num = 1; return T_WEEKDAY; }
Tu\. { yylval->num = 2; return T_WEEKDAY; }
We\. { yylval->num = 3; return T_WEEKDAY; }
Th\. { yylval->num = 4; return T_WEEKDAY; }
Fr\. { yylval->num = 5; return T_WEEKDAY; }
Sa\. { yylval->num = 6; return T_WEEKDAY; }
Su\. { yylval->num = 7; return T_WEEKDAY; }

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

Jan\.? { yylval->num = 1; return T_MONTH; }
Feb\.? { yylval->num = 2; return T_MONTH; }
Mar\.? { yylval->num = 3; return T_MONTH; }
Apr\.? { yylval->num = 4; return T_MONTH; }
May\.? { yylval->num = 5; return T_MONTH; }
Jun\.? { yylval->num = 6; return T_MONTH; }
Jul\.? { yylval->num = 7; return T_MONTH; }
Aug\.? { yylval->num = 8; return T_MONTH; }
Sept?\.? { yylval->num = 9; return T_MONTH; }
Oct\.? { yylval->num = 10; return T_MONTH; }
Nov\.? { yylval->num = 11; return T_MONTH; }
Dec\.? { yylval->num = 12; return T_MONTH; }

 /* Month names in French */
"Janvier" { yylval->num = 1; return T_MONTH; }
"Février" { yylval->num = 2; return T_MONTH; }
"Mars" { yylval->num = 3; return T_MONTH; }
"Avril" { yylval->num = 4; return T_MONTH; }
"Mai" { yylval->num = 5; return T_MONTH; }
"Juin" { yylval->num = 6; return T_MONTH; }
"Juillet" { yylval->num = 7; return T_MONTH; }
"Août" { yylval->num = 8; return T_MONTH; }
"Septembre" { yylval->num = 9; return T_MONTH; }
"Octobre" { yylval->num = 10; return T_MONTH; }
"Novembre" { yylval->num = 11; return T_MONTH; }
"Décembre" { yylval->num = 12; return T_MONTH; }

 /* different quote types are sometimes mixed and/or used nested, so this is a compromise to catch most of them */
["][^"]*["] {
    yylval->strRef.str = yytext + 1;
    yylval->strRef.len = yyleng - 2;
    return T_COMMENT;
}
(“|”|„)[^(\"|“|”|„)]*(\"|“|”|„) {
    int startOffset = 1; int endOffset = 1;
    for (const auto quote : { "\"", "“", "”", "„" }) {
        const int len = std::strlen(quote);
        if (yyleng <= len) {
            continue;
        }
        if (std::strncmp(yytext, quote, len) == 0) {
            startOffset = len;
        }
        if (std::strncmp(yytext + yyleng - len, quote, len) == 0) {
            endOffset = len;
        }
    }
    yylval->strRef.str = yytext + startOffset;
    yylval->strRef.len = yyleng - startOffset - endOffset;
    return T_COMMENT;
}

  /* various alternative formats, none of this is remotely compliant with the specification, but appears in reality nevertheless */

  /* alternative time formats */
：|︓|ː|\. { return T_ALT_TIME_SEP; }
h|時 { return T_ALT_TIME_SEP_OR_SUFFIX; }

  /* alternative range separators */
~|～|〜|to|à|bis|a|ás|às|as|au|al|→|до { return T_ALT_RANGE_SEP; }

  /* localized state names */
ferm(e|é)|geschlossen|ruhetag|encerrado|chiuso { yylval->state = State::Closed;  return T_STATE; }

 /* German localized day names. */
Montags?     { yylval->num = 1; return T_WEEKDAY; }
Die(nstags?)?   { yylval->num = 2; return T_WEEKDAY; }
Mittwochs?   { yylval->num = 3; return T_WEEKDAY; }
Donnerstags? { yylval->num = 4; return T_WEEKDAY; }
Freitags?    { yylval->num = 5; return T_WEEKDAY; }
Samstags?    { yylval->num = 6; return T_WEEKDAY; }
Sonntags?    { yylval->num = 7; return T_WEEKDAY; }
Feiertage?   { return T_PH; }

  /* French, Spanish, Italian */
Lu { yylval->num = 1; return T_WEEKDAY; }
Ma { yylval->num = 2; return T_WEEKDAY; }
  /* French, Italian */
Me { yylval->num = 3; return T_WEEKDAY; }
  /* German, Spanish, Italian */
Mi { yylval->num = 3; return T_WEEKDAY; }
  /* French */
Je { yylval->num = 4; return T_WEEKDAY; }
  /* Italian */
Gi { yylval->num = 4; return T_WEEKDAY; }
  /* French, Italian */
Ve { yylval->num = 5; return T_WEEKDAY; }
  /* German */
So { yylval->num = 7; return T_WEEKDAY; }

 /* "Di" conflicts between German and French...
    "Do" conflicts between German and Spanish+Italian... */

  /* French localized day names. */
Lun(di)?    { yylval->num = 1; return T_WEEKDAY; }
Mardi       { yylval->num = 2; return T_WEEKDAY; }
Mer(credi)? { yylval->num = 3; return T_WEEKDAY; }
Jeu(di)?    { yylval->num = 4; return T_WEEKDAY; }
Ven(dredi)? { yylval->num = 5; return T_WEEKDAY; }
Sam(edi)?   { yylval->num = 6; return T_WEEKDAY; }
Dim(anche)? { yylval->num = 7; return T_WEEKDAY; }
"jours fériés" { return T_PH; }

  /* Spanish localized day names */
Lunes         { yylval->num = 1; return T_WEEKDAY; }
Martes        { yylval->num = 2; return T_WEEKDAY; }
Mi(é|e)rcoles { yylval->num = 3; return T_WEEKDAY; }
Jueves        { yylval->num = 4; return T_WEEKDAY; }
Vie(rnes)?    { yylval->num = 5; return T_WEEKDAY; }
S(á|a)b(ado)? { yylval->num = 6; return T_WEEKDAY; }
Dom(ingo)?    { yylval->num = 7; return T_WEEKDAY; }

  /* Italian localized day names */
Luned(ì|i)     { yylval->num = 1; return T_WEEKDAY; }
Marted(ì|i)    { yylval->num = 2; return T_WEEKDAY; }
Mercoled(ì|i)  { yylval->num = 3; return T_WEEKDAY; }
Gio(ved(ì|i))? { yylval->num = 4; return T_WEEKDAY; }
Venerd(ì|i)    { yylval->num = 5; return T_WEEKDAY; }
Sabato         { yylval->num = 6; return T_WEEKDAY; }
Domenica       { yylval->num = 7; return T_WEEKDAY; }

  /* Portuguese localized day names */
feira   { yylval->num = 1; return T_WEEKDAY; }
segunda { yylval->num = 1; return T_WEEKDAY; }
ter(ç|c)a   { yylval->num = 2; return T_WEEKDAY; }
quarta  { yylval->num = 3; return T_WEEKDAY; }
quinta  { yylval->num = 4; return T_WEEKDAY; }
sexta   { yylval->num = 5; return T_WEEKDAY; }

  /* Japanese localized day names */
月|月曜|月曜日 { yylval->num = 1; return T_WEEKDAY; }
火|火曜|火曜日 { yylval->num = 2; return T_WEEKDAY; }
水|水曜|水曜日 { yylval->num = 3; return T_WEEKDAY; }
木|木曜|木曜日 { yylval->num = 4; return T_WEEKDAY; }
金|金曜|金曜日 { yylval->num = 5; return T_WEEKDAY; }
土|土曜|土曜日 { yylval->num = 6; return T_WEEKDAY; }
日|日曜|日曜日 { yylval->num = 7; return T_WEEKDAY; }
祝日 { return T_PH; }

  /* Indonesian localized day names */
Senin   { yylval->num = 1; return T_WEEKDAY; }
Selasa  { yylval->num = 2; return T_WEEKDAY; }
Rabu    { yylval->num = 3; return T_WEEKDAY; }
Kamis   { yylval->num = 4; return T_WEEKDAY; }
Jumat   { yylval->num = 5; return T_WEEKDAY; }
Sabtu   { yylval->num = 6; return T_WEEKDAY; }
Minggu  { yylval->num = 7; return T_WEEKDAY; }

  /* creative rule separators */
、|and|et|e|y|und|& { return T_ADDITIONAL_RULE_SEPARATOR; }

  /* skip filler words */
from|von|du|de|le|das|分|uhr|"en continu" {}

. {
    //printf("unexpected character: %s at %d:%d\n", yytext, yylloc->first_line, yylloc->first_column);
    return T_INVALID;
}

%%
