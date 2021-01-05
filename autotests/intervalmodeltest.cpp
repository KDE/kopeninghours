/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/Interval>
#include <KOpeningHours/IntervalModel>

#include <QAbstractItemModelTester>
#include <QDateTime>
#include <QTest>

using namespace KOpeningHours;

class IntervalModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testModel()
    {
        IntervalModel model;
        QAbstractItemModelTester modelTest(&model);

        OpeningHours oh("Mo,We,Fr 10:00-20:00; Su 08:00-14:00");
        QCOMPARE(oh.error(), OpeningHours::NoError);

        model.setOpeningHours(oh);
        model.setBeginDate({2020, 11, 2});
        model.setEndDate({2020, 11, 9});

        QCOMPARE(model.rowCount(), 7);
        for (int i = 0; i < model.rowCount(); ++i) {
            const auto idx = model.index(i, 0);
            const auto dt = idx.data(IntervalModel::DateRole).toDate();
            QCOMPARE(dt, QDate(2020, 11, 2).addDays(i));
            const auto intervals = idx.data(IntervalModel::IntervalsRole).value<std::vector<Interval>>();
            QCOMPARE(intervals.size(), (i % 2) ? 1 : 3);

            QCOMPARE(intervals.front().begin(), QDateTime(dt, {0, 0}));
            QCOMPARE(intervals.back().end(), QDateTime(dt.addDays(1), {0, 0}));
        }
    }

    void testModelOpenIntervals()
    {
        IntervalModel model;
        QAbstractItemModelTester modelTest(&model);

        OpeningHours oh("24/7");
        QCOMPARE(oh.error(), OpeningHours::NoError);

        model.setOpeningHours(oh);
        model.setBeginDate({2020, 11, 2});
        model.setEndDate({2020, 11, 9});

        QCOMPARE(model.rowCount(), 7);
        for (int i = 0; i < model.rowCount(); ++i) {
            const auto idx = model.index(i, 0);
            const auto dt = idx.data(IntervalModel::DateRole).toDate();
            QCOMPARE(dt, QDate(2020, 11, 2).addDays(i));
            const auto intervals = idx.data(IntervalModel::IntervalsRole).value<std::vector<Interval>>();
            QCOMPARE(intervals.size(), 1);

            QCOMPARE(intervals.front().begin(), QDateTime(dt, {0, 0}));
            QCOMPARE(intervals.back().end(), QDateTime(dt.addDays(1), {0, 0}));
        }
    }

    void testOpenEndTime()
    {
        IntervalModel model;
        QAbstractItemModelTester modelTest(&model);

        OpeningHours oh("23:00-01:00+");
        QCOMPARE(oh.error(), OpeningHours::NoError);

        model.setOpeningHours(oh);
        model.setBeginDate({2020, 11, 2});
        model.setEndDate({2020, 11, 3});

        QCOMPARE(model.rowCount(), 1);
        const auto intervals = model.index(0, 0).data(IntervalModel::IntervalsRole).value<std::vector<Interval>>();
        QCOMPARE(intervals.size(), 3);
        QVERIFY(intervals[0].hasOpenEndTime());
        QVERIFY(!intervals[1].hasOpenEndTime());
        QVERIFY(!intervals[2].hasOpenEndTime());
        QCOMPARE(intervals[0].estimatedEnd(), intervals[1].begin());
        QCOMPARE(intervals[1].end(), intervals[2].begin());
        QVERIFY(intervals[0].end() < intervals[0].estimatedEnd());

        QCOMPARE(intervals[0].state(), Interval::Open);
        QCOMPARE(intervals[1].state(), Interval::Closed);
        QCOMPARE(intervals[2].state(), Interval::Open);
    }
};

QTEST_GUILESS_MAIN(IntervalModelTest)

#include "intervalmodeltest.moc"
