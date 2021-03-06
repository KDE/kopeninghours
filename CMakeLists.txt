# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.5)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "21")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(KOpeningHours VERSION ${RELEASE_SERVICE_VERSION})

# Ubuntu bionic has ECM 5.44
find_package(ECM 5.44 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMAddTests)
include(ECMGenerateHeaders)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
ecm_setup_version(PROJECT VARIABLE_PREFIX KOPENINGHOURS
    VERSION_HEADER kopeninghours_version.h
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KOpeningHoursConfigVersion.cmake"
    SOVERSION 1
)

option(VALIDATOR_ONLY "Build only the validator, not the evaluator. This removes the kholidays and ki18n dependencies." OFF)
if (VALIDATOR_ONLY)
    set(REQUIRED_QT_VERSION 5.9)
else()
    set(REQUIRED_QT_VERSION 5.14) # 5.14 for QCalendar
endif()

find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core) # 5.14 for QCalendar
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG QUIET OPTIONAL_COMPONENTS Qml)
if (NOT VALIDATOR_ONLY)
    find_package(KF5 5.77 REQUIRED COMPONENTS Holidays I18n)
endif()

if (BUILD_TESTING)
    find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Test)
endif()

find_package(FLEX REQUIRED)
set_package_properties(FLEX PROPERTIES PURPOSE "Lexer generator for parsing opening hours expressions")
find_package(BISON REQUIRED)
set_package_properties(BISON PROPERTIES PURPOSE "Grammar generator for parsing opening hours expressions")

find_package(PythonLibs 3.6 MODULE)
if (PythonLibs_FOUND)
    # Extract major/minor python version
    string(REPLACE "." ";" VERSION_LIST ${PYTHONLIBS_VERSION_STRING})
    list(GET VERSION_LIST 0 PYTHONLIBS_VERSION_MAJOR)
    list(GET VERSION_LIST 1 PYTHONLIBS_VERSION_MINOR)

    # boost-python requires exact version number
    find_package(Boost COMPONENTS python${PYTHONLIBS_VERSION_MAJOR}${PYTHONLIBS_VERSION_MINOR} MODULE)
endif()

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050d00)
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x054400)
add_definitions(-DQT_NO_FOREACH)
add_subdirectory(src)
if (Boost_FOUND)
    add_subdirectory(PyKOpeningHours)
endif()

if (BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KOpeningHours")
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KOpeningHoursConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KOpeningHoursConfig.cmake"
    INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/KOpeningHoursConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/KOpeningHoursConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)
install(EXPORT KOpeningHoursTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KOpeningHoursTargets.cmake)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kopeninghours_version.h DESTINATION ${KDE_INSTALL_INCLUDEDIR} COMPONENT Devel)
if (NOT VALIDATOR_ONLY)
    ki18n_install(po)
endif()
