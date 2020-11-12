/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "interval.h"

namespace KOpeningHours {
class IntervalPrivate : public QSharedData {
public:
    QDateTime begin;
    QDateTime end;
    Interval::State state = Interval::Invalid;
    QString comment;
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

bool Interval::operator<(const Interval &other) const
{
    // TODO handle open intervals correctly
    if (d->begin == other.d->begin) {
        return d->end < other.d->end;
    }
    return d->begin < other.d->begin;
}

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

bool Interval::contains(const QDateTime &dt) const
{
    return (d->begin.isValid() ? d->begin <= dt : true) && (d->end.isValid() ? dt <= d->end : true);
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

QString Interval::comment() const
{
    return d->comment;
}

void Interval::setComment(const QString &comment)
{
    d.detach();
    d->comment = comment;
}

QDebug operator<<(QDebug debug, const Interval &interval)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '[' << interval.begin() << '-' << interval.end() << ' ' << interval.state() << ':' << interval.comment() << ']';
    return debug;
}