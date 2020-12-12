/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_OPENINGHOURS_P_H
#define KOPENINGHOURS_OPENINGHOURS_P_H

#include "openinghours.h"
#include "rule_p.h"

#ifndef KOPENINGHOURS_VALIDATOR_ONLY
#include <KHolidays/HolidayRegion>
#endif

#include <QSharedData>
#include <QTimeZone>

#include <cmath>
#include <memory>
#include <vector>

namespace KOpeningHours {
class OpeningHoursPrivate : public QSharedData {
public:
    void autocorrect();
    void validate();
    void addRule(Rule *rule);

    std::vector<std::unique_ptr<Rule>> m_rules;
    OpeningHours::Modes m_modes = OpeningHours::IntervalMode;
    OpeningHours::Error m_error = OpeningHours::NoError;

    float m_latitude = NAN;
    float m_longitude = NAN;
#ifndef KOPENINGHOURS_VALIDATOR_ONLY
    KHolidays::HolidayRegion m_region;
#endif
    QTimeZone m_timezone = QTimeZone::systemTimeZone();
};

}

#endif
