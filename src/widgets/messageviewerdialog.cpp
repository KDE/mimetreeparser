// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerdialog.h"

#include "messageviewer.h"
#include "mimetreeparser_widgets_debug.h"
#include <MimeTreeParserCore/CryptoHelper>
#include <MimeTreeParserCore/FileOpener>

#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageWidget>
#include <KMime/Message>

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

#include <memory>

using namespace MimeTreeParser::Widgets;

namespace
{

/// On windows, force the filename to end with .eml
/// On Linux, do nothing as this is handled by the file picker
inline QString changeExtension(const QString &fileName)
{
#ifdef Q_OS_WIN
    auto renamedFileName = fileName;
    renamedFileName.replace(QRegularExpression(QStringLiteral("(mbox|p7m|asc)$")), QStringLiteral("eml"));

    // In case the file name didn't contain any of the expected extension: mbox, p7m, asc and eml
    // or doesn't contains an extension at all.
    if (!renamedFileName.endsWith(QStringLiteral(".eml"))) {
        renamedFileName += QStringLiteral(".eml");
    }

    Q_ASSERT(renamedFileName.endsWith(QStringLiteral(".eml")));
    return renamedFileName;
#else
    // Handled automatically by the file picker on linux
    return fileName;
#endif
}

}

class MessageViewerDialog::Private
{
public:
    int currentIndex = 0;
    QVector<KMime::Message::Ptr> messages;
    QString fileName;
    MimeTreeParser::Widgets::MessageViewer *messageViewer = nullptr;
    QAction *nextAction = nullptr;
    QAction *previousAction = nullptr;

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
    const QString location = QFileDialog::getSaveFileName(parent,
                                                          i18nc("@title:window", "Save File"),
                                                          changeExtension(fileName),
                                                          i18nc("File dialog accepted files", "Email files (*.eml *.mbox)"));

    QSaveFile file(location);
    if (!file.open(QIODevice::WriteOnly)) {
        KMessageBox::error(parent, i18n("File %1 could not be created.", location), i18n("Error saving message"));
        return;
    }
    file.write(messages[currentIndex]->encodedContent());
    file.commit();
}

void MessageViewerDialog::Private::saveDecrypted(QWidget *parent)
{
    const QString location = QFileDialog::getSaveFileName(parent,
                                                          i18nc("@title:window", "Save Decrypted File"),
                                                          changeExtension(fileName),
                                                          i18nc("File dialog accepted files", "Email files (*.eml *.mbox)"));

    QSaveFile file(location);
    if (!file.open(QIODevice::WriteOnly)) {
        KMessageBox::error(parent, i18nc("Error message", "File %1 could not be created.", location), i18n("Error saving message"));
        return;
    }
    auto message = messages[currentIndex];
    bool wasEncrypted = false;
    auto decryptedMessage = CryptoUtils::decryptMessage(message, wasEncrypted);
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
    dialog.setWindowTitle(i18nc("@title:window", "Print Document"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    printInternal(&printer);
}

void MessageViewerDialog::Private::printPreview(QWidget *parent)
{
    auto dialog = new QPrintPreviewDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(800, 750);
    dialog->setWindowTitle(i18nc("@title:window", "Print Document"));
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

MessageViewerDialog::MessageViewerDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>())
{
    d->fileName = fileName;
    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins({});

    const auto layout = new QVBoxLayout;
    layout->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin, nullptr, this),
                               style()->pixelMetric(QStyle::PM_LayoutTopMargin, nullptr, this),
                               style()->pixelMetric(QStyle::PM_LayoutRightMargin, nullptr, this),
                               style()->pixelMetric(QStyle::PM_LayoutBottomMargin, nullptr, this));

    const auto menuBar = d->createMenuBar(this);
    mainLayout->setMenuBar(menuBar);

    d->messages += MimeTreeParser::Core::FileOpener::openFile(fileName);

    if (d->messages.isEmpty()) {
        auto errorMessage = new KMessageWidget(this);
        errorMessage->setMessageType(KMessageWidget::Error);
        errorMessage->setText(i18nc("@info", "Unable to read file"));
        layout->addWidget(errorMessage);
        return;
    }

    const bool multipleMessages = d->messages.length() > 1;
    if (multipleMessages) {
        const auto toolBar = new QToolBar(this);
#ifdef Q_OS_UNIX
        toolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
#else
        // on other platforms the default is IconOnly which is bad for
        // accessibility and can't be changed by the user.
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#endif

        toolBar->addAction(d->previousAction);
        connect(d->previousAction, &QAction::triggered, this, [this] {
            d->setCurrentIndex(d->currentIndex - 1);
        });

        const auto spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        toolBar->addWidget(spacer);

        toolBar->addAction(d->nextAction);
        connect(d->nextAction, &QAction::triggered, this, [this] {
            d->setCurrentIndex(d->currentIndex + 1);
        });
        d->nextAction->setEnabled(true);

        mainLayout->addWidget(toolBar);
    }

    mainLayout->addLayout(layout);

    d->messageViewer = new MimeTreeParser::Widgets::MessageViewer(this);
    d->messageViewer->setMessage(d->messages[0]);
    layout->addWidget(d->messageViewer);

    auto buttonBox = new QDialogButtonBox(this);
    auto closeButton = buttonBox->addButton(QDialogButtonBox::Close);
    connect(closeButton, &QPushButton::pressed, this, &QDialog::accept);
    layout->addWidget(buttonBox);
}

MessageViewerDialog::~MessageViewerDialog() = default;

QVector<KMime::Message::Ptr> MessageViewerDialog::messages() const
{
    return d->messages;
}
