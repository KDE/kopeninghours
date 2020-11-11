/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/


#include <KOpeningHours/Interval>
#include <KOpeningHours/OpeningHours>

#include <QQmlContext>
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
    Q_INVOKABLE KOpeningHours::OpeningHours parse(const QString &expression) const;
};

OpeningHours OpeningHoursFactory::parse(const QString &expression) const
{
    return OpeningHours(expression.toUtf8());
}

}

void KOpeningHoursQmlPlugin::registerTypes(const char*)
{
    qRegisterMetaType<KOpeningHours::Interval>();
    qRegisterMetaType<KOpeningHours::OpeningHours>();

    qmlRegisterUncreatableType<KOpeningHours::Interval>("org.kde.kopeninghours", 1, 0, "Interval", {});
    qmlRegisterUncreatableType<KOpeningHours::OpeningHours>("org.kde.kopeninghours", 1, 0, "OpeningHours", {});

    qmlRegisterSingletonType("org.kde.kopeninghours", 1, 0, "OpeningHoursParser", [](QQmlEngine*, QJSEngine *engine) -> QJSValue {
        return engine->toScriptValue(KOpeningHours::OpeningHoursFactory());
    });
}

#include "kopeninghoursqmlplugin.moc"
