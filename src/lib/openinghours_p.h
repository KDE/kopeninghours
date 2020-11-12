/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_OPENINGHOURS_P_H
#define KOPENINGHOURS_OPENINGHOURS_P_H

#include "openinghours.h"
#include "rule_p.h"

#include <KHolidays/HolidayRegion>

#include <QSharedData>

#include <cmath>
#include <memory>
#include <vector>

namespace KOpeningHours {
class OpeningHoursPrivate : public QSharedData {
public:
    void validate();
    void addRule(Rule *rule);

    std::vector<std::unique_ptr<Rule>> m_rules;
    OpeningHours::Error m_error = OpeningHours::NoError;

    float m_latitude = NAN;
    float m_longitude = NAN;
    KHolidays::HolidayRegion m_region;
};

}

#endif
