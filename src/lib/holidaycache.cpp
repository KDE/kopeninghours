/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "holidaycache_p.h"

#include <KHolidays/HolidayRegion>

#include <QDate>
#include <QDebug>
#include <QHash>

using namespace KOpeningHours;

KHolidays::HolidayRegion HolidayCache::resolveRegion(QStringView region)
{
    static QHash<QString, QString> s_holidayRegionCache;

    const auto idx = region.indexOf(QLatin1Char('_')); // compatibility with KHolidays region codes
    if (idx > 0) {
        region = region.left(idx);
    }

    const auto loc = region.toString();

    const auto it = s_holidayRegionCache.constFind(loc);
    if (it != s_holidayRegionCache.constEnd()) {
        return KHolidays::HolidayRegion(it.value());
    }

    const auto code = KHolidays::HolidayRegion::defaultRegionCode(loc);
    s_holidayRegionCache.insert(loc, code);
    return KHolidays::HolidayRegion(code);
}

struct HolidayCacheEntry {
    QDate begin;
    QDate end;
    KHolidays::Holiday::List holidays;
};

static KHolidays::Holiday nextHoliday(const KHolidays::Holiday::List &holidays, QDate date)
{
    for (const auto &h : holidays) {
        if (h.observedStartDate() >= date)
            return h;
    }
    return {};
}

KHolidays::Holiday HolidayCache::nextHoliday(const KHolidays::HolidayRegion &region, QDate date)
{
    static QHash<QString, HolidayCacheEntry> s_holidayCache;

    if (!region.isValid()) {
        return {};
    }

    const auto it = s_holidayCache.constFind(region.regionCode());
    if (it != s_holidayCache.constEnd() && date >= it.value().begin && date.addYears(1) < it.value().end) {
        return nextHoliday(it.value().holidays, date);
    }

    HolidayCacheEntry entry;
    entry.begin = date.addDays(-7);
    entry.end = date.addYears(2).addDays(7);
    if (it != s_holidayCache.constEnd()) {
        entry.begin = std::min(it.value().begin, entry.begin);
        entry.end = std::max(it.value().end, entry.end);
    }
    entry.holidays = region.holidays(entry.begin, entry.end);
    entry.holidays.erase(std::remove_if(entry.holidays.begin(), entry.holidays.end(), [](const auto &h) {
        return h.dayType() != KHolidays::Holiday::NonWorkday;
    }), entry.holidays.end());
    std::sort(entry.holidays.begin(), entry.holidays.end(), [](const auto &lhs, const auto &rhs) {
        return lhs.observedStartDate() < rhs.observedStartDate();
    });
    s_holidayCache.insert(region.regionCode(), entry);
    return nextHoliday(entry.holidays, date);
}
