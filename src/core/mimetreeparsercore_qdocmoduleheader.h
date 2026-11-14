/*
  SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// Module header for qdoc PCH
#include "attachmentmodel.h"
#include "bodypartformatter.h"
#include "bodypartformatterbasefactory.h"
#include "bodypartformatterbasefactory_p.h"
#include "cryptohelper.h"
#include "enums.h"
#include "errors.h"
#include "fileopener.h"
#include "htmlutils.h"
#include "messageparser.h"
#include "messagepart.h"
#include "objecttreeparser.h"
#include "partmetadata.h"
#include "partmodel.h"
#include "urlhandler.h"
#include "utils.h"
