// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mimetreeparser_widgets_export.h"

#include <KMime/Message>
#include <QMainWindow>

#include <memory>

class QToolBar;

namespace MimeTreeParser
{
namespace Widgets
{

/// MessageViewerWindow that displays the given email stored in the
/// file.
///
/// \author Carl Schwan <carl.schwan@gnupg.com>
class MIMETREEPARSER_WIDGETS_EXPORT MessageViewerWindow : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(QList<KMime::Message::Ptr> messages READ messages WRITE setMessages NOTIFY messagesChanged)

public:
    MessageViewerWindow(QWidget *parent = nullptr);
    ~MessageViewerWindow() override;

    [[nodiscard]] QToolBar *toolBar() const;

    [[nodiscard]] QList<KMime::Message::Ptr> messages() const;
    void setMessages(const QList<KMime::Message::Ptr> &messages);

Q_SIGNALS:
    void messagesChanged();

private:
    MIMETREEPARSER_WIDGETS_NO_EXPORT void initGUI();

    class Private;
    std::unique_ptr<Private> const d;
};

} // end namespace Widgets
} // end namespace MimeTreeParser
