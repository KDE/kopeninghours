/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "selectors_p.h"
#include "logging.h"
#include "openinghours_p.h"

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
        return twoDigits(hour) + ':' + twoDigits(minute);
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
        const QByteArray hhmm = twoDigits(qAbs(hour)) + ':' + twoDigits(qAbs(minute));
        expr = '(' + expr + (minutes > 0 ? '+' : '-') + hhmm + ')';
    }
    return expr;
}

int Timespan::requiredCapabilities() const
{
    int c = Capability::None;
    if ((interval > 0 || pointInTime) && !openEnd) {
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
    if (!pointInTime) {
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

Time Timespan::adjustedEnd() const
{
    if (begin == end && !pointInTime) {
        return { end.event, end.hour + 24, end.minute };
    }
    return end;
}

bool Timespan::operator==(Timespan &other) const
{
    return begin == other.begin &&
            end == other.end &&
            openEnd == other.openEnd &&
            interval == other.interval &&
            bool(next) == (bool)other.next &&
            (!next || *next == *other.next);
}

int WeekdayRange::requiredCapabilities() const
{
    // only ranges or nthSequence are allowed, not both at the same time, enforced by parser
    assert(beginDay == endDay || !nthSequence);

    int c = Capability::None;
    switch (holiday) {
        case NoHoliday:
            if ((offset > 0 && !nthSequence)) {
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

    c |= lhsAndSelector ? lhsAndSelector->requiredCapabilities() : Capability::None;
    c |= rhsAndSelector ? rhsAndSelector->requiredCapabilities() : Capability::None;
    c |= next ? next->requiredCapabilities() : Capability::None;
    return c;
}

static constexpr const char* s_weekDays[] = { "ERROR", "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};

QByteArray WeekdayRange::toExpression() const
{
    QByteArray expr;
    if (lhsAndSelector && rhsAndSelector) {
        expr = lhsAndSelector->toExpression() + ' ' + rhsAndSelector->toExpression();
    } else {
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
        if (nthSequence) {
            expr += '[' + nthSequence->toExpression() + ']';
        }
        if (offset > 0) {
            expr += " +" + QByteArray::number(offset) + ' ' + (offset > 1 ? "days" : "day");
        } else if (offset < 0) {
            expr += " -" + QByteArray::number(-offset) + ' ' + (offset < -1 ? "days" : "day");
        }
    }
    if (next) {
        expr += ',' + next->toExpression();
    }
    return expr;
}

void WeekdayRange::simplify()
{
    QMap<int, WeekdayRange *> endToSelectorMap;
    bool seenDays[8];
    const int endIdx = sizeof(seenDays);
    std::fill(std::begin(seenDays), std::end(seenDays), false);
    for (WeekdayRange *selector = this; selector; selector = selector->next.get()) {
        // Ensure it's all just week days, no other features
        if (selector->nthSequence || selector->lhsAndSelector || selector->holiday != NoHoliday || selector->offset) {
            return;
        }
        const bool wrap = selector->beginDay > selector->endDay;
        for (int day = selector->beginDay; day <= selector->endDay + (wrap ? 7 : 0); ++day) {
            seenDays[(day - 1) % 7 + 1] = true;
        }
        endToSelectorMap.insert(selector->endDay, selector);
    }

    QString str;
    for (int idx = 1; idx < endIdx; ++idx) {
        str += QLatin1Char(seenDays[idx] ? '1' : '0');
    }

    // Clear everything and refill
    next.reset(nullptr);

    int startIdx = 1;

    // -1 and +1 in a wrapping world
    auto prevIdx = [&](int idx) {
        Q_ASSERT(idx > 0 && idx < 8);
        return idx == 1 ? 7 : (idx - 1);
    };
    auto nextIdx = [&](int idx) {
        Q_ASSERT(idx > 0 && idx < 8);
        return idx % 7 + 1;
    };

    // like std::find, but let's use indexes - and wrap at 8
    auto find = [&](int idx, bool value) {
        do {
            if (seenDays[idx] == value)
                return idx;
            idx = nextIdx(idx);
        } while(idx != startIdx);
        return idx;
    };
    auto findPrev = [&](int idx, bool value) {
        for (; idx > 0; --idx) {
            if (seenDays[idx] == value)
                return idx;
        }
        return 0;
    };

    WeekdayRange *prev = nullptr;
    WeekdayRange *selector = this;

    auto addRange = [&](int from, int to) {
        if (prev) {
            selector = new WeekdayRange;
            prev->next.reset(selector);
        }
        selector->beginDay = from;
        selector->endDay = to;
        prev = selector;

    };

    int idx = 0;
    if (seenDays[1]) {
        // monday is set, try going further back
        idx = findPrev(7, false);
        if (idx) {
            idx = nextIdx(idx);
        }
    }
    if (idx == 0) {
        // start at first day being set (Tu or more)
        idx = find(1, true);
    }
    startIdx = idx;
    Q_ASSERT(startIdx > 0);
    do {
        // find end of 'true' range
        const int finishIdx = find(idx, false);
        // if the range is only 2 items, prefer Mo,Tu over Mo-Tu
        if (finishIdx == nextIdx(nextIdx(idx))) {
            addRange(idx, idx);
            const int n = nextIdx(idx);
            addRange(n, n);
        } else {
            addRange(idx, prevIdx(finishIdx));
        }
        idx = find(finishIdx, true);
    } while (idx != startIdx);
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

QByteArray Date::toExpression(const Date &refDate, const MonthdayRange &prev) const
{
    QByteArray expr;
    auto maybeSpace = [&]() {
        if (!expr.isEmpty()) {
            expr += ' ';
        }
    };
    switch (variableDate) {
    case FixedDate: {
        const bool needYear = year && year != refDate.year && year != prev.begin.year && year != prev.end.year;
        if (needYear) {
            expr += QByteArray::number(year);
        }
        if (month) {
            const bool combineWithPrev = prev.begin.month == prev.end.month && month == prev.begin.month;
            const bool implicitMonth = month == refDate.month || (refDate.month == 0 && combineWithPrev);
            if (needYear || !implicitMonth || hasOffset()) {
                static const char* s_monthName[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
                maybeSpace();
                expr += s_monthName[month-1];
            }
        }
        if (day && *this != refDate) {
            maybeSpace();
            expr += twoDigits(day);
        }
        break;
    }
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

DateOffset &DateOffset::operator+=(DateOffset other)
{
    // Only dayOffset really supports += (this is for whitsun)
    dayOffset += other.dayOffset;
    // The others can't possibly be set already
    Q_ASSERT(weekday == 0);
    Q_ASSERT(nthWeekday == 0);
    weekday = other.weekday;
    nthWeekday = other.nthWeekday;
    return *this;
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

QByteArray MonthdayRange::toExpression(const MonthdayRange &prev) const
{
    QByteArray expr = begin.toExpression({}, prev);
    if (end != begin) {
        expr += '-' + end.toExpression(begin, prev);
    }
    if (next) {
        expr += ',' + next->toExpression(*this);
    }
    return expr;
}

void MonthdayRange::simplify()
{
    // "Feb 1-29" => "Feb" (#446252)
    if (begin.variableDate == Date::FixedDate &&
            end.variableDate == Date::FixedDate &&
            begin.year == end.year &&
            begin.month && end.month  &&
            begin.month == end.month  &&
            begin.day && end.day) {
        // The year doesn't matter, but take one with a leap day, for Feb 1-29
         const int lastDay = QDate{2004, end.month, end.day}.daysInMonth();
         if (begin.day == 1 && end.day == lastDay) {
             begin.day = 0;
             end.day = 0;
         }
    }
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

void NthSequence::add(NthEntry range)
{
    sequence.push_back(std::move(range));
}

QByteArray NthSequence::toExpression() const
{
    QByteArray ret;
    for (const NthEntry &entry : sequence) {
        if (!ret.isEmpty())
            ret += ',';
        ret += entry.toExpression();
    }
    return ret;
}

QByteArray NthEntry::toExpression() const
{
    if (begin == end)
        return QByteArray::number(begin);
    return QByteArray::number(begin) + '-' + QByteArray::number(end);
}
