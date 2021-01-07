/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "selectors_p.h"
#include "logging.h"
#include "openinghours_p.h"
#include "consecutiveaccumulator_p.h"

#include <cstdlib>
#include <cassert>

using namespace KOpeningHours;

static QByteArray twoDigits(int n)
{
    QByteArray ret = QByteArray::number(n);
    if (ret.size() < 2) {
        ret.prepend('0');
    }
    return ret;
}

bool Time::isValid(Time t)
{
    return t.hour >= 0 && t.hour <= 48 && t.minute >= 0 && t.minute < 60;
}

void Time::convertFromAm(Time &t)
{
    if (t.hour == 12) {
        t.hour = 0;
    }
}

void Time::convertFromPm(Time &t)
{
    if (t.hour < 12) {
        t.hour += 12;
    }
}

Time Time::parse(const char *begin, const char *end)
{
    Time t{ Time::NoEvent, 0, 0 };

    char *it = nullptr;
    t.hour = std::strtol(begin, &it, 10);

    for (const auto sep : {':', 'h', 'H'}) {
        if (*it == sep) {
            ++it;
            break;
        }
    }
    if (it != end) {
        t.minute = std::strtol(it, nullptr, 10);
    }
    return t;
}

QByteArray Time::toExpression(bool end) const
{
    QByteArray expr;
    switch (event) {
    case Time::NoEvent:
        if (hour % 24 == 0 && minute == 0 && end)
            return "24:00";
        return twoDigits(hour % 24) + ':' + twoDigits(minute);
    case Time::Dawn:
        expr = "dawn";
        break;
    case Time::Sunrise:
        expr = "sunrise";
        break;
    case Time::Dusk:
        expr = "dusk";
        break;
    case Time::Sunset:
        expr = "sunset";
        break;
    }
    const int minutes = hour * 60 + minute;
    if (minutes != 0) {
        const QByteArray hhmm = twoDigits(qAbs(hour) % 24) + ':' + twoDigits(qAbs(minute));
        expr = '(' + expr + (minutes > 0 ? '+' : '-') + hhmm + ')';
    }
    return expr;
}

int Timespan::requiredCapabilities() const
{
    int c = Capability::None;
    if ((interval > 0 || begin == end) && !openEnd) {
        c |= Capability::PointInTime;
    } else {
        c |= Capability::Interval;
    }
    if (begin.event != Time::NoEvent || end.event != Time::NoEvent) {
        c |= Capability::Location;
    }
    return next ? (next->requiredCapabilities() | c) : c;
}

static QByteArray intervalToExpression(int minutes)
{
    if (minutes < 60) {
        return twoDigits(minutes);
    } else {
        const int hours = minutes / 60;
        minutes -= hours * 60;
        return twoDigits(hours) + ':' + twoDigits(minutes);
    }
}

QByteArray Timespan::toExpression() const
{
    QByteArray expr = begin.toExpression(false);
    if (!(end == begin)) {
        expr += '-' + end.toExpression(true);
    }
    if (openEnd) {
        expr += '+';
    }
    if (interval) {
        expr += '/' + intervalToExpression(interval);
    }
    if (next) {
        expr += ',' + next->toExpression();
    }
    return expr;
}

int WeekdayRange::requiredCapabilities() const
{
    // only ranges or nthMask are allowed, not both at the same time, enforced by parser
    assert(beginDay == endDay || nthMask == 0);

    int c = Capability::None;
    switch (holiday) {
        case NoHoliday:
            if ((offset > 0 && nthMask == 0)) {
                c |= Capability::NotImplemented;
            }
            break;
        case PublicHoliday:
            c |= Capability::PublicHoliday;
            break;
        case SchoolHoliday:
            c |= Capability::SchoolHoliday;
            break;
    }
    if (andSelector) {
        c |= andSelector->requiredCapabilities();
    }
    if (next) {
        c |= next->requiredCapabilities();
    }
    return c;
}

static constexpr const char* s_weekDays[] = { "ERROR", "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};

QByteArray WeekdayRange::toExpression() const
{
    QByteArray expr;
    switch (holiday) {
    case NoHoliday: {
        expr = s_weekDays[beginDay];
        if (endDay != beginDay) {
            expr += '-';
            expr += s_weekDays[endDay];
        }
        break;
    }
    case PublicHoliday:
        expr = "PH";
        break;
    case SchoolHoliday:
        expr = "SH";
        break;
    }
    if (nthMask > 0) {
        ConsecutiveAccumulator accu([](int i) { return QByteArray::number(i); });
        for (int i = 1; i <= 10; ++i) {
            if ((nthMask & (1 << i)) == 0) {
                continue;
            }
            const auto n = (i % 2) ? (-5 + (i / 2)) : (i / 2);
            accu.add(n);
        }
        expr += '[' + accu.result() + ']';
    }
    if (offset > 0) {
        expr += " +" + QByteArray::number(offset) + ' ' + (offset > 1 ? "days" : "day");
    } else if (offset < 0) {
        expr += " -" + QByteArray::number(-offset) + ' ' + (offset < -1 ? "days" : "day");
    }
    if (next) {
        expr += ',' + next->toExpression();
    }
    if (andSelector) {
        expr += ' ' + andSelector->toExpression();
    }
    return expr;
}

int Week::requiredCapabilities() const
{
    if (endWeek < beginWeek) { // is this even officially allowed?
        return Capability::NotImplemented;
    }
    return next ? next->requiredCapabilities() : Capability::None;
}

QByteArray Week::toExpression() const
{
    QByteArray expr = twoDigits(beginWeek);
    if (endWeek != beginWeek) {
        expr += '-';
        expr += twoDigits(endWeek);
    }
    if (interval > 1) {
        expr += '/';
        expr += QByteArray::number(interval);
    }
    if (next) {
        expr += ',' + next->toExpression();
    }
    return expr;
}

QByteArray Date::toExpression(Date refDate) const
{
    QByteArray expr;
    auto maybeSpace = [&]() {
        if (!expr.isEmpty()) {
            expr += ' ';
        }
    };
    switch (variableDate) {
    case FixedDate:
        if (year && year != refDate.year) {
            expr += QByteArray::number(year);
        }
        if (month && ((year && year != refDate.year) || month != refDate.month || hasOffset())) {
            static const char* s_monthName[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
            maybeSpace();
            expr += s_monthName[month-1];
        }
        if (day && *this != refDate) {
            maybeSpace();
            expr += twoDigits(day);
        }
        break;
    case Date::Easter:
        if (year) {
            expr += QByteArray::number(year) + ' ';
        }
        expr += "easter";
        break;
    }

    if (offset.nthWeekday) {
        expr += ' ';
        expr += s_weekDays[offset.weekday];
        expr += '[' + QByteArray::number(offset.nthWeekday) + ']';
    }

    if (offset.dayOffset > 0) {
        expr += " +" + QByteArray::number(offset.dayOffset) + ' ' + (offset.dayOffset > 1 ? "days" : "day");
    } else if (offset.dayOffset < 0) {
        expr += " -" + QByteArray::number(-offset.dayOffset) + ' ' + (offset.dayOffset < -1 ? "days" : "day");
    }
    return expr;
}

bool DateOffset::operator==(DateOffset other) const
{
    return weekday == other.weekday && nthWeekday == other.nthWeekday && dayOffset == other.dayOffset;
}

bool Date::operator==(Date other) const
{
    if (variableDate != other.variableDate)
        return false;
    if (variableDate == FixedDate && other.variableDate == FixedDate) {
        if (!(year == other.year && month == other.month && day == other.day)) {
            return false;
        }
    }
    return offset == other.offset;
}

bool Date::hasOffset() const
{
    return offset.dayOffset || offset.weekday;
}

int MonthdayRange::requiredCapabilities() const
{
    return Capability::None;
}

QByteArray MonthdayRange::toExpression() const
{
    QByteArray expr = begin.toExpression({});
    if (end != begin) {
        expr += '-' + end.toExpression(begin);
    }
    if (next) {
        expr += ',' + next->toExpression();
    }
    return expr;
}

int YearRange::requiredCapabilities() const
{
    return Capability::None;
}

QByteArray YearRange::toExpression() const
{
    QByteArray expr = QByteArray::number(begin);
    if (end == 0 && interval == 1) {
        expr += '+';
    } else if (end != begin && end != 0) {
        expr += '-';
        expr += QByteArray::number(end);
    }
    if (interval > 1) {
        expr += '/';
        expr += QByteArray::number(interval);
    }
    if (next) {
        expr += ',' + next->toExpression();
    }
    return expr;
}
