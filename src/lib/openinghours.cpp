/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "openinghours.h"
#include "openinghours_p.h"
#include "openinghoursparser_p.h"
#include "openinghoursscanner_p.h"
#include "holidaycache_p.h"
#include "interval.h"
#include "rule_p.h"
#include "logging.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QScopeGuard>
#include <QTimeZone>

using namespace KOpeningHours;

void OpeningHoursPrivate::autocorrect()
{
    if (m_rules.size() <= 1) {
        return;
    }

    // find incomplete additional rules, and merge them with the preceding rule
    // example: "Mo, We, Fr 06:30-21:30" becomes "Mo,We,Fr 06:30-21:30"
    // this matters as those two variants have widely varying semantics, and often occur technically wrong in the wild
    // the other case is "Mo-Fr 06:30-12:00, 13:00-18:00", which should become "Mo-Fr 06:30-12:00,13:00-18:00"

    for (auto it = std::next(m_rules.begin()); it != m_rules.end(); ++it) {
        auto rule = (*it).get();
        auto prevRule = (*(std::prev(it))).get();

        if (rule->m_ruleType != Rule::AdditionalRule) {
            continue;
        }

        const auto prevRuleWeekayOnly = prevRule->m_weekdaySelector && !prevRule->m_yearSelector && !prevRule->m_monthdaySelector && !prevRule->m_weekSelector && !prevRule->m_timeSelector;
        const auto curRuleTimeOnly = rule->m_timeSelector && !rule->m_yearSelector && !rule->m_monthdaySelector && !rule->m_weekSelector && !rule->m_weekdaySelector;
        if (!prevRuleWeekayOnly && !curRuleTimeOnly) {
            continue;
        }

        // the previous rule only has a weekday selector, so we fold that into the current rule
        if (prevRuleWeekayOnly && rule->m_weekdaySelector) {
            auto tmp = std::move(rule->m_weekdaySelector);
            rule->m_weekdaySelector = std::move(prevRule->m_weekdaySelector);
            appendSelector(rule->m_weekdaySelector.get(), std::move(tmp));
            rule->m_ruleType = prevRule->m_ruleType;
            std::swap(*it, *std::prev(it));
            it = std::prev(m_rules.erase(it));
        }

        // the current rule only has a time selector, so we append that to the previous rule
        else if (curRuleTimeOnly && prevRule->m_timeSelector) {
            appendSelector(prevRule->m_timeSelector.get(), std::move(rule->m_timeSelector));
            it = std::prev(m_rules.erase(it));
        }
    }

}

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

void OpeningHoursPrivate::addRule(Rule *rule)
{
    m_rules.push_back(std::unique_ptr<Rule>(rule));
}


OpeningHours::OpeningHours()
    : d(new OpeningHoursPrivate)
{
    d->m_error = OpeningHours::Null;
}

OpeningHours::OpeningHours(const QByteArray &openingHours, Modes modes)
    : d(new OpeningHoursPrivate)
{
    setExpression(openingHours.constData(), openingHours.size(), modes);
}

OpeningHours::OpeningHours(const char *openingHours, std::size_t size, Modes modes)
    : d(new OpeningHoursPrivate)
{
    setExpression(openingHours, size, modes);
}


OpeningHours::OpeningHours(const OpeningHours&) = default;
OpeningHours::OpeningHours(OpeningHours&&) = default;
OpeningHours::~OpeningHours() = default;

OpeningHours& OpeningHours::operator=(const OpeningHours&) = default;
OpeningHours& OpeningHours::operator=(OpeningHours&&) = default;

void OpeningHours::setExpression(const QByteArray &openingHours, OpeningHours::Modes modes)
{
    setExpression(openingHours.constData(), openingHours.size(), modes);
}

void OpeningHours::setExpression(const char *openingHours, std::size_t size, Modes modes)
{
    d->m_modes = modes;

    d->m_error = OpeningHours::Null;
    d->m_rules.clear();

    // trim trailing spaces
    // the parser would handle most of this by itself, but fails if a trailing space would produce a trailing rule separator
    // so it's easier to just clean this here
    while (size > 0 && std::isspace(openingHours[size - 1])) {
        --size;
    }

    yyscan_t scanner;
    if (yylex_init(&scanner)) {
        qCWarning(Log) << "Failed to initialize scanner?!";
        d->m_error = SyntaxError;
        return;
    }
    const auto lexerCleanup = qScopeGuard([&scanner]{ yylex_destroy(scanner); });

    YY_BUFFER_STATE state;
    state = yy_scan_bytes(openingHours, size, scanner);
    if (yyparse(d.data(), scanner)) {
        d->m_error = SyntaxError;
        return;
    }

    yy_delete_buffer(state, scanner);
    d->autocorrect();
    d->validate();

}

QByteArray OpeningHours::normalizedExpression() const
{
    if (d->m_error == SyntaxError) {
        return {};
    }

    QByteArray ret;
    for (const auto &rule : d->m_rules) {
        if (!ret.isEmpty()) {
            switch (rule->m_ruleType) {
                case Rule::NormalRule:
                    ret += "; ";
                    break;
                case Rule::AdditionalRule:
                    ret += ", ";
                    break;
                case Rule::FallbackRule:
                    ret += " || ";
                    break;
            }
        }
        ret += rule->toExpression();
    }
    return ret;
}

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
    d->m_region = HolidayCache::resolveRegion(region);
    d->validate();
}

QTimeZone OpeningHours::timeZone() const
{
    return d->m_timezone;
}

void OpeningHours::setTimeZone(const QTimeZone &tz)
{
    d->m_timezone = tz;
}

QString OpeningHours::timeZoneId() const
{
    return QString::fromUtf8(d->m_timezone.id());
}

void OpeningHours::setTimeZoneId(const QString &tzId)
{
    d->m_timezone = QTimeZone(tzId.toUtf8());
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
        if (rule->state() == Interval::Closed) {
            continue;
        }
        if (i.isValid() && i.contains(dt) && rule->m_ruleType == Rule::FallbackRule) {
            continue;
        }
        auto res = rule->nextInterval(alignedTime, d.data());
        if (!res.interval.isValid()) {
            continue;
        }
        if (i.isValid() && res.mode == RuleResult::Override) {
            if (res.interval.begin().isValid() && res.interval.begin().date() > alignedTime.date()) {
                i = Interval();
                i.setBegin(alignedTime);
                i.setEnd({alignedTime.date().addDays(1), {0, 0}});
                i.setState(Interval::Closed),
                i.setComment({});
            } else {
                i = res.interval;
            }
        } else {
            if (!i.isValid()) {
                i = res.interval;
            } else {
                // fallback rule intervals needs to be capped to the next occurrence of one of its preceding rules
                if (rule->m_ruleType == Rule::FallbackRule) {
                    res.interval.setEnd(res.interval.hasOpenEnd() ? i.begin() : std::min(res.interval.end(), i.begin()));
                }
                i = i.isValid() ? std::min(i, res.interval) : res.interval;
            }
        }
    }

    QDateTime closeEnd = i.begin(), closeBegin = i.end();
    Interval closedInterval;
    for (const auto &rule : d->m_rules) {
        if (rule->state() != Interval::Closed) {
            continue;
        }
        const auto j = rule->nextInterval(i.begin(), d.data()).interval;
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
    i2.setState(Interval::Closed);
    i2.setBegin(dt);
    i2.setEnd(i.begin());
    // TODO do we need to intersect this with closed rules as well?
    return i2;
}

Interval OpeningHours::nextInterval(const Interval &interval) const
{
    if (!interval.hasOpenEnd()) {
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
    r->setState(State::Open);
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
    const auto ohs = obj.value(QLatin1String("openingHoursSpecification")).toArray();
    for (const auto &ohsV : ohs) {
        const auto r = openingHoursSpecToRule(ohsV.toObject());
        if (r) {
            rules.push_back(std::unique_ptr<Rule>(r));
        }
    }
    const auto sohs = obj.value(QLatin1String("specialOpeningHoursSpecification")).toArray();
    for (const auto &ohsV : sohs) {
        const auto r = openingHoursSpecToRule(ohsV.toObject());
        if (r) {
            rules.push_back(std::unique_ptr<Rule>(r));
        }
    }
    for (auto &r : rules) {
        result.d->m_rules.push_back(std::move(r));
    }

    result.d->validate();
    return result;
}
