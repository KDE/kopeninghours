/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "easter_p.h"

#include <QDate>

using namespace KOpeningHours;

QDate Easter::easterDate(int year)
{
    // algorithm from KHolidays
    const int g = year % 19;
    const int c = year / 100;
    const int h = (c - (c / 4) - (((8 * c) + 13) / 25) + (19 * g) + 15) % 30;
    const int i = h - ((h / 28) * (1 - ((29 / (h + 1)) * ((21 - g) / 11))));
    const int j = (year + (year / 4) + i + 2 - c + (c / 4)) % 7;
    const int l = i - j;
    const int month = 3 + ((l + 40) / 44);
    const int day = l + 28 - (31 * (month / 4));

    return QDate(year, month, day);
}
