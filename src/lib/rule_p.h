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

enum class State { // must be in same order as Interval::State
    Invalid,
    Open,
    Closed,
    Unknown,
    Off // extension, not in Interval::State
};

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
    enum Type : short {
        NormalRule,
        AdditionalRule,
        FallbackRule,
        GuessRuleType,
    };
    enum StateFlags : char {
        NoFlags,
        Off // "off" was used rather than "closed"
    };

    Interval::State state() const;
    bool hasImplicitState() const;
    void setState(State state);
    bool hasComment() const;
    void setComment(const char *str, int len);
    int requiredCapabilities() const;
    /** Empty rules contain no selectors, have no comment and only implicit state. */
    bool isEmpty() const;
    bool hasWideRangeSelector() const;

    RuleResult nextInterval(const QDateTime &dt, OpeningHoursPrivate *context) const;
    QByteArray toExpression() const;

    /** Amount of selectors for this rule. */
    int selectorCount() const;

    QString m_comment;
    QString m_wideRangeSelectorComment;

    std::unique_ptr<Timespan> m_timeSelector;
    std::unique_ptr<WeekdayRange> m_weekdaySelector;
    std::unique_ptr<Week> m_weekSelector;
    std::unique_ptr<MonthdayRange> m_monthdaySelector;
    std::unique_ptr<YearRange> m_yearSelector;
    bool m_seen_24_7 = false;
    bool m_colonAfterWideRangeSelector = false;

    StateFlags m_stateFlags = NoFlags;
    Type m_ruleType = NormalRule;

private:
    Interval::State m_state = Interval::Invalid;

    enum { RecursionLimit = 64 };
    RuleResult nextInterval(const QDateTime &dt, OpeningHoursPrivate *context, int recursionBudget) const;
};

}

#endif // KOPENINGHOURS_RULE_P_H
