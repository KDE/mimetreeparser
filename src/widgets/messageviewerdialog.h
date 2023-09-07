// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mimetreeparser_widgets_export.h"

#include <QDialog>

#include <memory>

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
    MessageViewerDialog(const QString &fileName, QWidget *parent = nullptr);
    ~MessageViewerDialog() override;

private:
    class Private;
    std::unique_ptr<Private> const d;
};

} // end namespace Widgets
} // end namespace MimeTreeParser
