/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "rule_p.h"
#include "logging.h"
#include "openinghours_p.h"

#include <QDateTime>

using namespace KOpeningHours;

void Rule::setComment(const char *str, int len)
{
    m_comment = QString::fromUtf8(str, len);
}

int Rule::requiredCapabilities() const
{
    int c = Capability::None;
    c |= m_timeSelector ? m_timeSelector->requiredCapabilities() : Capability::None;
    c |= m_weekdaySelector ? m_weekdaySelector->requiredCapabilities() : Capability::None;
    c |= m_weekSelector ? m_weekSelector->requiredCapabilities() : Capability::None;
    c |= m_monthdaySelector ? m_monthdaySelector->requiredCapabilities() : Capability::None;
    c |= m_yearSelector ? m_yearSelector->requiredCapabilities() : Capability::None;

    return c;
}

RuleResult Rule::nextInterval(const QDateTime &dt, OpeningHoursPrivate *context, int recursionBudget) const
{
    const auto resultMode = (recursionBudget == Rule::RecursionLimit && !isAdditional && m_state != Interval::Closed) ? RuleResult::Override : RuleResult::Merge;

    if (recursionBudget == 0) {
        qCWarning(Log) << "Recursion limited reached!";
        return {{}, resultMode};
    }

    Interval i;
    i.setState(m_state);
    i.setComment(m_comment);
    if (!m_timeSelector && !m_weekdaySelector && !m_monthdaySelector && !m_weekSelector && !m_yearSelector) {
        // 24/7 has no selectors
        return {i, resultMode};
    }

    if (m_yearSelector) {
        SelectorResult r;
        for (auto s = m_yearSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {{}, resultMode};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context, --recursionBudget);
        }
        i = r.interval();
    }

    if (m_monthdaySelector) {
        SelectorResult r;
        for (auto s = m_monthdaySelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {{}, resultMode};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context, --recursionBudget);
        }
        i = r.interval();
    }

    if (m_weekSelector) {
        SelectorResult r;
        for (auto s = m_weekSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {{}, resultMode};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context, --recursionBudget);
        }
        i = r.interval();
    }

    if (m_weekdaySelector) {
        SelectorResult r;
        for (auto s = m_weekdaySelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {{}, resultMode};
        }
        if (r.matchOffset() > 0) {
            return nextInterval(dt.addSecs(r.matchOffset()), context, --recursionBudget);
        }
        i = r.interval();
    }

    if (m_timeSelector) {
        SelectorResult r;
        for (auto s = m_timeSelector.get(); s; s = s->next.get()) {
            r = std::min(r, s->nextInterval(i, dt, context));
        }
        if (!r.canMatch()) {
            return {{}, resultMode};
        }
        if (r.matchOffset() > 0) {
            return {nextInterval(dt.addSecs(r.matchOffset()), context, --recursionBudget).interval, resultMode};
        }
        i = r.interval();
    }

    return {i, resultMode};
}
