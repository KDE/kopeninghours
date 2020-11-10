# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_executable(osm-opening-hours osm-opening-hours.cpp)
target_link_libraries(osm-opening-hours KOpeningHours)
