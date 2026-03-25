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

    void setCurrentIndex(int currentIndex);
    QMenuBar *createMenuBar(QWidget *parent);
};

void MessageViewerDialog::Private::setCurrentIndex(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < messages.count());

    currentIndex = index;
    messageViewer->setMessage(messages[currentIndex]);
    q->setWindowTitle(messageViewer->subject());

    previousAction->setEnabled(currentIndex != 0);
    nextAction->setEnabled(currentIndex != messages.count() - 1);
}

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

    if (d->messages.isEmpty()) {
        auto errorMessage = new KMessageWidget(this);
        errorMessage->setMessageType(KMessageWidget::Error);
        errorMessage->setText(i18nc("@info", "Unable to read file"));
        layout->addWidget(errorMessage);
        return;
    }

    const bool multipleMessages = d->messages.length() > 1;
    d->toolBar = new QToolBar(this);

    if (multipleMessages) {
#ifdef Q_OS_UNIX
        d->toolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
#else
        // on other platforms the default is IconOnly which is bad for
        // accessibility and can't be changed by the user.
        d->toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#endif

        d->toolBar->addAction(d->previousAction);
        connect(d->previousAction, &QAction::triggered, this, [this] {
            d->setCurrentIndex(d->currentIndex - 1);
        });

        const auto spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        d->toolBar->addWidget(spacer);

        d->toolBar->addAction(d->nextAction);
        connect(d->nextAction, &QAction::triggered, this, [this] {
            d->setCurrentIndex(d->currentIndex + 1);
        });
        d->nextAction->setEnabled(true);

        mainLayout->addWidget(d->toolBar);
    } else {
        mainLayout->addWidget(d->toolBar);
        d->toolBar->hide();
    }

    mainLayout->addLayout(layout);

    d->messageViewer = new MimeTreeParser::Widgets::MessageViewer(this);
    d->messageViewer->setMessage(d->messages[0]);
    setWindowTitle(d->messageViewer->subject());
    layout->addWidget(d->messageViewer);

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
