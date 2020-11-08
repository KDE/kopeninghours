/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_INTERVAL_H
#define KOPENINGHOURS_INTERVAL_H

#include "kopeninghours_export.h"

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

class QDateTime;

namespace KOpeningHours {

class IntervalPrivate;

/** A time interval for which an opening hours expression has been evaluated. */
class KOPENINGHOURS_EXPORT Interval
{
    Q_GADGET
public:
    Interval();
    Interval(const Interval&);
    Interval(Interval&&);
    ~Interval();
    Interval& operator=(const Interval&);
    Interval& operator=(Interval&&);

    /** Begin of the interval. */
    QDateTime begin() const;
    void setBegin(const QDateTime &begin);

    /** End of the interval. */
    QDateTime end() const;
    void setEnd(const QDateTime &end);

    /** Opening state during a time interval */
    enum State {
        Open,
        Closed,
        Unknown
    };
    Q_ENUM(State)

    /** The opening state for this time interval. */
    State state() const;
    void setState(State state);

    // TODO
    //QString comment() const;

private:
    QExplicitlySharedDataPointer<IntervalPrivate> d;
};

}

#endif // KOPENINGHOURS_INTERVAL_H
