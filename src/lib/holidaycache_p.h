/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_HOLIDAYCACHE_P_H
#define KOPENINGHOURS_HOLIDAYCACHE_P_H

namespace KHolidays {
class Holiday;
class HolidayRegion;
}

class QDate;
class QStringView;

namespace KOpeningHours {

/** Cache of holiday region lookups, and holidays for holiday regions
 *  This is necessary as KHolidays is too slow for high-frequency evaluation.
 */
namespace HolidayCache
{
    /** Find KHoliday region for a given ISO 3166-1/2 code. */
    KHolidays::HolidayRegion resolveRegion(QStringView region);

    /** Returns the next holiday at or after @p dt in the given holiday region. */
    KHolidays::Holiday nextHoliday(const KHolidays::HolidayRegion &region, QDate date);
}

}

#endif // KOPENINGHOURS_HOLIDAYCACHE_P_H
