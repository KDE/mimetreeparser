# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

add_executable(mimetreeparser-qml
    main.cpp
    fileopener.h
    fileopener.cpp
    resources.qrc
)

find_package(Qt${KF_MAJOR_VERSION}Qml ${QT_MIN_VERSION} CONFIG REQUIRED)
find_package(Qt${KF_MAJOR_VERSION}Quick ${QT_MIN_VERSION} CONFIG REQUIRED)
find_package(Qt${KF_MAJOR_VERSION}Widgets ${QT_MIN_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}Mbox ${KPIM_MIME_VERSION} CONFIG REQUIRED)
target_link_libraries(mimetreeparser-qml PUBLIC
    Qt::Quick
    Qt::Widgets
    Qt::Qml
    Qt::Gui
    KF${KF_MAJOR_VERSION}::I18n
    KPim${KF_MAJOR_VERSION}::MimeTreeParserCore
    KPim${KF_MAJOR_VERSION}::Mbox
)
