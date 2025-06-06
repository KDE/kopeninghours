/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/Interval>
#include <KOpeningHours/OpeningHours>

#include <kholidays_version.h>

#include <QDirIterator>
#include <QFile>
#include <QLocale>
#include <QProcess>
#include <QTest>
#include <QTimeZone>

using namespace KOpeningHours;

void initLocale()
{
    qputenv("LC_ALL", "en_US.utf-8");
    qputenv("LANG", "en_US");
    qputenv("TZ", "Pacific/Auckland"); // way off from what we set below
}

Q_CONSTRUCTOR_FUNCTION(initLocale)

class IterationTest : public QObject
{
    Q_OBJECT
private:
    QByteArray intervalToString(const Interval &i) const
    {
        QByteArray b;
        i.begin().isValid() ? b += QLocale::c().toString(i.begin(), QStringLiteral("ddd yyyy-MM-dd hh:mm")).toUtf8() : b += "-inf";
        b += " - ";
        i.end().isValid() ? b += QLocale::c().toString(i.end(), QStringLiteral("ddd yyyy-MM-dd hh:mm")).toUtf8() : b += "inf";
        if (i.hasOpenEndTime()) {
            b += '+';
        }
        b += ": ";
        switch (i.state()) {
            case Interval::Open:
                b += "open";
                break;
            case Interval::Closed:
                b += "closed";
                break;
            case Interval::Unknown:
                b += "unknown";
                break;
            case Interval::Invalid:
                Q_UNREACHABLE();
                break;
        }
        if (!i.comment().isEmpty()) {
            b += " (" + i.comment().toUtf8() + ")";
        }
        return b + '\n';
    }

private Q_SLOTS:
    void initTestCase()
    {
        QLocale::setDefault(QLocale::c());
    }

    void testIterate_data()
    {
        QTest::addColumn<QString>("inputFile");

        QDirIterator it(QStringLiteral(SOURCE_DIR "/data"), {QStringLiteral("*.intervals")}, QDir::Files | QDir::Readable | QDir::NoSymLinks);
        while (it.hasNext()) {
            it.next();
#if KHOLIDAYS_VERSION < QT_VERSION_CHECK(6, 1, 0)
            const auto n = it.fileInfo().baseName();
            if (n == QLatin1String("ph") || n == QLatin1String("and_condition1") || n == QLatin1String("and_condition2")) {
                qWarning() << "skipping test" << n;
                continue;
            }
#endif
            QTest::newRow(it.fileInfo().fileName().toLatin1().constData()) << it.filePath();
        }
    }

    void testIterate()
    {
        QFETCH(QString, inputFile);

        QFile inFile(inputFile);
        QVERIFY(inFile.open(QFile::ReadOnly | QFile::Text));

        const auto expr = inFile.readLine();
        OpeningHours oh(expr);
        oh.setLocation(52.5, 13.0);
        oh.setRegion(QStringLiteral("DE-BE"));
        oh.setTimeZone(QTimeZone("Europe/Berlin"));
        QCOMPARE(oh.error(), OpeningHours::NoError);

        const auto iterationCount = inFile.readLine().toInt();
        QVERIFY(iterationCount >= 10);

        QByteArray b = expr + QByteArray::number(iterationCount) + "\n\n";
        auto interval = oh.interval(QDateTime({2020, 11, 7}, {18, 32, 14}));
        b += intervalToString(interval);
        for (int i = 0; interval.isValid() && i < iterationCount; ++i) {
            interval = oh.nextInterval(interval);
            if (interval.isValid()) {
                b += intervalToString(interval);
            }
        }

        inFile.seek(0);
        const auto refData = inFile.readAll();
        if (refData != b) {
            QFile failFile(inputFile + QLatin1String(".fail"));
            QVERIFY(failFile.open(QFile::WriteOnly | QFile::Text));
            failFile.write(b);
            failFile.close();

            QProcess proc;
            proc.setProcessChannelMode(QProcess::ForwardedChannels);
            proc.start(QStringLiteral("diff"), {QStringLiteral("-u"), inputFile, failFile.fileName()});
            QVERIFY(proc.waitForFinished());
        }

#if KHOLIDAYS_VERSION < QT_VERSION_CHECK(6, 15, 0)
        QEXPECT_FAIL("sunrise.intervals", "sunrise computation changed in KF::Holidays >= 6.15", Continue);
#endif
        QCOMPARE(refData, b);
    }
};

QTEST_GUILESS_MAIN(IterationTest)

#include "iterationtest.moc"
