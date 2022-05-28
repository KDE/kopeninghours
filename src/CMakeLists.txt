# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"kopeninghours\")

add_subdirectory(lib)
if (TARGET Qt${QT_MAJOR_VERSION}::Qml AND NOT VALIDATOR_ONLY)
    add_subdirectory(qml)
endif()
