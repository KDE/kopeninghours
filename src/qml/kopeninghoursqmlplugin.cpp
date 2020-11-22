/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/


#include <KOpeningHours/Interval>
#include <KOpeningHours/IntervalModel>
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
    Q_INVOKABLE KOpeningHours::OpeningHours parse(const QString &expression, int modes = OpeningHours::IntervalMode) const;
};

OpeningHours OpeningHoursFactory::parse(const QString &expression, int modes) const
{
    qDebug() << modes;
    return OpeningHours(expression.toUtf8(), OpeningHours::Modes(modes));
}

}

void KOpeningHoursQmlPlugin::registerTypes(const char*)
{
    qRegisterMetaType<KOpeningHours::Interval>();
    qRegisterMetaType<KOpeningHours::OpeningHours>();
    qRegisterMetaType<KOpeningHours::OpeningHours::Modes>();

    qmlRegisterUncreatableType<KOpeningHours::Interval>("org.kde.kopeninghours", 1, 0, "Interval", {});
    qmlRegisterUncreatableType<KOpeningHours::OpeningHours>("org.kde.kopeninghours", 1, 0, "OpeningHours", {});

    qmlRegisterType<KOpeningHours::IntervalModel>("org.kde.kopeninghours", 1, 0, "IntervalModel");

    qmlRegisterSingletonType("org.kde.kopeninghours", 1, 0, "OpeningHoursParser", [](QQmlEngine*, QJSEngine *engine) -> QJSValue {
        return engine->toScriptValue(KOpeningHours::OpeningHoursFactory());
    });
}

#include "kopeninghoursqmlplugin.moc"
