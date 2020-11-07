/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_INTERVAL_H
#define KOPENINGHOURS_INTERVAL_H

#include "kopeninghours_export.h"

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace KOpeningHours {

/** A time interval for which an opening hours expression has been evaluated. */
class KOPENINGHOURS_EXPORT Interval
{
    Q_GADGET
public:
    // TODO
    // QDateTime begin() const;
    // QDateTime end() const;

    /** Opening state during a time interval */
    enum State {
        Open,
        Closed,
        Unknown
    };
    Q_ENUM(State)

    /** The opening state for this time interval. */
    State state() const;

    // TODO
    //QString comment() const;

private:
    // TODO
};

}

#endif // KOPENINGHOURS_INTERVAL_H
