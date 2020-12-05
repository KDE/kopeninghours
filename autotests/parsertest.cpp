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
        QTest::addColumn<QByteArray>("expectedOutput");

#define T(x) QTest::newRow(x) << QByteArray(x) << QByteArray(x)
#define T2(x, y) QTest::newRow(x) << QByteArray(x) << QByteArray(y)
        T("24/7");
        T("24/7 \"comment\"");
        T2("24/7 closed", "24/7 off");
        T2("24/7 unknown \"comment\"", "unknown \"comment\"");
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
        T("PH off || open"); // https://openingh.openstreetmap.de/evaluation_tool/ says this means always open... bug in opening.js?
        T("PH off || unknown \"foo\"");
        T("2020 Jan-Apr");
        T("1980-2030/4");
        T2("\"comment\"", "unknown \"comment\"");
        T("PH off || 2020 open");
        T("Mo[1-2,4]");
        T2("We[-1] + 2 days", "We[-1] +2 days");
        T("10:00-16:00/90");
        T("10:00-16:00/1:30");
        T("10:00-10:00");

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
        T2("week 1-53/2 Fr 09:00-12:00; week 2-52/2 We 09:00-12:00", "week 01-53/2 Fr 09:00-12:00; week 02-52/2 We 09:00-12:00");
        T("Mo-Sa 08:00-13:00,14:00-17:00 || \"by appointment\"");
        T("Su-Tu 11:00-01:00, We-Th 11:00-03:00, Fr 11:00-06:00, Sa 11:00-07:00");
        T("Mo-Su,PH 15:00-03:00; easter -2 days off");

        // from https://openingh.openstreetmap.de/evaluation_tool/
        T("Mo-Fr 10:00-20:00; PH off");
        T("Mo,Tu,Th,Fr 12:00-18:00; Sa,PH 12:00-17:00; Th[3],Th[-1] off");
        T("00:00-24:00; Tu-Su,PH 08:30-09:00 off; Tu-Su 14:00-14:30 off; Mo 08:00-13:00 off");
        T("Fr-Sa 18:00-06:00; PH off");
        T("Mo 10:00-12:00,12:30-15:00");
        T("Mo 10:00-12:00,12:30-15:00; Tu-Fr 08:00-12:00,12:30-15:00; Sa 08:00-12:00");
        T("\"only after registration\"; PH off");
        T("22:00-23:00; PH off");
        T("08:00-11:00; PH off");
        T("open; Mo 15:00-16:00 off; PH off");
        T("Mo-Su 22:00-23:00; We,PH off");
        T("We-Fr 10:00-24:00 open \"it is open\" || \"please call\"; PH off");
        T("Mo-Fr 08:00-11:00 || Tu-Th,PH open \"Emergency only\"");
        T("Tu-Th,We 22:00-23:00 open \"Hot meals\"; PH off");
        T("Mo 12:00-14:00 open \"female only\", Mo 14:00-16:00 open \"male only\"; PH off");
        T2("Apr: 22:00-23:00; PH off", "Apr 22:00-23:00; PH off");
        T2("Jul-Jan: 22:00-23:00; PH off", "Jul-Jan 22:00-23:00; PH off");
        T2("Jan-Jul: 22:00-23:00; PH off", "Jan-Jul 22:00-23:00; PH off");
        T2("Jul 23-Jan 3: \"needs reservation by phone\"; PH off", "Jul 23-Jan 03 \"needs reservation by phone\"; PH off");
        T2("Jul 23-Jan 3: 22:00-23:00 \"Please make a reservation by phone.\"; PH off", "Jul 23-Jan 03 22:00-23:00 \"Please make a reservation by phone.\"; PH off");
        T2("Jul 23-Jan 3: 08:00-11:00 \"Please make a reservation by phone.\"; PH off", "Jul 23-Jan 03 08:00-11:00 \"Please make a reservation by phone.\"; PH off");
        T2("Jan 23-Jul 3: 22:00-23:00 \"Please make a reservation by phone.\"; PH off", "Jan 23-Jul 03 22:00-23:00 \"Please make a reservation by phone.\"; PH off");
//         T("Mar Su[-1]-Dec Su[1] -2 days: 22:00-23:00; PH off");
        T("Sa[1],Sa[1] +1 day 10:00-12:00 open \"first weekend in the month\"; PH off");
        T("Sa[-1],Sa[-1] +1 day 10:00-12:00 open \"last weekend in the month\"; PH off");
        T("Sa-Su 00:00-24:00; PH off");
        T("Mo-Fr 00:00-24:00; PH off");
        T("sunrise-sunset open \"Beware of sunburn!\"; PH off");
        T("sunset-sunrise open \"Beware of vampires!\"; PH off");
        T("(sunset+01:00)-24:00 || closed \"No drink before sunset!\"; PH off");
        T("22:00+; PH off");
        T("Tu,PH 23:59-22:59");
        T("We-Mo,PH 23:59-22:59");
        T2("week 2-52/2 We 00:00-24:00; week 1-53/2 Sa 00:00-24:00; PH off", "week 02-52/2 We 00:00-24:00; week 01-53/2 Sa 00:00-24:00; PH off");
        T2("week 4-16 We 00:00-24:00; week 38-42 Sa 00:00-24:00; PH off", "week 04-16 We 00:00-24:00; week 38-42 Sa 00:00-24:00; PH off");
//         T("2012 easter -2 days-2012 easter +2 days: open \"Around easter\"; PH off");
        T("24/7 closed \"always closed\"");
        T("Jan 23-Feb 11,Feb 12 00:00-24:00; PH off");
        T("Apr-Oct Su[2] 14:00-18:00; Aug Su[-1] -1 day 10:00-18:00; Aug Su[-1] 10:00-18:00; PH off");
        T("Mo-Fr 08:00-12:00, We 14:00-18:00; Su,PH off"); // open We morning too
        T("Mo-Fr 08:00-12:00; We 14:00-18:00; Su,PH off"); // closed We morning

        T("We; PH off");
        T("PH");
        T("PH Mo-Fr");
        T("PH -1 day");
        T("SH");
        T("SH,PH");
        T("PH,SH");
        T("We[1-3]");
        T("We[3-5]");
        T("Sa");
        T("Sa[1]");
        T("Sa[1-3]");
        T("Tu-Th");
        T("Fr-Mo");
        T("Mo-Su; We \"only after registration\"");
        T2("Oct: We[1]", "Oct We[1]");

        // from https://github.com/dfaure/DataNovaImportScripts/blob/master/saved_opening_hours
        T("Mo-Tu,Th-Fr 09:30-12:00; 2020 Dec 28 off; 2020 Dec 22,2020 Dec 29 off; We 15:00-17:00; 2020 Dec 23,2020 Dec 30 off; 2020 Dec 24,2020 Dec 31 off; Sa 10:00-12:00; 2020 Dec 26,2021 Jan 02 off; PH off");

        // technically wrong but often found content in OSM for which we have error recovery
        T2("So", "Su");
        T2("Ph", "PH");
        T2("9:00-12:00", "09:00-12:00");
        T2("Mo-Fr 09:00-18:30;Sa 09:00-17:00", "Mo-Fr 09:00-18:30; Sa 09:00-17:00");
        T2("08:00-12:00;", "08:00-12:00");
        T2("14:00-20:00,", "14:00-20:00");
        T("Mo 14:00-21:00; Tu-Th 10:00-21:00; Fr 10:00-18:00;Su, PH off|| \"Samstag zweimal im Monat, Details siehe Webseite\"");
        T2("we-mo 11:30-14:00, 17:30-22:00; tu off", "We-Mo 11:30-14:00, 17:30-22:00; Tu off");

        // Tolerance for incorrect casing
        T2("mo-fr 10:00-20:00", "Mo-Fr 10:00-20:00");
        T2("jan-feb 10:00-20:00", "Jan-Feb 10:00-20:00");
        T2("jan-feb,aug 10:00-20:00", "Jan-Feb,Aug 10:00-20:00");
        T2("SUNRISE-SUNSET", "sunrise-sunset");
        T2("(SUNrISE-01:00)-(SUnsET+01:00)", "(sunrise-01:00)-(sunset+01:00)");
        T2("su,sh off", "Su,SH off");
        T2("mo-fr CLOSED", "Mo-Fr off");

        // Unicode symbols
        T("Mo–Tu");
#undef T
    }

    void testSuccess()
    {
        QFETCH(QByteArray, input);
        QFETCH(QByteArray, expectedOutput);
        OpeningHours oh(input);
        QVERIFY(oh.error() != OpeningHours::SyntaxError);
        //QCOMPARE(oh.normalizedExpression(), expectedOutput);
    }

    void testFail_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<OpeningHours::Error>("error");

#define T(x) QTest::newRow(x) << QByteArray(x) << OpeningHours::SyntaxError
        T("23/7");
        T("24/7 geöffnet");
        T("2020-2000");
        T("PH off || open || unknown");
        T("Jan-Apr 1");
        T("Feb-2020 Apr 1");
        T("Apr 1-Nov");
        T("Su[0]");
        T("Mo[6]");
        T("Mo[-0]");
        T("Tu[-6]");
        T("Mo[0-5]");
        T("We[4-2]");

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

        // from https://openingh.openstreetmap.de/evaluation_tool/
        T("2013,2015,2050-2053,2055/2,2020-2029/3,2060+ Jan 1"); // periodic open end year selectors are a non-standard extension
        T("00:00-24:00 week 6 Mo-Su Feb; PH off");
        T("monday, Tu, wE, TH 12:00 - 20:00 ; 14:00-16:00 Off ; closed public Holiday");
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
        QTest::newRow("time interval") << QByteArray("10:00-16:00/90") << OpeningHours::IncompatibleMode;
        QTest::newRow("time interval 2") << QByteArray("10:00-16:00/1:30") << OpeningHours::IncompatibleMode;
        QTest::newRow("week wrap") << QByteArray("week 45-13") << OpeningHours::UnsupportedFeature;
        QTest::newRow("unsupported") << QByteArray("Su 10:00+") << OpeningHours::UnsupportedFeature;
        QTest::newRow("single timepoint") << QByteArray("10:00") << OpeningHours::IncompatibleMode;
        QTest::newRow("month timepoint") << QByteArray("Dec 08:00") << OpeningHours::IncompatibleMode;
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
