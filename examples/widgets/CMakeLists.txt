# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

add_executable(mimetreeparser-widgets
    main.cpp
)

find_package(KPim${KF_MAJOR_VERSION}Mbox ${KPIM_MIME_VERSION} CONFIG REQUIRED)

target_link_libraries(mimetreeparser-widgets PUBLIC
    Qt::Widgets
    KPim${KF_MAJOR_VERSION}::MimeTreeParserWidgets
    KPim${KF_MAJOR_VERSION}::Mbox
)
