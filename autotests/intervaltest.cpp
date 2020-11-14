/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/Interval>

#include <QTest>

using namespace KOpeningHours;

class IntervalTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testContains()
    {
        QDateTime dt({2020, 11, 7}, {18, 0});
        Interval i;
        QVERIFY(i.contains(dt));
        i.setBegin(QDateTime({2020, 11, 1}, {0, 0}));
        QVERIFY(i.contains(dt));
        QVERIFY(!i.contains(QDateTime({2020, 10, 1}, {})));
        i.setEnd(QDateTime({2020, 12, 1}, {}));
        QVERIFY(i.contains(dt));
        QVERIFY(!i.contains(QDateTime({2020, 12, 31}, {})));
        i.setBegin({});
        QVERIFY(i.contains(dt));
        QVERIFY(!i.contains(QDateTime({2020, 12, 31}, {})));

        i.setBegin(dt);
        QVERIFY(i.contains(dt));
    }

    void testIntersects()
    {
        Interval i, j;
        QVERIFY(i.intersects(j));
        i.setBegin(QDateTime({2020, 11, 7}, {18, 0}));
        QVERIFY(i.intersects(j));
        QVERIFY(j.intersects(i));
        i.setEnd(QDateTime({2020, 11, 7}, {20, 0}));
        QVERIFY(i.intersects(j));
        QVERIFY(j.intersects(i));

        j.setBegin(QDateTime({2020, 11, 7}, {19, 0}));
        QVERIFY(i.intersects(j));
        QVERIFY(j.intersects(i));
        j.setEnd(QDateTime({2020, 11, 7}, {21, 0}));
        QVERIFY(i.intersects(j));
        QVERIFY(j.intersects(i));

        j.setBegin(QDateTime({2020, 11, 7}, {20, 0}));
        QVERIFY(!i.intersects(j));
        QVERIFY(!j.intersects(i));
    }
};

QTEST_GUILESS_MAIN(IntervalTest)

#include "intervaltest.moc"
