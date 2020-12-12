# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_executable(osm-opening-hours-validate osm-opening-hours-validate.cpp)
target_link_libraries(osm-opening-hours-validate KOpeningHours)

if (NOT VALIDATOR_ONLY)
    add_executable(osm-opening-hours-evaluate osm-opening-hours-evaluate.cpp)
    target_link_libraries(osm-opening-hours-evaluate KOpeningHours)
endif()
