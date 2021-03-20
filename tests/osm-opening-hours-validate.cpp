/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/OpeningHours>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>

#include <cstring>
#include <iostream>

using namespace KOpeningHours;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption verifyNormalizationOpt({QStringLiteral("verify-normalization")}, QStringLiteral("verify normalized expression themselves have valid syntax"));
    parser.addOption(verifyNormalizationOpt);
    parser.addPositionalArgument(QStringLiteral("expression"), QStringLiteral("OSM opening hours expression, omit for using stdin."));
    parser.process(app);

    const auto verifyNormalization = parser.isSet(verifyNormalizationOpt);
    if (parser.positionalArguments().isEmpty()) {
        OpeningHours oh;
        QFile in;
        in.open(stdin, QFile::ReadOnly);
        int total = 0;
        int normalized = 0;
        int simplified = 0;
        int errors = 0;
        char line[4096];
        while (!in.atEnd()) {
            auto size = in.readLine(line, sizeof(line));
            if (size <= 1) {
                continue;
            }
            --size; // trailing linebreak
            ++total;
            oh.setExpression(line, size);
            if (oh.error() == OpeningHours::SyntaxError) {
                std::cerr << "Syntax error: " << QByteArray(line, size).constData() << std::endl;
                ++errors;
            } else {
                const auto n = oh.normalizedExpression();
                if (verifyNormalization) {
                    oh.setExpression(n);
                    if (oh.error() == OpeningHours::SyntaxError) {
                        std::cerr << "Syntax error in normalized expression! " << QByteArray(line, size).constData() << " normalized: " << n.constData() << std::endl;
                    }
                }
                if (n.size() != size || std::strncmp(line, n.constData(), size) != 0) {
                    ++normalized;
                    std::cerr << "Expression " << QByteArray(line, size).constData() << " normalized to " << n.constData() << std::endl;
                }
                const auto simplifiedExpr = oh.simplifiedExpression();
                if (n != simplifiedExpr) {
                    ++simplified;
                    std::cerr << "Expression " << n.constData() << " simplified to " << simplifiedExpr.constData() << std::endl;
                }
            }
        }

        std::cerr << total << " expressions checked, "
                  << errors << " invalid, "
                  << normalized << " not in normal form, "
                  << simplified << " can be simplified" << std::endl;
        return errors;
    } else {
        OpeningHours oh(parser.positionalArguments().at(0).toUtf8());
        std::cout << oh.normalizedExpression().constData() << std::endl;
        return oh.error() != OpeningHours::SyntaxError ? 0 : 1;
    }
}
