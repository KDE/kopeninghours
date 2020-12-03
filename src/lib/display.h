/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_DISPLAY_H
#define KOPENINGHOURS_DISPLAY_H

#include "kopeninghours_export.h"

class QString;

namespace KOpeningHours {

class OpeningHours;

/** Utilities for displaying human-readable/localized opening hours information. */
namespace Display
{
    /** Localized description of the current opening state, and upcoming transitions. */
    KOPENINGHOURS_EXPORT QString currentState(const OpeningHours &oh);
}

}

#endif // KOPENINGHOURS_DISPLAY_H
