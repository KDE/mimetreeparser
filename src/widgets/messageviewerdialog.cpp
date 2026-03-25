// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerdialog.h"

#include "messageviewerbase_p.h"

#include <MimeTreeParserCore/FileOpener>

#include <KColorScheme>
#include <KLocalizedString>
#include <KMessageWidget>
#include <KSeparator>

#include <Libkleo/Compliance>

#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
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

    const auto saveAction = new QAction(QIcon::fromTheme(u"document-save"_s), i18nc("@action:inmenu", "&Save"), parent);
    QObject::connect(saveAction, &QAction::triggered, parent, [parent, this] {
        save(parent);
    });
    fileMenu->addAction(saveAction);

    const auto saveDecryptedAction = new QAction(QIcon::fromTheme(u"document-save"_s), i18nc("@action:inmenu", "Save Decrypted"), parent);
    QObject::connect(saveDecryptedAction, &QAction::triggered, parent, [parent, this] {
        saveDecrypted(parent);
    });
    fileMenu->addAction(saveDecryptedAction);

    const auto printPreviewAction = new QAction(QIcon::fromTheme(u"document-print-preview"_s), i18nc("@action:inmenu", "Print Preview"), parent);
    QObject::connect(printPreviewAction, &QAction::triggered, parent, [parent, this] {
        printPreview(parent);
    });
    fileMenu->addAction(printPreviewAction);

    const auto printAction = new QAction(QIcon::fromTheme(u"document-print"_s), i18nc("@action:inmenu", "&Print"), parent);
    QObject::connect(printAction, &QAction::triggered, parent, [parent, this] {
        print(parent);
    });
    fileMenu->addAction(printAction);

    // Navigation menu
    const auto navigationMenu = menuBar->addMenu(i18nc("@action:inmenu", "&Navigation"));
    previousAction = new QAction(QIcon::fromTheme(u"go-previous"_s), i18nc("@action:button Previous email", "Previous Message"), parent);
    previousAction->setEnabled(false);
    navigationMenu->addAction(previousAction);

    nextAction = new QAction(QIcon::fromTheme(u"go-next"_s), i18nc("@action:button Next email", "Next Message"), parent);
    nextAction->setEnabled(false);
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

    const auto layout = new QVBoxLayout;

    const auto menuBar = d->createMenuBar(this);
    mainLayout->setMenuBar(menuBar);

    d->createToolBar(this);
    mainLayout->addWidget(d->toolBar);
    if (d->messages.size() <= 1) {
        d->toolBar->hide();
    }

    mainLayout->addLayout(layout);

    if (d->messages.isEmpty()) {
        auto errorMessage = new KMessageWidget(this);
        errorMessage->setMessageType(KMessageWidget::Error);
        errorMessage->setText(i18nc("@info", "Unable to read file"));
        layout->addWidget(errorMessage);
        layout->addStretch();
    } else {
        d->messageViewer = new MimeTreeParser::Widgets::MessageViewer(this);
        layout->addWidget(d->messageViewer);
    }

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin, nullptr, this),
                                  style()->pixelMetric(QStyle::PM_LayoutTopMargin, nullptr, this),
                                  style()->pixelMetric(QStyle::PM_LayoutRightMargin, nullptr, this),
                                  style()->pixelMetric(QStyle::PM_LayoutBottomMargin, nullptr, this));
    auto closeButton = buttonBox->addButton(QDialogButtonBox::Close);
    connect(closeButton, &QPushButton::pressed, this, &QDialog::accept);
    layout->addWidget(buttonBox);

    if (Kleo::DeVSCompliance::isActive()) {
        auto separator = new KSeparator(Qt::Horizontal, this);
        layout->addWidget(separator);

        auto statusBar = new QStatusBar(this);
        auto statusLbl = std::make_unique<QLabel>(Kleo::DeVSCompliance::name());
        {
            auto statusPalette = qApp->palette();
            KColorScheme::adjustForeground(statusPalette,
                                           Kleo::DeVSCompliance::isCompliant() ? KColorScheme::NormalText : KColorScheme::NegativeText,
                                           statusLbl->foregroundRole(),
                                           KColorScheme::View);
            statusLbl->setAutoFillBackground(true);
            KColorScheme::adjustBackground(statusPalette,
                                           Kleo::DeVSCompliance::isCompliant() ? KColorScheme::PositiveBackground : KColorScheme::NegativeBackground,
                                           QPalette::Window,
                                           KColorScheme::View);
            statusLbl->setPalette(statusPalette);
        }
        statusBar->addPermanentWidget(statusLbl.release());
        layout->addWidget(statusBar);
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
