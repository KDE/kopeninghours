/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/Interval>
#include <KOpeningHours/OpeningHours>

#include <QTest>

using namespace KOpeningHours;

class EvaluateTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testNext_data()
    {
        QTest::addColumn<QByteArray>("expression");
        QTest::addColumn<QDateTime>("begin");

        QTest::newRow("20:00-22:00") << QByteArray("20:00-22:00") << QDateTime({2020, 11, 7}, {20, 0});
    }

    void testNext()
    {
        QFETCH(QByteArray, expression);
        QFETCH(QDateTime, begin);

        OpeningHours oh(expression);
        const auto i = oh.interval(QDateTime({2020, 11, 7}, {18, 0}));
        QCOMPARE(i.begin(), begin);
    }
};

QTEST_GUILESS_MAIN(EvaluateTest)

#include "evaluatetest.moc"
