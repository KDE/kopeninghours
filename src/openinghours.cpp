/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "openinghours.h"
#include "openinghours_p.h"
#include "openinghoursparser_p.h"
#include "openinghoursscanner_p.h"
#include "logging.h"

#include <QScopeGuard>

using namespace KOpeningHours;

void OpeningHoursPrivate::addRule(Rule *rule)
{
    m_rules.push_back(std::unique_ptr<Rule>(rule));
}


OpeningHours::OpeningHours()
    : d(new OpeningHoursPrivate)
{
}

OpeningHours::OpeningHours(const QByteArray &openingHours)
    : d(new OpeningHoursPrivate)
{
    yyscan_t scanner;
    if (yylex_init(&scanner)) {
        qCWarning(Log) << "Failed to initialize scanner?!";
        d->m_error = SyntaxError;
        return;
    }
    const auto lexerCleanup = qScopeGuard([&scanner]{ yylex_destroy(scanner); });

    YY_BUFFER_STATE state;
    state = yy_scan_string(openingHours.constData(), scanner);
    if (yyparse(d.data(), scanner)) {
        d->m_error = SyntaxError;
        return;
    }

    yy_delete_buffer(state, scanner);
}

OpeningHours::OpeningHours(const OpeningHours&) = default;
OpeningHours::OpeningHours(OpeningHours&&) = default;
OpeningHours::~OpeningHours() = default;

OpeningHours& OpeningHours::operator=(const OpeningHours&) = default;
OpeningHours& OpeningHours::operator=(OpeningHours&&) = default;

OpeningHours::Error OpeningHours::error() const
{
    return d->m_error;
}
