/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_RULE_P_H
#define KOPENINGHOURS_RULE_P_H

#include "interval.h"

#include <memory>

namespace KOpeningHours {

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

/** Opening hours expression rule. */
class Rule
{
public:
    void setComment(const char *str, int len);

    QString m_comment;
    Interval::State m_state = Interval::Open;

    std::unique_ptr<Timespan> m_timeSelector;
};

}

#endif // KOPENINGHOURS_RULE_P_H
