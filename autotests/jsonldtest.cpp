/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/Interval>
#include <KOpeningHours/OpeningHours>

#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QProcess>
#include <QTest>

using namespace KOpeningHours;

class JsonLdTest : public QObject
{
    Q_OBJECT
private:
    QByteArray intervalToString(const Interval &i) const
    {
        QByteArray b;
        i.begin().isValid() ? b += QLocale::c().toString(i.begin(), QStringLiteral("ddd yyyy-MM-dd hh:mm")).toUtf8() : b += "-inf";
        b += " - ";
        i.end().isValid() ? b += QLocale::c().toString(i.end(), QStringLiteral("ddd yyyy-MM-dd hh:mm")).toUtf8() : b += "inf";
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

    void testJsonLd_data()
    {
        QTest::addColumn<QString>("inputFile");

        QDirIterator it(QStringLiteral(SOURCE_DIR "/jsonlddata"), {QStringLiteral("*.json")}, QDir::Files | QDir::Readable | QDir::NoSymLinks);
        while (it.hasNext()) {
            it.next();
            QTest::newRow(it.fileInfo().fileName().toLatin1().constData()) << it.filePath();
        }
    }

    void testJsonLd()
    {
        QFETCH(QString, inputFile);

        QFile inFile(inputFile);
        QVERIFY(inFile.open(QFile::ReadOnly));

        const auto obj = QJsonDocument::fromJson(inFile.readAll()).object();
        QVERIFY(!obj.isEmpty());

        auto oh = OpeningHours::fromJsonLd(obj);
        oh.setLocation(52.5, 13.0);
        oh.setRegion(QStringLiteral("DE-BE"));
        QCOMPARE(oh.error(), OpeningHours::NoError);

        QByteArray b;
        auto interval = oh.interval(QDateTime({2020, 11, 7}, {18, 32, 14}));
        b += intervalToString(interval);
        for (int i = 0; interval.isValid() && i < 20; ++i) {
            interval = oh.nextInterval(interval);
            if (interval.isValid()) {
                b += intervalToString(interval);
            }
        }

        QFile refFile(inputFile.left(inputFile.size() - 5) + QLatin1String(".intervals"));
        refFile.open(QFile::ReadOnly);
        const auto refData = refFile.readAll();
        if (refData != b) {
            QFile failFile(refFile.fileName() + QLatin1String(".fail"));
            QVERIFY(failFile.open(QFile::WriteOnly));
            failFile.write(b);
            failFile.close();

            QProcess proc;
            proc.setProcessChannelMode(QProcess::ForwardedChannels);
            proc.start(QStringLiteral("diff"), {QStringLiteral("-u"), refFile.fileName(), failFile.fileName()});
            QVERIFY(proc.waitForFinished());
        }

        QCOMPARE(refData, b);
    }
};

QTEST_GUILESS_MAIN(JsonLdTest)

#include "jsonldtest.moc"

