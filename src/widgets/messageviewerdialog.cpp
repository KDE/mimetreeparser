// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerdialog.h"

#include "messageviewer.h"
#include "messageviewerutils_p.h"
#include <MimeTreeParserCore/CryptoHelper>
#include <MimeTreeParserCore/FileOpener>

#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageWidget>

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

using namespace MimeTreeParser::Widgets;

class MessageViewerDialog::Private
{
public:
    Private(MessageViewerDialog *dialog)
        : q(dialog)
    {
    }

    MessageViewerDialog *const q;
    int currentIndex = 0;
    QList<KMime::Message::Ptr> messages;
    QString fileName;
    MimeTreeParser::Widgets::MessageViewer *messageViewer = nullptr;
    QAction *nextAction = nullptr;
    QAction *previousAction = nullptr;
    QToolBar *toolBar = nullptr;

    void setCurrentIndex(int currentIndex);
    QMenuBar *createMenuBar(QWidget *parent);

private:
    void save(QWidget *parent);
    void saveDecrypted(QWidget *parent);
    void print(QWidget *parent);
    void printPreview(QWidget *parent);
    void printInternal(QPrinter *printer);
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

    const auto saveAction = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18nc("@action:inmenu", "&Save"));
    QObject::connect(saveAction, &QAction::triggered, parent, [parent, this] {
        save(parent);
    });
    fileMenu->addAction(saveAction);

    const auto saveDecryptedAction = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18nc("@action:inmenu", "Save Decrypted"));
    QObject::connect(saveDecryptedAction, &QAction::triggered, parent, [parent, this] {
        saveDecrypted(parent);
    });
    fileMenu->addAction(saveDecryptedAction);

    const auto printPreviewAction = new QAction(QIcon::fromTheme(QStringLiteral("document-print-preview")), i18nc("@action:inmenu", "Print Preview"));
    QObject::connect(printPreviewAction, &QAction::triggered, parent, [parent, this] {
        printPreview(parent);
    });
    fileMenu->addAction(printPreviewAction);

    const auto printAction = new QAction(QIcon::fromTheme(QStringLiteral("document-print")), i18nc("@action:inmenu", "&Print"));
    QObject::connect(printAction, &QAction::triggered, parent, [parent, this] {
        print(parent);
    });
    fileMenu->addAction(printAction);

    // Navigation menu
    const auto navigationMenu = menuBar->addMenu(i18nc("@action:inmenu", "&Navigation"));
    previousAction = new QAction(QIcon::fromTheme(QStringLiteral("go-previous")), i18nc("@action:button Previous email", "Previous Message"), parent);
    previousAction->setEnabled(false);
    navigationMenu->addAction(previousAction);

    nextAction = new QAction(QIcon::fromTheme(QStringLiteral("go-next")), i18nc("@action:button Next email", "Next Message"), parent);
    nextAction->setEnabled(false);
    navigationMenu->addAction(nextAction);

    return menuBar;
}

void MessageViewerDialog::Private::save(QWidget *parent)
{
    QString extension;
    QString alternatives;
    auto message = messages[currentIndex];
    bool wasEncrypted = false;
    GpgME::Protocol protocol;
    auto decryptedMessage = CryptoUtils::decryptMessage(message, wasEncrypted, protocol);
    Q_UNUSED(decryptedMessage); // we save the message without modifying it

    if (wasEncrypted) {
        extension = QStringLiteral(".mime");
        if (protocol == GpgME::OpenPGP) {
            alternatives = i18nc("File dialog accepted files", "Email files (*.eml *.mbox *.mime)");
        } else {
            alternatives = i18nc("File dialog accepted files", "Encrypted S/MIME files (*.p7m)");
        }
    } else {
        extension = QStringLiteral(".eml");
        alternatives = i18nc("File dialog accepted files", "Email files (*.eml *.mbox *.mime)");
    }

    const QString location =
        QFileDialog::getSaveFileName(parent,
                                     i18nc("@title:window", "Save File"),
                                     MesageViewerUtils::changeExtension(MesageViewerUtils::changeFileName(fileName, messageViewer->subject()), extension),
                                     alternatives);

    QSaveFile file(location);
    if (!file.open(QIODevice::WriteOnly)) {
        KMessageBox::error(parent, i18n("File %1 could not be created.", location), i18nc("@title:window", "Error saving message"));
        return;
    }
    file.write(messages[currentIndex]->encodedContent());
    file.commit();
}

void MessageViewerDialog::Private::saveDecrypted(QWidget *parent)
{
    const QString location = QFileDialog::getSaveFileName(
        parent,
        i18nc("@title:window", "Save Decrypted File"),
        MesageViewerUtils::changeExtension(MesageViewerUtils::changeFileName(fileName, messageViewer->subject()), QStringLiteral(".eml")),
        i18nc("File dialog accepted files", "Email files (*.eml *.mbox *.mime)"));

    QSaveFile file(location);
    if (!file.open(QIODevice::WriteOnly)) {
        KMessageBox::error(parent, i18nc("Error message", "File %1 could not be created.", location), i18nc("@title:window", "Error saving message"));
        return;
    }
    auto message = messages[currentIndex];
    bool wasEncrypted = false;
    GpgME::Protocol protocol;
    auto decryptedMessage = CryptoUtils::decryptMessage(message, wasEncrypted, protocol);
    if (!wasEncrypted) {
        decryptedMessage = message;
    }
    file.write(decryptedMessage->encodedContent());

    file.commit();
}

void MessageViewerDialog::Private::print(QWidget *parent)
{
    QPrinter printer;
    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle(i18nc("@title:window", "Print"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    printInternal(&printer);
}

void MessageViewerDialog::Private::printPreview(QWidget *parent)
{
    auto dialog = new QPrintPreviewDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(800, 750);
    dialog->setWindowTitle(i18nc("@title:window", "Print Preview"));
    QObject::connect(dialog, &QPrintPreviewDialog::paintRequested, parent, [this](QPrinter *printer) {
        printInternal(printer);
    });
    dialog->open();
}

void MessageViewerDialog::Private::printInternal(QPrinter *printer)
{
    QPainter painter;
    painter.begin(printer);
    const auto pageLayout = printer->pageLayout();
    const auto pageRect = pageLayout.paintRectPixels(printer->resolution());
    const double xscale = pageRect.width() / double(messageViewer->width());
    const double yscale = pageRect.height() / double(messageViewer->height());
    const double scale = qMin(qMin(xscale, yscale), 1.);
    painter.translate(pageRect.x(), pageRect.y());
    painter.scale(scale, scale);
    messageViewer->print(&painter, pageRect.width());
}

MessageViewerDialog::MessageViewerDialog(const QList<KMime::Message::Ptr> &messages, QWidget *parent)
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

    setMinimumSize(300, 300);
    resize(600, 600);
}

MessageViewerDialog::~MessageViewerDialog() = default;

QToolBar *MessageViewerDialog::toolBar() const
{
    return d->toolBar;
}

QList<KMime::Message::Ptr> MessageViewerDialog::messages() const
{
    return d->messages;
}

#include "moc_messageviewerdialog.cpp"
