# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

add_executable(mimetreeparser-qml
    main.cpp
    messagehandler.cpp
    messagehandler.h
    resources.qrc
)

find_package(Qt6Qml ${QT_MIN_VERSION} CONFIG REQUIRED)
find_package(Qt6Quick ${QT_MIN_VERSION} CONFIG REQUIRED)
find_package(KPim6Mbox ${KPIM_MIME_VERSION} CONFIG REQUIRED)
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
