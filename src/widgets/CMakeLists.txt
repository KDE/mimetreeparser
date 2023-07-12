# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_setup_version(PROJECT
    VARIABLE_PREFIX MIMETREEPARSER_WIDGETS
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_widgets_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserWidgetsConfigVersion.cmake"
    SOVERSION 5
)

add_library(KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets)
add_library(KPim${KF_MAJOR_VERSION}::MimeTreeParserWidgets
    ALIAS KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets)

target_sources(KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets PRIVATE
     messageviewer.h
     messageviewer.cpp
     attachmentview_p.h
     attachmentview.cpp
)

ecm_qt_declare_logging_category(KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets
    HEADER mimetreeparser_widgets_debug.h
    IDENTIFIER MIMETREEPARSER_WIDGET_LOG
    CATEGORY_NAME org.kde.pim.mimetreeparser.widgets
    DESCRIPTION "mimetreeparser (pim lib)"
    EXPORT MIMETREEPARSER
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets BASE_NAME mimetreeparser_widgets)


target_include_directories(KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/MimeTreeParserWidget>")
target_include_directories(KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets PUBLIC "$<BUILD_INTERFACE:${Akonadi-Calendar_SOURCE_DIR}/src;${MimeTreeParserWidget_BINARY_DIR}/src>")

target_link_libraries(KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets
PUBLIC
    Qt${KF_MAJOR_VERSION}::Widgets
    KPim${KF_MAJOR_VERSION}::MimeTreeParserCore
PRIVATE
    KF${KF_MAJOR_VERSION}::Codecs
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::CalendarCore
    Gpgme::Gpgme
)

set_target_properties(KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets PROPERTIES
    VERSION ${MIMETREEPARSER_VERSION}
    SOVERSION ${MIMETREEPARSER_SOVERSION}
    EXPORT_NAME MimeTreeParserWidgets
)

ecm_generate_pri_file(BASE_NAME MimeTreeParserWidgets
    LIB_NAME KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets
    DEPS "MimeTreeParserWidgets" 
    FILENAME_VAR PRI_FILENAME
)

install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

install(TARGETS
    KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets
    EXPORT KPim${KF_MAJOR_VERSION}MimeTreeParserWidgetsTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

ecm_generate_headers(MimeTreeParserWidgets_CamelCase_HEADERS
    HEADER_NAMES
        MessageViewer
    REQUIRED_HEADERS MimeTreeParserWidgets_HEADERS
    PREFIX MimeTreeParserWidgets
)

install(FILES
    ${MimeTreeParserWidgets_CamelCase_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/MimeTreeParserWidgets/MimeTreeParserWidgets
    COMPONENT Devel
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_widgets_export.h
    ${MimeTreeParserWidgets_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/MimeTreeParserWidgets/mimetreeparserwidgets
    COMPONENT Devel
)

if (BUILD_QCH)
    ecm_add_qch(
        KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets_QCH
        NAME MimeTreeParserWidgets
        BASE_NAME KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        # using only public headers, to cover only public API
        SOURCES ${MimeTreeParserWidget_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt${QT_MAJOR_VERSION}Core_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            MIMETREEPARSER_WIDGETS_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()

########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets_QCH
        FILE KPim${KF_MAJOR_VERSION}MimeTreeParserWidgetsQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserQchTargets.cmake\")")
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KPimMimeTreeParserWidgetsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserWidgetsConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserWidgetsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserWidgetsConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(EXPORT KPim${KF_MAJOR_VERSION}MimeTreeParserWidgetsTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KPim${KF_MAJOR_VERSION}MimeTreeParserTargets.cmake NAMESPACE KPim${KF_MAJOR_VERSION}::)

install(FILES
   ${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_widgets_version.h
   DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/MimeTreeParserWidgets COMPONENT Devel
)
