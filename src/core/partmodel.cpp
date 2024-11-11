// SPDX-FileCopyrightText: 2016 Sandro Knauß <knauss@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "partmodel.h"

#include "enums.h"
#include "htmlutils.h"
#include "messagepart.h"
#include "mimetreeparser_core_debug.h"
#include "objecttreeparser.h"
#include "utils.h"

#include <KLocalizedString>
#include <Libkleo/Compliance>
#include <Libkleo/Formatting>
#include <Libkleo/KeyCache>

#include <QDebug>
#include <QGpgME/Protocol>
#include <QRegularExpression>
#include <QStringLiteral>
#include <QTextDocument>

#include <gpgme++/verificationresult.h>

static std::optional<GpgME::Signature> signatureFromMessagePart(MimeTreeParser::MessagePart *messagePart)
{
    const auto signatureState = messagePart->signatureState();
    const bool messageIsSigned = signatureState == MimeTreeParser::KMMsgPartiallySigned || signatureState == MimeTreeParser::KMMsgFullySigned;

    if (!messageIsSigned) {
        return std::nullopt;
    }

    const auto signatureParts = messagePart->signatures();
    Q_ASSERT(!signatureParts.isEmpty());
    if (signatureParts.empty()) {
        return std::nullopt;
    }
    const auto signaturePart = signatureParts.front(); // TODO add support for multiple signature

    const auto signatures = signaturePart->partMetaData()->verificationResult.signatures();
    Q_ASSERT(!signatures.empty());
    if (signatures.empty()) {
        return std::nullopt;
    }
    const auto signature = signatures.front(); // TODO add support for multiple signature
    return signature;
}

// We return a pair containing the trimmed string, as well as a boolean indicating whether the string was trimmed or not
std::pair<QString, bool> PartModel::trim(const QString &text)
{
    // The delimiters have <p>.? prefixed including the .? because sometimes we get a byte order mark <feff> (seen with user-agent:
    // Microsoft-MacOutlook/10.1d.0.190908) We match both regulard withspace with \s and non-breaking spaces with \u00A0
    const QList<QRegularExpression> delimiters{
        // English
        QRegularExpression{QStringLiteral("<p>.?-+Original(\\s|\u00A0)Message-+"), QRegularExpression::CaseInsensitiveOption},
        // The remainder is not quoted
        QRegularExpression{QStringLiteral("<p>.?On.*wrote:"), QRegularExpression::CaseInsensitiveOption},
        // The remainder is quoted
        QRegularExpression{QStringLiteral("&gt; On.*wrote:"), QRegularExpression::CaseInsensitiveOption},

        // German
        // Forwarded
        QRegularExpression{QStringLiteral("<p>.?Von:.*</p>"), QRegularExpression::CaseInsensitiveOption},
        // Reply
        QRegularExpression{QStringLiteral("<p>.?Am.*schrieb.*:</p>"), QRegularExpression::CaseInsensitiveOption},
        // Signature
        QRegularExpression{QStringLiteral("<p>.?--(\\s|\u00A0)<br>"), QRegularExpression::CaseInsensitiveOption},
    };

    for (const auto &expression : delimiters) {
        auto i = expression.globalMatchView(text);
        while (i.hasNext()) {
            const auto match = i.next();
            const int startOffset = match.capturedStart(0);
            // This is a very simplistic detection for an inline reply where we would have the patterns before the actual message content.
            // We simply ignore anything we find within the first few lines.
            if (startOffset >= 5) {
                return {text.mid(0, startOffset), true};
            } else {
                // Search for the next delimiter
                continue;
            }
        }
    }

    return {text, false};
}

static QString addCss(const QString &s)
{
    // Get the default font from QApplication
    static const QString fontFamily = QFont{}.family();
    // overflow:hidden ensures no scrollbars are ever shown.
    static const QString css = QStringLiteral("<style>\n")
        + QStringLiteral(
              "body {\n"
              "  overflow:hidden;\n"
              "  font-family: \"%1\" ! important;\n"
              "  color: #31363b ! important;\n"
              "  background-color: #fcfcfc ! important\n"
              "}\n")
              .arg(fontFamily)
        + QStringLiteral("blockquote { \n"
                         "  border-left: 2px solid #bdc3c7 ! important;\n"
                         "}\n")
        + QStringLiteral("</style>");

    const QString header = QLatin1StringView(
                               "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n"
                               "<html><head><title></title>")
        + css + QLatin1StringView("</head>\n<body>\n");
    return header + s + QStringLiteral("</body></html>");
}

class PartModelPrivate
{
public:
    PartModelPrivate(PartModel *q_ptr, const std::shared_ptr<MimeTreeParser::ObjectTreeParser> &parser)
        : q(q_ptr)
        , mParser(parser)
    {
        collectContents();
    }

    ~PartModelPrivate() = default;

    void checkPart(const MimeTreeParser::MessagePart::Ptr part)
    {
        mMimeTypeCache[part.data()] = part->mimeType();
        // Extract the content of the part and
        mContents.insert(part.data(), extractContent(part.data()));
    }

    // Recursively find encapsulated messages
    void findEncapsulated(const MimeTreeParser::EncapsulatedRfc822MessagePart::Ptr &e)
    {
        mEncapsulatedParts[e.data()] = mParser->collectContentParts(e);
        for (const auto &subPart : std::as_const(mEncapsulatedParts[e.data()])) {
            checkPart(subPart);
            mParents[subPart.data()] = e.data();
            if (auto encapsulatedSub = subPart.dynamicCast<MimeTreeParser::EncapsulatedRfc822MessagePart>()) {
                findEncapsulated(encapsulatedSub);
            }
        }
    }

    QVariant extractContent(MimeTreeParser::MessagePart *messagePart)
    {
        if (auto alternativePart = dynamic_cast<MimeTreeParser::AlternativeMessagePart *>(messagePart)) {
            if (alternativePart->availableModes().contains(MimeTreeParser::AlternativeMessagePart::MultipartIcal)) {
                return alternativePart->icalContent();
            }
        }

        auto preprocessPlaintext = [&](const QString &text) {
            // Reduce consecutive new lines to never exceed 2
            auto cleaned = text;
            cleaned.replace(QRegularExpression(QStringLiteral("[\n\r]{2,}")), QStringLiteral("\n\n"));

            // We always do rich text (so we get highlighted links and stuff).
            const auto html = Qt::convertFromPlainText(cleaned, Qt::WhiteSpaceNormal);
            if (trimMail) {
                const auto result = PartModel::trim(html);
                isTrimmed = result.second;
                Q_EMIT q->trimMailChanged();
                return MimeTreeParser::linkify(result.first);
            }
            return MimeTreeParser::linkify(html);
        };

        if (messagePart->isHtml()) {
            if (dynamic_cast<MimeTreeParser::AlternativeMessagePart *>(messagePart)) {
                containsHtmlAndPlain = true;
                Q_EMIT q->containsHtmlChanged();
                if (!showHtml) {
                    return preprocessPlaintext(messagePart->plaintextContent());
                }
            }
            return addCss(mParser->resolveCidLinks(messagePart->htmlContent()));
        }

        if (auto attachmentPart = dynamic_cast<MimeTreeParser::AttachmentMessagePart *>(messagePart)) {
            auto node = attachmentPart->node();
            if (node && mMimeTypeCache[attachmentPart] == QByteArrayLiteral("text/calendar")) {
                return messagePart->text();
            }
        }

        return preprocessPlaintext(messagePart->text());
    }

    QVariant contentForPart(MimeTreeParser::MessagePart *messagePart) const
    {
        return mContents.value(messagePart);
    }

    void collectContents()
    {
        mEncapsulatedParts.clear();
        mParents.clear();
        mContents.clear();
        containsHtmlAndPlain = false;
        isTrimmed = false;

        const auto parts = mParser->collectContentParts();
        MimeTreeParser::MessagePart::List filteredParts;

        for (const auto &part : parts) {
            if (part->node()) {
                const auto contentType = part->node()->contentType();
                if (contentType && contentType->hasParameter("protected-headers")) {
                    const auto contentDisposition = part->node()->contentDisposition();
                    if (contentDisposition && contentDisposition->disposition() == KMime::Headers::CDinline) {
                        continue;
                    }
                }
            }

            filteredParts << part;
        }

        for (const auto &part : std::as_const(filteredParts)) {
            checkPart(part);
            if (auto encapsulatedPart = part.dynamicCast<MimeTreeParser::EncapsulatedRfc822MessagePart>()) {
                findEncapsulated(encapsulatedPart);
            }
        }

        for (const auto &part : std::as_const(filteredParts)) {
            if (mMimeTypeCache[part.data()] == QByteArrayLiteral("text/calendar")) {
                mParts.prepend(part);
            } else {
                mParts.append(part);
            }
        }
    }

    PartModel *q;
    MimeTreeParser::MessagePart::List mParts;
    QHash<MimeTreeParser::MessagePart *, QByteArray> mMimeTypeCache;
    QHash<MimeTreeParser::MessagePart *, MimeTreeParser::MessagePart::List> mEncapsulatedParts;
    QHash<MimeTreeParser::MessagePart *, MimeTreeParser::MessagePart *> mParents;
    QMap<MimeTreeParser::MessagePart *, QVariant> mContents;
    std::shared_ptr<MimeTreeParser::ObjectTreeParser> mParser;
    bool showHtml{false};
    bool containsHtmlAndPlain{false};
    bool trimMail{false};
    bool isTrimmed{false};
};

PartModel::PartModel(std::shared_ptr<MimeTreeParser::ObjectTreeParser> parser)
    : d(std::unique_ptr<PartModelPrivate>(new PartModelPrivate(this, parser)))
{
}

PartModel::~PartModel() = default;

void PartModel::setShowHtml(bool html)
{
    if (d->showHtml != html) {
        beginResetModel();
        d->showHtml = html;
        d->collectContents();
        endResetModel();
        Q_EMIT showHtmlChanged();
    }
}

bool PartModel::showHtml() const
{
    return d->showHtml;
}

void PartModel::setTrimMail(bool trim)
{
    if (d->trimMail != trim) {
        beginResetModel();
        d->trimMail = trim;
        d->collectContents();
        endResetModel();
        Q_EMIT trimMailChanged();
    }
}

bool PartModel::trimMail() const
{
    return d->trimMail;
}

bool PartModel::isTrimmed() const
{
    return d->isTrimmed;
}

bool PartModel::containsHtml() const
{
    return d->containsHtmlAndPlain;
}

QHash<int, QByteArray> PartModel::roleNames() const
{
    return {
        {TypeRole, QByteArrayLiteral("type")},
        {ContentRole, QByteArrayLiteral("content")},
        {IsEmbeddedRole, QByteArrayLiteral("isEmbedded")},
        {SidebarSecurityLevelRole, QByteArrayLiteral("sidebarSecurityLevel")},
        {EncryptionSecurityLevelRole, QByteArrayLiteral("encryptionSecurityLevel")},
        {SignatureSecurityLevelRole, QByteArrayLiteral("signatureSecurityLevel")},
        {EncryptionSecurityLevelRole, QByteArrayLiteral("encryptionSecurityLevel")},
        {ErrorType, QByteArrayLiteral("errorType")},
        {ErrorString, QByteArrayLiteral("errorString")},
        {IsErrorRole, QByteArrayLiteral("error")},
        {SenderRole, QByteArrayLiteral("sender")},
        {SignatureDetailsRole, QByteArrayLiteral("signatureDetails")},
        {SignatureIconNameRole, QByteArrayLiteral("signatureIconName")},
        {EncryptionDetails, QByteArrayLiteral("encryptionDetails")},
        {EncryptionIconNameRole, QByteArrayLiteral("encryptionIconName")},
        {DateRole, QByteArrayLiteral("date")},
    };
}

QModelIndex PartModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column != 0) {
        return QModelIndex();
    }
    if (parent.isValid()) {
        const auto part = static_cast<MimeTreeParser::MessagePart *>(parent.internalPointer());
        auto encapsulatedPart = dynamic_cast<MimeTreeParser::EncapsulatedRfc822MessagePart *>(part);

        if (encapsulatedPart) {
            const auto parts = d->mEncapsulatedParts[encapsulatedPart];
            if (row < parts.size()) {
                return createIndex(row, column, parts.at(row).data());
            }
        }
        return QModelIndex();
    }
    if (row < d->mParts.size()) {
        return createIndex(row, column, d->mParts.at(row).data());
    }
    return QModelIndex();
}

SignatureInfo encryptionInfo(MimeTreeParser::MessagePart *messagePart)
{
    SignatureInfo signatureInfo;
    const auto encryptions = messagePart->encryptions();
    if (encryptions.size() > 1) {
        qWarning() << "Can't deal with more than one encryption";
    }
    for (const auto &encryptionPart : encryptions) {
        signatureInfo.keyId = encryptionPart->partMetaData()->keyId;
        signatureInfo.cryptoProto = encryptionPart->cryptoProto();
        signatureInfo.decryptRecipients = encryptionPart->decryptRecipients();
    }
    return signatureInfo;
};
template<typename T>
const T *findHeader(KMime::Content *content)
{
    if (!content) {
        return {};
    }
    auto header = content->header<T>();
    if (header || !content->parent()) {
        return header;
    }
    return findHeader<T>(content->parent());
}

PartModel::SecurityLevel PartModel::signatureSecurityLevel(MimeTreeParser::MessagePart *messagePart)
{
    auto signature = signatureFromMessagePart(messagePart);
    if (!signature) {
        return SecurityLevel::Unknow;
    }

    const auto summary = signature->summary();

    if (summary & GpgME::Signature::Summary::Red) {
        return SecurityLevel::Bad;
    }
    if (summary & GpgME::Signature::Summary::Valid) {
        return SecurityLevel::Good;
    }

    return SecurityLevel::NotSoGood;
}

QString PartModel::signatureDetails(MimeTreeParser::MessagePart *messagePart)
{
    auto signature = signatureFromMessagePart(messagePart);
    if (!signature) {
        return QString{};
    }

    // guess sender from mime node or parent node
    auto from = findHeader<KMime::Headers::From>(messagePart->node());
    if (from) {
        const auto mailboxes = from->mailboxes();
        if (!mailboxes.isEmpty()) {
            auto mailBox = mailboxes.front();
            if (mailBox.hasAddress()) {
                return Kleo::Formatting::prettySignature(*signature, QString::fromUtf8(mailboxes.front().address()));
            }
        }
    }
    return Kleo::Formatting::prettySignature(*signature, {});
}

QVariant PartModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.internalPointer()) {
        const auto messagePart = static_cast<MimeTreeParser::MessagePart *>(index.internalPointer());
        // qWarning() << "Found message part " << messagePart->metaObject()->className() << messagePart->partMetaData()->status << messagePart->error();
        Q_ASSERT(messagePart);
        switch (role) {
        case Qt::DisplayRole:
            return QStringLiteral("Content%1");
        case SenderRole: {
            if (auto e = dynamic_cast<MimeTreeParser::EncapsulatedRfc822MessagePart *>(messagePart)) {
                return e->from();
            }
            return {};
        }
        case DateRole: {
            if (auto e = dynamic_cast<MimeTreeParser::EncapsulatedRfc822MessagePart *>(messagePart)) {
                return e->date();
            }
            return {};
        }
        case TypeRole: {
            if (messagePart->error()) {
                return QVariant::fromValue(Types::Error);
            }
            if (dynamic_cast<MimeTreeParser::EncapsulatedRfc822MessagePart *>(messagePart)) {
                return QVariant::fromValue(Types::Encapsulated);
            }
            if (auto alternativePart = dynamic_cast<MimeTreeParser::AlternativeMessagePart *>(messagePart)) {
                if (alternativePart->availableModes().contains(MimeTreeParser::AlternativeMessagePart::MultipartIcal)) {
                    return QVariant::fromValue(Types::Ical);
                }
            }
            if (auto attachmentPart = dynamic_cast<MimeTreeParser::AttachmentMessagePart *>(messagePart)) {
                auto node = attachmentPart->node();
                if (!node) {
                    qWarning() << "no content for attachment";
                    return {};
                }
                if (d->mMimeTypeCache[attachmentPart] == QByteArrayLiteral("text/calendar")) {
                    return QVariant::fromValue(Types::Ical);
                }
            }
            if (!d->showHtml && d->containsHtmlAndPlain) {
                return QVariant::fromValue(Types::Plain);
            }
            // For simple html we don't need a browser
            auto complexHtml = [&] {
                if (messagePart->isHtml()) {
                    const auto text = messagePart->htmlContent();
                    if (text.contains(QStringLiteral("<!DOCTYPE html PUBLIC"))) {
                        // We can probably deal with this if it adheres to the strict dtd
                        //(that's what our composer produces as well)
                        if (!text.contains(QStringLiteral("http://www.w3.org/TR/REC-html40/strict.dtd"))) {
                            return true;
                        }
                    }
                    // Blockquotes don't support any styling which would be necessary so they become readable.
                    if (text.contains(QStringLiteral("blockquote"))) {
                        return true;
                    }
                    // Media queries are too advanced
                    if (text.contains(QStringLiteral("@media"))) {
                        return true;
                    }
                    // auto css properties are not supported e.g margin-left: auto;
                    if (text.contains(QStringLiteral(": auto;"))) {
                        return true;
                    }
                    return false;
                } else {
                    return false;
                }
            }();
            if (complexHtml) {
                return QVariant::fromValue(Types::Html);
            }
            return QVariant::fromValue(Types::Plain);
        }
        case IsEmbeddedRole:
            return false;
        case IsErrorRole:
            return messagePart->error();
        case ContentRole:
            return d->contentForPart(messagePart);
        case SidebarSecurityLevelRole: {
            const auto signature = index.data(SignatureSecurityLevelRole).value<SecurityLevel>();
            const auto encryption = index.data(EncryptionSecurityLevelRole).value<SecurityLevel>();

            if (signature == SecurityLevel::Bad || encryption == SecurityLevel::Bad) {
                return SecurityLevel::Bad;
            }

            if (signature == SecurityLevel::NotSoGood || encryption == SecurityLevel::NotSoGood) {
                return SecurityLevel::NotSoGood;
            }

            if (signature == SecurityLevel::Good || encryption == SecurityLevel::Good) {
                return SecurityLevel::Good;
            }

            return SecurityLevel::Unknow;
        }
        case SignatureSecurityLevelRole:
            // Color displayed for the signature info box
            return signatureSecurityLevel(messagePart);
        case EncryptionSecurityLevelRole: {
            // Color displayed for the encryption info box
            const auto encryption = messagePart->encryptionState();
            const bool messageIsEncrypted = encryption == MimeTreeParser::KMMsgPartiallyEncrypted || encryption == MimeTreeParser::KMMsgFullyEncrypted;

            if (messagePart->error()) {
                return SecurityLevel::Bad;
            }

            return messageIsEncrypted ? SecurityLevel::Good : SecurityLevel::Unknow;
        }
        case EncryptionIconNameRole: {
            const auto encryption = messagePart->encryptionState();
            const bool messageIsEncrypted = encryption == MimeTreeParser::KMMsgPartiallyEncrypted || encryption == MimeTreeParser::KMMsgFullyEncrypted;

            if (messagePart->error()) {
                return QStringLiteral("data-error");
            }

            return messageIsEncrypted ? QStringLiteral("mail-encrypted") : QString();
        }
        case SignatureIconNameRole: {
            auto signature = signatureFromMessagePart(messagePart);
            if (!signature) {
                return QString{};
            }

            const auto summary = signature->summary();
            if (summary & GpgME::Signature::Valid) {
                return QStringLiteral("mail-signed");
            } else if (summary & GpgME::Signature::Red) {
                return QStringLiteral("data-error");
            } else {
                return QStringLiteral("data-warning");
            }
        }
        case SignatureDetailsRole:
            return signatureDetails(messagePart);
        case EncryptionDetails:
            return QVariant::fromValue(encryptionInfo(messagePart));
        case ErrorType:
            return messagePart->error();
        case ErrorString: {
            switch (messagePart->error()) {
            case MimeTreeParser::MessagePart::NoKeyError: {
                if (auto encryptedMessagePart = dynamic_cast<MimeTreeParser::EncryptedMessagePart *>(messagePart)) {
                    if (encryptedMessagePart->isNoSecKey()) {
                        QString errorMessage;
                        if (encryptedMessagePart->cryptoProto() == QGpgME::smime()) {
                            errorMessage += i18ndc("mimetreeparser", "@info:status", "You cannot decrypt this message.");
                        } else {
                            errorMessage += i18ndc("mimetreeparser", "@info:status", "You cannot decrypt this message.");
                        }
                        if (!encryptedMessagePart->decryptRecipients().empty()) {
                            errorMessage += QLatin1Char(' ')
                                + i18ndcp("mimetreeparser",
                                          "@info:status",
                                          "The message is encrypted for the following recipient:",
                                          "The message is encrypted for the following recipients:",
                                          encryptedMessagePart->decryptRecipients().size());
                            errorMessage +=
                                MimeTreeParser::decryptRecipientsToHtml(encryptedMessagePart->decryptRecipients(), encryptedMessagePart->cryptoProto());
                        }
                        return errorMessage;
                    }
                }
            }

                return messagePart->errorString();

            case MimeTreeParser::MessagePart::PassphraseError:
                return i18ndc("mimetreeparser", "@info:status", "Wrong passphrase.");
            case MimeTreeParser::MessagePart::UnknownError:
                break;
            default:
                break;
            }
            return messagePart->errorString();
        }
        }
    }
    return QVariant();
}

QModelIndex PartModel::parent(const QModelIndex &index) const
{
    if (index.isValid()) {
        if (auto indexPart = static_cast<MimeTreeParser::MessagePart *>(index.internalPointer())) {
            for (const auto &part : std::as_const(d->mParts)) {
                if (part.data() == indexPart) {
                    return QModelIndex();
                }
            }
            const auto parentPart = d->mParents[indexPart];
            Q_ASSERT(parentPart);
            int row = 0;
            const auto parts = d->mEncapsulatedParts[parentPart];
            for (const auto &part : parts) {
                if (part.data() == indexPart) {
                    break;
                }
                row++;
            }
            return createIndex(row, 0, parentPart);
        }
        return {};
    }
    return {};
}

int PartModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        const auto part = static_cast<MimeTreeParser::MessagePart *>(parent.internalPointer());
        auto encapsulatedPart = dynamic_cast<MimeTreeParser::EncapsulatedRfc822MessagePart *>(part);

        if (encapsulatedPart) {
            const auto parts = d->mEncapsulatedParts[encapsulatedPart];
            return parts.size();
        }
        return 0;
    }
    return d->mParts.count();
}

int PartModel::columnCount(const QModelIndex &) const
{
    return 1;
}

#include "moc_partmodel.cpp"
