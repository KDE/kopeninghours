/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_OPENINGHOURS_P_H
#define KOPENINGHOURS_OPENINGHOURS_P_H

#include "openinghours.h"

#include <QSharedData>

namespace KOpeningHours {
class OpeningHoursPrivate : public QSharedData {
public:
    OpeningHours::Error m_error = OpeningHours::NoError;
};
}

#endif
