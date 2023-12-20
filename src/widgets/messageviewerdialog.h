// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mimetreeparser_widgets_export.h"

#include <KMime/Message>
#include <QDialog>

#include <memory>

class QToolBar;

namespace MimeTreeParser
{
namespace Widgets
{

/// MessageViewerDialog that displays the given email stored in the
/// file.
///
/// \author Carl Schwan <carl.schwan@gnupg.com>
class MIMETREEPARSER_WIDGETS_EXPORT MessageViewerDialog : public QDialog
{
    Q_OBJECT

public:
    MessageViewerDialog(const QList<KMime::Message::Ptr> &messages, QWidget *parent = nullptr);
    MessageViewerDialog(const QString &fileName, QWidget *parent = nullptr);
    ~MessageViewerDialog() override;

    QToolBar *toolBar() const;

    QList<KMime::Message::Ptr> messages() const;

private:
    void initGUI();

    class Private;
    std::unique_ptr<Private> const d;
};

} // end namespace Widgets
} // end namespace MimeTreeParser
