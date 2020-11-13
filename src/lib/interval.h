/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_INTERVAL_H
#define KOPENINGHOURS_INTERVAL_H

#include "kopeninghours_export.h"

#include <QDateTime>
#include <QDebug>
#include <QExplicitlySharedDataPointer>
#include <QMetaType>

class QDateTime;

namespace KOpeningHours {

class IntervalPrivate;

/** A time interval for which an opening hours expression has been evaluated. */
class KOPENINGHOURS_EXPORT Interval
{
    Q_GADGET
    Q_PROPERTY(State state READ state)
    Q_PROPERTY(QDateTime begin READ begin)
    Q_PROPERTY(QDateTime end READ end)
    Q_PROPERTY(QString comment READ comment)
public:
    Interval();
    Interval(const Interval&);
    Interval(Interval&&);
    ~Interval();
    Interval& operator=(const Interval&);
    Interval& operator=(Interval&&);

    bool operator<(const Interval &other) const;

    /** Default constructed empty/invalid interval. */
    bool isValid() const;

    /** Begin of the interval.
     *  This is the first point in time included in the interval.
     */
    QDateTime begin() const;
    void setBegin(const QDateTime &begin);

    /** End of the interval.
     *  This is the first point in time not included in the interval anymore.
     *  That is, the end of an interval describing the year 2020 would be Jan 1st 2021 at midnight (00:00).
     */
    QDateTime end() const;
    void setEnd(const QDateTime &end);

    /** Check if this interval contains @p dt. */
    bool contains(const QDateTime &dt) const;

    /** Opening state during a time interval */
    enum State {
        Invalid,
        Open,
        Closed,
        Unknown
    };
    Q_ENUM(State)

    /** The opening state for this time interval. */
    State state() const;
    void setState(State state);

    /** Comment. */
    QString comment() const;
    void setComment(const QString &comment);

private:
    QExplicitlySharedDataPointer<IntervalPrivate> d;
};

}

Q_DECLARE_METATYPE(KOpeningHours::Interval)

KOPENINGHOURS_EXPORT QDebug operator<<(QDebug debug, const KOpeningHours::Interval &interval);

#endif // KOPENINGHOURS_INTERVAL_H
