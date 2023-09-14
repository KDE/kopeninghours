/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_DISPLAY_H
#define KOPENINGHOURS_DISPLAY_H

#include "kopeninghours_export.h"

#include <qobjectdefs.h>

class QString;

namespace KOpeningHours {

class OpeningHours;

/** Utilities for displaying human-readable/localized opening hours information. */
class KOPENINGHOURS_EXPORT Display
{
    Q_GADGET
public:
    /** Localized description of the current opening state, and upcoming transitions. */
    Q_INVOKABLE static QString currentState(const KOpeningHours::OpeningHours &oh);
};

}

#endif // KOPENINGHOURS_DISPLAY_H
