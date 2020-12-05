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
        while (!in.atEnd()) {
            auto expr = in.readLine();
            expr.replace('\n', "");
            expr.replace('\r', "");
            if (expr.isEmpty()) {
                continue;
            }
            ++total;
            oh.setExpression(expr);
            if (oh.error() == OpeningHours::SyntaxError) {
                std::cerr << "Syntax error: " << expr.constData() << std::endl;
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
