/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_RULE_P_H
#define KOPENINGHOURS_RULE_P_H

#include "interval.h"
#include "selectors_p.h"

#include <QDebug>

#include <memory>

namespace KOpeningHours {

/** Result of a rule evalution. */
class RuleResult
{
public:
    Interval interval;
    enum Mode {
        Override,
        Merge,
    };
    Mode mode;
};

/** Opening hours expression rule. */
class Rule
{
public:
    enum Type {
        NormalRule,
        AdditionalRule,
        FallbackRule,
    };

    Interval::State state() const;
    void setState(Interval::State state);
    void setComment(const char *str, int len);
    int requiredCapabilities() const;

    RuleResult nextInterval(const QDateTime &dt, OpeningHoursPrivate *context) const;
    QByteArray toExpression() const;

    QString m_comment;

    std::unique_ptr<Timespan> m_timeSelector;
    std::unique_ptr<WeekdayRange> m_weekdaySelector;
    std::unique_ptr<Week> m_weekSelector;
    std::unique_ptr<MonthdayRange> m_monthdaySelector;
    std::unique_ptr<YearRange> m_yearSelector;
    bool m_seen_24_7 = false;

    Type m_ruleType = NormalRule;

private:
    Interval::State m_state = Interval::Invalid;

    enum { RecursionLimit = 64 };
    RuleResult nextInterval(const QDateTime &dt, OpeningHoursPrivate *context, int recursionBudget) const;
};

}

#endif // KOPENINGHOURS_RULE_P_H
