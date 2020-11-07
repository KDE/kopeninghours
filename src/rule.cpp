/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "rule_p.h"

using namespace KOpeningHours;

void Rule::setComment(const char *str, int len)
{
    m_comment = QString::fromUtf8(str, len);
}
