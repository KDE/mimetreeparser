// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "attachmentview_p.h"
#include <QHeaderView>

AttachmentView::AttachmentView(QWidget *parent)
    : QTreeView(parent)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setStretchLastSection(false);
    setColumnWidth(0, 200);
}

void AttachmentView::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event);
    Q_EMIT contextMenuRequested();
}
