/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <../src/lib/easter.cpp>

#include <QTest>

using namespace KOpeningHours;

class EasterTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testEaster_data()
    {
        QTest::addColumn<int>("year");
        QTest::addColumn<QDate>("date");

        QTest::newRow("2001") << 2001 << QDate(2001, 4, 15);
        QTest::newRow("2020") << 2020 << QDate(2020, 4, 12);
        QTest::newRow("2021") << 2021 << QDate(2021, 4, 4);
        QTest::newRow("2022") << 2022 << QDate(2022, 4, 17);
        QTest::newRow("2023") << 2023 << QDate(2023, 4, 9);
        QTest::newRow("2024") << 2024 << QDate(2024, 3, 31);
        QTest::newRow("2025") << 2025 << QDate(2025, 4, 20);
    }

    void testEaster()
    {
        QFETCH(int, year);
        QFETCH(QDate, date);
        QCOMPARE(Easter::easterDate(year), date);
        QCOMPARE(date.dayOfWeek(), 7);
    }
};

QTEST_GUILESS_MAIN(EasterTest)

#include "eastertest.moc"
