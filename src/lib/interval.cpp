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
    bool openEndTime = false;
    QString comment;
    QDateTime estimatedEnd;
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
    if (hasOpenBegin() && !other.hasOpenBegin()) {
        return true;
    }
    if (other.hasOpenBegin() && !hasOpenBegin()) {
        return false;
    }

    if (d->begin == other.d->begin) {
        return d->end < other.d->end;
    }
    return d->begin < other.d->begin;
}

bool Interval::intersects(const Interval &other) const
{
    if (d->end.isValid() && other.d->begin.isValid() && d->end <= other.d->begin) {
        return false;
    }
    if (other.d->end.isValid() && d->begin.isValid() && other.d->end <= d->begin) {
        return false;
    }
    return true;
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

bool Interval::hasOpenBegin() const
{
    return !d->begin.isValid();
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

bool Interval::hasOpenEnd() const
{
    return !d->end.isValid();
}

bool Interval::hasOpenEndTime() const
{
    return d->openEndTime;
}

void Interval::setOpenEndTime(bool openEndTime)
{
    d.detach();
    d->openEndTime = openEndTime;
}

bool Interval::contains(const QDateTime &dt) const
{
    if (d->openEndTime && d->begin.isValid() && d->begin == d->end) {
        return dt == d->begin;
    }
    return (d->begin.isValid() ? d->begin <= dt : true) && (d->end.isValid() ? dt < d->end : true);
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

QDateTime Interval::estimatedEnd() const
{
    if (d->openEndTime && d->estimatedEnd.isValid()) {
        return d->estimatedEnd;
    }
    return end();
}

void Interval::setEstimatedEnd(const QDateTime& estimatedEnd)
{
    d.detach();
    d->estimatedEnd = estimatedEnd;
}

int Interval::dstOffset() const
{
    if (d->begin.isValid() && estimatedEnd().isValid()) {
        return estimatedEnd().offsetFromUtc() - d->begin.offsetFromUtc();
    }
    return 0;
}

QDebug operator<<(QDebug debug, const Interval &interval)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << '[' << interval.begin().toString(QStringLiteral("yyyy-MM-ddThh:mm")) << " - " << interval.end().toString(QStringLiteral("yyyy-MM-ddThh:mm")) << ' ' << interval.state() << " (\"" << interval.comment() << "\")]";
    return debug;
}

#include "moc_interval.cpp"
