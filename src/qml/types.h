/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_QMLPLUGIN_TYPES_H
#define KOPENINGHOURS_QMLPLUGIN_TYPES_H

#include <KOpeningHours/IntervalModel>

#include <QQmlEngine>

struct InvervalModelForeign {
    Q_GADGET
    QML_FOREIGN(KOpeningHours::IntervalModel)
    QML_NAMED_ELEMENT(IntervalModel)
};

#endif
