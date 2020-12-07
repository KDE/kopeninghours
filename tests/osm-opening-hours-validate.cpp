/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/OpeningHours>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>

#include <iostream>

using namespace KOpeningHours;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument(QStringLiteral("expression"), QStringLiteral("OSM opening hours expression, omit for using stdin."));
    parser.process(app);

    if (parser.positionalArguments().isEmpty()) {
        OpeningHours oh;
        QFile in;
        in.open(stdin, QFile::ReadOnly);
        int total = 0;
        int errors = 0;
        char line[4096];
        while (!in.atEnd()) {
            auto size = in.readLine(line, sizeof(line));
            while (size > 0 && (std::isspace(line[size-1]))) {
                --size;
            }
            if (size <= 0) {
                continue;
            }
            ++total;
            oh.setExpression(line, size);
            if (oh.error() == OpeningHours::SyntaxError) {
                std::cerr << "Syntax error: " << QByteArray(line, size).constData() << std::endl;
                ++errors;
            }
        }

        std::cerr << total << " expressions checked, " << errors << " invalid" << std::endl;
        return errors;
    } else {
        OpeningHours oh(parser.positionalArguments().at(0).toUtf8());
        std::cout << oh.normalizedExpression().constData() << std::endl;
        return oh.error() != OpeningHours::SyntaxError ? 0 : 1;
    }
}
