/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/OpeningHours>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace KOpeningHours;

class JsonLdTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QLocale::setDefault(QLocale::c());
    }

    void testJsonLd_data()
    {
        QTest::addColumn<QString>("inputFile");
        QTest::addColumn<QByteArray>("osmExpr");

#define T(json, osm) QTest::newRow(json) << QStringLiteral(SOURCE_DIR "/jsonlddata/" json ".json") << QByteArray(osm)
        T("oh-simple", "Mo,Tu,We,Th 09:00-12:00");
        T("oh-array", "Mo-Fr 10:00-19:00; Sa 10:00-22:00; Su 10:00-21:00");
        T("ohs-example", "Su 09:00-17:00 open; Sa 09:00-16:00 open; Th 09:00-15:00 open; Tu 09:00-14:00 open; Fr 09:00-13:00 open; Mo 09:00-12:00 open; We 09:00-11:00 open");
        T("ohs-mixed", "Mo,Tu,We,Th,Fr,Sa,Su 09:00-14:00; 2013 Dec 24-25 09:00-11:00 open; 2014 Jan 01 12:00-14:00 open");
#undef T
    }

    void testJsonLd()
    {
        QFETCH(QString, inputFile);
        QFETCH(QByteArray, osmExpr);

        QFile inFile(inputFile);
        QVERIFY(inFile.open(QFile::ReadOnly));

        const auto obj = QJsonDocument::fromJson(inFile.readAll()).object();
        QVERIFY(!obj.isEmpty());

        auto oh = OpeningHours::fromJsonLd(obj);
        QCOMPARE(oh.error(), OpeningHours::NoError);
        QCOMPARE(oh.normalizedExpression(), osmExpr);
    }
};

QTEST_GUILESS_MAIN(JsonLdTest)

#include "jsonldtest.moc"
