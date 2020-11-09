/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_RULE_P_H
#define KOPENINGHOURS_RULE_P_H

#include "interval.h"

#include <QDebug>

#include <memory>

namespace KOpeningHours {

// see https://wiki.openstreetmap.org/wiki/Key:opening_hours/specification

namespace  Capability {
    enum RequiredCapabilities {
        None = 0,
        PublicHoliday = 1,
        SchoolHoliday = 2,
        Location = 4,
        NotImplemented = 8,
    };
}

/** Time */
class Time
{
public:
    enum Event {
        NoEvent,
        Dawn,
        Sunrise,
        Sunset,
        Dusk,
    };
    Event event;
    int hour;
    int minute;
};

/** Time span selector. */
class Timespan
{
public:
    int requiredCapabilities() const;
    int nextInterval(Interval &interval, const QDateTime &dt) const;

    Time begin = { Time::NoEvent, -1, -1 };
    Time end = { Time::NoEvent, -1, -1 };
    int interval = 0;
    std::unique_ptr<Timespan> next;
};

/** Weekday range. */
class WeekdayRange
{
public:
    int requiredCapabilities() const;
    int nextInterval(Interval &interval, const QDateTime &dt) const;

    uint8_t beginDay = 0;
    uint8_t endDay = 0;
    uint8_t nthMask = 0;
    int16_t offset = 0;
    enum Holiday : uint8_t {
        NoHoliday = 0,
        PublicHoliday = 1,
        SchoolHoliday = 2,
    };
    Holiday holiday = NoHoliday;
    std::unique_ptr<WeekdayRange> next;
    std::unique_ptr<WeekdayRange> next2;
};

/** Week */
class Week
{
public:
    uint8_t beginWeek = 0;
    uint8_t endWeek = 0;
    uint8_t interval = 1;
    std::unique_ptr<Week> next;
};

/** Date */
class Date
{
public:
    uint16_t year;
    uint8_t month;
    uint8_t day;
    enum VariableDate : uint8_t {
        FixedDate,
        Easter
    };
    VariableDate variableDate;
};

/** Monthday range. */
class MonthdayRange
{
public:
    Date begin = { 0, 0, 0, Date::FixedDate };
    Date end = { 0, 0, 0, Date::FixedDate };
    // TODO offsets
    std::unique_ptr<MonthdayRange> next;
};

/** Year range. */
class YearRange
{
public:
    int requiredCapabilities() const;

    int begin = 0;
    int end = 0;
    int interval = 1;
    std::unique_ptr<YearRange> next;
};

/** Opening hours expression rule. */
class Rule
{
public:
    void setComment(const char *str, int len);
    int requiredCapabilities() const;

    Interval nextInterval(const QDateTime &dt) const;

    QString m_comment;
    Interval::State m_state = Interval::Open;

    std::unique_ptr<Timespan> m_timeSelector;
    std::unique_ptr<WeekdayRange> m_weekdaySelector;
    std::unique_ptr<Week> m_weekSelector;
    std::unique_ptr<MonthdayRange> m_monthdaySelector;
    std::unique_ptr<YearRange> m_yearSelector;
};

}

QDebug operator<<(QDebug debug, const KOpeningHours::Time &time);
QDebug operator<<(QDebug debug, const KOpeningHours::Timespan *timeSpan);
QDebug operator<<(QDebug debug, const KOpeningHours::WeekdayRange *weekdayRange);
QDebug operator<<(QDebug debug, KOpeningHours::Week *week);
QDebug operator<<(QDebug debug, const KOpeningHours::Date &date);
QDebug operator<<(QDebug debug, const KOpeningHours::MonthdayRange *monthdayRange);
QDebug operator<<(QDebug debug, const KOpeningHours::YearRange *yearRange);

#endif // KOPENINGHOURS_RULE_P_H
