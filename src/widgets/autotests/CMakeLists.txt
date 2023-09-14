# SPDX-FileCopyrightText: 2023 g10 Code GmbH 
# SPDX-FileContributor: Carl Schwan <carlschwan@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DMAIL_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../core/testdata")

ecm_add_test(messageviewerdialogtest.cpp
    LINK_LIBRARIES KPim${KF_MAJOR_VERSION}MimeTreeParserWidgets Qt::Test
    NAME_PREFIX "widget-"
)
