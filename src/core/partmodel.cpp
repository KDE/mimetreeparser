// SPDX-FileCopyrightText: 2016 Sandro Knauß <knauss@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "partmodel.h"

#include "enums.h"
#include "htmlutils.h"
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
#include <verificationresult.h>

std::optional<GpgME::Signature> signatureFromMessagePart(MimeTreeParser::MessagePart *messagePart)
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

static QString formatValidSignatureWithTrustLevel(const GpgME::UserID &id)
{
    if (id.isNull()) {
        return QString();
    }
    switch (id.validity()) {
    case GpgME::UserID::Marginal:
        return i18n("The signature is valid but the trust in the certificate's validity is only marginal.");
    case GpgME::UserID::Full:
        return i18n("The signature is valid and the certificate's validity is fully trusted.");
    case GpgME::UserID::Ultimate:
        return i18n("The signature is valid and the certificate's validity is ultimately trusted.");
    case GpgME::UserID::Never:
        return i18n("The signature is valid but the certificate's validity is <em>not trusted</em>.");
    case GpgME::UserID::Unknown:
        return i18n("The signature is valid but the certificate's validity is unknown.");
    case GpgME::UserID::Undefined:
    default:
        return i18n("The signature is valid but the certificate's validity is undefined.");
    }
}

static QString renderKeyLink(const QString &fpr, const QString &text)
{
    return QStringLiteral("<a href=\"key:%1\">%2</a>").arg(fpr, text);
}

static QString renderKey(const GpgME::Key &key)
{
    if (key.isNull()) {
        return i18n("Unknown certificate");
    }

    if (key.primaryFingerprint() && strlen(key.primaryFingerprint()) > 16 && key.numUserIDs()) {
        const QString text = QStringLiteral("%1 (%2)")
                                 .arg(Kleo::Formatting::prettyNameAndEMail(key).toHtmlEscaped())
                                 .arg(Kleo::Formatting::prettyID(QString::fromLocal8Bit(key.primaryFingerprint()).right(16).toLatin1().constData()));
        return renderKeyLink(QLatin1StringView(key.primaryFingerprint()), text);
    }

    return renderKeyLink(QLatin1StringView(key.primaryFingerprint()), Kleo::Formatting::prettyID(key.primaryFingerprint()));
}

static QString formatDate(const QDateTime &dt)
{
    return QLocale().toString(dt);
}

static QString formatSigningInformation(const GpgME::Signature &sig, const GpgME::Key &key)
{
    if (sig.isNull()) {
        return QString();
    }
    const QDateTime dt = sig.creationTime() != 0 ? QDateTime::fromSecsSinceEpoch(quint32(sig.creationTime())) : QDateTime();
    QString text;

    if (key.isNull()) {
        return text += i18n("With unavailable certificate:") + QStringLiteral("<br>ID: 0x%1").arg(QString::fromLatin1(sig.fingerprint()).toUpper());
    }

    text += i18n("With certificate: %1", renderKey(key));

    if (Kleo::DeVSCompliance::isCompliant()) {
        text += (QStringLiteral("<br/>")
                 + (sig.isDeVs() ? i18nc("%1 is a placeholder for the name of a compliance mode. E.g. NATO RESTRICTED compliant or VS-NfD compliant",
                                         "The signature is %1",
                                         Kleo::DeVSCompliance::name(true))
                                 : i18nc("%1 is a placeholder for the name of a compliance mode. E.g. NATO RESTRICTED compliant or VS-NfD compliant",
                                         "The signature <b>is not</b> %1.",
                                         Kleo::DeVSCompliance::name(true))));
    }

    return text;
}

static QString signatureSummaryToString(GpgME::Signature::Summary summary)
{
    if (summary & GpgME::Signature::None) {
        return i18n("Error: Signature not verified");
    } else if (summary & GpgME::Signature::Valid || summary & GpgME::Signature::Green) {
        return i18n("Good signature");
    } else if (summary & GpgME::Signature::KeyRevoked) {
        return i18n("Signing certificate was revoked");
    } else if (summary & GpgME::Signature::KeyExpired) {
        return i18n("Signing certificate is expired");
    } else if (summary & GpgME::Signature::KeyMissing) {
        return i18n("Certificate is not available");
    } else if (summary & GpgME::Signature::SigExpired) {
        return i18n("Signature expired");
    } else if (summary & GpgME::Signature::CrlMissing) {
        return i18n("CRL missing");
    } else if (summary & GpgME::Signature::CrlTooOld) {
        return i18n("CRL too old");
    } else if (summary & GpgME::Signature::BadPolicy) {
        return i18n("Bad policy");
    } else if (summary & GpgME::Signature::SysError) {
        return i18n("System error"); // ### retrieve system error details?
    } else if (summary & GpgME::Signature::Red) {
        return i18n("Bad signature");
    }
    return QString();
}

static bool addrspec_equal(const KMime::Types::AddrSpec &lhs, const KMime::Types::AddrSpec &rhs, Qt::CaseSensitivity cs)
{
    return lhs.localPart.compare(rhs.localPart, cs) == 0 && lhs.domain.compare(rhs.domain, Qt::CaseInsensitive) == 0;
}

static std::string stripAngleBrackets(const std::string &str)
{
    if (str.empty()) {
        return str;
    }
    if (str[0] == '<' && str[str.size() - 1] == '>') {
        return str.substr(1, str.size() - 2);
    }
    return str;
}

static std::string email(const GpgME::UserID &uid)
{
    if (uid.parent().protocol() == GpgME::OpenPGP) {
        if (const char *const email = uid.email()) {
            return stripAngleBrackets(email);
        } else {
            return std::string();
        }
    }

    Q_ASSERT(uid.parent().protocol() == GpgME::CMS);

    if (const char *const id = uid.id())
        if (*id == '<') {
            return stripAngleBrackets(id);
        } else {
            return Kleo::DN(id)[QStringLiteral("EMAIL")].trimmed().toUtf8().constData();
        }
    else {
        return std::string();
    }
}

static bool mailbox_equal(const KMime::Types::Mailbox &lhs, const KMime::Types::Mailbox &rhs, Qt::CaseSensitivity cs)
{
    return addrspec_equal(lhs.addrSpec(), rhs.addrSpec(), cs);
}

static KMime::Types::Mailbox mailbox(const GpgME::UserID &uid)
{
    const std::string e = email(uid);
    KMime::Types::Mailbox mbox;
    if (!e.empty()) {
        mbox.setAddress(e.c_str());
    }
    return mbox;
}

static GpgME::UserID findUserIDByMailbox(const GpgME::Key &key, const KMime::Types::Mailbox &mbox)
{
    const auto userIDs{key.userIDs()};
    for (const GpgME::UserID &id : userIDs) {
        if (mailbox_equal(mailbox(id), mbox, Qt::CaseInsensitive)) {
            return id;
        }
    }
    return {};
}

static QString formatSignature(const GpgME::Signature &sig, const GpgME::Key &key, const KMime::Types::Mailbox &sender)
{
    if (sig.isNull()) {
        return QString();
    }

    const QString text = formatSigningInformation(sig, key) + QLatin1StringView("<br/>");

    // Green
    if (sig.summary() & GpgME::Signature::Valid) {
        const GpgME::UserID id = findUserIDByMailbox(key, sender);
        return text + formatValidSignatureWithTrustLevel(!id.isNull() ? id : key.userID(0));
    }

    // Red
    if ((sig.summary() & GpgME::Signature::Red)) {
        const QString ret = text + i18n("The signature is invalid: %1", signatureSummaryToString(sig.summary()));
        if (sig.summary() & GpgME::Signature::SysError) {
            return ret + QStringLiteral(" (%1)").arg(Kleo::Formatting::errorAsString(sig.status()));
        }
        return ret;
    }

    // Key missing
    if ((sig.summary() & GpgME::Signature::KeyMissing)) {
        return text + i18n("You can search the certificate on a keyserver or import it from a file.");
    }

    // Yellow
    if ((sig.validity() & GpgME::Signature::Validity::Undefined) //
        || (sig.validity() & GpgME::Signature::Validity::Unknown) //
        || (sig.summary() == GpgME::Signature::Summary::None)) {
        return text
            + (key.protocol() == GpgME::OpenPGP
                   ? i18n("The used key is not certified by you or any trusted person.")
                   : i18n("The used certificate is not certified by a trustworthy Certificate Authority or the Certificate Authority is unknown."));
    }

    // Catch all fall through
    const QString ret = text + i18n("The signature is invalid: %1", signatureSummaryToString(sig.summary()));
    if (sig.summary() & GpgME::Signature::SysError) {
        return ret + QStringLiteral(" (%1)").arg(Kleo::Formatting::errorAsString(sig.status()));
    }
    return ret;
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
    auto header = content->header<T>();
    if (header || !content->parent()) {
        return header;
    }
    return findHeader<T>(content->parent());
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
        case SignatureSecurityLevelRole: {
            // Color displayed for the signature info box
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
        case SignatureDetailsRole: {
            auto signature = signatureFromMessagePart(messagePart);
            if (!signature) {
                return QString{};
            }

            GpgME::Key key = signature->key();
            if (key.isNull()) {
                key = Kleo::KeyCache::instance()->findByFingerprint(signature->fingerprint());
            }
            if (key.isNull() && signature->fingerprint()) {
                // try to find a subkey that was used for signing;
                // assumes that the key ID is the last 16 characters of the fingerprint
                const auto fpr = std::string_view{signature->fingerprint()};
                const auto keyID = std::string{fpr, fpr.size() - 16, 16};
                const auto subkeys = Kleo::KeyCache::instance()->findSubkeysByKeyID({keyID});
                if (subkeys.size() > 0) {
                    key = subkeys[0].parent();
                }
            }
            if (key.isNull()) {
                qCWarning(MIMETREEPARSER_CORE_LOG) << "Found no key or subkey for fingerprint" << signature->fingerprint();
            }

            QStringList senderUserId;
            for (int i = 0, count = key.userIDs().size(); i < count; i++) {
                senderUserId << QString::fromUtf8(key.userID(i).email());
            }

            // guess sender from mime node or parent node
            auto from = findHeader<KMime::Headers::From>(messagePart->node());
            if (from) {
                const auto mailboxes = from->mailboxes();
                if (!mailboxes.isEmpty()) {
                    auto mailBox = mailboxes.front();
                    if (mailBox.hasAddress()) {
                        return formatSignature(*signature, key, mailboxes.front());
                    }
                }
            }

            // use first non empty userId as fallback
            KMime::Types::Mailbox mailBox;
            for (int i = 0, count = key.userIDs().size(); i < count; i++) {
                const auto address = QString::fromUtf8(key.userID(1).email());
                if (!address.isEmpty()) {
                    mailBox.fromUnicodeString(address);
                    break;
                }
            }

            return formatSignature(*signature, key, mailBox);
        }
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
                            errorMessage +=
                                i18ndc("mimetreeparser", "@info:status", "You cannot decrypt this message.");
                        } else {
                            errorMessage +=
                                i18ndc("mimetreeparser", "@info:status", "You cannot decrypt this message.");
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
