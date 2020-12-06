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

RuleResult Rule::nextInterval(const QDateTime &dt, OpeningHoursPrivate *context) const
{
    // handle time selectors spanning midnight
    // consider e.g. "Tu 12:00-12:00" being evaluated with dt being Wednesday 08:00
    // we need to look one day back to find a matching day selector and the correct start
    // of the interval here
    if (m_timeSelector && m_timeSelector->isMultiDay(dt.date(), context)) {
        const auto res = nextInterval(dt.addDays(-1), context, RecursionLimit);
        if (res.interval.contains(dt)) {
            return res;
        }
    }

    return nextInterval(dt, context, RecursionLimit);
}

QByteArray Rule::toExpression(bool singleRule) const
{
    QByteArray expr;
    if (!m_timeSelector && !m_weekdaySelector && !m_monthdaySelector && !m_weekSelector && !m_yearSelector) {
        if (singleRule && m_state != Interval::Unknown) {
            expr = "24/7";
        }
    }
    auto maybeSpace = [&]() {
        if (!expr.isEmpty()) {
            expr += ' ';
        }
    };
    if (m_yearSelector) {
        expr = m_yearSelector->toExpression();
    }
    if (m_monthdaySelector) {
        maybeSpace();
        expr += m_monthdaySelector->toExpression();
    }
    if (m_weekSelector) {
        maybeSpace();
        expr += m_weekSelector->toExpression();
    }
    if (m_weekdaySelector) {
        maybeSpace();
        expr += m_weekdaySelector->toExpression();
    }
    if (m_timeSelector) {
        maybeSpace();
        expr += m_timeSelector->toExpression();
    }
    switch (m_state) {
    case Interval::Open:
        if (!singleRule && expr.isEmpty()) { // ### or fallback
            maybeSpace();
            expr += "open";
        }
        break;
    case Interval::Closed:
        maybeSpace();
        expr += "off"; // or closed, but off is more common
        break;
    case Interval::Unknown:
        maybeSpace();
        expr += "unknown";
        break;
    case Interval::Invalid:
        break;
    }
    if (!m_comment.isEmpty()) {
        if (!expr.isEmpty()) {
            expr += ' ';
        }
        expr += '"' + m_comment.toUtf8() + '"';
    }
    return expr;
}

RuleResult Rule::nextInterval(const QDateTime &dt, OpeningHoursPrivate *context, int recursionBudget) const
{
    const auto resultMode = (recursionBudget == Rule::RecursionLimit && m_ruleType == NormalRule && m_state != Interval::Closed) ? RuleResult::Override : RuleResult::Merge;

    if (recursionBudget == 0) {
        context->m_error = OpeningHours::EvaluationError;
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
