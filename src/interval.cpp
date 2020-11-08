/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "interval.h"

#include <QDateTime>

namespace KOpeningHours {
class IntervalPrivate : public QSharedData {
public:
    QDateTime begin;
    QDateTime end;
    Interval::State state = Interval::Invalid;
};
}

using namespace KOpeningHours;

Interval::Interval()
    : d(new IntervalPrivate)
{
}

Interval::Interval(const Interval&) = default;
Interval::Interval(Interval&&) = default;
Interval::~Interval() = default;
Interval& Interval::operator=(const Interval&) = default;
Interval& Interval::operator=(Interval&&) = default;

bool Interval::isValid() const
{
    return d->state != Invalid;
}

QDateTime Interval::begin() const
{
    return d->begin;
}

void Interval::setBegin(const QDateTime &begin)
{
    d.detach();
    d->begin = begin;
}

QDateTime Interval::end() const
{
    return d->end;
}

void Interval::setEnd(const QDateTime &end)
{
    d.detach();
    d->end = end;
}

Interval::State Interval::state() const
{
    return d->state;
}

void Interval::setState(Interval::State state)
{
    d.detach();
    d->state = state;
}
