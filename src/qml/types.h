/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_QMLPLUGIN_TYPES_H
#define KOPENINGHOURS_QMLPLUGIN_TYPES_H

#include <KOpeningHours/Interval>
#include <KOpeningHours/IntervalModel>
#include <KOpeningHours/OpeningHours>

#include <QQmlEngine>

struct IntervalForeign {
    Q_GADGET
    QML_FOREIGN(KOpeningHours::Interval)
    QML_VALUE_TYPE(interval)
};

class IntervalEnums : public KOpeningHours::Interval {
    Q_GADGET
};
namespace IntervalEnumsForeign {
    Q_NAMESPACE
    QML_NAMED_ELEMENT(Interval)
    QML_FOREIGN_NAMESPACE(IntervalEnums)
}

struct InvervalModelForeign {
    Q_GADGET
    QML_FOREIGN(KOpeningHours::IntervalModel)
    QML_NAMED_ELEMENT(IntervalModel)
};

struct OpeningHoursForeign {
    Q_GADGET
    QML_FOREIGN(KOpeningHours::OpeningHours)
    QML_VALUE_TYPE(openingHours)
};

class OpeningHoursEnums : public KOpeningHours::OpeningHours {
    Q_GADGET
};
namespace OpeningHoursEnumsForeign {
    Q_NAMESPACE
    QML_NAMED_ELEMENT(OpeningHours)
    QML_FOREIGN_NAMESPACE(OpeningHoursEnums)
}

#endif
