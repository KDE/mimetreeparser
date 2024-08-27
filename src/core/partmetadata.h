// SPDX-FileCopyrightText: 2002-2003 Karl-Heinz Zimmer <khz@kde.org>
// SPDX-FileCopyrightText: 2003 Marc Mutz <mutz@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDateTime>
#include <QStringList>
#include <gpgme++/context.h>
#include <gpgme++/verificationresult.h>

namespace MimeTreeParser
{

class PartMetaData
{
public:
    PartMetaData()
        : isEncrypted(false)
        , isDecryptable(false)
        , inProgress(false)
        , technicalProblem(false)
        , isEncapsulatedRfc822Message(false)
        , isCompliant(false)
        , keyRevoked(false)
    {
    }

    inline bool isSigned() const
    {
        return verificationResult.signatures().size() > 0;
    }

    GpgME::VerificationResult verificationResult;

    QString signClass;
    QString signer;
    QStringList signerMailAddresses;
    QByteArray keyId;
    QString status; // to be used for unknown plug-ins
    int status_code = 0; // = GPGME_SIG_STAT_NONE; to be used for i18n of OpenPGP and S/MIME CryptPlugs
    QString errorText;
    QString decryptionError;
    QString auditLog;
    QString compliance; // textual representation of compliance status; empty if compliance isn't enforced
    GpgME::Error auditLogError;
    bool isEncrypted : 1;
    bool isDecryptable : 1;
    bool inProgress : 1;
    bool technicalProblem : 1;
    bool isEncapsulatedRfc822Message : 1;
    bool isCompliant : 1; // corresponds to the isDeVS flag of signature or decryption result
    bool keyRevoked : 1;
};

}
