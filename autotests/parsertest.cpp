/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/OpeningHours>

#include <QTest>

using namespace KOpeningHours;

class ParserTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSuccess_data()
    {
        QTest::addColumn<QByteArray>("input");

#define T(x) QTest::newRow(x) << QByteArray(x)
        T("24/7");
        T("24/7 \"comment\"");
        T("24/7 closed");
        T("24/7 unknown \"comment\"");
        T("Dec off");
        T("Dec 25 off");
        T("Dec 25-26 off");
        T("Dec 08:00");
        T("Dec 08:00-14:00");
        T("easter off");
        T("easter +1 day off");
        T("easter -2 days off");
        T("2020");
        T("2020-2021");
        T("1970-2022/2");
        T("2020+");
        T("PH off || open");
        T("PH off || unknown \"foo\"");

        // from https://wiki.openstreetmap.org/wiki/Key:opening_hours#Simple_examples
        T("Mo-Fr 08:00-17:30");
        T("Mo-Fr 08:00-12:00,13:00-17:30");
        T("Mo-Fr 08:00-12:00,13:00-17:30; Sa 08:00-12:00");
        T("Mo-Fr 08:00-12:00,13:00-17:30; Sa 08:00-12:00; PH off");
        T("Mo-Fr 08:00-12:00,13:00-17:30; Sa 08:00-12:00; PH 09:00-12:00");

        // from https://wiki.openstreetmap.org/wiki/Key:opening_hours#Examples
        T("Sa-Su 00:00-24:00");
        T("Mo-Fr 08:30-20:00");
        T("Mo 10:00-12:00,12:30-15:00; Tu-Fr 08:00-12:00,12:30-15:00; Sa 08:00-12:00");
        T("Mo-Su 08:00-18:00; Apr 10-15 off; Jun 08:00-14:00; Aug off; Dec 25 off");
        T("Mo-Sa 10:00-20:00; Tu off");
        T("Mo-Sa 10:00-20:00; Tu 10:00-14:00");
        T("sunrise-sunset");
        T("Su 10:00+");
        T("week 1-53/2 Fr 09:00-12:00; week 2-52/2 We 09:00-12:00");
        T("Mo-Sa 08:00-13:00,14:00-17:00 || \"by appointment\"");
        T("Su-Tu 11:00-01:00, We-Th 11:00-03:00, Fr 11:00-06:00, Sa 11:00-07:00");
        T("Mo-Su,PH 15:00-03:00; easter -2 days off");
#undef T
    }

    void testSuccess()
    {
        QFETCH(QByteArray, input);
        OpeningHours oh(input);
        QVERIFY(oh.error() != OpeningHours::SyntaxError);
    }

    void testFail_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<OpeningHours::Error>("error");

#define T(x) QTest::newRow(x) << QByteArray(x) << OpeningHours::SyntaxError
        T("23/7");
        T("\"comment\"");
        T("24/7 geöffnet");
        T("2020-2000");
        T("PH off || 2020 open");
        T("PH off || open || unknown");

        // from https://wiki.openstreetmap.org/wiki/Key:opening_hours#Common_mistakes
        T("7/8-23");
        T("0600-1800");
        T("07;00-2;00pm");
        T("08.00-16.00, public room till 03.00 a.m");
        T("09:00-21:00 TEL/072(360)3200");
        T("10:00 - 13:30 / 17:00 - 20:30");
        T("April-September; Mo-Fr 09:00-13:00, 14:00-18:00, Sa 10:00-13:00");
        T("Dining in: 6am to 11pm; Drive thru: 24/7");
        T("MWThF: 1200-1800; SaSu: 1200-1700");
        T("BAR: Su-Mo 18:00-02:00; Tu-Th 18:00-03:00; Fr-Sa 18:00-04:00; CLUB: Tu-Th 20:00-03:00; Fr-Sa 20:00-04:00");
#undef T
    }

    void testFail()
    {
        QFETCH(QByteArray, input);
        QFETCH(OpeningHours::Error, error);
        OpeningHours oh(input);
        QCOMPARE(oh.error(), error);
    }

    void testValidation_data()
    {
        QTest::addColumn<QByteArray>("expression");
        QTest::addColumn<OpeningHours::Error>("error");

        QTest::newRow("location") << QByteArray("sunrise-sunset") << OpeningHours::MissingLocation;
        QTest::newRow("PH") << QByteArray("PH off") << OpeningHours::MissingRegion;
        QTest::newRow("SH") << QByteArray("SH off") << OpeningHours::UnsupportedFeature;
        QTest::newRow("year interval") << QByteArray("1980-2030/4") << OpeningHours::UnsupportedFeature;
        QTest::newRow("time interval") << QByteArray("10:00-16:00/90") << OpeningHours::UnsupportedFeature;
        QTest::newRow("nth day") << QByteArray("Mo[1-2,4]") << OpeningHours::UnsupportedFeature;
        QTest::newRow("nth day offset") << QByteArray("We[-1] + 2 days") << OpeningHours::UnsupportedFeature;
        QTest::newRow("week wrap") << QByteArray("week 45-13") << OpeningHours::UnsupportedFeature;
        QTest::newRow("single timepoint") << QByteArray("10:00") << OpeningHours::UnsupportedFeature;
        QTest::newRow("single timepoint range") << QByteArray("10:00-10:00") << OpeningHours::UnsupportedFeature;
    }

    void testValidation()
    {
        QFETCH(QByteArray, expression);
        QFETCH(OpeningHours::Error, error);

        OpeningHours oh(expression);
        QCOMPARE(oh.error(), error);
    }
};

QTEST_GUILESS_MAIN(ParserTest)

#include "parsertest.moc"
