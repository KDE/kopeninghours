# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

ecm_qt_declare_logging_category(kopeninghours_srcs HEADER logging.h IDENTIFIER KOpeningHours::Log CATEGORY_NAME org.kde.kopeninghours)
flex_target(openinghoursscanner
    openinghourslexer.l
    ${CMAKE_CURRENT_BINARY_DIR}/openinghourslexer.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/openinghoursscanner_p.h
    COMPILE_FLAGS "--nounistd"
)
bison_target(openinghoursparser
    openinghoursparser.y
    ${CMAKE_CURRENT_BINARY_DIR}/openinghoursparser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/openinghoursparser_p.h
)
add_flex_bison_dependency(openinghoursscanner openinghoursparser)
set_source_files_properties(${BISON_openinghoursparser_OUTPUTS} ${FLEX_openinghoursscanner_OUTPUTS} PROPERTIES SKIP_AUTOMOC ON)

if (NOT VALIDATOR_ONLY)
    list(APPEND kopeninghours_srcs
        display.cpp
        easter.cpp
        evaluator.cpp
        holidaycache.cpp
        intervalmodel.cpp
        display.h
        easter_p.h
        holidaycache_p.h
        intervalmodel.h
    )
endif()

add_library(KOpeningHours
    ${kopeninghours_srcs}
    ${BISON_openinghoursparser_OUTPUTS}
    ${FLEX_openinghoursscanner_OUTPUTS}
    interval.cpp
    openinghours.cpp
    rule.cpp
    selectors.cpp
    interval.h
    openinghours.h
    rule_p.h
    selectors_p.h
)

generate_export_header(KOpeningHours BASE_NAME KOpeningHours)

set_target_properties(KOpeningHours PROPERTIES
    VERSION ${KOPENINGHOURS_VERSION_STRING}
    SOVERSION ${KOPENINGHOURS_SOVERSION}
    EXPORT_NAME KOpeningHours
)
target_link_libraries(KOpeningHours
    PUBLIC
        Qt::Core
)
target_include_directories(KOpeningHours INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}>")
if (VALIDATOR_ONLY)
    target_compile_definitions(KOpeningHours PUBLIC KOPENINGHOURS_VALIDATOR_ONLY)
else()
    target_link_libraries(KOpeningHours
        PRIVATE
            KF${KF_MAJOR_VERSION}::Holidays
            KF${KF_MAJOR_VERSION}::I18n
    )
endif()

ecm_generate_headers(KOpeningHours_FORWARDING_HEADERS
    HEADER_NAMES
        Display
        Interval
        IntervalModel
        OpeningHours
    PREFIX KOpeningHours
    REQUIRED_HEADERS KOpeningHours_HEADERS
)

install(TARGETS KOpeningHours EXPORT KOpeningHoursTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
if (KDE_INSTALL_LOGGINGCATEGORIESDIR) # added by ECM 5.59
    install(FILES org_kde_kopeninghours.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
endif()
install(FILES
    ${KOpeningHours_FORWARDING_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KOpeningHours
)
install(FILES
    ${KOpeningHours_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/kopeninghours_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/kopeninghours
)

if (BUILD_QCH)
    ecm_add_qch(
        KOpeningHours_QCH
        NAME KOpeningHours
        BASE_NAME KOpeningHours
        VERSION ${RELEASE_SERVICE_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
        ${KOpeningHours_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        #IMAGE_DIRS "${CMAKE_SOURCE_DIR}/docs/pics"
        LINK_QCHS
            Qt5Core_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            KOPENINGHOURS_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()
