// SPDX-FileCopyrightText: 2016 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <gpgme++/decryptionresult.h>
#include <gpgme++/key.h>

#include "mimetreeparser_core_export.h"
#include <memory>

namespace QGpgME
{
class Protocol;
}

namespace MimeTreeParser
{
class ObjectTreeParser;
}
class PartModelPrivate;

class MIMETREEPARSER_CORE_EXPORT PartModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(bool showHtml READ showHtml WRITE setShowHtml NOTIFY showHtmlChanged)
    Q_PROPERTY(bool containsHtml READ containsHtml NOTIFY containsHtmlChanged)
    Q_PROPERTY(bool trimMail READ trimMail WRITE setTrimMail NOTIFY trimMailChanged)
    Q_PROPERTY(bool isTrimmed READ isTrimmed NOTIFY trimMailChanged)
public:
    PartModel(std::shared_ptr<MimeTreeParser::ObjectTreeParser> parser);
    ~PartModel() override;

    static std::pair<QString, bool> trim(const QString &text);

public:
    enum class Types : quint8 {
        Error,
        Encapsulated,
        Ical,
        Plain,
        None,
        Html,
    };
    Q_ENUM(Types);

    enum Roles {
        TypeRole = Qt::UserRole + 1,
        ContentRole,
        IsEmbeddedRole,
        IsEncryptedRole,
        IsSignedRole,
        IsErrorRole,
        SecurityLevelRole,
        EncryptionSecurityLevelRole,
        SignatureSecurityLevelRole,
        SignatureDetails,
        EncryptionDetails,
        ErrorType,
        ErrorString,
        SenderRole,
        DateRole,
    };

    enum SecurityLevel {
        Unknow,
        Good,
        NotSoGood,
        Bad,
    };
    Q_ENUM(SecurityLevel);

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex &index) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void setShowHtml(bool html);
    [[nodiscard]] bool showHtml() const;
    [[nodiscard]] bool containsHtml() const;

    void setTrimMail(bool trim);
    [[nodiscard]] bool trimMail() const;
    [[nodiscard]] bool isTrimmed() const;

Q_SIGNALS:
    void showHtmlChanged();
    void trimMailChanged();
    void containsHtmlChanged();

private:
    std::unique_ptr<PartModelPrivate> d;
};

class MIMETREEPARSER_CORE_EXPORT SignatureInfo
{
    Q_GADGET
    Q_PROPERTY(QByteArray keyId MEMBER keyId CONSTANT)
    Q_PROPERTY(bool keyMissing MEMBER keyMissing CONSTANT)
    Q_PROPERTY(bool keyRevoked MEMBER keyRevoked CONSTANT)
    Q_PROPERTY(bool keyExpired MEMBER keyExpired CONSTANT)
    Q_PROPERTY(bool sigExpired MEMBER sigExpired CONSTANT)
    Q_PROPERTY(bool crlMissing MEMBER crlMissing CONSTANT)
    Q_PROPERTY(bool crlTooOld MEMBER crlTooOld CONSTANT)

    Q_PROPERTY(QString signer MEMBER signer CONSTANT)
    Q_PROPERTY(QStringList signerMailAddresses MEMBER signerMailAddresses CONSTANT)
    Q_PROPERTY(bool signatureIsGood MEMBER signatureIsGood CONSTANT)
    Q_PROPERTY(bool isCompliant MEMBER isCompliant CONSTANT)

    /// Validity information of the key who signed the message.
    Q_PROPERTY(QString keyTrust MEMBER keyTrust CONSTANT)

public:
    bool keyRevoked = false;
    bool keyExpired = false;
    bool sigExpired = false;
    bool keyMissing = false;
    bool crlMissing = false;
    bool crlTooOld = false;
    bool isCompliant = false;
    QString keyTrust;
    QByteArray keyId;
    const QGpgME::Protocol *cryptoProto = nullptr;
    std::vector<std::pair<GpgME::DecryptionResult::Recipient, GpgME::Key>> decryptRecipients;

    QString signer;
    QStringList signerMailAddresses;
    bool signatureIsGood = false;
};
