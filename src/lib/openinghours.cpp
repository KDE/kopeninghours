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
#include "consecutiveaccumulator_p.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimeZone>

#include <memory>

using namespace KOpeningHours;

static bool isWiderThan(Rule *lhs, Rule *rhs)
{
    if ((lhs->m_yearSelector && !rhs->m_yearSelector)) {
        return true;
    }
    if (lhs->m_monthdaySelector && rhs->m_monthdaySelector) {
        if (lhs->m_monthdaySelector->begin.year > 0 && rhs->m_monthdaySelector->end.year == 0) {
            return true;
        }
    }

    // this is far from handling all cases, expand as needed
    return false;
}

void OpeningHoursPrivate::autocorrect()
{
    if (m_rules.size() <= 1 || m_error == OpeningHours::SyntaxError) {
        return;
    }

    // find incomplete additional rules, and merge them with the preceding rule
    // example: "Mo, We, Fr 06:30-21:30" becomes "Mo,We,Fr 06:30-21:30"
    // this matters as those two variants have widely varying semantics, and often occur technically wrong in the wild
    // the other case is "Mo-Fr 06:30-12:00, 13:00-18:00", which should become "Mo-Fr 06:30-12:00,13:00-18:00"

    for (auto it = std::next(m_rules.begin()); it != m_rules.end(); ++it) {
        auto rule = (*it).get();
        auto prevRule = (*(std::prev(it))).get();

        if (rule->hasComment() || prevRule->hasComment() || !prevRule->hasImplicitState()) {
            continue;
        }
        const auto prevRuleSingleSelector = prevRule->selectorCount() == 1;
        const auto curRuleSingleSelector = rule->selectorCount() == 1;

        if (rule->m_ruleType == Rule::AdditionalRule) {
            // the previous rule has no time selector, the current rule only has a weekday selector
            // so we fold the two rules together
            if (!prevRule->m_timeSelector && prevRule->m_weekdaySelector && rule->m_weekdaySelector && !rule->hasWideRangeSelector()) {
                auto tmp = std::move(rule->m_weekdaySelector);
                rule->m_weekdaySelector = std::move(prevRule->m_weekdaySelector);
                rule->m_weekSelector = std::move(prevRule->m_weekSelector);
                rule->m_monthdaySelector = std::move(prevRule->m_monthdaySelector);
                rule->m_yearSelector = std::move(prevRule->m_yearSelector);
                rule->m_colonAfterWideRangeSelector = prevRule->m_colonAfterWideRangeSelector;
                auto *selector = rule->m_weekdaySelector.get();
                while (selector->rhsAndSelector)
                    selector = selector->rhsAndSelector.get();
                appendSelector(selector, std::move(tmp));
                rule->m_ruleType = prevRule->m_ruleType;
                std::swap(*it, *std::prev(it));
                it = std::prev(m_rules.erase(it));
            }

            // the current rule only has a time selector, so we append that to the previous rule
            else if (curRuleSingleSelector && rule->m_timeSelector && prevRule->m_timeSelector) {
                appendSelector(prevRule->m_timeSelector.get(), std::move(rule->m_timeSelector));
                prevRule->copyStateFrom(*rule);
                it = std::prev(m_rules.erase(it));
            }

            // previous is a single weekday selector and current is a single time selector
            else if (curRuleSingleSelector && prevRuleSingleSelector && rule->m_timeSelector && prevRule->m_weekdaySelector) {
                prevRule->m_timeSelector = std::move(rule->m_timeSelector);
                it = std::prev(m_rules.erase(it));
            }

            // previous is a single monthday selector
            else if (rule->m_monthdaySelector && prevRuleSingleSelector && prevRule->m_monthdaySelector && !isWiderThan(prevRule, rule)) {
                auto tmp = std::move(rule->m_monthdaySelector);
                rule->m_monthdaySelector = std::move(prevRule->m_monthdaySelector);
                appendSelector(rule->m_monthdaySelector.get(), std::move(tmp));
                rule->m_ruleType = prevRule->m_ruleType;
                std::swap(*it, *std::prev(it));
                it = std::prev(m_rules.erase(it));
            }

            // previous has no time selector and the current one is a misplaced 24/7 rule:
            // convert the 24/7 to a 00:00-24:00 time selector
            else if (rule->selectorCount() == 0 && rule->m_seen_24_7 && !prevRule->m_timeSelector) {
                prevRule->m_timeSelector.reset(new Timespan);
                prevRule->m_timeSelector->begin = { Time::NoEvent, 0, 0 };
                prevRule->m_timeSelector->end = { Time::NoEvent, 24, 0 };
                it = std::prev(m_rules.erase(it));
            }
        } else if (rule->m_ruleType == Rule::NormalRule) {
            // Previous rule has time and other selectors
            // Current rule is only a time selector
            // "Mo-Sa 12:00-15:00; 18:00-24:00" => "Mo-Sa 12:00-15:00,18:00-24:00"
            if (curRuleSingleSelector && rule->m_timeSelector
                    && prevRule->selectorCount() > 1 && prevRule->m_timeSelector
                    && rule->state() == prevRule->state()) {
                appendSelector(prevRule->m_timeSelector.get(), std::move(rule->m_timeSelector));
                it = std::prev(m_rules.erase(it));
            }

            // Both rules have exactly the same selector apart from time
            // Ex: "Mo-Sa 12:00-15:00; Mo-Sa 18:00-24:00" => "Mo-Sa 12:00-15:00,18:00-24:00"
            // Obviously a bug, it was overwriting the 12:00-15:00 range.
            // For now this only supports weekday selectors, could be extended
            else if (rule->selectorCount() == prevRule->selectorCount()
                     && rule->m_timeSelector && prevRule->m_timeSelector
                     && !rule->hasComment() && !prevRule->hasComment()
                     && rule->selectorCount() == 2 && rule->m_weekdaySelector && prevRule->m_weekdaySelector
                     // slower than writing an operator==, but so much easier to write :)
                     && rule->m_weekdaySelector->toExpression() == prevRule->m_weekdaySelector->toExpression()
                     && rule->state() == prevRule->state()
                     ) {
                appendSelector(prevRule->m_timeSelector.get(), std::move(rule->m_timeSelector));
                it = std::prev(m_rules.erase(it));
            }
        }
    }
}

void OpeningHoursPrivate::simplify()
{
    if (m_error == OpeningHours::SyntaxError || m_rules.empty()) {
        return;
    }

    for (auto it = std::next(m_rules.begin()); it != m_rules.end(); ++it) {
        auto rule = (*it).get();
        auto prevRule = (*(std::prev(it))).get();

        if (rule->m_ruleType == Rule::AdditionalRule || rule->m_ruleType == Rule::NormalRule) {

            auto hasNoHoliday = [](WeekdayRange *selector) {
                return selector->holiday == WeekdayRange::NoHoliday
                        && !selector->lhsAndSelector;
            };
            // Both rules have the same time and a different weekday selector
            // Mo 08:00-13:00; Tu 08:00-13:00 => Mo,Tu 08:00-13:00
            if (rule->selectorCount() == prevRule->selectorCount()
                    && rule->m_timeSelector && prevRule->m_timeSelector
                    && rule->selectorCount() == 2 && rule->m_weekdaySelector && prevRule->m_weekdaySelector
                    && hasNoHoliday(rule->m_weekdaySelector.get())
                    && hasNoHoliday(prevRule->m_weekdaySelector.get())
                    && *rule->m_timeSelector == *prevRule->m_timeSelector
                    ) {
                // We could of course also turn Mo,Tu,We,Th into Mo-Th...
                appendSelector(prevRule->m_weekdaySelector.get(), std::move(rule->m_weekdaySelector));
                it = std::prev(m_rules.erase(it));
                continue;
            }
        }

        if (rule->m_ruleType == Rule::AdditionalRule) {
            // Both rules have exactly the same selector apart from time
            // Ex: "Mo 12:00-15:00, Mo 18:00-24:00" => "Mo 12:00-15:00,18:00-24:00"
            // For now this only supports weekday selectors, could be extended
            if (rule->selectorCount() == prevRule->selectorCount()
                    && rule->m_timeSelector && prevRule->m_timeSelector
                    && !rule->hasComment() && !prevRule->hasComment()
                    && rule->selectorCount() == 2 && rule->m_weekdaySelector && prevRule->m_weekdaySelector
                    // slower than writing an operator==, but so much easier to write :)
                    && rule->m_weekdaySelector->toExpression() == prevRule->m_weekdaySelector->toExpression()
                    ) {
                appendSelector(prevRule->m_timeSelector.get(), std::move(rule->m_timeSelector));
                it = std::prev(m_rules.erase(it));
            }
        }
    }

    // Now try collapsing adjacent week days: Mo,Tu,We => Mo-We
    for (auto it = m_rules.begin(); it != m_rules.end(); ++it) {
        auto rule = (*it).get();
        if (rule->m_weekdaySelector) {
            rule->m_weekdaySelector->simplify();
        }
        if (rule->m_monthdaySelector) {
            rule->m_monthdaySelector->simplify();
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
#ifndef KOPENINGHOURS_VALIDATOR_ONLY
    if (c & Capability::PublicHoliday && !m_region.isValid()) {
        m_error = OpeningHours::MissingRegion;
        return;
    }
#endif
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

void OpeningHoursPrivate::addRule(Rule *parsedRule)
{
    std::unique_ptr<Rule> rule(parsedRule);

    // discard empty rules
    if (rule->isEmpty()) {
        return;
    }

    if (m_initialRuleType != Rule::NormalRule && rule->m_ruleType == Rule::NormalRule) {
        rule->m_ruleType = m_initialRuleType;
        m_initialRuleType = Rule::NormalRule;
    }

    // error recovery after a missing rule separator
    // only continue here if whatever we got is somewhat plausible
    if (m_ruleSeparatorRecovery && !m_rules.empty()) {
        if (rule->selectorCount() <= 1) {
            // missing separator was actually between time selectors, not rules
            if (m_rules.back()->m_timeSelector && rule->m_timeSelector && m_rules.back()->state() == rule->state()) {
                appendSelector(m_rules.back()->m_timeSelector.get(), std::move(rule->m_timeSelector));
                rule.reset();
                return;
            } else {
                m_error = OpeningHours::SyntaxError;
            }
        }

        // error recovery in the middle of a wide-range selector
        // the likely meaning is that the wide-range selectors should be merged, which we can only do if the first
        // part is "wider" than the right hand side
        if (m_rules.back()->hasWideRangeSelector() && rule->hasWideRangeSelector()
            && !m_rules.back()->hasSmallRangeSelector() && rule->hasSmallRangeSelector()
            && isWiderThan(rule.get(), m_rules.back().get()))
        {
            m_error = OpeningHours::SyntaxError;
        }

        // error recovery in case of a wide range selector followed by two wrongly separated small range selectors
        // the likely meaning here is that the wide range selector should apply to both small range selectors,
        // but that cannot be modeled without duplicating the wide range selector
        // therefore we consider such a case invalid, to be on the safe side
        if (m_rules.back()->hasWideRangeSelector() && !rule->hasWideRangeSelector()) {
            m_error = OpeningHours::SyntaxError;
        }
    }

    m_ruleSeparatorRecovery = false;
    m_rules.push_back(std::move(rule));
}

void OpeningHoursPrivate::restartFrom(int pos, Rule::Type nextRuleType)
{
    m_restartPosition = pos;
    if (nextRuleType == Rule::GuessRuleType) {
        if (m_rules.empty()) {
            m_recoveryRuleType = Rule::NormalRule;
        } else {
            // if autocorrect() could merge the previous rule, we assume that's the intended meaning
            const auto &prev = m_rules.back();
            const auto couldBeMerged = prev->selectorCount() == 1 && !prev->hasComment() && prev->hasImplicitState();
            m_recoveryRuleType = couldBeMerged ? Rule::AdditionalRule : Rule::NormalRule;
        }
    } else {
        m_recoveryRuleType = nextRuleType;
    }
}

bool OpeningHoursPrivate::isRecovering() const
{
    return m_restartPosition > 0;
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
    d->m_initialRuleType = Rule::NormalRule;
    d->m_recoveryRuleType = Rule::NormalRule;
    d->m_ruleSeparatorRecovery = false;

    // trim trailing spaces
    // the parser would handle most of this by itself, but fails if a trailing space would produce a trailing rule separator
    // so it's easier to just clean this here
    while (size > 0 && std::isspace(openingHours[size - 1])) {
        --size;
    }
    if (size == 0) {
        return;
    }

    d->m_restartPosition = 0;
    int offset = 0;
    do {
        yyscan_t scanner;
        if (yylex_init(&scanner)) {
            qCWarning(Log) << "Failed to initialize scanner?!";
            d->m_error = SyntaxError;
            return;
        }
        const std::unique_ptr<void, decltype(&yylex_destroy)> lexerCleanup(scanner, &yylex_destroy);

        YY_BUFFER_STATE state;
        state = yy_scan_bytes(openingHours + offset, size - offset, scanner);
        if (yyparse(d.data(), scanner)) {
            if (d->m_restartPosition > 1 && d->m_restartPosition + offset < (int)size) {
                offset += d->m_restartPosition - 1;
                d->m_initialRuleType = d->m_recoveryRuleType;
                d->m_recoveryRuleType = Rule::NormalRule;
                d->m_restartPosition = 0;
            } else {
                d->m_error = SyntaxError;
                return;
            }
            d->m_error = NoError;
        } else {
            if (d->m_error != SyntaxError) {
                d->m_error = NoError;
            }
            offset = -1;
        }

        yy_delete_buffer(state, scanner);
    } while (offset > 0);

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
                case Rule::GuessRuleType:
                    Q_UNREACHABLE();
                    break;
            }
        }
        ret += rule->toExpression();
    }
    return ret;
}

QByteArray OpeningHours::simplifiedExpression() const
{
    d->simplify();
    return normalizedExpression();
}

QString OpeningHours::normalizedExpressionString() const
{
    return QString::fromUtf8(normalizedExpression());
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

#ifndef KOPENINGHOURS_VALIDATOR_ONLY
QString OpeningHours::region() const
{
    return d->m_region.regionCode();
}

void OpeningHours::setRegion(QStringView region)
{
    d->m_region = HolidayCache::resolveRegion(region);
    d->validate();
}
#endif

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

#ifndef KOPENINGHOURS_VALIDATOR_ONLY
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
        auto endDt = interval.end();
        // ensure we move forward even on zero-length open-end intervals, otherwise we get stuck in a loop
        if (interval.hasOpenEndTime() && interval.begin() == interval.end()) {
            endDt = endDt.addSecs(3600);
        }
        auto i = this->interval(endDt);
        if (i.begin() < interval.end() && i.end() > interval.end()) {
            i.setBegin(interval.end());
        }
        return i;
    }
    return {};
}
#endif

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
        r->m_monthdaySelector->begin = { validFrom.year(), validFrom.month(), validFrom.day(), Date::FixedDate, { 0, 0, 0 } };
        r->m_monthdaySelector->end = { validTo.year(), validTo.month(), validTo.day(), Date::FixedDate, { 0, 0, 0 } };
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
