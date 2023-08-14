// SPDX-FileCopyrightText: 2017 Volker Krause <vkrause@kde.org>
// SPDX-FileCopyrightText: 2023 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "objecttreeparser.h"

namespace MimeTreeParser
{
namespace Itinerary
{

class ItineraryMemento;

class ItineraryProcessor
{
public:
    ItineraryMemento *process(std::shared_ptr<MimeTreeParser::ObjectTreeParser> parser);
};

} // end namespace Itinerary
} // end namespace MimeTreeParser
