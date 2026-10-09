// SPDX-FileCopyrightText: 2016 Sandro Knauß <knauss@kolabsys.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mimetreeparser_core_export.h"

namespace MimeTreeParser::Core
{

enum class RecurseMode {
    NoRecurse,
    WithinEncapsulatedMessage,
    FullRecursion,
};
}
