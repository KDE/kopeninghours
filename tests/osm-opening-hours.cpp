/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/Interval>
#include <KOpeningHours/OpeningHours>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>

#include <iostream>

using namespace KOpeningHours;

void printInterval(const Interval &i)
{
    if (i.begin().isValid()) {
        std::cout << qPrintable(i.begin().toString(QStringLiteral("ddd yyyy-MM-dd hh:mm")));
    } else {
        std::cout << "since ever";
    }
    std::cout << " - ";
    if (i.end().isValid()) {
        std::cout << qPrintable(i.end().toString(QStringLiteral("ddd yyyy-MM-dd hh:mm")));
    } else {
        std::cout << "until all eternity";
    }
    std::cout << ": ";
    switch (i.state()) {
        case Interval::Open:
            std::cout << "open";
            break;
        case Interval::Closed:
            std::cout << "closed";
            break;
        case Interval::Unknown:
            std::cout << "unknown";
            break;
        case Interval::Invalid:
            break;
    }
    if (!i.comment().isEmpty()) {
        std::cout << " (" << qPrintable(i.comment()) << ")";
    }
    std::cout << std::endl;
}

static QString errorString(OpeningHours::Error error)
{
    switch (error) {
    case OpeningHours::Null:
        return QStringLiteral("Empty specification");
    case OpeningHours::SyntaxError:
        return QStringLiteral("Syntax error");
    case OpeningHours::MissingLocation:
        return QStringLiteral("Missing location");
    case OpeningHours::MissingRegion:
        return QStringLiteral("Missing region");
    case OpeningHours::UnsupportedFeature:
        return QStringLiteral("Unsupported feature");
    case OpeningHours::IncompatibleMode:
        return QStringLiteral("Incompatible mode");
    case OpeningHours::EvaluationError:
        return QStringLiteral("Evaluation error");
    case OpeningHours::NoError:
        break;
    }
    return {};
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument(QStringLiteral("expression"), QStringLiteral("OSM opening hours expression."));
    parser.process(app);

    if (parser.positionalArguments().isEmpty()) {
        parser.showHelp(1);
    }

    OpeningHours oh(parser.positionalArguments().at(0).toUtf8());
    if (oh.error() != OpeningHours::NoError) {
        qWarning() << errorString(oh.error());
        return 1;
    }

    auto interval = oh.interval(QDateTime::currentDateTime());
    printInterval(interval);
    for (int i = 0; interval.isValid() && i < 20; ++i) {
        interval = oh.nextInterval(interval);
        if (interval.isValid()) {
            printInterval(interval);
        }
    }

    return 0;
}
