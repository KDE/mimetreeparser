// SPDX-FileCopyCopyright: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "mimetreeparser_widgets_export.h"
#include <KMime/Message>
#include <QSplitter>
#include <QWidget>
#include <memory>

/// MessageViewer that displays the given KMime::Message::Ptr
class MIMETREEPARSER_WIDGETS_EXPORT MessageViewer : public QSplitter
{
public:
    explicit MessageViewer(QWidget *parent = nullptr);
    ~MessageViewer();

    KMime::Message::Ptr message() const;
    void setMessage(const KMime::Message::Ptr message);

private:
    class Private;
    std::unique_ptr<Private> d;
};
