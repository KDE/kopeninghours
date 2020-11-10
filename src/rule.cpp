/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "rule_p.h"
#include "logging.h"

#include <QCalendar>
#include <QDateTime>

using namespace KOpeningHours;

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
    if (begin.event != Time::NoEvent || end.event != Time::NoEvent) {
        return Capability::Location;
    }
    if (interval > 0) {
        return Capability::NotImplemented;
    }
    return next ? next->requiredCapabilities() : Capability::None;
}

SelectorResult Timespan::nextInterval(const Interval &interval, const QDateTime &dt) const
{
    if (dt.time() < QTime(begin.hour, begin.minute)) {
        return dt.secsTo(QDateTime(dt.date(), {begin.hour, begin.minute}));
    }
    if (dt.time() >= QTime(end.hour, end.minute)) {
        return dt.secsTo(QDateTime(dt.date().addDays(1), {begin.hour, begin.minute}));
    }
    auto i = interval;
    i.setBegin(QDateTime(dt.date(), {begin.hour, begin.minute}));
    i.setEnd(QDateTime(dt.date(), {end.hour, end.minute}));
    return i;
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
    if (nthMask > 1 || offset > 0) { // TODO
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

SelectorResult WeekdayRange::nextInterval(const Interval &interval, const QDateTime &dt) const
{
    // TODO wrap around?
    if (dt.date().dayOfWeek() < beginDay) {
        const auto d = beginDay - dt.date().dayOfWeek();
        return dt.secsTo(QDateTime(dt.date().addDays(d), {0, 0}));
    }
    if (dt.date().dayOfWeek() > endDay) {
        const auto d = 7 + beginDay - dt.date().dayOfWeek();
        return dt.secsTo(QDateTime(dt.date().addDays(d), {0, 0}));
    }

    auto i = interval;
    const auto d = beginDay - dt.date().dayOfWeek();
    i.setBegin(QDateTime(dt.date().addDays(d), {0, 0}));
    i.setEnd(QDateTime(i.begin().date().addDays(endDay - beginDay), {23, 59}));
    return i;
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
    if (interval > 1) {
        return Capability::NotImplemented;
    }
    return next ? next->requiredCapabilities() : Capability::None;
}

SelectorResult Week::nextInterval(const Interval &interval, const QDateTime &dt) const
{
    // TODO is endWeek < beginWeek for year wrap-around allowed?
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
    i.setEnd(QDateTime(i.begin().date().addDays(((endWeek-beginWeek) * 7) + 6), {23, 59}));
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
    if (offset != 0 || begin.variableDate != Date::FixedDate || end.variableDate != Date::FixedDate) {
        return Capability::NotImplemented;
    }
    return next ? next->requiredCapabilities() : Capability::None;
}

static int daysInMonth(int month)
{
    return QCalendar(QCalendar::System::Gregorian).daysInMonth(month);
}

SelectorResult MonthdayRange::nextInterval(const Interval &interval, const QDateTime &dt) const
{
    if (begin.year > 0 && begin.year > dt.date().year()) {
        return dt.secsTo(QDateTime({begin.year, begin.month, std::max<int>(begin.day, 1)}, {0, 0}));
    }
    if (end.year > 0 && end.year < dt.date().year()) {
        return false;
    }
    if (begin.month > dt.date().month()) {
        return dt.secsTo(QDateTime({dt.date().year(), begin.month, std::max<int>(begin.day, 1)}, {0, 0}));
    }
    if (end.month && end.month < dt.date().month()) {
        return dt.secsTo(QDateTime({dt.date().year() + 1, begin.month, std::max<int>(begin.day, 1)}, {0, 0}));
    }
    if (begin.day > 0 && begin.day > dt.date().day()) {
        return dt.secsTo(QDateTime({dt.date().year(), dt.date().month(), begin.day}, {0, 0}));
    }
    if (end.day > 0 && end.day < dt.date().day()) {
        return dt.secsTo(QDateTime({dt.date().year(), dt.date().month(), dt.date().daysInMonth()}, {23, 59})) + 60;
    }

    auto i = interval;
    i.setBegin(QDateTime({begin.year ? begin.year : dt.date().year(), begin.month, begin.day ? begin.day : 1}, {0, 0}));
    // TODO this does not handle year wrapping
    // TODO this does not handle open ended intervals
    i.setEnd(QDateTime({end.year ? end.year : dt.date().year(), end.month, end.day ? end.day : daysInMonth(end.month)}, {23, 59}));
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
    if (interval != 1) {
        return Capability::NotImplemented;
    }
    return next ? next->requiredCapabilities() : Capability::None;
}

SelectorResult YearRange::nextInterval(const Interval &interval, const QDateTime &dt) const
{
    const auto y = dt.date().year();
    if (begin > dt.date().year()) {
        return dt.secsTo(QDateTime({begin, 1, 1}, {0, 0}));
    }
    if (end > 0 && end < y) {
        return false;
    }

    auto i = interval;
    i.setBegin(QDateTime({begin, 1, 1}, {0, 0}));
    i.setEnd(end > 0 ? QDateTime({end, 12, 31}, {23, 59}) : QDateTime());
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

Interval Rule::nextInterval(const QDateTime &dt) const
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
            r = std::min(r, s->nextInterval(i, dt));
        }
        if (!r.canMatch()) {
            return {};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()));
        }
        i = r.interval();
    }

    if (m_monthdaySelector) {
        SelectorResult r;
        for (auto s = m_monthdaySelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt));
        }
        if (!r.canMatch()) {
            return {};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()));
        }
        i = r.interval();
    }

    if (m_weekSelector) {
        SelectorResult r;
        for (auto s = m_weekSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt));
        }
        if (!r.canMatch()) {
            return {};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()));
        }
        i = r.interval();
    }

    if (m_weekdaySelector) {
        SelectorResult r;
        for (auto s = m_weekdaySelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt));
        }
        if (!r.canMatch()) {
            return {};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()));
        }
        i = r.interval();
    }

    if (m_timeSelector) {
        SelectorResult r;
        for (auto s = m_timeSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt));
        }
        if (!r.canMatch()) {
            return {};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()));
        }
        i = r.interval();
    }

    return i;
}
