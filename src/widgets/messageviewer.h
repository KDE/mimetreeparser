// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "mimetreeparser_widgets_export.h"

#include <KMime/Message>

#include <QSplitter>
#include <QWidget>

#include <memory>

class QPainter;

namespace MimeTreeParser
{
namespace Widgets
{

/// MessageViewer that displays the given KMime::Message::Ptr
/// \author Carl Schwan <carl.schwan@gnupg.com>
class MIMETREEPARSER_WIDGETS_EXPORT MessageViewer : public QSplitter
{
public:
    explicit MessageViewer(QWidget *parent = nullptr);
    ~MessageViewer() override;

    KMime::Message::Ptr message() const;
    void setMessage(const KMime::Message::Ptr message);

    /// Return the message subject
    QString subject() const;

    void print(QPainter *painter, int width);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // end namespace Widgets
} // end namespace MimeTreeParser
