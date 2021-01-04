/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KHolidays/SunRiseSet>
#include "selectors_p.h"
#include "logging.h"
#include "openinghours_p.h"

#include "easter_p.h"
#include "holidaycache_p.h"

#include <QCalendar>
#include <QDateTime>

using namespace KOpeningHours;

static int daysInMonth(int month)
{
    return QCalendar(QCalendar::System::Gregorian).daysInMonth(month);
}

static QDateTime resolveTime(Time t, QDate date, OpeningHoursPrivate *context)
{
    QDateTime dt;
    switch (t.event) {
        case Time::NoEvent:
            return QDateTime(date, {t.hour % 24, t.minute});
        case Time::Dawn:
            dt = QDateTime(date, KHolidays::SunRiseSet::utcDawn(date, context->m_latitude, context->m_longitude), Qt::UTC).toTimeZone(context->m_timezone);            break;
        case Time::Sunrise:
            dt = QDateTime(date, KHolidays::SunRiseSet::utcSunrise(date, context->m_latitude, context->m_longitude), Qt::UTC).toTimeZone(context->m_timezone);
            break;
        case Time::Dusk:
            dt = QDateTime(date, KHolidays::SunRiseSet::utcDusk(date, context->m_latitude, context->m_longitude), Qt::UTC).toTimeZone(context->m_timezone);
            break;
        case Time::Sunset:
            dt = QDateTime(date, KHolidays::SunRiseSet::utcSunset(date, context->m_latitude, context->m_longitude), Qt::UTC).toTimeZone(context->m_timezone);
            break;
    }

    dt.setTimeSpec(Qt::LocalTime);
    dt = dt.addSecs(t.hour * 3600 + t.minute * 60);
    return dt;
}

bool Timespan::isMultiDay(QDate date, OpeningHoursPrivate *context) const
{
    const auto beginDt = resolveTime(begin, date, context);
    auto endDt = resolveTime(end, date, context);
    if (endDt < beginDt || (end.hour >= 24 && begin.hour < 24)) {
        return true;
    }

    return next ? next->isMultiDay(date, context) : false;
}

SelectorResult Timespan::nextInterval(const Interval &interval, const QDateTime &dt, OpeningHoursPrivate *context) const
{
    const auto beginDt = resolveTime(begin, dt.date(), context);
    auto endDt = resolveTime(end, dt.date(), context);
    if (endDt < beginDt || (end.hour >= 24 && begin.hour < 24)) {
        endDt = endDt.addDays(1);
    }

    if ((dt >= beginDt && dt < endDt) || (beginDt == endDt && beginDt == dt)) {
        auto i = interval;
        i.setBegin(beginDt);
        i.setEnd(endDt);
        i.setOpenEndTime(openEnd);
        return i;
    }

    return dt < beginDt ? dt.secsTo(beginDt) : dt.secsTo(beginDt.addDays(1));
}

static QDate nthWeekday(QDate month, int weekDay, int n)
{
    if (n > 0) {
        const auto firstOfMonth = QDate{month.year(), month.month(), 1};
        const auto delta = (7 + weekDay - firstOfMonth.dayOfWeek()) % 7;
        const auto day = firstOfMonth.addDays(7 * (n - 1) + delta);
        return day.month() == month.month() ? day : QDate();
    } else {
        const auto lastOfMonth = QDate{month.year(), month.month(), daysInMonth(month.month())};
        const auto delta = (7 + lastOfMonth.dayOfWeek() - weekDay) % 7;
        const auto day = lastOfMonth.addDays(7 * (n + 1) - delta);
        return day.month() == month.month() ? day : QDate();
    }
}

SelectorResult WeekdayRange::nextInterval(const Interval &interval, const QDateTime &dt, OpeningHoursPrivate *context) const
{
    SelectorResult r1;
    for (auto s = this; s; s = s->next.get()) {
        r1 = std::min(r1, s->nextIntervalLocal(interval, dt, context));
    }
    if (!andSelector || r1.matchOffset() > 0 || !r1.canMatch()) {
        return r1;
    }

    const auto r2 = andSelector->nextInterval(interval, dt, context);
    if (r2.matchOffset() > 0 || !r2.canMatch()) {
        return r2;
    }

    auto i = r1.interval();
    i.setBegin(std::max(i.begin(), r2.interval().begin()));
    i.setEnd(std::min(i.end(), r2.interval().end()));
    return i;
}

SelectorResult WeekdayRange::nextIntervalLocal(const Interval &interval, const QDateTime &dt, OpeningHoursPrivate *context) const
{
    switch (holiday) {
        case NoHoliday:
        {
            if (nthMask > 0) {
                for (int i = 1; i <= 10; ++i) {
                    if ((nthMask & (1 << i)) == 0) {
                        continue;
                    }
                    const auto n = (i % 2) ? (-5 + (i /2)) : (i / 2);
                    const auto d = nthWeekday(dt.date().addDays(-offset), beginDay, n);
                    if (!d.isValid() || d.addDays(offset) < dt.date()) {
                        continue;
                    }
                    if (d.addDays(offset) == dt.date()) {
                        auto i = interval;
                        i.setBegin(QDateTime(d.addDays(offset), {0, 0}));
                        i.setEnd(QDateTime(d.addDays(offset + 1), {0, 0}));
                        return i;
                    }
                    // d > dt.date()
                    return dt.secsTo(QDateTime(d.addDays(offset), {0, 0}));
                }

                // skip to next month
                return dt.secsTo(QDateTime(dt.date().addDays(dt.date().daysTo({dt.date().year(), dt.date().month(), daysInMonth(dt.date().month())}) + 1 + offset) , {0, 0}));
            }

            if (beginDay <= endDay) {
                if (dt.date().dayOfWeek() < beginDay) {
                    const auto d = beginDay - dt.date().dayOfWeek();
                    return dt.secsTo(QDateTime(dt.date().addDays(d), {0, 0}));
                }
                if (dt.date().dayOfWeek() > endDay) {
                    const auto d = 7 + beginDay - dt.date().dayOfWeek();
                    return dt.secsTo(QDateTime(dt.date().addDays(d), {0, 0}));
                }
            } else {
                if (dt.date().dayOfWeek() < beginDay && dt.date().dayOfWeek() > endDay) {
                    const auto d = beginDay - dt.date().dayOfWeek();
                    return dt.secsTo(QDateTime(dt.date().addDays(d), {0, 0}));
                }
            }

            auto i = interval;
            const auto d = beginDay - dt.date().dayOfWeek();
            i.setBegin(QDateTime(dt.date().addDays(d), {0, 0}));
            i.setEnd(QDateTime(i.begin().date().addDays(1 + (beginDay <= endDay ? endDay - beginDay : 7 - (beginDay - endDay))), {0, 0}));
            return i;
        }
        case PublicHoliday:
        {
            const auto h = HolidayCache::nextHoliday(context->m_region, dt.date().addDays(-offset));
            if (h.name().isEmpty()) {
                return false;
            }
            if (dt.date() < h.observedStartDate().addDays(offset)) {
                return dt.secsTo(QDateTime(h.observedStartDate().addDays(offset), {0, 0}));
            }

            auto i = interval;
            i.setBegin(QDateTime(h.observedStartDate().addDays(offset), {0, 0}));
            i.setEnd(QDateTime(h.observedEndDate().addDays(1).addDays(offset), {0, 0}));
            if (i.comment().isEmpty() && offset == 0) {
                i.setComment(h.name());
            }
            return i;
        }
        case SchoolHoliday:
            Q_UNREACHABLE();
    }
    return {};
}

SelectorResult Week::nextInterval(const Interval &interval, const QDateTime &dt, OpeningHoursPrivate *context) const
{
    Q_UNUSED(context);
    if (dt.date().weekNumber() < beginWeek) {
        const auto days = (7 - dt.date().dayOfWeek()) + 7 * (beginWeek - dt.date().weekNumber() - 1) + 1;
        return dt.secsTo(QDateTime(dt.date().addDays(days), {0, 0}));
    }
    if (dt.date().weekNumber() > endWeek) {
        // "In accordance with ISO 8601, weeks start on Monday and the first Thursday of a year is always in week 1 of that year."
        auto d = QDateTime({dt.date().year() + 1, 1, 1}, {0, 0});
        while (d.date().weekNumber() != 1) {
            d = d.addDays(1);
        }
        return dt.secsTo(d);
    }

    if (this->interval > 1) {
        const int wd = (dt.date().weekNumber() - beginWeek) % this->interval;
        if (wd) {
            const auto days = (7 - dt.date().dayOfWeek()) + 7 * (this->interval - wd - 1) + 1;
            return dt.secsTo(QDateTime(dt.date().addDays(days), {0, 0}));
        }
    }

    auto i = interval;
    if (this->interval > 1) {
        i.setBegin(QDateTime(dt.date().addDays(1 - dt.date().dayOfWeek()), {0, 0}));
        i.setEnd(QDateTime(i.begin().date().addDays(7), {0, 0}));
    } else {
        i.setBegin(QDateTime(dt.date().addDays(1 - dt.date().dayOfWeek() - 7 * (dt.date().weekNumber() - beginWeek)), {0, 0}));
        i.setEnd(QDateTime(i.begin().date().addDays((1 + endWeek - beginWeek) * 7), {0, 0}));
    }
    return i;
}

static QDate resolveDate(Date d, int year)
{
    QDate date;
    switch (d.variableDate) {
        case Date::FixedDate:
            date = {d.year ? d.year : year, d.month ? d.month : 1, d.day ? d.day : 1};
            break;
        case Date::Easter:
            date = Easter::easterDate(d.year ? d.year : year);
            break;
    }

    if (d.offset.weekday && d.offset.nthWeekday) {
        date = nthWeekday(date, d.offset.weekday, d.offset.nthWeekday);
    }
    date = date.addDays(d.offset.dayOffset);

    return date;
}

static QDate resolveDateEnd(Date d, int year)
{
    auto date = resolveDate(d, year);
    if (d.variableDate == Date::FixedDate) {
        if (!d.day && !d.month) {
            return date.addYears(1);
        } else if (!d.day) {
            return date.addDays(daysInMonth(d.month));
        }
    }
    return date.addDays(1);
}

SelectorResult MonthdayRange::nextInterval(const Interval &interval, const QDateTime &dt, OpeningHoursPrivate *context) const
{
    Q_UNUSED(context);
    auto beginDt = resolveDate(begin, dt.date().year());
    auto endDt = resolveDateEnd(end, dt.date().year());

    // note that for any of the following we cannot just do addYears(1), as that will break
    // for leap years. instead we have to recompute the date again for each year
    if (endDt < beginDt || (endDt <= beginDt && begin != end)) {
        // month range wraps over the year boundary
        endDt = resolveDateEnd(end, dt.date().year() + 1);
    }

    if (end.year && dt.date() >= endDt) {
        return false;
    }

    // if the current range is in the future, check if we are still in the previous one
    if (dt.date() < beginDt && end.month < begin.month) {
        auto lookbackBeginDt = resolveDate(begin, dt.date().year() - 1);
        auto lookbackEndDt = resolveDateEnd(end, dt.date().year() - 1);
        if (lookbackEndDt < lookbackEndDt || (lookbackEndDt <= lookbackBeginDt && begin != end)) {
            lookbackEndDt = resolveDateEnd(end, dt.date().year());
        }
        if (lookbackEndDt >= dt.date()) {
            beginDt = lookbackBeginDt;
            endDt = lookbackEndDt;
        }
    }

    if (dt.date() >= endDt) {
        beginDt = resolveDate(begin, dt.date().year() + 1);
        endDt = resolveDateEnd(end, dt.date().year() + 1);
    }

    if (dt.date() < beginDt) {
        return dt.secsTo(QDateTime(beginDt, {0, 0}));
    }

    auto i = interval;
    i.setBegin(QDateTime(beginDt, {0, 0}));
    i.setEnd(QDateTime(endDt, {0, 0}));
    return i;
}

SelectorResult YearRange::nextInterval(const Interval &interval, const QDateTime &dt, OpeningHoursPrivate *context) const
{
    Q_UNUSED(context);
    const auto y = dt.date().year();
    if (begin > y) {
        return dt.secsTo(QDateTime({begin, 1, 1}, {0, 0}));
    }
    if (end > 0 && end < y) {
        return false;
    }

    if (this->interval > 1) {
        const int yd = (y - begin) % this->interval;
        if (yd) {
            return dt.secsTo(QDateTime({y + (this->interval - yd), 1, 1}, {0, 0}));
        }
    }

    auto i = interval;
    if (this->interval > 1) {
        i.setBegin(QDateTime({y, 1, 1}, {0, 0}));
        i.setEnd(QDateTime({y + 1, 1, 1}, {0, 0}));
    } else {
        i.setBegin(QDateTime({begin, 1, 1}, {0, 0}));
        i.setEnd(end > 0 ? QDateTime({end + 1, 1, 1}, {0, 0}) : QDateTime());
    }
    return i;
}

RuleResult Rule::nextInterval(const QDateTime &dt, OpeningHoursPrivate *context) const
{
    // handle time selectors spanning midnight
    // consider e.g. "Tu 12:00-12:00" being evaluated with dt being Wednesday 08:00
    // we need to look one day back to find a matching day selector and the correct start
    // of the interval here
    if (m_timeSelector && m_timeSelector->isMultiDay(dt.date(), context)) {
        const auto res = nextInterval(dt.addDays(-1), context, RecursionLimit);
        if (res.interval.contains(dt)) {
            return res;
        }
    }

    return nextInterval(dt, context, RecursionLimit);
}

RuleResult Rule::nextInterval(const QDateTime &dt, OpeningHoursPrivate *context, int recursionBudget) const
{
    const auto resultMode = (recursionBudget == Rule::RecursionLimit && m_ruleType == NormalRule && state() != Interval::Closed) ? RuleResult::Override : RuleResult::Merge;

    if (recursionBudget == 0) {
        context->m_error = OpeningHours::EvaluationError;
        qCWarning(Log) << "Recursion limited reached!";
        return {{}, resultMode};
    }

    Interval i;
    i.setState(state());
    i.setComment(m_comment);
    if (!m_timeSelector && !m_weekdaySelector && !m_monthdaySelector && !m_weekSelector && !m_yearSelector) {
        // 24/7 has no selectors
        return {i, resultMode};
    }

    if (m_yearSelector) {
        SelectorResult r;
        for (auto s = m_yearSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {{}, resultMode};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context, --recursionBudget);
        }
        i = r.interval();
    }

    if (m_monthdaySelector) {
        SelectorResult r;
        for (auto s = m_monthdaySelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {{}, resultMode};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context, --recursionBudget);
        }
        i = r.interval();
    }

    if (m_weekSelector) {
        SelectorResult r;
        for (auto s = m_weekSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {{}, resultMode};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context, --recursionBudget);
        }
        i = r.interval();
    }

    if (m_weekdaySelector) {
        SelectorResult r = m_weekdaySelector->nextInterval(i, dt, context);
        if (!r.canMatch()) {
            return {{}, resultMode};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context, --recursionBudget);
        }
        i = r.interval();
    }

    if (m_timeSelector) {
        SelectorResult r;
        for (auto s = m_timeSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {{}, resultMode};
        }
        if (r.matchOffset() > 0) {
            return {nextInterval(dt.addSecs(r.matchOffset()), context, --recursionBudget).interval, resultMode};
        }
        i = r.interval();
    }

    return {i, resultMode};
}
