/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_EASTER_P_H
#define KOPENINGHOURS_EASTER_P_H

class QDate;

namespace KOpeningHours {

/** Basic easter holiday in Gregorian calendar computation.
 *  This should be provided by KHolidays, but is tricky to expose there due to its
 *  internal outdated QCalendarSystem copy.
 */
namespace Easter
{
    QDate easterDate(int year);
}

}

#endif // KOPENINGHOURS_EASTER_P_H
