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
add_library(KPim6::MimeTreeParserWidgets
    ALIAS KPim6MimeTreeParserWidgets)

target_sources(KPim6MimeTreeParserWidgets PRIVATE
     attachmentview_p.h
     attachmentview.cpp
     messagecontainerwidget_p.h
     messagecontainerwidget.cpp
     messageviewer.h
     messageviewer.cpp
     messageviewerdialog.h
     messageviewerdialog.cpp
     urlhandler_p.h
     urlhandler.cpp
)

ecm_qt_declare_logging_category(KPim6MimeTreeParserWidgets
    HEADER mimetreeparser_widgets_debug.h
    IDENTIFIER MIMETREEPARSER_WIDGET_LOG
    CATEGORY_NAME org.kde.pim.mimetreeparser.widgets
    DESCRIPTION "mimetreeparser (pim lib)"
    EXPORT MIMETREEPARSER
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim6MimeTreeParserWidgets PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(KPim6MimeTreeParserWidgets BASE_NAME mimetreeparser_widgets)


target_include_directories(KPim6MimeTreeParserWidgets INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserWidgets>")

target_link_libraries(KPim6MimeTreeParserWidgets
PUBLIC
    Qt6::Widgets
    KPim6::MimeTreeParserCore
PRIVATE
    Qt6::PrintSupport
    KF6::Codecs
    KF6::I18n
    KF6::CalendarCore
    KF6::WidgetsAddons
    KPim6::Libkleo
    Gpgmepp
)

set_target_properties(KPim6MimeTreeParserWidgets PROPERTIES
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

install(TARGETS
    KPim6MimeTreeParserWidgets
    EXPORT KPim6MimeTreeParserWidgetsTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

ecm_generate_headers(MimeTreeParserWidgets_CamelCase_HEADERS
    HEADER_NAMES
        MessageViewer
        MessageViewerDialog
    REQUIRED_HEADERS MimeTreeParserWidgets_HEADERS
    PREFIX MimeTreeParserWidgets
)

install(FILES
    ${MimeTreeParserWidgets_CamelCase_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserWidgets/MimeTreeParserWidgets
    COMPONENT Devel
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_widgets_export.h
    ${MimeTreeParserWidgets_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserWidgets/mimetreeparserwidgets
    COMPONENT Devel
)

if (BUILD_QCH)
    ecm_add_qch(
        KPim6MimeTreeParserWidgets_QCH
        NAME MimeTreeParserWidgets
        BASE_NAME KPim6MimeTreeParserWidgets
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        # using only public headers, to cover only public API
        SOURCES ${MimeTreeParserWidgets_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt6Core_QCH
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
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KPim6MimeTreeParserWidgets")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KPim6MimeTreeParserWidgets_QCH
        FILE KPim6MimeTreeParserWidgetsQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KPim6MimeTreeParserWidgetsQchTargets.cmake\")")
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KPimMimeTreeParserWidgetsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserWidgetsConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserWidgetsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserWidgetsConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(EXPORT KPim6MimeTreeParserWidgetsTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KPim6MimeTreeParserWidgetsTargets.cmake
    NAMESPACE KPim6::
)

install(FILES
   ${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_widgets_version.h
   DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserWidgets
   COMPONENT Devel
)
