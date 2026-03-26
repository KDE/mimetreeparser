// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerdialog.h"

#include "messageviewerbase_p.h"

#include <MimeTreeParserCore/FileOpener>

#include <KLocalizedString>
#include <KSeparator>

#include <QDialogButtonBox>
#include <QMenuBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

using namespace MimeTreeParser::Widgets;
using namespace Qt::Literals::StringLiterals;

class MessageViewerDialog::Private : public MessageViewerBasePrivate
{
public:
    explicit Private(MessageViewerDialog *dialog)
        : MessageViewerBasePrivate{dialog}
    {
    }

    QMenuBar *createMenuBar(QWidget *parent);
};

QMenuBar *MessageViewerDialog::Private::createMenuBar(QWidget *parent)
{
    const auto menuBar = new QMenuBar(parent);

    // File menu
    const auto fileMenu = menuBar->addMenu(i18nc("@action:inmenu", "&File"));
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveDecryptedAction);
    fileMenu->addAction(printPreviewAction);
    fileMenu->addAction(printAction);

    // Navigation menu
    const auto navigationMenu = menuBar->addMenu(i18nc("@action:inmenu", "&Navigation"));
    navigationMenu->addAction(previousAction);
    navigationMenu->addAction(nextAction);

    return menuBar;
}

MessageViewerDialog::MessageViewerDialog(const QList<std::shared_ptr<KMime::Message>> &messages, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>(this))
{
    initGUI();
    d->setMessages(messages);
}

MessageViewerDialog::MessageViewerDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>(this))
{
    initGUI();
    d->fileName = fileName;
    d->setMessages(MimeTreeParser::Core::FileOpener::openFile(fileName));
}

void MessageViewerDialog::initGUI()
{
    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins({});
    mainLayout->setSpacing(0);

    const auto menuBar = d->createMenuBar(this);
    mainLayout->setMenuBar(menuBar);

    mainLayout->addWidget(d->toolBar);

    mainLayout->addWidget(d->centralWidget);

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin, nullptr, this),
                                  style()->pixelMetric(QStyle::PM_LayoutTopMargin, nullptr, this),
                                  style()->pixelMetric(QStyle::PM_LayoutRightMargin, nullptr, this),
                                  style()->pixelMetric(QStyle::PM_LayoutBottomMargin, nullptr, this));
    auto closeButton = buttonBox->addButton(QDialogButtonBox::Close);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox);

    if (d->statusBar) {
        auto separator = new KSeparator(Qt::Horizontal, this);
        mainLayout->addWidget(separator);
        mainLayout->addWidget(d->statusBar);
    }

    setMinimumSize(300, 300);
    resize(600, 600);
}

MessageViewerDialog::~MessageViewerDialog() = default;

QToolBar *MessageViewerDialog::toolBar() const
{
    return d->toolBar;
}

QList<std::shared_ptr<KMime::Message>> MessageViewerDialog::messages() const
{
    return d->getMessages();
}

#include "moc_messageviewerdialog.cpp"
