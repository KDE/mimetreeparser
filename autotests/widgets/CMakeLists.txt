# SPDX-FileCopyrightText: 2023 g10 Code GmbH 
# SPDX-FileContributor: Carl Schwan <carlschwan@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DMAIL_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../testdata")
include(${CMAKE_SOURCE_DIR}/cmake/modules/add_gpg_crypto_test.cmake)

ecm_add_test(messageviewerdialogtest.cpp
    LINK_LIBRARIES KPim6MimeTreeParserWidgets Qt::Test
    NAME_PREFIX "widgets-"
)

add_executable(messageviewertest messageviewertest.cpp ../setupenv.cpp)
add_gpg_crypto_test(messageviewertest widgets-messageviewertest)
target_link_libraries(messageviewertest PUBLIC
    Qt::Test
    KPim6::MimeTreeParserWidgets
    KF6::WidgetsAddons
    Gpgmepp
)
