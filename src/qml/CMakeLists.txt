# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(kopeninghoursqmlplugin URI org.kde.kopeninghours)

target_sources(kopeninghoursqmlplugin PRIVATE kopeninghoursqmlplugin.cpp)

target_link_libraries(kopeninghoursqmlplugin PRIVATE
    Qt::Qml
    KOpeningHours
)

ecm_finalize_qml_module(kopeninghoursqmlplugin DESTINATION ${KDE_INSTALL_QMLDIR})
