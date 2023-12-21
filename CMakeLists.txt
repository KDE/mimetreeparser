# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(PIM_VERSION "5.240.90")
project(MimeTreeParserNG VERSION ${PIM_VERSION})

# ECM setup
set(KF_MIN_VERSION "5.246.0")
find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${ECM_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)
set(QT_REQUIRED_VERSION "6.6.0")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(GenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)

include(ECMQmlModule)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEGitCommitHooks)
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
include(ECMAddQch)
include(ECMAddTests)

set(KPIM_MIME_VERSION "5.240.90")
set(KPIM_LIBKLEO_VERSION "5.240.90")
set(KPIM_MBOX_VERSION "5.240.90")
set(GPGME_REQUIRED_VERSION "1.16.0")

ecm_setup_version(PROJECT
    VARIABLE_PREFIX MIMETREEPARSERNG
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/mimetreeparserng_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KPim6MimeTreeParserCoreConfigVersion.cmake"
    SOVERSION 6
)

configure_file(mimetreeparserng-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/mimetreeparserng-version.h @ONLY)

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

if(BUILD_TESTING)
   add_definitions(-DBUILD_TESTING)
endif()

########### Find packages ###########
find_package(Qt6Gui ${QT_MIN_VERSION} CONFIG REQUIRED)
find_package(Qt6PrintSupport ${QT_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6I18n ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Codecs ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6CalendarCore ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6WidgetsAddons ${QT_MIN_VERSION} CONFIG)
find_package(KPim6Mime ${KPIM_MIME_VERSION} CONFIG REQUIRED)
find_package(KPim6Mbox ${KPIM_MBOX_VERSION} CONFIG REQUIRED)
find_package(KPim6Libkleo ${KPIM_LIBKLEO_VERSION} CONFIG REQUIRED)
find_package(Gpgmepp ${GPGME_REQUIRED_VERSION} CONFIG REQUIRED)

find_package(Qt6Quick ${QT_MIN_VERSION} CONFIG)
find_package(Qt6Widgets ${QT_MIN_VERSION} CONFIG)

if (BUILD_TESTING)
    find_package(Qt6Test ${QT_MIN_VERSION} CONFIG REQUIRED)
endif()

########### Targets ###########

add_definitions(-DQT_NO_CONTEXTLESS_CONNECT)
ecm_set_disabled_deprecation_versions(QT 6.6.0 KF 5.246.0)

option(USE_UNITY_CMAKE_SUPPORT "Use UNITY cmake support (speedup compile time)" OFF)

set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if (USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
    add_definitions(-DCOMPILE_WITH_UNITY_CMAKE_SUPPORT)
endif()

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(examples)
    add_subdirectory(autotests)
endif()

install(FILES
   ${CMAKE_CURRENT_BINARY_DIR}/mimetreeparserng_version.h
   DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/MimeTreeParserCore
   COMPONENT Devel
)
ecm_qt_install_logging_categories(
    EXPORT MIMETREEPARSERNG
    FILE mimetreeparser2.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
ki18n_install(po)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
