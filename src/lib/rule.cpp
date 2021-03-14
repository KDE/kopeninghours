/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "rule_p.h"
#include "logging.h"
#include "openinghours_p.h"

#include <QDateTime>

using namespace KOpeningHours;

Interval::State Rule::state() const
{
    if (m_state == Interval::Invalid) { // implicitly defined state
        return m_comment.isEmpty() ? Interval::Open : Interval::Unknown;
    }
    return m_state;
}

bool Rule::hasImplicitState() const
{
    return m_state == Interval::Invalid;
}

void Rule::setState(State state)
{
    if (state == State::Off) {
        m_state = Interval::Closed;
        m_stateFlags = Off;
    } else {
        m_state = static_cast<Interval::State>(state);
    }
}

bool Rule::hasComment() const
{
    return !m_comment.isEmpty();
}

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
    c |= m_wideRangeSelectorComment.isEmpty() ? Capability::None : Capability::NotImplemented;

    return c;
}

bool Rule::isEmpty() const
{
    return !hasComment()
        && hasImplicitState()
        && (selectorCount() == 0)
        && !m_seen_24_7
        && m_wideRangeSelectorComment.isEmpty();
}

bool Rule::hasSmallRangeSelector() const
{
    return m_weekdaySelector || m_timeSelector;
}

bool Rule::hasWideRangeSelector() const
{
    return m_yearSelector || m_weekSelector || m_monthdaySelector || !m_wideRangeSelectorComment.isEmpty();
}

QByteArray Rule::toExpression() const
{
    QByteArray expr;
    if (!m_timeSelector && !m_weekdaySelector && !m_monthdaySelector && !m_weekSelector && !m_yearSelector) {
        if (m_seen_24_7) {
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
        expr += "week " + m_weekSelector->toExpression();
    }
    if (!m_wideRangeSelectorComment.isEmpty()) {
        expr += '"' + m_wideRangeSelectorComment.toUtf8() + '"';
    }
    if (m_colonAfterWideRangeSelector) {
        expr += ':';
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
        maybeSpace();
        expr += "open";
        break;
    case Interval::Closed:
        maybeSpace();
        expr += m_stateFlags & Off ? "off" : "closed";
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

int Rule::selectorCount() const
{
    const auto selectors = { (bool)m_yearSelector, (bool)m_monthdaySelector, (bool)m_weekSelector, (bool)m_weekdaySelector, (bool)m_timeSelector };
    return std::count(std::begin(selectors), std::end(selectors), true);
}
