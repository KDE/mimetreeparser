# SPDX-FileCopyrightText: 2023 g10 Code GmbH
# SPDX-FileContributor: Carl Schwan <carlschwan@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

ecm_setup_version(PROJECT
    VARIABLE_PREFIX MIMETREEPARSER_WIDGETS
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_widgets_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserWidgetsConfigVersion.cmake"
    SOVERSION 6
)

add_library(KPim6MimeTreeParserWidgets)
add_library(KPim6::MimeTreeParserWidgets ALIAS KPim6MimeTreeParserWidgets)

target_sources(
    KPim6MimeTreeParserWidgets
    PRIVATE
        attachmentview_p.h
        attachmentview.cpp
        messagecontainerwidget_p.h
        messagecontainerwidget.cpp
        messageviewer.h
        messageviewer.cpp
        messageviewerdialog.h
        messageviewerdialog.cpp
        messageviewerwindow.h
        messageviewerwindow.cpp
        messageviewerutils_p.h
        messageviewerutils_p.cpp
)

ecm_qt_declare_logging_category(KPim6MimeTreeParserWidgets
    HEADER mimetreeparser_widgets_debug.h
    IDENTIFIER MIMETREEPARSER_WIDGET_LOG
    CATEGORY_NAME org.kde.pim.mimetreeparser.widgets
    DESCRIPTION "mimetreeparser (pim lib)"
    EXPORT MIMETREEPARSER
)

if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim6MimeTreeParserWidgets PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(
    KPim6MimeTreeParserWidgets
    BASE_NAME mimetreeparser_widgets
)

target_include_directories(
    KPim6MimeTreeParserWidgets
    INTERFACE
        "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserWidgets>"
)

target_link_libraries(
    KPim6MimeTreeParserWidgets
    PUBLIC Qt::Widgets KPim6::Mime
    PRIVATE
        KPim6::MimeTreeParserCore
        Qt::PrintSupport
        KF6::I18n
        KF6::ColorScheme
        KF6::CalendarCore
        KF6::WidgetsAddons
        KPim6::Libkleo
        Gpgmepp
)

set_target_properties(
    KPim6MimeTreeParserWidgets
    PROPERTIES
        VERSION ${MIMETREEPARSER_WIDGETS_VERSION}
        SOVERSION ${MIMETREEPARSER_WIDGETS_SOVERSION}
        EXPORT_NAME MimeTreeParserWidgets
)

ecm_generate_pri_file(BASE_NAME MimeTreeParserWidgets
    LIB_NAME KPim6MimeTreeParserWidgets
    DEPS "MimeTreeParserWidgets"
    FILENAME_VAR PRI_FILENAME
)

install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

install(
    TARGETS KPim6MimeTreeParserWidgets
    EXPORT KPim6MimeTreeParserWidgetsTargets
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

ecm_generate_headers(MimeTreeParserWidgets_CamelCase_HEADERS
    HEADER_NAMES
        MessageViewer
        MessageViewerDialog
        MessageViewerWindow
    REQUIRED_HEADERS MimeTreeParserWidgets_HEADERS
    PREFIX MimeTreeParserWidgets
)

install(
    FILES ${MimeTreeParserWidgets_CamelCase_HEADERS}
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserWidgets/MimeTreeParserWidgets
    COMPONENT Devel
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_widgets_export.h
        ${MimeTreeParserWidgets_HEADERS}
    DESTINATION
        ${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserWidgets/mimetreeparserwidgets
    COMPONENT Devel
)

########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR
    "${KDE_INSTALL_CMAKEPACKAGEDIR}/KPim6MimeTreeParserWidgets"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KPimMimeTreeParserWidgetsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserWidgetsConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserWidgetsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserWidgetsConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(
    EXPORT KPim6MimeTreeParserWidgetsTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KPim6MimeTreeParserWidgetsTargets.cmake
    NAMESPACE KPim6::
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_widgets_version.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserWidgets
    COMPONENT Devel
)
