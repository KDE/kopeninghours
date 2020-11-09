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
        QTest::addColumn<QDateTime>("end");

        // "now" is 2020-11-07 18:00 Saturday
        QTest::newRow("time only, following") << QByteArray("20:00-22:00") << QDateTime({2020, 11, 7}, {20, 0}) << QDateTime({2020, 11, 7}, {22, 0});
        QTest::newRow("time only, next day") << QByteArray("08:00-10:00") << QDateTime({2020, 11, 8}, {8, 0}) << QDateTime({2020, 11, 8}, {10, 0});
        QTest::newRow("time only, overlapping") << QByteArray("08:00-22:00") << QDateTime({2020, 11, 7}, {8, 0}) << QDateTime({2020, 11, 7}, {22, 0});

        //QTest::newRow("two time ranges") << QByteArray("08:00-10:00,20:00-22:00") << QDateTime({2020, 11, 7}, {20, 0}) << QDateTime({2020, 11, 7}, {22, 0});

        QTest::newRow("matching day") << QByteArray("Sa") << QDateTime({2020, 11, 7}, {0, 0}) << QDateTime({2020, 11, 7}, {23, 59});
        QTest::newRow("next day") << QByteArray("Su") << QDateTime({2020, 11, 8}, {0, 0}) << QDateTime({2020, 11, 8}, {23, 59});
        QTest::newRow("previous day") << QByteArray("Fr") << QDateTime({2020, 11, 13}, {0, 0}) << QDateTime({2020, 11, 13}, {23, 59});

        //QTest::newRow("matching day range") << QByteArray("Sa-Su") << QDateTime({2020, 11, 7}, {0, 0}) << QDateTime({2020, 11, 8}, {23, 59});

        QTest::newRow("day and time") << QByteArray("Su 20:00-22:00") << QDateTime({2020, 11, 8}, {20, 0}) << QDateTime({2020, 11, 8}, {22, 0});

        QTest::newRow("24/7") << QByteArray("24/7") << QDateTime() << QDateTime();

        QTest::newRow("current year") << QByteArray("2020") << QDateTime({2020, 1, 1}, {0, 0}) << QDateTime({2020, 12, 31}, {23, 59});
        QTest::newRow("overlapping year range") << QByteArray("1999-2022") << QDateTime({1999, 1, 1}, {0, 0}) << QDateTime({2022, 12, 31}, {23, 59});
        QTest::newRow("year set") << QByteArray("2010,2020,2030") << QDateTime({2020, 1, 1}, {0, 0}) << QDateTime({2020, 12, 31}, {23, 59});
        QTest::newRow("year set of ranges") << QByteArray("2010-2015,2020-2025,2030") << QDateTime({2020, 1, 1}, {0, 0}) << QDateTime({2025, 12, 31}, {23, 59});
    }

    void testNext()
    {
        QFETCH(QByteArray, expression);
        QFETCH(QDateTime, begin);
        QFETCH(QDateTime, end);

        OpeningHours oh(expression);
        QCOMPARE(oh.error(), OpeningHours::NoError);
        const auto i = oh.interval(QDateTime({2020, 11, 7}, {18, 0}));
        QVERIFY(i.isValid());
        QCOMPARE(i.begin(), begin);
        QCOMPARE(i.end(), end);
        QCOMPARE(i.state(), Interval::Open);
    }

    void testNoMatch()
    {
        OpeningHours oh("1980-2000");
        QCOMPARE(oh.error(), OpeningHours::NoError);
        const auto i = oh.interval(QDateTime({2020, 11, 7}, {18, 0}));
        QVERIFY(!i.isValid());
    }
};

QTEST_GUILESS_MAIN(EvaluateTest)

#include "evaluatetest.moc"
