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
        QTest::addColumn<QByteArray>("expectedSimplifiedOutput");

#define T(x) QTest::newRow(x) << QByteArray(x) << QByteArray(x) << QByteArray(x)
#define T2(x, y) QTest::newRow(x) << QByteArray(x) << QByteArray(y) << QByteArray(y)
#define T3(x, y, z) QTest::newRow(x) << QByteArray(x) << QByteArray(y ? y : x) << QByteArray(z)
        T("24/7");
        T("24/7 \"comment\"");
        T("24/7 closed");
        T("24/7 unknown \"comment\"");
        T("unknown \"comment\"");
        T("off");
        T("Dec off");
        T("Dec 25 off");
        T("Dec 25-26 off");
        T2("Dec 24-26,31 off", "Dec 24-26,Dec 31 off");
        T2("Jan 1,6 off", "Jan 01,Jan 06 off");
        T2("Dec 24,25,26", "Dec 24,Dec 25,Dec 26");
        T2("Jan 03,Dec 04,24 off", "Jan 03,Dec 04,Dec 24 off");
        T2("Jan 03, Dec 04, 24 off", "Jan 03,Dec 04,Dec 24 off");
        T2("07:30-20:00; Jan 03,13,23,Dec 04,14,24 off", "07:30-20:00; Jan 03,Jan 13,Jan 23,Dec 04,Dec 14,Dec 24 off");
        T("Dec 08:00");
        T("Dec 08:00-14:00");
        T("easter off");
        T("easter +1 day off");
        T("easter -2 days off");
        T2("whitsun off", "easter +49 days off");
        T2("whitsun +1 day off", "easter +50 days off");
        T("2020");
        T("2020-2021");
        T("1970-2022/2");
        T("2020+");
        T("2010,2020,2030");
        T("2010-2015,2020-2025,2030");
        T("2020-2022 Dec");
        T("2020 Dec-2022 Dec");
        T("2020-2022 Dec 24-26");
        T("2020 Dec 24-26");
        T("2021 10:00-20:00");
        T("PH off || open"); // https://openingh.openstreetmap.de/evaluation_tool/ says this means always open... bug in opening.js?
        T("PH off || unknown \"foo\"");
        T("2020 Jan-Apr");
        T("1980-2030/4");
        T("\"comment\"");
        T("PH off || 2020 open");
        T("Mo[1-2,4]");
        T2("We[-1] + 2 days", "We[-1] +2 days");
        T("10:00-16:00/15");
        T2("10:00-16:00/90", "10:00-16:00/01:30");
        T2("10:00-16:00/1:30", "10:00-16:00/01:30");
        T("10:00-10:00");
        T("PH off || open || unknown");
        T("10:00-12:00+");
        T("Jun 15-Aug 15 Mo-Fr 10:00-12:30");
        T2("Dec 01 +Su", "Dec 01 Su[1]");
        T2("Dec 01 -Su 08:00-12:00", "Dec 01 Su[-1] 08:00-12:00");
        T("Aug Mo[1]-Aug Sa[-1] closed");
        T("2020/2");
        T("\"Außerhalb der Semesterferien\": Mo-Fr 08:00-22:00; Sa,Su 10:00-20:00; \"Innerhalb der Semesterferien\": Mo-Fr 08:00-18:00; Sa,Su 10:00-16:00");
        T("PH Mo-Th 14:00-23:00");
        T2("Mo-Th PH 14:00-23:00", "PH Mo-Th 14:00-23:00");
        T("PH Fr,SH Fr 11:30-02:00");
        T2("PH Fr, SH Fr 11:30-02:00", "PH Fr,SH Fr 11:30-02:00");
        T2("PH Th,Fr, SH Tu,Fr 11:30-02:00", "PH Th,Fr,SH Tu,Fr 11:30-02:00");
        T3("Mo-Th 17:00-23:00, Mo-Th SH 14:00-23:00, Fr 14:00-02:00, Sa 11:30-22:00, Su 11:30-22:00, PH Mo-Th 11:30-23:00, PH Fr, SH Fr 11:30-02:00, PH Sa-Su 11:30-22:00",
           "Mo-Th 17:00-23:00, SH Mo-Th 14:00-23:00, Fr 14:00-02:00, Sa 11:30-22:00, Su 11:30-22:00, PH Mo-Th 11:30-23:00, PH Fr,SH Fr 11:30-02:00, PH Sa-Su 11:30-22:00",
           "Mo-Th 17:00-23:00, SH Mo-Th 14:00-23:00, Fr 14:00-02:00, Sa,Su 11:30-22:00, PH Mo-Th 11:30-23:00, PH Fr,SH Fr 11:30-02:00, PH Sa-Su 11:30-22:00");
        T("Sa,Su,PH,Mo");
        T2("Tu-Fr 10:00-17:00; Th 10:00-20:00; Sa,Su,PH Mo 11:00-18:00", "Tu-Fr 10:00-17:00; Th 10:00-20:00; Sa,Su,PH,Mo 11:00-18:00");
        T2("Mo,We,Th PH off", "PH Mo,We,Th off");
        T2("PH closed \"\"", "PH closed");
        T("Mar 01-Mar Su[-1] 17:00-18:00; PH off");
        T("Mar 01-Mar Su[-2] Mo 17:00-18:00; PH off");
        T("Oct Su[-1]-Dec 31 08:00-18:00");
        T("Oct Su[-1]-Dec 31 Su 08:00-18:00");
        T("Mar Su[1]-Oct Su[1]: 11:00-20:00; PH 11:00-20:00");
        T2("Mo 20:00-26:00", "Mo 20:00-26:00"); // https://github.com/osm-fr/osmose-backend/issues/1344

        // from https://wiki.openstreetmap.org/wiki/Key:opening_hours#Simple_examples
        T("Mo-Fr 08:00-17:30");
        T("Mo-Fr 08:00-12:00,13:00-17:30");
        T("Mo-Fr 08:00-12:00,13:00-17:30; Sa 08:00-12:00");
        T("Mo-Fr 08:00-12:00,13:00-17:30; Sa 08:00-12:00; PH off");
        T("Mo-Fr 08:00-12:00,13:00-17:30; Sa 08:00-12:00; PH 09:00-12:00");

        // from https://wiki.openstreetmap.org/wiki/Key:opening_hours#Examples
        T3("Sa-Su 00:00-24:00", nullptr, "Sa,Su 00:00-24:00");
        T("Mo-Fr 08:30-20:00");
        T("Mo 10:00-12:00,12:30-15:00; Tu-Fr 08:00-12:00,12:30-15:00; Sa 08:00-12:00");
        T("Mo-Su 08:00-18:00; Apr 10-15 off; Jun 08:00-14:00; Aug off; Dec 25 off");
        T("Mo-Sa 10:00-20:00; Tu off");
        T("Mo-Sa 10:00-20:00; Tu 10:00-14:00");
        T("sunrise-sunset");
        T("Su 10:00+");
        T2("week 1-53/2 Fr 09:00-12:00; week 2-52/2 We 09:00-12:00", "week 01-53/2 Fr 09:00-12:00; week 02-52/2 We 09:00-12:00");
        T("Mo-Sa 08:00-13:00,14:00-17:00 || \"by appointment\"");
        T3("Su-Tu 11:00-01:00, We-Th 11:00-03:00, Fr 11:00-06:00, Sa 11:00-07:00", nullptr, "Su-Tu 11:00-01:00, We,Th 11:00-03:00, Fr 11:00-06:00, Sa 11:00-07:00");
        T("Mo-Su,PH 15:00-03:00; easter -2 days off");

        // from https://openingh.openstreetmap.de/evaluation_tool/
        T("Mo-Fr 10:00-20:00; PH off");
        T("Mo,Tu,Th,Fr 12:00-18:00; Sa,PH 12:00-17:00; Th[3],Th[-1] off");
        T("00:00-24:00; Tu-Su,PH 08:30-09:00 off; Tu-Su 14:00-14:30 off; Mo 08:00-13:00 off");
        T3("Fr-Sa 18:00-06:00; PH off", nullptr, "Fr,Sa 18:00-06:00; PH off");
        T("Mo 10:00-12:00,12:30-15:00");
        T("Mo 10:00-12:00,12:30-15:00; Tu-Fr 08:00-12:00,12:30-15:00; Sa 08:00-12:00");
        T("\"only after registration\"; PH off");
        T("22:00-23:00; PH off");
        T("08:00-11:00; PH off");
        T("open; Mo 15:00-16:00 off; PH off");
        T("Mo-Su 22:00-23:00; We,PH off");
        T("We-Fr 10:00-24:00 open \"it is open\" || \"please call\"; PH off");
        T("Mo-Fr 08:00-11:00 || Tu-Th,PH open \"Emergency only\"");
        T3("Tu-Th,We 22:00-23:00 open \"Hot meals\"; PH off", nullptr, "Tu-Th 22:00-23:00 open \"Hot meals\"; PH off"); // We redundant
        T("Mo 12:00-14:00 open \"female only\", Mo 14:00-16:00 open \"male only\"; PH off");
        T("Apr: 22:00-23:00; PH off");
        T("Jul-Jan: 22:00-23:00; PH off");
        T("Jan-Jul: 22:00-23:00; PH off");
        T2("Jul 23-Jan 3: \"needs reservation by phone\"; PH off", "Jul 23-Jan 03: \"needs reservation by phone\"; PH off");
        T2("Jul 23-Jan 3: 22:00-23:00 \"Please make a reservation by phone.\"; PH off", "Jul 23-Jan 03: 22:00-23:00 \"Please make a reservation by phone.\"; PH off");
        T2("Jul 23-Jan 3: 08:00-11:00 \"Please make a reservation by phone.\"; PH off", "Jul 23-Jan 03: 08:00-11:00 \"Please make a reservation by phone.\"; PH off");
        T2("Jan 23-Jul 3: 22:00-23:00 \"Please make a reservation by phone.\"; PH off", "Jan 23-Jul 03: 22:00-23:00 \"Please make a reservation by phone.\"; PH off");
        T("Mar Su[-1]-Dec Su[1] -2 days: 22:00-23:00; PH off");
        T("Sa[1],Sa[1] +1 day 10:00-12:00 open \"first weekend in the month\"; PH off");
        T("Sa[-1],Sa[-1] +1 day 10:00-12:00 open \"last weekend in the month\"; PH off");
        T3("Sa-Su 00:00-24:00; PH off", nullptr, "Sa,Su 00:00-24:00; PH off");
        T("Mo-Fr 00:00-24:00; PH off");
        T("sunrise-sunset open \"Beware of sunburn!\"; PH off");
        T("sunset-sunrise open \"Beware of vampires!\"; PH off");
        T("(sunrise-00:30)-(sunrise+00:30)");
        T("(sunset+01:00)-24:00 || closed \"No drink before sunset!\"; PH off");
        T("22:00+; PH off");
        T("Tu,PH 23:59-22:59");
        T("We-Mo,PH 23:59-22:59");
        T2("week 2-52/2 We 00:00-24:00; week 1-53/2 Sa 00:00-24:00; PH off", "week 02-52/2 We 00:00-24:00; week 01-53/2 Sa 00:00-24:00; PH off");
        T2("week 4-16 We 00:00-24:00; week 38-42 Sa 00:00-24:00; PH off", "week 04-16 We 00:00-24:00; week 38-42 Sa 00:00-24:00; PH off");
        T("2012 easter -2 days-2012 easter +2 days: open \"Around easter\"; PH off");
        T("24/7 closed \"always closed\"");
        T2("2013,2015,2050-2053,2055/2,2020-2029/3,2060+ Jan 1", "2013,2015,2050-2053,2055/2,2020-2029/3,2060+ Jan 01");
        T("Jan 23-Feb 11,Feb 12 00:00-24:00; PH off");
        T("Apr-Oct Su[2] 14:00-18:00; Aug Su[-1] -1 day 10:00-18:00; Aug Su[-1] 10:00-18:00; PH off");
        T("Mo-Fr 08:00-12:00, We 14:00-18:00; Su,PH off"); // open We morning too
        T("Mo-Fr 08:00-12:00; We 14:00-18:00; Su,PH off"); // closed We morning
        T2("April-September; Mo-Fr 09:00-13:00, 14:00-18:00, Sa 10:00-13:00", "Apr-Sep; Mo-Fr 09:00-13:00,14:00-18:00, Sa 10:00-13:00");

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
        T("Su[-1,1]");
        T("Tu-Th");
        T("Fr-Mo");
        T("Mo-Su; We \"only after registration\"");
        T("Oct: We[1]");

        // from https://github.com/dfaure/DataNovaImportScripts/blob/master/saved_opening_hours
        T3("Mo-Tu,Th-Fr 09:30-12:00; 2020 Dec 28 off; 2020 Dec 22,2020 Dec 29 off; We 15:00-17:00; 2020 Dec 23,2020 Dec 30 off; 2020 Dec 24,2020 Dec 31 off; Sa 10:00-12:00; 2020 Dec 26,2021 Jan 02 off; PH off", nullptr,
           "Mo,Tu,Th,Fr 09:30-12:00; 2020 Dec 28 off; 2020 Dec 22,2020 Dec 29 off; We 15:00-17:00; 2020 Dec 23,2020 Dec 30 off; 2020 Dec 24,2020 Dec 31 off; Sa 10:00-12:00; 2020 Dec 26,2021 Jan 02 off; PH off");

        // real-world tests from Osmose that we were handling wrongly
        T("Tu-Fr 11:30-14:30 open, 14:30-18:00 open \"pickup only\", 18:00-22:00 open");
        T("SH Tu,Th 10:00-19:00");
        T2("Apr-Sep 09:00-19:00; Mar-Oct. 09:00-18:00; Nov.-Feb. 09:00-17:00", "Apr-Sep 09:00-19:00; Mar-Oct 09:00-18:00; Nov-Feb 09:00-17:00");
        T2("week 23-37 12:30-15:00,20:00-23:00; week 01-22,38-53 off", "week 23-37 12:30-15:00,20:00-23:00; week 01-22,38-53 off");
        T("Mo-Sa 09:00-20:00; Su[-2,-1] 12:30-18:00");
        T("Su off, Mo-Fr 08:30-13:00; Mo-Th 08:30-13:30,16:00-20:00");
        T3("Mo-Tu 09:00-12:00,14:00-18:00, We closed, Th-Sa 09:00-12:00,14:00-18:00; Su 09:30-12:30,14:30-18:00", nullptr,
           "Mo,Tu 09:00-12:00,14:00-18:00, We closed, Th-Sa 09:00-12:00,14:00-18:00; Su 09:30-12:30,14:30-18:00");
        T2("SH Sep-Jun Mo 10:52-15:52", "SH, Sep-Jun Mo 10:52-15:52"); // likely incorrect, but at least no information loss

        // weekday autocorrect
        T2("Tu, Th 13:30-19:00; SH Tu, Th 10:00-19:00; Fr 13:30-18:00; SH Fr 10:00-18:00; We, Sa 10:00-18:00; SH We, Sa 10:00-18:00", "Tu,Th 13:30-19:00; SH Tu,Th 10:00-19:00; Fr 13:30-18:00; SH Fr 10:00-18:00; We,Sa 10:00-18:00; SH We,Sa 10:00-18:00");
        T2("Mar Su[-1] - Oct Su[-1] - 1 days: Th 09:00-18:00, Tu 15:00-18:00; Sa 09:00-12:00; Oct Su[-1] - Mar Su[-1] - 1 days: Tu 09:00-17:00, Th 09:00-17:00, Sa 09:00-12:00; Mo, We, Fr, Su, PH Off",
           "Mar Su[-1]-Oct Su[-1] -1 day: Th 09:00-18:00, Tu 15:00-18:00; Sa 09:00-12:00; Oct Su[-1]-Mar Su[-1] -1 day: Tu 09:00-17:00, Th 09:00-17:00, Sa 09:00-12:00; Mo,We,Fr,Su,PH off");
        T2("Sep 16-Jun 15: Tu-Fr, Sa, Su [1,3] 09:00-14:00; Jun 16-Sep 15: Sa, Su [1,3] 13:00-19:00; Mo off, Su [2,4] off",
           "Sep 16-Jun 15: Tu-Fr,Sa,Su[1,3] 09:00-14:00; Jun 16-Sep 15: Sa,Su[1,3] 13:00-19:00; Mo off, Su[2,4] off");
        T2("2020-2021 Mo, Tu-Fr, Sa [-1] 09:00-14:00", "2020-2021 Mo,Tu-Fr,Sa[-1] 09:00-14:00");
        T2("week 1-3 Mo[2], Tu-Fr, Sa [-1] 09:00-14:00", "week 01-03 Mo[2],Tu-Fr,Sa[-1] 09:00-14:00");
        T2("Mo Fr 09:30-12:30 13:30-18:30", "Mo,Fr 09:30-12:30,13:30-18:30");
        T2("Mo, We, Fr 06:30-21:30; Tu, Th 09:00-21:30; Sa 09:00-17:00; Su 09:00-14:00", "Mo,We,Fr 06:30-21:30; Tu,Th 09:00-21:30; Sa 09:00-17:00; Su 09:00-14:00");

        // technically wrong but often found content in OSM for which we have error recovery
        T2("So", "Su");
        T2("Ph", "PH");
        T2("9:00-12:00", "09:00-12:00");
        T2("Mo-Fr 09:00-18:30;Sa 09:00-17:00", "Mo-Fr 09:00-18:30; Sa 09:00-17:00");
        T2("08:00-12:00;", "08:00-12:00");
        T2("14:00-20:00,", "14:00-20:00");
        T2("Mo 14:00-21:00; Tu-Th 10:00-21:00; Fr 10:00-18:00;Su, PH off|| \"Samstag zweimal im Monat, Details siehe Webseite\"", "Mo 14:00-21:00; Tu-Th 10:00-21:00; Fr 10:00-18:00; Su,PH off || \"Samstag zweimal im Monat, Details siehe Webseite\"");
        T2("Mo-Fr 06:30-12:00, 13:00-18:00", "Mo-Fr 06:30-12:00,13:00-18:00"); // see autocorrect()
        T2("we-mo 11:30-14:00, 17:30-22:00; tu off", "We-Mo 11:30-14:00,17:30-22:00; Tu off");
        T2("01:00-23:00; ", "01:00-23:00");
        T2("02:00-22:00,\n", "02:00-22:00");
        T2("Friday 08:00-12:00", "Fr 08:00-12:00");
        T2("Sat", "Sa");
        T2("december", "Dec");
        T2("Dec 24,25,26, Jan 1,6 off", "Dec 24,Dec 25,Dec 26,Jan 01,Jan 06 off");
        T2("Dec 24,25,26 open, Jan 1,6 off", "Dec 24,Dec 25,Dec 26 open, Jan 01,Jan 06 off");
        T2("07:30-20:00; Jan 03, 13, 23, Feb 03, 13, 23, Mar 03, 13, 23, Apr 03, 13, 23, Jun 03, 13, 23, Jul 03, 13, 23, Aug 03, 13, 23, Sep 03, 13, 23, Oct 03, 13, 23, Nov 03, 13, 23, Dec 03, 13, 23 off", "07:30-20:00; Jan 03,Jan 13,Jan 23,Feb 03,Feb 13,Feb 23,Mar 03,Mar 13,Mar 23,Apr 03,Apr 13,Apr 23,Jun 03,Jun 13,Jun 23,Jul 03,Jul 13,Jul 23,Aug 03,Aug 13,Aug 23,Sep 03,Sep 13,Sep 23,Oct 03,Oct 13,Oct 23,Nov 03,Nov 13,Nov 23,Dec 03,Dec 13,Dec 23 off");
        T2("Apr, May, Oct, Nov, Dec: Mo-Su, 10:00-19:00; Jun-Sep: Mo-Su:10:00-20:00", "Apr,May,Oct,Nov,Dec: Mo-Su, 10:00-19:00; Jun-Sep: Mo-Su 10:00-20:00");

        // Tolerance for incorrect casing
        T2("mo-fr 10:00-20:00", "Mo-Fr 10:00-20:00");
        T2("jan-feb 10:00-20:00", "Jan-Feb 10:00-20:00");
        T2("jan-feb,aug 10:00-20:00", "Jan-Feb,Aug 10:00-20:00");
        T2("SUNRISE-SUNSET", "sunrise-sunset");
        T2("(SUNrISE-01:00)-(SUnsET+01:00)", "(sunrise-01:00)-(sunset+01:00)");
        T2("su,sh off", "Su,SH off");
        T2("mo-fr CLOSED", "Mo-Fr closed");

        // Time correction
        T2("9h00-12h00", "09:00-12:00");
        T2("9h-12h", "09:00-12:00");
        T2("5H", "05:00");
        T2("06:00am", "06:00");
        T2("06:30pm", "18:30");
        T2("07:00 am", "07:00");
        T2("07:00 pm", "19:00");
        T2("5:00AM", "05:00");
        T2("5:02 PM", "17:02");
        T2("10a", "10:00");
        T2("10p", "22:00");
        T2("12:00 am", "00:00");
        T2("12:00pm", "12:00");
        T2("1 a.m", "01:00");
        T2("3p.m", "15:00");
        T2("12:01a.m.", "00:01");
        T2("12:01p.m.", "12:01");
        T2("11:59a", "11:59");
        T2("11:59p", "23:59");
        T2("9h00-12h00,14:00-17:00", "09:00-12:00,14:00-17:00");
        T2("9:00 am - 12:00 am", "09:00-24:00");
        T2("9 am - 12 am", "09:00-24:00");
        T2("11:00 am - 11:00 pm", "11:00-23:00");
        T2("09 : 00 - 12 : 00 , 13 : 00 - 19 : 00", "09:00-12:00,13:00-19:00");
        T2("10.30am - 4.30pm", "10:30-16:30");
        T2("17時00分～23時30分", "17:00-23:30");

        // alternative range separators
        T2("Mo-Fri 10am to 7pm, Saturday 11am to 6pm, Sun 11am to 4pm", "Mo-Fr 10:00-19:00, Sa 11:00-18:00, Su 11:00-16:00");
        T2("Monday to Friday 8:00AM to 4:30PM", "Mo-Fr 08:00-16:30");
        T2("1pm-3pm and 7pm-11pm", "13:00-15:00,19:00-23:00");
        T2("8h00 à 12h00 et 13h30 à 18h00", "08:00-12:00,13:30-18:00");
        T2("Samedi et Dimanche 5h30 - 12h30 Lundi 13h45 - 15h15", "Sa,Su 05:30-12:30; Mo 13:45-15:15");
        T2("Mo-Th 11:00-20:00 Friday & Saturday 11:00-21:00 Sunday 12:00-19:00", "Mo-Th 11:00-20:00; Fr,Sa 11:00-21:00; Su 12:00-19:00");
        T2("11:30-14:00、16:30-22:00", "11:30-14:00,16:30-22:00");
        T2("Lunedì al Venerdì 08:00-13:00", "Mo-Fr 08:00-13:00");

        // (mis)use of colon as a small-range selector separator
        T2("Fr: 17:00-19:00", "Fr 17:00-19:00");
        T2("Fr: closed", "Fr closed");
        T2("Tu-Su:07:00-00:00", "Tu-Su 07:00-24:00");
        T2("Du lundi au vendredi : 9:00-18:00", "Mo-Fr 09:00-18:00");

        // Unicode symbols
        T3("Mo–Tu", "Mo-Tu", "Mo,Tu");
        T2("13：41", "13:41");
        T2("10：00〜19：00", "10:00-19:00");
        T2("10：00－17：00", "10:00-17:00");
        T2("11:00−23:00", "11:00-23:00");
        T2("11:00ー15:00", "11:00-15:00");
        T2("We 09:00-18:00\xC2\xA0; Sa 09:00-19:00", "We 09:00-18:00; Sa 09:00-19:00"); // weird space
        T2("LUNDI 08:30 – 17:00", "Mo 08:30-17:00");
        T3("月,木,金,土,日 11:00-19:00", "Mo,Th,Fr,Sa,Su 11:00-19:00", "Th-Mo 11:00-19:00");
        T2("月-土 09:00-18:00", "Mo-Sa 09:00-18:00");
        T2("水曜日～土曜日10:00～19:00", "We-Sa 10:00-19:00");
        T2("月～土 　17:00～23:00", "Mo-Sa 17:00-23:00");
        T2("Mo-Fr 08:00-17:00 “Visa Applications\"", "Mo-Fr 08:00-17:00 \"Visa Applications\"");
        T2("„nach Vereinbarung“", "\"nach Vereinbarung\"");
        T("\"„Termine nach Vereinbarung“\"");

        // non-English
        T2("Lundi au Vendredi 8h - 17h en continu", "Mo-Fr 08:00-17:00");
        T2("Lun-Ven 08:00-13:00", "Mo-Fr 08:00-13:00");
        T2("Lu-Sa 08:00-13:00", "Mo-Sa 08:00-13:00");
        T2("Domingo de 9: 00 am. a 1:00 pm", "Su 09:00-13:00");
        T2("Gio, Sab e Dom: 9.00-12.30", "Th,Sa,Su 09:00-12:30");
        T2("Segunda a Sexta 08:00h a 16:00h", "Mo-Fr 08:00-16:00");
        T2("lunedi,mercoledi,venerdi,sabato 09:00-20:00", "Mo,We,Fr,Sa 09:00-20:00");
        T2("Mi 08:00-18:00", "We 08:00-18:00");
        T2("Du Mardi au Vendredi 11h00-13h30 Le samedi 10h-19h", "Tu-Fr 11:00-13:30; Sa 10:00-19:00");
        T2("Senin-Sabtu 09:00-16:00, Minggu 09:00-18:00", "Mo-Sa 09:00-16:00, Su 09:00-18:00");
        T2("Lundi-samedi 8H00-12H00 16H00-20H00 dimanche,jours fériés 9H00-12H00 17H00-20H00", "Mo-Sa 08:00-12:00,16:00-20:00; Su,PH 09:00-12:00,17:00-20:00");
        T2("De février à novembre", "Feb-Nov");
        T3("Ma,Me,Je,Ve 8h-12h30, 14h-19h; Sa 8h-12h30, 14h-18h", "Tu,We,Th,Fr 08:00-12:30,14:00-19:00; Sa 08:00-12:30,14:00-18:00", "Tu-Fr 08:00-12:30,14:00-19:00; Sa 08:00-12:30,14:00-18:00");
        T2("Mo-Fr: 10:00-18:30 Uhr Sa: 10:00-13:30 Uhr", "Mo-Fr 10:00-18:30; Sa 10:00-13:30");
        T2("Montag & Dienstag Ruhetag", "Mo,Tu closed");
        T2("Понедельник - Суббота 09:00 - 21:00", "Mo-Sa 09:00-21:00");
        T2("Пон-Пт 08:00-20:00, Суб 08:00-19:00", "Mo-Fr 08:00-20:00, Sa 08:00-19:00");
        T2("Ноябрь-Март", "Nov-Mar");
        T2("с 08:30 до 23:00", "08:30-23:00");
        T2("рассвет - сумерки", "dawn-dusk");
        T2("от рассвета до сумерек", "dawn-dusk");
        T2("восход - закат", "sunrise-sunset");
        T2("с восхода пo закат", "sunrise-sunset");
        T2("от восхода дo заката", "sunrise-sunset");
        T2("Среда открыто; Пятница закрыто; Суббота неизвестно", "We open; Fr closed; Sa unknown");
        T2("Вт Выходной", "Tu closed");

        // recovery from wrong rule separators
        T2("Fr,Sa 10:00-02:00,Su 10:00-20:00", "Fr,Sa 10:00-02:00, Su 10:00-20:00");
        T2("tu-sa 12:00-14:30,mo-sa 18:30-22:00", "Tu-Sa 12:00-14:30, Mo-Sa 18:30-22:00");
        T2("Mo 07:00-12:00,Tu 15:00-20:00,We 07:00-12:00,Fr 15:00-20:00", "Mo 07:00-12:00, Tu 15:00-20:00, We 07:00-12:00, Fr 15:00-20:00");
        T2("Mo-Fr 09:00-17:00 Sa 09:00-14:00", "Mo-Fr 09:00-17:00; Sa 09:00-14:00");
        T2("Friday 11AM–2:30AM Saturday 10AM–3:30AM Sunday 9AM–4:30AM", "Fr 11:00-02:30; Sa 10:00-03:30; Su 09:00-04:30");
        T2("Mo-Sa 12:00-15:00; 18:00-24:00", "Mo-Sa 12:00-15:00,18:00-24:00");
        T2("Mo-Sa 12:00-15:00; Mo-Sa 18:00-24:00", "Mo-Sa 12:00-15:00,18:00-24:00");
        T2("Mo 12:00-15:00; Mo 18:00-24:00", "Mo 12:00-15:00,18:00-24:00");

        // recovery from wrong time selector separators
        T3("Dimanche Fermé Lundi 08:00 – 12:30 14:00 – 19:00 Mardi 08:00 – 12:30 14:00 – 19:00 Mercredi 08:00 – 12:30 14:00 – 19:00 Jeudi 08:00 – 12:30 14:00 – 19:00 Vendredi 08:00 – 12:30 14:00 – 19:00 Samedi 08:00 – 12:30 14:30 – 18:00",
           "Su closed; Mo 08:00-12:30,14:00-19:00; Tu 08:00-12:30,14:00-19:00; We 08:00-12:30,14:00-19:00; Th 08:00-12:30,14:00-19:00; Fr 08:00-12:30,14:00-19:00; Sa 08:00-12:30,14:30-18:00",
           "Su closed; Mo-Fr 08:00-12:30,14:00-19:00; Sa 08:00-12:30,14:30-18:00");

        // recovery from slashes abused as rule or timespan separators
        T2("09:00-12:00/13:00-19:00", "09:00-12:00,13:00-19:00");
        T2("10:00 - 13:30 / 17:00 - 20:30", "10:00-13:30,17:00-20:30");
        T2("Mo-Fr 6:00-18:00 / Sa 6:00-13:00 / So 7:00-17:00", "Mo-Fr 06:00-18:00; Sa 06:00-13:00; Su 07:00-17:00");

        // Simplification of the output
        T3("Mo 08:00-13:00; Tu 08:00-13:00", nullptr, "Mo,Tu 08:00-13:00");
        T3("Mo-Th 08:00-13:00; Sa[1],Su[-1] 08:00-13:00", "Mo-Th 08:00-13:00; Sa[1],Su[-1] 08:00-13:00", "Mo-Th,Sa[1],Su[-1] 08:00-13:00");
        T("easter +1 day 08:00-13:00; Tu,Sa,Su 08:00-13:00"); // does not simplify
        T3("Mo-Sa 12:00-15:00, Mo-Sa 18:00-24:00", "Mo-Sa 12:00-15:00, Mo-Sa 18:00-24:00", "Mo-Sa 12:00-15:00,18:00-24:00");
        T3("Mo 12:00-15:00, Mo 18:00-24:00", "Mo 12:00-15:00, Mo 18:00-24:00", "Mo 12:00-15:00,18:00-24:00");
        T3("Mo-We,Fr,Su 08:00-13:00", "Mo-We,Fr,Su 08:00-13:00", "Su-We,Fr 08:00-13:00");
        T3("Mo,We,Th,Tu,Sa 08:00-13:00", "Mo,We,Th,Tu,Sa 08:00-13:00", "Mo-Th,Sa 08:00-13:00"); // reordering
        T3("Mo-Fr,Tu,We 08:00-13:00", "Mo-Fr,Tu,We 08:00-13:00", "Mo-Fr 08:00-13:00"); // Tu,We already included
        T3("Sa-Mo 10:00-23:00, Th 10:00-23:00", "Sa-Mo 10:00-23:00, Th 10:00-23:00", "Sa-Mo,Th 10:00-23:00"); // beginDay > endDay
        T3("Sa-Mo 10:00-23:00, Fr 10:00-23:00", "Sa-Mo 10:00-23:00, Fr 10:00-23:00", "Fr-Mo 10:00-23:00"); // beginDay > endDay
        T3("Su-Th 10:00-23:00, Fr-Sa 10:00-23:00", "Su-Th 10:00-23:00, Fr-Sa 10:00-23:00", "Mo-Su 10:00-23:00"); // beginDay > endDay

        // complex or creative 24/7 use
        T("06:00-01:00 open \"Dining in\" || 24/7 \"Drive-through\"");
        T2("Friday and Saturday 24/7 Sunday-Thursday 4:00 am to 12:00 am", "Fr,Sa 00:00-24:00; Su-Th 04:00-24:00"); // bug 445962
        // 24/7 as standalone small range selector not supported yet
        //T2("Apr-Oct: 24/7; Nov-Mar: Mo-Su 06:00-22:00", "Apr-Oct: 00:00-24:00; Nov-Mar: Mo-Su 06:00-22:00");
#undef T
#undef T2
#undef T3
    }

    void testSuccess()
    {
        QFETCH(QByteArray, input);
        QFETCH(QByteArray, expectedOutput);
        QFETCH(QByteArray, expectedSimplifiedOutput);
        OpeningHours oh(input);
        QVERIFY(oh.error() != OpeningHours::SyntaxError);
        QCOMPARE(oh.normalizedExpression(), expectedOutput);
        QCOMPARE(oh.simplifiedExpression(), expectedSimplifiedOutput);

        // verify the expressions we generate are parsed correctly as well
        OpeningHours oh2(oh.normalizedExpression());
        QVERIFY(oh2.error() != OpeningHours::SyntaxError);
        QCOMPARE(oh2.normalizedExpression(), oh.normalizedExpression());

        OpeningHours oh3(oh.simplifiedExpression());
        QVERIFY(oh3.error() != OpeningHours::SyntaxError);
        QCOMPARE(oh3.normalizedExpression(), oh.simplifiedExpression());
    }

    void testFail_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<OpeningHours::Error>("error");

#define T(x) QTest::newRow(x) << QByteArray(x) << OpeningHours::SyntaxError
        T("23/7");
        T("24/7 geöffnet");
        T("2020-2000");
        T("Jan-Apr 1");
        T("Feb-2020 Apr 1");
        T("Apr 1-Nov");
        T("Su[0]");
        T("Mo[6]");
        T("Mo[-0]");
        T("Tu[-6]");
        T("Mo[0-5]");
        T("We[4-2]");
        T("49:00");
        T("12:61");
        T("60p");

        T("Dec 6,4");
        T("Dec 24-Jan 1,6");
        T("Dec 3,2,1");
        T("Mo, 1:100");

        // from https://wiki.openstreetmap.org/wiki/Key:opening_hours#Common_mistakes
        T("7/8-23");
        T("0600-1800");
        T("07;00-2;00pm");
        T("08.00-16.00, public room till 03.00 a.m");
        T("09:00-21:00 TEL/072(360)3200");
        T("Dining in: 6am to 11pm; Drive thru: 24/7");
        T("MWThF: 1200-1800; SaSu: 1200-1700");
        T("BAR: Su-Mo 18:00-02:00; Tu-Th 18:00-03:00; Fr-Sa 18:00-04:00; CLUB: Tu-Th 20:00-03:00; Fr-Sa 20:00-04:00");

        // from https://openingh.openstreetmap.de/evaluation_tool/
        T("00:00-24:00 week 6 Mo-Su Feb; PH off");
        T("monday, Tu, wE, TH 12:00 - 20:00 ; 14:00-16:00 Off ; closed public Holiday");

        // from Osmose, should fail rather than silently drop the last part
        T("Mo-Th, Su 17:00-01:00, Fr-Sa 1700:0300");

        // creative uses found in the wild
        T("May 01-Jun 15,Sept 01-30 Mo-Fr 10:00-18:00 Sa-Su 09:00-19:00");
        T("Jul 22-Aug 18 2019: Tu-Su 10:00-13:00");
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
#ifdef KOPENINGHOURS_VALIDATOR_ONLY
        QTest::newRow("PH") << QByteArray("PH off") << OpeningHours::NoError;
#else
        QTest::newRow("PH") << QByteArray("PH off") << OpeningHours::MissingRegion;
#endif
        QTest::newRow("SH") << QByteArray("SH off") << OpeningHours::UnsupportedFeature;
        QTest::newRow("time interval") << QByteArray("10:00-16:00/90") << OpeningHours::IncompatibleMode;
        QTest::newRow("time interval 2") << QByteArray("10:00-16:00/1:30") << OpeningHours::IncompatibleMode;
        QTest::newRow("week wrap") << QByteArray("week 45-13") << OpeningHours::UnsupportedFeature;
        QTest::newRow("single timepoint") << QByteArray("10:00") << OpeningHours::IncompatibleMode;
        QTest::newRow("month timepoint") << QByteArray("Dec 08:00") << OpeningHours::IncompatibleMode;
        QTest::newRow("wide range selector comment") << QByteArray("\"Außerhalb der Semesterferien\": Mo-Fr 08:00-22:00; Sa-Su 10:00-20:00; \"Innerhalb der Semesterferien\": Mo-Fr 08:00-18:00; Sa-Su 10:00-16:00;") << OpeningHours::UnsupportedFeature;

        QTest::newRow("empty") << QByteArray("") << OpeningHours::Null;
        QTest::newRow("empty comment") << QByteArray("\"\"") << OpeningHours::Null;
    }

    void testValidation()
    {
        QFETCH(QByteArray, expression);
        QFETCH(OpeningHours::Error, error);

        OpeningHours oh(expression);
        QCOMPARE(oh.error(), error);
        (void)oh.normalizedExpression(); // don't crash
        (void)oh.simplifiedExpression(); // don't crash
    }
};

QTEST_GUILESS_MAIN(ParserTest)

#include "parsertest.moc"
