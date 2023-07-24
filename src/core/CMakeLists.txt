# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_setup_version(PROJECT
    VARIABLE_PREFIX MIMETREEPARSER_CORE
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_core_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserCoreConfigVersion.cmake"
    SOVERSION 5
)

# private library

add_library(mimetreeparserprivate STATIC)
set_target_properties(mimetreeparserprivate PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_sources(mimetreeparserprivate PUBLIC
     messagepart.h
     messagepart.cpp
     crypto.h
     crypto.cpp
     cryptohelper.h
     cryptohelper.cpp
     utils.h
     utils.cpp
)
target_link_libraries(mimetreeparserprivate
PUBLIC
    KPim${KF_MAJOR_VERSION}::Mime
    KF${KF_MAJOR_VERSION}::I18n
    Qt${KF_MAJOR_VERSION}::Gui
    KF${KF_MAJOR_VERSION}::Codecs
    Gpgme::Gpgme
)

ecm_qt_declare_logging_category(mimetreeparserprivate
    HEADER mimetreeparser_core_debug.h
    IDENTIFIER MIMETREEPARSER_CORE_LOG
    CATEGORY_NAME org.kde.pim.mimetreeparser.core
    DESCRIPTION "mimetreeparser (pim lib)"
    EXPORT MIMETREEPARSERNG
)

# public dynamic library


add_library(KPim${KF_MAJOR_VERSION}MimeTreeParserCore)
add_library(KPim${KF_MAJOR_VERSION}::MimeTreeParserCore
    ALIAS KPim${KF_MAJOR_VERSION}MimeTreeParserCore
)

target_sources(KPim${KF_MAJOR_VERSION}MimeTreeParserCore PRIVATE
     errors.h
     async.h
     attachmentmodel.h
     bodypartformatter.h
     bodypartformatterbasefactory.h
     bodypartformatterbasefactory_p.h
     enums.h
     htmlutils.h
     mailcrypto.h
     messageparser.h
     objecttreeparser.h
     partmetadata.h
     partmodel.h
     attachmentmodel.cpp
     bodypartformatter.cpp
     bodypartformatter_impl.cpp
     bodypartformatterbasefactory.cpp
     htmlutils.cpp
     mailcrypto.cpp
     messageparser.cpp
     objecttreeparser.cpp
     partmodel.cpp
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim${KF_MAJOR_VERSION}MimeTreeParserCore PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(KPim${KF_MAJOR_VERSION}MimeTreeParserCore BASE_NAME mimetreeparser_core)


target_include_directories(KPim${KF_MAJOR_VERSION}MimeTreeParserCore
    INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/MimeTreeParserCore>")

target_link_libraries(KPim${KF_MAJOR_VERSION}MimeTreeParserCore
PUBLIC
    KPim${KF_MAJOR_VERSION}::Mime
    KF${KF_MAJOR_VERSION}::I18n
    Qt${KF_MAJOR_VERSION}::Gui
PRIVATE
    mimetreeparserprivate
)

set_target_properties(KPim${KF_MAJOR_VERSION}MimeTreeParserCore PROPERTIES
    VERSION ${MIMETREEPARSERNG_VERSION}
    SOVERSION ${MIMETREEPARSERNG_SOVERSION}
    EXPORT_NAME MimeTreeParserCore
)

ecm_generate_pri_file(BASE_NAME MimeTreeParserCore
    LIB_NAME KPim${KF_MAJOR_VERSION}MimeTreeParserCore
    DEPS "MimeTreeParserCore" 
    FILENAME_VAR PRI_FILENAME
)

install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

install(TARGETS
    KPim${KF_MAJOR_VERSION}MimeTreeParserCore
    EXPORT KPim${KF_MAJOR_VERSION}MimeTreeParserCoreTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

ecm_generate_headers(MimeTreeParserCore_CamelCase_HEADERS
    HEADER_NAMES
        AttachmentModel
        ObjectTreeParser
        MessageParser
        PartModel
    REQUIRED_HEADERS MimeTreeParserCore_HEADERS
    PREFIX MimeTreeParserCore
)

install(FILES
    ${MimeTreeParserCore_CamelCase_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/MimeTreeParserCore/MimeTreeParserCore
    COMPONENT Devel
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_core_export.h
    ${MimeTreeParserCore_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/MimeTreeParserCore/mimetreeparsercore
    COMPONENT Devel
)

if (BUILD_QCH)
    ecm_add_qch(
        KPim${KF_MAJOR_VERSION}MimeTreeParserCore_QCH
        NAME MimeTreeParserCore
        BASE_NAME KPim${KF_MAJOR_VERSION}MimeTreeParserCore
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        # using only public headers, to cover only public API
        SOURCES ${MimeTreeParser_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt${QT_MAJOR_VERSION}Core_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            MIMETREEPARSERCORE_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()

########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserCore")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KPim${KF_MAJOR_VERSION}MimeTreeParserCore_QCH
        FILE KPim${KF_MAJOR_VERSION}MimeTreeParserCoreQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserCoreQchTargets.cmake\")")
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KPimMimeTreeParserCoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserCoreConfig.cmake"
    INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserCoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KPim${KF_MAJOR_VERSION}MimeTreeParserCoreConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(EXPORT KPim${KF_MAJOR_VERSION}MimeTreeParserCoreTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KPim${KF_MAJOR_VERSION}MimeTreeParserCoreTargets.cmake
    NAMESPACE KPim${KF_MAJOR_VERSION}::
)

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
