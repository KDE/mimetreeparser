# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(mimetreeparser_plugin URI "org.kde.pim.mimetreeparser" VERSION 1.0)

target_sources(mimetreeparser_plugin PRIVATE
    mimetreeparserplugin.cpp
    mimetreeparserplugin.h
)

target_link_libraries(mimetreeparser_plugin PRIVATE
    Qt::Quick
    KPim6::MimeTreeParserCore Gpgmepp
)

ecm_target_qml_sources(mimetreeparser_plugin
    SOURCES
        qml/MailViewer.qml
)

ecm_target_qml_sources(mimetreeparser_plugin PRIVATE
    PATH private
    SOURCES
    qml/private/AttachmentDelegate.qml
    qml/private/Banner.qml
    qml/private/ErrorPart.qml
    qml/private/HtmlPart.qml
    qml/private/ICalPart.qml
    qml/private/MailPart.qml
    qml/private/MailPartModel.qml
    qml/private/MailPartView.qml
    qml/private/TextPart.qml
)

ecm_finalize_qml_module(mimetreeparser_plugin
    DESTINATION ${KDE_INSTALL_QMLDIR}
    BUILD_SHARED_LIBS OFF
)
