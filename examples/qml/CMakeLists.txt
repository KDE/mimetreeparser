# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause
find_package(Qt6Qml ${QT_REQUIRED_VERSION} CONFIG REQUIRED)
find_package(Qt6Quick ${QT_REQUIRED_VERSION} CONFIG REQUIRED)
find_package(KPim6Mbox ${KPIM_MBOX_VERSION} CONFIG REQUIRED)

add_executable(mimetreeparser-qml
    main.cpp
    messagehandler.cpp
    messagehandler.h
    resources.qrc
)

target_link_libraries(mimetreeparser-qml PUBLIC
    Qt::Quick
    Qt::Widgets
    Qt::Qml
    Qt::Gui
    KF6::I18n
    KPim6::MimeTreeParserCore
    KPim6::Mbox
    Gpgmepp
)
