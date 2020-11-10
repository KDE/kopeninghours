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
        QTest::newRow("minutes") << QByteArray("18:45-21:15") << QDateTime({2020, 11, 7}, {18, 45}) << QDateTime({2020, 11, 7}, {21, 15});

        QTest::newRow("two time ranges") << QByteArray("08:00-10:00,20:00-22:00") << QDateTime({2020, 11, 7}, {20, 0}) << QDateTime({2020, 11, 7}, {22, 0});
        QTest::newRow("time ranges next day") << QByteArray("08:30-10:45,12:00-13:30,14:00-15:00") << QDateTime({2020, 11, 8}, {8, 30}) << QDateTime({2020, 11, 8}, {10, 45});

        QTest::newRow("matching day") << QByteArray("Sa") << QDateTime({2020, 11, 7}, {0, 0}) << QDateTime({2020, 11, 7}, {23, 59});
        QTest::newRow("next day") << QByteArray("Su") << QDateTime({2020, 11, 8}, {0, 0}) << QDateTime({2020, 11, 8}, {23, 59});
        QTest::newRow("previous day") << QByteArray("Fr") << QDateTime({2020, 11, 13}, {0, 0}) << QDateTime({2020, 11, 13}, {23, 59});
        QTest::newRow("day set") << QByteArray("Fr,Sa,Su") << QDateTime({2020, 11, 7}, {0, 0}) << QDateTime({2020, 11, 7}, {23, 59});

        QTest::newRow("matching day range1") << QByteArray("Sa-Su") << QDateTime({2020, 11, 7}, {0, 0}) << QDateTime({2020, 11, 8}, {23, 59});
        QTest::newRow("matching day range2") << QByteArray("Fr-Su") << QDateTime({2020, 11, 6}, {0, 0}) << QDateTime({2020, 11, 8}, {23, 59});
        QTest::newRow("matching day range2") << QByteArray("Mo-Tu,Th-Fr") << QDateTime({2020, 11, 9}, {0, 0}) << QDateTime({2020, 11, 10}, {23, 59});

        QTest::newRow("day and time") << QByteArray("Su 20:00-22:00") << QDateTime({2020, 11, 8}, {20, 0}) << QDateTime({2020, 11, 8}, {22, 0});

        QTest::newRow("24/7") << QByteArray("24/7") << QDateTime() << QDateTime();

        QTest::newRow("current year") << QByteArray("2020") << QDateTime({2020, 1, 1}, {0, 0}) << QDateTime({2020, 12, 31}, {23, 59});
        QTest::newRow("overlapping year range") << QByteArray("1999-2022") << QDateTime({1999, 1, 1}, {0, 0}) << QDateTime({2022, 12, 31}, {23, 59});
        QTest::newRow("year set") << QByteArray("2010,2020,2030") << QDateTime({2020, 1, 1}, {0, 0}) << QDateTime({2020, 12, 31}, {23, 59});
        QTest::newRow("year set of ranges") << QByteArray("2010-2015,2020-2025,2030") << QDateTime({2020, 1, 1}, {0, 0}) << QDateTime({2025, 12, 31}, {23, 59});
        QTest::newRow("year open end") << QByteArray("2010+") << QDateTime({2010, 1, 1}, {0, 0}) << QDateTime();

        QTest::newRow("current month") << QByteArray("Nov") << QDateTime({2020, 11, 1}, {0, 0}) << QDateTime({2020, 11, 30}, {23, 59});
        QTest::newRow("next month") << QByteArray("Dec") << QDateTime({2020, 12, 1}, {0, 0}) << QDateTime({2020, 12, 31}, {23, 59});
        QTest::newRow("prev month") << QByteArray("Oct") << QDateTime({2021, 10, 1}, {0, 0}) << QDateTime({2021, 10, 31}, {23, 59});
        QTest::newRow("overlapping month range") << QByteArray("Oct-Dec") << QDateTime({2020, 10, 1}, {0, 0}) << QDateTime({2020, 12, 31}, {23, 59});
        QTest::newRow("month sets") << QByteArray("Oct,Nov,Dec") << QDateTime({2020, 11, 1}, {0, 0}) << QDateTime({2020, 11, 30}, {23, 59});

        QTest::newRow("month/day") << QByteArray("Nov 7") << QDateTime({2020, 11, 7}, {0, 0}) << QDateTime({2020, 11, 7}, {23, 59});
        QTest::newRow("next month/day") << QByteArray("Nov 8") << QDateTime({2020, 11, 8}, {0, 0}) << QDateTime({2020, 11, 8}, {23, 59});
        QTest::newRow("prev month/day") << QByteArray("Nov 6") << QDateTime({2021, 11, 6}, {0, 0}) << QDateTime({2021, 11, 6}, {23, 59});
        QTest::newRow("month/day set")  << QByteArray("Oct 6,Nov 8,Dec 10") << QDateTime({2020, 11, 8}, {0, 0}) << QDateTime({2020, 11, 8}, {23, 59});
        QTest::newRow("month/day range")  << QByteArray("Oct 6-Dec 10") << QDateTime({2020, 10, 6}, {0, 0}) << QDateTime({2020, 12, 10}, {23, 59});
        // TODO infinite eval loop
//         QTest::newRow("month/day range year wrap")  << QByteArray("Oct 6-Mar 10") << QDateTime({2020, 10, 6}, {0, 0}) << QDateTime({2021, 3, 10}, {23, 59});

        // TODO parser error
//         QTest::newRow("year/month") << QByteArray("2020 Nov") << QDateTime({2020, 11, 1}, {0, 0}) << QDateTime({2020, 11, 30}, {23, 59});

        QTest::newRow("full date") << QByteArray("2020 Nov 7") << QDateTime({2020, 11, 7}, {0, 0}) << QDateTime({2020, 11, 7}, {23, 59});
        QTest::newRow("full date next year") << QByteArray("2021 Nov 7") << QDateTime({2021, 11, 7}, {0, 0}) << QDateTime({2021, 11, 7}, {23, 59});
        QTest::newRow("full date range") << QByteArray("2020 Nov 6-2020 Dec 13") << QDateTime({2020, 11, 6}, {0, 0}) << QDateTime({2020, 12, 13}, {23, 59});

        QTest::newRow("week") << QByteArray("week 45") << QDateTime({2020, 11, 2}, {0, 0}) << QDateTime({2020, 11, 8}, {23, 59});
        QTest::newRow("next week") << QByteArray("week 46") << QDateTime({2020, 11, 9}, {0, 0}) << QDateTime({2020, 11, 15}, {23, 59});
        QTest::newRow("prev week") << QByteArray("week 44") << QDateTime({2021, 11, 1}, {0, 0}) << QDateTime({2021, 11, 7}, {23, 59});
        QTest::newRow("week range") << QByteArray("week 44-46") << QDateTime({2020, 10, 26}, {0, 0}) << QDateTime({2020, 11, 15}, {23, 59});
        QTest::newRow("week set") << QByteArray("week 42,45,46") << QDateTime({2020, 11, 2}, {0, 0}) << QDateTime({2020, 11, 8}, {23, 59});
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

    void testNoMatch_data()
    {
        QTest::addColumn<QByteArray>("expression");
        QTest::newRow("year range") << QByteArray("1980-2000");
        QTest::newRow("full date") << QByteArray("2020 Nov 6");
        QTest::newRow("date range") << QByteArray("1980 Jan 1-2020 Nov 6");
    }

    void testNoMatch()
    {
        QFETCH(QByteArray, expression);
        OpeningHours oh(expression);
        QCOMPARE(oh.error(), OpeningHours::NoError);
        const auto i = oh.interval(QDateTime({2020, 11, 7}, {18, 0}));
        QVERIFY(!i.isValid());
    }

    void testFractions()
    {
        OpeningHours oh("14:00-15:00");
        QCOMPARE(oh.error(), OpeningHours::NoError);
        const auto i = oh.interval(QDateTime({2020, 11, 7}, {15, 55, 34, 123}));
        QVERIFY(i.isValid());
        QCOMPARE(i.begin(), QDateTime({2020, 11, 8}, {14, 0}));
        QCOMPARE(i.end(), QDateTime({2020, 11, 8}, {15, 0}));
    }
};

QTEST_GUILESS_MAIN(EvaluateTest)

#include "evaluatetest.moc"
