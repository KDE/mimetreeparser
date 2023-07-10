// SPDX-FileCopyCopyright: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <KMime/Message>
#include <QScrollArea>
#include <memory>

/// MessageViewer that displays the given KMime::Message::Ptr
class MessageViewer : public QScrollArea
{
public:
    explicit MessageViewer(QWidget *parent = nullptr);

    KMime::Message::Ptr message() const;
    void setMessage(const KMime::Message::Ptr message);

private:
    class Private;
    std::unique_ptr<Private> d;
};
