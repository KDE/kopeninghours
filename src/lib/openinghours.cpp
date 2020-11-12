/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "openinghours.h"
#include "openinghours_p.h"
#include "openinghoursparser_p.h"
#include "openinghoursscanner_p.h"
#include "interval.h"
#include "logging.h"

#include <QDateTime>
#include <QScopeGuard>

using namespace KOpeningHours;

void OpeningHoursPrivate::validate()
{
    if (m_error == OpeningHours::SyntaxError) {
        return;
    }

    int c = Capability::None;
    for (const auto &rule : m_rules) {
        c |= rule->requiredCapabilities();
    }

    if ((c & Capability::Location) && (std::isnan(m_latitude) || std::isnan(m_longitude))) {
        m_error = OpeningHours::MissingLocation;
        return;
    }
    if (c & Capability::PublicHoliday /* TODO */) {
        m_error = OpeningHours::MissingRegion;
        return;
    }
    if (c & (Capability::SchoolHoliday | Capability::NotImplemented)) {
        m_error = OpeningHours::UnsupportedFeature;
        return;
    }

    m_error = OpeningHours::NoError;
}

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
    d->validate();
}

OpeningHours::OpeningHours(const OpeningHours&) = default;
OpeningHours::OpeningHours(OpeningHours&&) = default;
OpeningHours::~OpeningHours() = default;

OpeningHours& OpeningHours::operator=(const OpeningHours&) = default;
OpeningHours& OpeningHours::operator=(OpeningHours&&) = default;

void OpeningHours::setLocation(float latitude, float longitude)
{
    d->m_latitude = latitude;
    d->m_longitude = longitude;
    d->validate();
}

float OpeningHours::latitude() const
{
    return d->m_latitude;
}

void OpeningHours::setLatitude(float latitude)
{
    d->m_latitude = latitude;
    d->validate();
}

float OpeningHours::longitude() const
{
    return d->m_longitude;
}

void OpeningHours::setLongitude(float longitude)
{
    d->m_longitude = longitude;
    d->validate();
}

OpeningHours::Error OpeningHours::error() const
{
    return d->m_error;
}

Interval OpeningHours::interval(const QDateTime &dt) const
{
    if (d->m_error != NoError) {
        return {};
    }

    const auto alignedTime = QDateTime(dt.date(), {dt.time().hour(), dt.time().minute()});
    Interval i;
    for (const auto &rule : d->m_rules) {
        const auto j = rule->nextInterval(alignedTime, d.data());
        if (!j.isValid()) {
            continue;
        }
        i = i.isValid() ? std::min(i, j) : j;
    }

    // check if the resulting interval contains dt, otherwise create a synthetic closed interval
    if (!i.isValid() || i.contains(dt)) {
        return i;
    }

    Interval i2;
    i2.setState(Interval::Closed);
    i2.setBegin(dt);
    i2.setEnd(i.begin());
    return i2;
}

Interval OpeningHours::nextInterval(const Interval &interval) const
{
    if (interval.end().isValid()) {
        return this->interval(interval.end());
    }
    return {};
}
