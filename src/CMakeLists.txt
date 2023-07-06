# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"libmimetreeparser5\")

add_library(KPim${KF_MAJOR_VERSION}MimeTreeParser)
add_library(KPim${KF_MAJOR_VERSION}::MimeTreeParser ALIAS KPim${KF_MAJOR_VERSION}MimeTreeParser)

target_sources(KPim${KF_MAJOR_VERSION}MimeTreeParser PRIVATE
     crypto.h
     crypto.cpp
     errors.h
  
     async.h
     attachmentmodel.h
     bodypartformatter.h
     bodypartformatterbasefactory.h
     bodypartformatterbasefactory_p.h
     cryptohelper.h
     enums.h
     htmlutils.h
     mailcrypto.h
     messageparser.h
     messagepart.h
     mimetreeparser_debug.h
     objecttreeparser.h
     partmetadata.h
     partmodel.h
     utils.h
     attachmentmodel.cpp
     bodypartformatter.cpp
     bodypartformatter_impl.cpp
     bodypartformatterbasefactory.cpp
     cryptohelper.cpp
     htmlutils.cpp
     mailcrypto.cpp
     messageparser.cpp
     messagepart.cpp
     mimetreeparser_debug.cpp
     objecttreeparser.cpp
     partmodel.cpp
     utils.cpp
)

ecm_qt_declare_logging_category(KPim${KF_MAJOR_VERSION}MimeTreeParser
    HEADER mimetreeparser_debug.h
    IDENTIFIER MIMETREEPARSER_LOG
    CATEGORY_NAME org.kde.pim.mimetreeparser
    DESCRIPTION "mimetreeparser (pim lib)"
    EXPORT MIMETREEPARSER
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim${KF_MAJOR_VERSION}MimeTreeParser PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(KPim${KF_MAJOR_VERSION}MimeTreeParser BASE_NAME mimetreeparser)


target_include_directories(KPim${KF_MAJOR_VERSION}MimeTreeParser INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/AkonadiCalendar>")
target_include_directories(KPim${KF_MAJOR_VERSION}MimeTreeParser PUBLIC "$<BUILD_INTERFACE:${Akonadi-Calendar_SOURCE_DIR}/src;${Akonadi-Calendar_BINARY_DIR}/src>")

target_link_libraries(KPim${KF_MAJOR_VERSION}MimeTreeParser
PUBLIC
    KPim${KF_MAJOR_VERSION}::Mime
    KF${KF_MAJOR_VERSION}::I18n
    Qt${KF_MAJOR_VERSION}::Gui
PRIVATE
    KF${KF_MAJOR_VERSION}::Codecs
    Gpgme::Gpgme
)

set_target_properties(KPim${KF_MAJOR_VERSION}MimeTreeParser PROPERTIES
    VERSION ${MIMETREEPARSER_VERSION}
    SOVERSION ${MIMETREEPARSER_SOVERSION}
    EXPORT_NAME MimeTreeParser
)

ecm_generate_pri_file(BASE_NAME MimeTreeParser
    LIB_NAME KPim${KF_MAJOR_VERSION}MimeTreeParser
    DEPS "MimeTreeParser" 
    FILENAME_VAR PRI_FILENAME
)

install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

install(TARGETS
    KPim${KF_MAJOR_VERSION}MimeTreeParser
    EXPORT KPim${KF_MAJOR_VERSION}MimeTreeParserTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

ecm_generate_headers(MimeTreeParser_CamelCase_HEADERS
    HEADER_NAMES
        AttachmentModel
        ObjectTreeParser
        MessageParser
    REQUIRED_HEADERS MimeTreeParser_HEADERS
    PREFIX MimeTreeParser
)

install(FILES
    ${MimeTreeParser_CamelCase_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/MimeTreeParser/MimeTreeParser
    COMPONENT Devel
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mimetreeparser_export.h
    ${MimeTreeParser_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/MimeTreeParser/mimetreeparser
    COMPONENT Devel
)


if (BUILD_QCH)
    ecm_add_qch(
        KPim${KF_MAJOR_VERSION}MimeTreeParser_QCH
        NAME MimeTreeParser
        BASE_NAME KPim${KF_MAJOR_VERSION}MimeTreeParser
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
            MIMETREEPARSER_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()
