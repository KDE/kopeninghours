# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_library(kopeninghoursqmlplugin kopeninghoursqmlplugin.cpp)
target_link_libraries(kopeninghoursqmlplugin
    Qt5::Qml
    KOpeningHours
)

# make examples work without installation
set_property(TARGET kopeninghoursqmlplugin PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/org/kde/kopeninghours)
file(COPY qmldir DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/org/kde/kopeninghours)

install(TARGETS kopeninghoursqmlplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kopeninghours)
install(FILES qmldir ${qml_SRC} DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kopeninghours)
