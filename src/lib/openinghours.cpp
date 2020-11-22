/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "openinghours.h"
#include "openinghours_p.h"
#include "openinghoursparser_p.h"
#include "openinghoursscanner_p.h"
#include "interval.h"
#include "rule_p.h"
#include "logging.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QScopeGuard>

using namespace KOpeningHours;

QHash<QString, QString> OpeningHoursPrivate::s_holidayRegionCache;

void OpeningHoursPrivate::validate()
{
    if (m_error == OpeningHours::SyntaxError) {
        return;
    }
    if (m_rules.empty()) {
        m_error = OpeningHours::Null;
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
    if (c & Capability::PublicHoliday && !m_region.isValid()) {
        m_error = OpeningHours::MissingRegion;
        return;
    }
    if (((c & Capability::PointInTime) && (m_modes & OpeningHours::PointInTimeMode) == 0)
     || ((c & Capability::Interval) && (m_modes & OpeningHours::IntervalMode) == 0)) {
        m_error = OpeningHours::IncompatibleMode;
        return;
    }
    if (c & (Capability::SchoolHoliday | Capability::NotImplemented | Capability::PointInTime)) {
        m_error = OpeningHours::UnsupportedFeature;
        return;
    }

    m_error = OpeningHours::NoError;
}

void OpeningHoursPrivate::addRule(Rule *rule, RuleType type)
{
    switch (type) {
        case AdditionalRule:
            // TODO
            [[fallthrough]];
        case NormalRule:
            m_rules.push_back(std::unique_ptr<Rule>(rule));
            break;
        case FallbackRule:
            m_fallbackRule.reset(rule);
    }
}


OpeningHours::OpeningHours()
    : d(new OpeningHoursPrivate)
{
}

OpeningHours::OpeningHours(const QByteArray &openingHours, Modes modes)
    : d(new OpeningHoursPrivate)
{
    d->m_modes = modes;

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

QString OpeningHours::region() const
{
    return d->m_region.regionCode();
}

void OpeningHours::setRegion(QStringView region)
{
    const auto idx = region.indexOf(QLatin1Char('_')); // compatibility with KHolidays region codes
    if (idx > 0) {
        region = region.left(idx);
    }

    const auto loc = region.toString();
    const auto it = OpeningHoursPrivate::s_holidayRegionCache.constFind(loc);
    if (it != OpeningHoursPrivate::s_holidayRegionCache.constEnd()) {
        d->m_region = KHolidays::HolidayRegion(it.value());
    } else {
        const auto code = KHolidays::HolidayRegion::defaultRegionCode(loc);
        d->m_region = KHolidays::HolidayRegion(code);
        OpeningHoursPrivate::s_holidayRegionCache.insert(loc, code);
    }
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
    // first try to find the nearest open interval, and afterwards check closed rules
    for (const auto &rule : d->m_rules) {
        if (rule->m_state == Interval::Closed) {
            continue;
        }
        const auto j = rule->nextInterval(alignedTime, d.data());
        if (!j.isValid()) {
            continue;
        }
        i = i.isValid() ? std::min(i, j) : j;
    }

    QDateTime closeEnd = i.begin(), closeBegin = i.end();
    Interval closedInterval;
    for (const auto &rule : d->m_rules) {
        if (rule->m_state != Interval::Closed) {
            continue;
        }
        const auto j = rule->nextInterval(i.begin(), d.data());
        if (!j.isValid() || !i.intersects(j)) {
            continue;
        }

        if (j.contains(alignedTime)) {
            if (closedInterval.isValid()) {
                // TODO we lose comment information here
                closedInterval.setBegin(std::min(closedInterval.begin(), j.begin()));
                closedInterval.setEnd(std::max(closedInterval.end(), j.end()));
            } else {
                closedInterval = j;
            }
        } else if (alignedTime < j.begin()) {
            closeBegin = std::min(j.begin(), closeBegin);
        } else if (j.end() <= alignedTime) {
            closeEnd = std::max(closeEnd, j.end());
        }
    }
    if (closedInterval.isValid()) {
        i = closedInterval;
    } else {
        i.setBegin(closeEnd);
        i.setEnd(closeBegin);
    }

    // check if the resulting interval contains dt, otherwise create a synthetic fallback interval
    if (!i.isValid() || i.contains(dt)) {
        return i;
    }

    Interval i2;
    if (d->m_fallbackRule) {
        i2.setState(d->m_fallbackRule->m_state);
        i2.setComment(d->m_fallbackRule->m_comment);
    } else {
        i2.setState(Interval::Closed);
    }
    i2.setBegin(dt);
    i2.setEnd(i.begin());
    // TODO do we need to intersect this with closed rules as well?
    return i2;
}

Interval OpeningHours::nextInterval(const Interval &interval) const
{
    if (interval.end().isValid()) {
        auto i = this->interval(interval.end());
        if (i.begin() < interval.end() && i.end() > interval.end()) {
            i.setBegin(interval.end());
        }
        return i;
    }
    return {};
}

static Rule* openingHoursSpecToRule(const QJsonObject &obj)
{
    if (obj.value(QLatin1String("@type")).toString() != QLatin1String("OpeningHoursSpecification")) {
        return nullptr;
    }

    const auto opens = QTime::fromString(obj.value(QLatin1String("opens")).toString());
    const auto closes = QTime::fromString(obj.value(QLatin1String("closes")).toString());

    if (!opens.isValid() || !closes.isValid()) {
        return nullptr;
    }

    auto r = new Rule;
    r->m_state = Interval::Open;
    // ### is name or description used for comments?

    r->m_timeSelector.reset(new Timespan);
    r->m_timeSelector->begin = { Time::NoEvent, opens.hour(), opens.minute() };
    r->m_timeSelector->end = { Time::NoEvent, closes.hour(), closes.minute() };

    const auto validFrom = QDate::fromString(obj.value(QLatin1String("validFrom")).toString(), Qt::ISODate);
    const auto validTo = QDate::fromString(obj.value(QLatin1String("validThrough")).toString(), Qt::ISODate);
    if (validFrom.isValid() || validTo.isValid()) {
        r->m_monthdaySelector.reset(new MonthdayRange);
        r->m_monthdaySelector->begin = { validFrom.year(), validFrom.month(), validFrom.day(), Date::FixedDate, 0, 0 };
        r->m_monthdaySelector->end = { validTo.year(), validTo.month(), validTo.day(), Date::FixedDate, 0, 0 };
    }

    const auto weekday = obj.value(QLatin1String("dayOfWeek")).toString();
    if (!weekday.isEmpty()) {
        r->m_weekdaySelector.reset(new WeekdayRange);
        int i = 1;
        for (const auto &d : { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"}) {
            if (weekday.endsWith(QLatin1String(d))) {
                r->m_weekdaySelector->beginDay = r->m_weekdaySelector->endDay = i;
                break;
            }
            ++i;
        }
    }

    return r;
}

OpeningHours OpeningHours::fromJsonLd(const QJsonObject &obj)
{
    OpeningHours result;

    const auto oh = obj.value(QLatin1String("openingHours"));
    if (oh.isString()) {
        result = OpeningHours(oh.toString().toUtf8());
    } else if (oh.isArray()) {
        const auto ohA = oh.toArray();
        QByteArray expr;
        for (const auto &exprV : ohA) {
            const auto exprS = exprV.toString();
            if (exprS.isEmpty()) {
                continue;
            }
            expr += (expr.isEmpty() ? "" : "; ") + exprS.toUtf8();
        }
        result = OpeningHours(expr);
    }

    std::vector<std::unique_ptr<Rule>> rules;
    const auto sohs = obj.value(QLatin1String("specialOpeningHoursSpecification")).toArray();
    for (const auto &ohsV : sohs) {
        const auto r = openingHoursSpecToRule(ohsV.toObject());
        if (r) {
            rules.push_back(std::unique_ptr<Rule>(r));
        }
    }
    const auto ohs = obj.value(QLatin1String("openingHoursSpecification")).toArray();
    for (const auto &ohsV : ohs) {
        const auto r = openingHoursSpecToRule(ohsV.toObject());
        if (r) {
            rules.push_back(std::unique_ptr<Rule>(r));
        }
    }
    if (!rules.empty()) {
        std::swap(result.d->m_rules, rules);
        for (auto &r : rules) {
            result.d->m_rules.push_back(std::move(r));
        }
    }

    result.d->validate();
    return result;
}
