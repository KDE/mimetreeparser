# SPDX-FileCopyrightText: 2023 g10 Code GmbH
# SPDX-FileContributor: Carl Schwan <carlschwan@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DMAIL_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../testdata")
set(GNUPGHOME ${CMAKE_BINARY_DIR}/autotests/gnupg_home)
add_definitions(-DGNUPGHOME="${GNUPGHOME}")
include(${LIBKLEO_TEST_MODULE_PATH}/KleoAddGpgCryptoTest.cmake)

ecm_add_test(messageviewerdialogtest.cpp
    LINK_LIBRARIES KPim6MimeTreeParserWidgets Qt::Test
    NAME_PREFIX "widgets-"
)

add_executable(messageviewertest messageviewertest.cpp ../setupenv.cpp)
add_gpg_crypto_test(messageviewertest widgets-messageviewertest)
target_link_libraries(
    messageviewertest
    PUBLIC
        Qt::Test
        KPim6::MimeTreeParserWidgets
        KPim6::MimeTreeParserCore
        KF6::WidgetsAddons
        Gpgmepp
)
