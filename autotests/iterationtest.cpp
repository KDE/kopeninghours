/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/Interval>
#include <KOpeningHours/OpeningHours>

#include <QDirIterator>
#include <QFile>
#include <QLocale>
#include <QProcess>
#include <QTest>

using namespace KOpeningHours;

class IterationTest : public QObject
{
    Q_OBJECT
private:
    QByteArray intervalToString(const Interval &i) const
    {
        QByteArray b;
        i.begin().isValid() ? b += QLocale::c().toString(i.begin(), QLatin1String("ddd yyyy-MM-dd hh:mm")).toUtf8() : b += "-inf";
        b += " - ";
        i.end().isValid() ? b += QLocale::c().toString(i.end(), QLatin1String("ddd yyyy-MM-dd hh:mm")).toUtf8() : b += "inf";
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
            QTest::newRow(it.fileInfo().fileName().toLatin1().constData()) << it.filePath();
        }
    }

    void testIterate()
    {
        QFETCH(QString, inputFile);

        QFile inFile(inputFile);
        QVERIFY(inFile.open(QFile::ReadOnly));

        const auto expr = inFile.readLine();
        OpeningHours oh(expr);
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
            QVERIFY(failFile.open(QFile::WriteOnly));
            failFile.write(b);
            failFile.close();

            QProcess proc;
            proc.setProcessChannelMode(QProcess::ForwardedChannels);
            proc.start(QStringLiteral("diff"), {QStringLiteral("-u"), inputFile, failFile.fileName()});
            QVERIFY(proc.waitForFinished());
        }

        QCOMPARE(refData, b);
    }
};

QTEST_GUILESS_MAIN(IterationTest)

#include "iterationtest.moc"
