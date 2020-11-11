# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(lib)
if (TARGET Qt5::Qml)
    add_subdirectory(qml)
endif()
