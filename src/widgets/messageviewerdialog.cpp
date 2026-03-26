// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerdialog.h"

#include "messageviewerbase_p.h"

#include <MimeTreeParserCore/FileOpener>

#include <KLocalizedString>
#include <KMessageWidget>
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
    d->messages += messages;
    initGUI();
}

MessageViewerDialog::MessageViewerDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>(this))
{
    d->fileName = fileName;
    d->messages += MimeTreeParser::Core::FileOpener::openFile(fileName);
    initGUI();
}

void MessageViewerDialog::initGUI()
{
    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins({});
    mainLayout->setSpacing(0);

    const auto menuBar = d->createMenuBar(this);
    mainLayout->setMenuBar(menuBar);

    mainLayout->addWidget(d->toolBar);
    if (d->messages.size() > 1) {
        d->toolBar->show();
    }

    mainLayout->addWidget(d->centralWidget);

    if (d->messages.isEmpty()) {
        if (d->fileName.isEmpty()) {
            d->errorMessage->setText(xi18nc("@info", "No messages to display."));
        } else {
            d->errorMessage->setText(
                xi18nc("@info", "Unable to read the file <filename>%1</filename>, or the file doesn't contain any messages.", d->fileName));
        }
        d->centralWidget->setCurrentIndex(1);
    }

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin, nullptr, this),
                                  style()->pixelMetric(QStyle::PM_LayoutTopMargin, nullptr, this),
                                  style()->pixelMetric(QStyle::PM_LayoutRightMargin, nullptr, this),
                                  style()->pixelMetric(QStyle::PM_LayoutBottomMargin, nullptr, this));
    auto closeButton = buttonBox->addButton(QDialogButtonBox::Close);
    connect(closeButton, &QPushButton::pressed, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox);

    if (d->statusBar) {
        auto separator = new KSeparator(Qt::Horizontal, this);
        mainLayout->addWidget(separator);
        mainLayout->addWidget(d->statusBar);
    }

    setMinimumSize(300, 300);
    resize(600, 600);

    if (!d->messages.isEmpty()) {
        d->setCurrentIndex(0);
    }
}

MessageViewerDialog::~MessageViewerDialog() = default;

QToolBar *MessageViewerDialog::toolBar() const
{
    return d->toolBar;
}

QList<std::shared_ptr<KMime::Message>> MessageViewerDialog::messages() const
{
    return d->messages;
}

#include "moc_messageviewerdialog.cpp"
