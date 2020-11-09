/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "rule_p.h"

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
    if (dt.time().hour() > end.hour) { // TODO
        return dt.secsTo(QDateTime(dt.date().addDays(1), {}));
    }
    auto i = interval;
    i.setBegin(QDateTime(interval.begin().date(), {begin.hour, begin.minute}));
    i.setEnd(QDateTime(interval.begin().date(), {end.hour, end.minute}));
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

int WeekdayRange::nextInterval(Interval &interval, const QDateTime &dt) const
{
    if (interval.begin().date().dayOfWeek() <= beginDay && interval.end().date().dayOfWeek() >= endDay) {
        return 0;
    }
    return dt.secsTo(QDateTime(dt.date().addDays(std::abs(beginDay - interval.begin().date().dayOfWeek())), {}));
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
    return Capability::NotImplemented;
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
    return Capability::NotImplemented;
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
    if (interval != 1 || end == 0) {
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
    i.setEnd(QDateTime({end, 12, 31}, {23, 59}));
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
    // ### temporary
    else {
    i.setBegin(QDateTime(dt.date(), {0, 0}));
    i.setEnd(QDateTime(dt.date(), {23, 59}));
    }

    if (m_weekdaySelector) {
        if (const auto offset = m_weekdaySelector->nextInterval(i, dt)) {
            return nextInterval(dt.addSecs(offset));
        }
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
