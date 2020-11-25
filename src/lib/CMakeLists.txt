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

add_library(KOpeningHours
    ${kopeninghours_srcs}
    ${BISON_openinghoursparser_OUTPUTS}
    ${FLEX_openinghoursscanner_OUTPUTS}
    easter.cpp
    holidaycache.cpp
    interval.cpp
    intervalmodel.cpp
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
    PRIVATE
        KF5::Holidays
)

ecm_generate_headers(KOpeningHours_FORWARDING_HEADERS
    HEADER_NAMES
        Interval
        IntervalModel
        OpeningHours
    PREFIX KOpeningHours
    REQUIRED_HEADERS KOpeningHours_HEADERS
)

install(TARGETS KOpeningHours EXPORT KOpeningHoursTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org_kde_kopeninghours.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
install(FILES
    ${KOpeningHours_FORWARDING_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KOpeningHours
)
install(FILES
    ${KOpeningHours_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/kopeninghours_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/kopeninghours
)
