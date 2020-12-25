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
)

generate_export_header(KOpeningHours BASE_NAME KOpeningHours)

set_target_properties(KOpeningHours PROPERTIES
    VERSION ${KOPENINGHOURS_VERSION_STRING}
    SOVERSION ${KOPENINGHOURS_SOVERSION}
    EXPORT_NAME KOpeningHours
)
target_link_libraries(KOpeningHours
    PUBLIC
        Qt5::Core
)
if (VALIDATOR_ONLY)
    target_compile_definitions(KOpeningHours PUBLIC KOPENINGHOURS_VALIDATOR_ONLY)
else()
    target_link_libraries(KOpeningHours
        PRIVATE
            KF5::Holidays
            KF5::I18n
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

install(TARGETS KOpeningHours EXPORT KOpeningHoursTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
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
