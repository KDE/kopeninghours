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
    return next ? next->requiredCapabilities() : Capability::None;
}

int Timespan::nextInterval(Interval &interval, const QDateTime &dt) const
{
    if (dt.time().hour() > end.hour) { // TODO
        return dt.secsTo(QDateTime(dt.date().addDays(1), {}));
    }
    interval.setBegin(QDateTime(interval.begin().date(), {begin.hour, begin.minute}));
    interval.setEnd(QDateTime(interval.begin().date(), {end.hour, end.minute}));
    return 0;
}

QDebug operator<<(QDebug debug, Timespan *timeSpan)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "T " << timeSpan->begin << '-' << timeSpan->end;
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

QDebug operator<<(QDebug debug, Week *week)
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
    return Capability::NotImplemented;
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
    c |= m_yearSelector ? m_yearSelector->requiredCapabilities() : Capability::None;

    return c;
}

Interval Rule::nextInterval(const QDateTime &dt) const
{
    Interval i;
    i.setState(m_state);
    i.setComment(m_comment);
    if (!m_timeSelector && !m_weekdaySelector && !m_monthdaySelector && !m_weekSelector) {
        // 24/7 has no selectors
        return i;
    }

    // ### temporary
    i.setBegin(QDateTime(dt.date(), {0, 0}));
    i.setEnd(QDateTime(dt.date(), {23, 59}));

    if (m_weekdaySelector) {
        if (const auto offset = m_weekdaySelector->nextInterval(i, dt)) {
            return nextInterval(dt.addSecs(offset));
        }
    }

    if (m_timeSelector) {
        if (const auto offset = m_timeSelector->nextInterval(i, dt)) {
            return nextInterval(dt.addSecs(offset));
        }
    }

    return i;
}
