/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "display.h"
#include "interval.h"
#include "openinghours.h"

#include <KLocalizedString>

#include <cmath>

using namespace KOpeningHours;

QString Display::currentState(const OpeningHours &oh)
{
    if (oh.error() != OpeningHours::NoError) {
        return {};
    }

    const auto now = QDateTime::currentDateTime();
    const auto i = oh.interval(now);
    if (i.hasOpenEnd()) {
        switch (i.state()) {
            case Interval::Open:
                return i.comment().isEmpty() ? i18n("Open") : i18n("Open (%1)", i.comment());
            case Interval::Closed:
                return i.comment().isEmpty() ? i18n("Closed") : i18n("Closed (%1)", i.comment());
            case Interval::Unknown:
            case Interval::Invalid:
                return {};
        }
    }

    const auto next = oh.nextInterval(i);
    // common transitions we have texts for
    if ((i.state() == Interval::Closed && next.state() == Interval::Open)
     || (i.state() == Interval::Open && next.state() == Interval::Closed)
    ) {

        // time to change is 90 min or less
        const auto timeToChange = now.secsTo(i.end());
        if (timeToChange < 90 * 60) {
            const int minDiff = std::ceil(timeToChange / 60.0);
            switch (i.state()) {
                case Interval::Open:
                    return i.comment().isEmpty()
                        ? i18np("Open for one more minute", "Open for %1 more minutes", minDiff)
                        : i18np("Open for one more minute (%2)", "Open for %1 more minutes (%2)", minDiff, i.comment());
                case Interval::Closed:
                    return i.comment().isEmpty()
                        ? i18np("Currently closed, opens in one minute", "Currently closed, opens in %1 minutes", minDiff)
                        : i18np("Currently closed (%2), opens in one minute", "Currently closed (%2), opens in %1 minutes", minDiff, i.comment());
                case Interval::Unknown:
                case Interval::Invalid:
                    return {};
            }
        }

        // time to change is 24h or less
        if (timeToChange < 24 * 60 * 60) {
            const int hourDiff = std::round(timeToChange / (60.0 * 60.0));
            switch (i.state()) {
                case Interval::Open:
                    return i.comment().isEmpty()
                        ? i18np("Open for one more hour", "Open for %1 more hours", hourDiff)
                        : i18np("Open for one more hour (%2)", "Open for %1 more hours (%2)", hourDiff, i.comment());
                case Interval::Closed:
                    return i.comment().isEmpty()
                        ? i18np("Currently closed, opens in one hour", "Currently closed, opens in %1 hours", hourDiff)
                        : i18np("Currently closed (%2), opens in one hour", "Currently closed (%2), opens in %1 hours", hourDiff, i.comment());
                case Interval::Unknown:
                case Interval::Invalid:
                    return {};
            }
        }

        // time to change is 7 days or less
        if (timeToChange < 7 * 24 * 60 * 60) {
            const int dayDiff = std::round(timeToChange / (24.0 * 60.0 * 60.0));
            switch (i.state()) {
                case Interval::Open:
                    return i.comment().isEmpty()
                        ? i18np("Open for one more day", "Open for %1 more days", dayDiff)
                        : i18np("Open for one more day (%2)", "Open for %1 more days (%2)", dayDiff, i.comment());
                case Interval::Closed:
                    return i.comment().isEmpty()
                        ? i18np("Currently closed, opens in one day", "Currently closed, opens in %1 days", dayDiff)
                        : i18np("Currently closed (%2), opens in one day", "Currently closed (%2), opens in %1 days", dayDiff, i.comment());
                case Interval::Unknown:
                case Interval::Invalid:
                    return {};
            }
        }
    }

    // next change is further ahead than one week, or we are transitioning in a way we don't handle above
    switch (i.state()) {
        case Interval::Open:
            return i.comment().isEmpty() ? i18n("Currently open") : i18n("Currently open (%1)", i.comment());
        case Interval::Closed:
            return i.comment().isEmpty() ? i18n("Currently closed") : i18n("Currently closed (%1)", i.comment());
        case Interval::Unknown:
            return i.comment();
        case Interval::Invalid:
            return {};
    }

    return {};
}
