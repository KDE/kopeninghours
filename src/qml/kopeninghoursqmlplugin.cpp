/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KOpeningHours/Display>
#include <KOpeningHours/Interval>
#include <KOpeningHours/OpeningHours>

#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>

class KOpeningHoursQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    void registerTypes(const char* uri) override;
};

namespace KOpeningHours {
class OpeningHoursFactory
{
    Q_GADGET
public:
    Q_INVOKABLE KOpeningHours::OpeningHours parse(const QString &expression, int modes = OpeningHours::IntervalMode) const;
};

OpeningHours OpeningHoursFactory::parse(const QString &expression, int modes) const
{
    return OpeningHours(expression.toUtf8(), OpeningHours::Modes(modes));
}

}

void KOpeningHoursQmlPlugin::registerTypes(const char*)
{
    // HACK qmlplugindump chokes on gadget singletons, to the point of breaking ecm_find_qmlmodule()
    if (QCoreApplication::applicationName() != QLatin1String("qmlplugindump")) {
        qmlRegisterSingletonType("org.kde.kopeninghours", 1, 0, "OpeningHoursParser", [](QQmlEngine*, QJSEngine *engine) -> QJSValue {
            return engine->toScriptValue(KOpeningHours::OpeningHoursFactory());
        });
        qmlRegisterSingletonType("org.kde.kopeninghours", 1, 0, "Display", [](QQmlEngine*, QJSEngine *engine) -> QJSValue {
            return engine->toScriptValue(KOpeningHours::Display());
        });
    }
}

#include "kopeninghoursqmlplugin.moc"
