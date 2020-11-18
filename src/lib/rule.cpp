/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "rule_p.h"
#include "easter_p.h"
#include "logging.h"
#include "openinghours_p.h"

#include <KHolidays/SunRiseSet>

#include <QCalendar>
#include <QDateTime>

using namespace KOpeningHours;

static int daysInMonth(int month)
{
    return QCalendar(QCalendar::System::Gregorian).daysInMonth(month);
}

QDebug operator<<(QDebug debug, const Time &time)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << time.hour << ':' << time.minute;
    if (time.event != Time::NoEvent) {
        debug.nospace() << "[Event" << time.event << ']';
    }
    return debug;
}

int Timespan::requiredCapabilities() const
{
    if (interval > 0 || begin == end) {
        return Capability::NotImplemented;
    }
    if (begin.event != Time::NoEvent || end.event != Time::NoEvent) {
        return Capability::Location;
    }
    return next ? next->requiredCapabilities() : Capability::None;
}

static QDateTime resolveTime(const Time &t, const QDate &date, OpeningHoursPrivate *context)
{
    QDateTime dt;
    switch (t.event) {
        case Time::NoEvent:
            return QDateTime(date, {t.hour % 24, t.minute});
        // TODO we probably want an explicit timezone selection as well, for evaluating this in other locations
        case Time::Dawn:
            dt = QDateTime(date, KHolidays::SunRiseSet::utcDawn(date, context->m_latitude, context->m_longitude), Qt::UTC).toLocalTime();
            break;
        case Time::Sunrise:
            dt = QDateTime(date, KHolidays::SunRiseSet::utcSunrise(date, context->m_latitude, context->m_longitude), Qt::UTC).toLocalTime();
            break;
        case Time::Dusk:
            dt = QDateTime(date, KHolidays::SunRiseSet::utcDusk(date, context->m_latitude, context->m_longitude), Qt::UTC).toLocalTime();
            break;
        case Time::Sunset:
            dt = QDateTime(date, KHolidays::SunRiseSet::utcSunset(date, context->m_latitude, context->m_longitude), Qt::UTC).toLocalTime();
            break;
    }

    dt = dt.addSecs(t.hour * 3600 + t.minute * 60);
    return dt;
}

SelectorResult Timespan::nextInterval(const Interval &interval, const QDateTime &dt, OpeningHoursPrivate *context) const
{
    const auto beginDt = resolveTime(begin, dt.date(), context);
    auto endDt = resolveTime(end, dt.date(), context);
    if (endDt < beginDt || (end.hour >= 24 && begin.hour < 24)) {
        endDt = endDt.addDays(1);
    }

    if (dt >= beginDt && dt < endDt) {
        auto i = interval;
        i.setBegin(beginDt);
        i.setEnd(endDt);
        return i;
    }

    if (dt < endDt.addDays(-1)) {
        auto i = interval;
        i.setBegin(beginDt.addDays(-1));
        i.setEnd(endDt.addDays(-1));
        return i;
    }

    return dt < beginDt ? dt.secsTo(beginDt) : dt.secsTo(beginDt.addDays(1));
}

QDebug operator<<(QDebug debug, const Timespan *timeSpan)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "T " << timeSpan->begin << '-' << timeSpan->end << '/' << timeSpan->interval;
    if (timeSpan->next) {
        debug.nospace() << ", " << timeSpan->next.get();
    }
    return debug;
}

int WeekdayRange::requiredCapabilities() const
{
    // only ranges or nthMask are allowed, not both at the same time, enforced by parser
    assert(beginDay == endDay || nthMask == 0);

    if (offset > 0) { // TODO
        return Capability::NotImplemented;
    }

    switch (holiday) {
        case NoHoliday:
            return (next ? next->requiredCapabilities() : Capability::None) | (next2 ? next2->requiredCapabilities() : Capability::None);
        case PublicHoliday:
            return Capability::PublicHoliday;
        case SchoolHoliday:
            return Capability::SchoolHoliday;
    }
    return Capability::NotImplemented;
}

static QDate nthWeekday(const QDate &month, int weekDay, int n)
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
    switch (holiday) {
        case NoHoliday:
        {
            if (nthMask > 0) {
                for (int i = 1; i <= 10; ++i) {
                    if ((nthMask & (1 << i)) == 0) {
                        continue;
                    }
                    const auto n = (i % 2) ? (-5 + (i /2)) : (i / 2);
                    const auto d = nthWeekday(dt.date(), beginDay, n);
                    if (!d.isValid() || d < dt.date()) {
                        continue;
                    }
                    if (d == dt.date()) {
                        auto i = interval;
                        i.setBegin(QDateTime(d, {0, 0}));
                        i.setEnd(QDateTime(d.addDays(1), {0, 0}));
                        return i;
                    }
                    // d > dt.date()
                    return dt.secsTo(QDateTime(d, {0, 0}));
                }

                // skip to next month
                return dt.secsTo(QDateTime(dt.date().addDays(dt.date().daysTo({dt.date().year(), dt.date().month(), daysInMonth(dt.date().month())}) + 1) , {0, 0}));
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
            auto holidays = context->m_region.holidays(dt.date(), dt.date().addYears(1));
            holidays.erase(std::remove_if(holidays.begin(), holidays.end(), [](const auto &h) { return h.dayType() != KHolidays::Holiday::NonWorkday; }), holidays.end());
            std::sort(holidays.begin(), holidays.end(), [](const auto &lhs, const auto &rhs) { return lhs.observedStartDate() < rhs.observedStartDate(); });
            if (holidays.empty()) {
                return false;
            }

            const auto h = holidays.at(0);
            if (dt.date() < h.observedStartDate()) {
                return dt.secsTo(QDateTime(h.observedStartDate(), {0, 0}));
            }

            auto i = interval;
            i.setBegin(QDateTime(h.observedStartDate(), {0, 0}));
            i.setEnd(QDateTime(h.observedEndDate().addDays(1), {0, 0}));
            return i;
        }
        case SchoolHoliday:
            Q_UNREACHABLE();
    }
    return {};
}

QDebug operator<<(QDebug debug, const WeekdayRange *weekdayRange)
{
    debug << "WD" << weekdayRange->beginDay << weekdayRange->endDay << weekdayRange->nthMask << weekdayRange->offset << weekdayRange->holiday;
    if (weekdayRange->next) {
        debug << "  " << weekdayRange->next.get();
    }
    if (weekdayRange->next2) {
        debug << "  " << weekdayRange->next2.get();
    }
    return debug;
}

int Week::requiredCapabilities() const
{
    if (interval > 1 || endWeek < beginWeek) {
        return Capability::NotImplemented;
    }
    return next ? next->requiredCapabilities() : Capability::None;
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

    auto i = interval;
    i.setBegin(QDateTime(dt.date().addDays(1 - dt.date().dayOfWeek() - 7 * (dt.date().weekNumber() - beginWeek)), {0, 0}));
    // TODO year wrap around
    i.setEnd(QDateTime(i.begin().date().addDays((1 + endWeek - beginWeek) * 7), {0, 0}));
    return i;
}

QDebug operator<<(QDebug debug, const Week *week)
{
    debug.nospace() << "W " << week->beginWeek << '-' << week->endWeek << '/' << week->interval;
    if (week->next) {
        debug << ", " << week->next.get();
    }
    return debug;
}

QDebug operator<<(QDebug debug, const Date &date)
{
    switch (date.variableDate) {
        case Date::FixedDate:
            debug.nospace() << date.year << '-' << date.month << '-' << date.day;
            break;
        case Date::Easter:
            debug << "easter";
            break;
    }
    return debug;
}

int MonthdayRange::requiredCapabilities() const
{
    if (begin.weekdayOffset != 0 || end.weekdayOffset != 0) {
        return Capability::NotImplemented;
    }
    return next ? next->requiredCapabilities() : Capability::None;
}

static QDate resolveDate(Date d, int year)
{
    QDate date;
    switch (d.variableDate) {
        case Date::FixedDate:
            date = {d.year ? d.year : year, d.month, d.day ? d.day : 1};
            break;
        case Date::Easter:
            date = Easter::easterDate(d.year ? d.year : year);
            break;
    }
    date = date.addDays(d.dayOffset);

    // TODO consider weekday offsets
    return date;
}

static QDate resolveDateEnd(Date d, int year)
{
    auto date = resolveDate(d, year);
    if (!d.day && d.variableDate == Date::FixedDate) {
        return date.addDays(daysInMonth(d.month));
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
    if (endDt < beginDt) {
        // month range wraps over the year boundary
        endDt = resolveDateEnd(end, dt.date().year() + 1);
    }

    if (end.year && dt.date() >= endDt) {
        return false;
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

QDebug operator<<(QDebug debug, const MonthdayRange *monthdayRange)
{
    debug.nospace() << "M " << monthdayRange->begin << '-' << monthdayRange->end;
    if (monthdayRange->next) {
        debug << ", " << monthdayRange->next.get();
    }
    return debug;
}

int YearRange::requiredCapabilities() const
{
    return Capability::None;
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

QDebug operator<<(QDebug debug, const YearRange *yearRange)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Y " << yearRange->begin << '-' << yearRange->end << '/' << yearRange->interval;
    if (yearRange->next) {
        debug << ", " << yearRange->next.get();
    }
    return debug;
}

void Rule::setComment(const char *str, int len)
{
    m_comment = QString::fromUtf8(str, len);
}

int Rule::requiredCapabilities() const
{
    int c = Capability::None;
    c |= m_timeSelector ? m_timeSelector->requiredCapabilities() : Capability::None;
    c |= m_weekdaySelector ? m_weekdaySelector->requiredCapabilities() : Capability::None;
    c |= m_weekSelector ? m_weekSelector->requiredCapabilities() : Capability::None;
    c |= m_monthdaySelector ? m_monthdaySelector->requiredCapabilities() : Capability::None;
    c |= m_yearSelector ? m_yearSelector->requiredCapabilities() : Capability::None;

    return c;
}

Interval Rule::nextInterval(const QDateTime &dt, OpeningHoursPrivate *context) const
{
    qCDebug(Log) << dt;

    Interval i;
    i.setState(m_state);
    i.setComment(m_comment);
    if (!m_timeSelector && !m_weekdaySelector && !m_monthdaySelector && !m_weekSelector && !m_yearSelector) {
        // 24/7 has no selectors
        return i;
    }

    if (m_yearSelector) {
        SelectorResult r;
        for (auto s = m_yearSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context);
        }
        i = r.interval();
    }

    if (m_monthdaySelector) {
        SelectorResult r;
        for (auto s = m_monthdaySelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context);
        }
        i = r.interval();
    }

    if (m_weekSelector) {
        SelectorResult r;
        for (auto s = m_weekSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context);
        }
        i = r.interval();
    }

    if (m_weekdaySelector) {
        SelectorResult r;
        for (auto s = m_weekdaySelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context);
        }
        i = r.interval();
    }

    if (m_timeSelector) {
        SelectorResult r;
        for (auto s = m_timeSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context);
        }
        i = r.interval();
    }

    return i;
}
