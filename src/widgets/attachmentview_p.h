// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QTreeView>

class AttachmentView : public QTreeView
{
    Q_OBJECT

public:
    explicit AttachmentView(QWidget *parent);
    void contextMenuEvent(QContextMenuEvent *event) override;

Q_SIGNALS:
    void contextMenuRequested();
};
