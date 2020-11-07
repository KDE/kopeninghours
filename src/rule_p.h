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
    Time begin = { Time::NoEvent, -1, -1 };
    Time end = { Time::NoEvent, -1, -1 };
    std::unique_ptr<Timespan> next;
};

/** Weekday range. */
class WeekdayRange
{
public:
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

/** Opening hours expression rule. */
class Rule
{
public:
    void setComment(const char *str, int len);

    QString m_comment;
    Interval::State m_state = Interval::Open;

    std::unique_ptr<Timespan> m_timeSelector;
    std::unique_ptr<WeekdayRange> m_weekdaySelector;
    std::unique_ptr<Week> m_weekSelector;
};

}

QDebug operator<<(QDebug debug, KOpeningHours::WeekdayRange *weekdayRange);
QDebug operator<<(QDebug debug, KOpeningHours::Week *week);

#endif // KOPENINGHOURS_RULE_P_H
