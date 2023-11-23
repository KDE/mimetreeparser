# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_setup_version(PROJECT
    VARIABLE_PREFIX MIMETREEPARSER_CORE
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_core_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserCoreConfigVersion.cmake"
    SOVERSION 6
)

# public dynamic library


add_library(KPim6MimeTreeParserCore)
add_library(KPim6::MimeTreeParserCore
    ALIAS KPim6MimeTreeParserCore
)

ecm_qt_declare_logging_category(KPim6MimeTreeParserCore
    HEADER mimetreeparser_core_debug.h
    IDENTIFIER MIMETREEPARSER_CORE_LOG
    CATEGORY_NAME org.kde.pim.mimetreeparser.core
    DESCRIPTION "mimetreeparser (pim lib)"
    EXPORT MIMETREEPARSERNG
)

target_sources(KPim6MimeTreeParserCore PRIVATE
     errors.h
     attachmentmodel.h
     bodypartformatter.h
     bodypartformatterbasefactory.h
     bodypartformatterbasefactory_p.h
     cryptohelper.h
     enums.h
     fileopener.h
     htmlutils.h
     messageparser.h
     messagepart.h
     objecttreeparser.h
     partmetadata.h
     partmodel.h
     utils.h
     attachmentmodel.cpp
     bodypartformatter.cpp
     bodypartformatter_impl.cpp
     bodypartformatterbasefactory.cpp
     cryptohelper.cpp
     fileopener.cpp
     htmlutils.cpp
     messageparser.cpp
     messagepart.cpp
     objecttreeparser.cpp
     partmodel.cpp
     utils.cpp
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim6MimeTreeParserCore PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(KPim6MimeTreeParserCore BASE_NAME mimetreeparser_core)


target_include_directories(KPim6MimeTreeParserCore
    INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserCore>")

target_link_libraries(KPim6MimeTreeParserCore
PUBLIC
    KPim6::Mime
    KPim6::Mbox
    KF6::I18n
    Qt6::Gui
PRIVATE
    KF6::Codecs
    Gpgmepp
    KPim6::Libkleo

)

set_target_properties(KPim6MimeTreeParserCore PROPERTIES
    VERSION ${MIMETREEPARSERNG_VERSION}
    SOVERSION ${MIMETREEPARSERNG_SOVERSION}
    EXPORT_NAME MimeTreeParserCore
)

ecm_generate_pri_file(BASE_NAME MimeTreeParserCore
    LIB_NAME KPim6MimeTreeParserCore
    DEPS "MimeTreeParserCore" 
    FILENAME_VAR PRI_FILENAME
)

install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

install(TARGETS
    KPim6MimeTreeParserCore
    EXPORT KPim6MimeTreeParserCoreTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

ecm_generate_headers(MimeTreeParserCore_CamelCase_HEADERS
    HEADER_NAMES
        AttachmentModel
        CryptoHelper
        Errors
        FileOpener
        MessageParser
        MessagePart
        ObjectTreeParser
        PartModel
        PartMetaData
    REQUIRED_HEADERS MimeTreeParserCore_HEADERS
    PREFIX MimeTreeParserCore
)

install(FILES
    ${MimeTreeParserCore_CamelCase_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserCore/MimeTreeParserCore
    COMPONENT Devel
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_core_export.h
    ${MimeTreeParserCore_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserCore/mimetreeparsercore
    COMPONENT Devel
)

if (BUILD_QCH)
    ecm_add_qch(
        KPim6MimeTreeParserCore_QCH
        NAME MimeTreeParserCore
        BASE_NAME KPim6MimeTreeParserCore
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        # using only public headers, to cover only public API
        SOURCES ${MimeTreeParserCore_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt6Core_QCH
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
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KPim6MimeTreeParserCore")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KPim6MimeTreeParserCore_QCH
        FILE KPim6MimeTreeParserCoreQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KPim6MimeTreeParserCoreQchTargets.cmake\")")
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KPimMimeTreeParserCoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserCoreConfig.cmake"
    INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserCoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserCoreConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(EXPORT KPim6MimeTreeParserCoreTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KPim6MimeTreeParserCoreTargets.cmake
    NAMESPACE KPim6::
)
