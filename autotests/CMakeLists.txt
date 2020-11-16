# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

ecm_add_test(parsertest.cpp LINK_LIBRARIES Qt5::Test KOpeningHours)
ecm_add_test(intervaltest.cpp LINK_LIBRARIES Qt5::Test KOpeningHours)
ecm_add_test(eastertest.cpp LINK_LIBRARIES Qt5::Test KOpeningHours)
ecm_add_test(evaluatetest.cpp LINK_LIBRARIES Qt5::Test KOpeningHours)
ecm_add_test(iterationtest.cpp LINK_LIBRARIES Qt5::Test KOpeningHours)
ecm_add_test(intervalmodeltest.cpp LINK_LIBRARIES Qt5::Test KOpeningHours)
ecm_add_test(jsonldtest.cpp LINK_LIBRARIES Qt5::Test KOpeningHours)
