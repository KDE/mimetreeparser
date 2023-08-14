// SPDX-FileCopyrightText: 2017 Volker Krause <vkrause@kde.org>
// SPDX-FileCopyrightText: 2023 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "itineraryprocessor.h"

#include <KItinerary/CreativeWork>
#include <KItinerary/DocumentUtil>
#include <KItinerary/Event>
#include <KItinerary/ExtractorDocumentNode>
#include <KItinerary/ExtractorDocumentNodeFactory>
#include <KItinerary/ExtractorEngine>
#include <KItinerary/JsonLdDocument>
#include <KItinerary/Reservation>

#include <KPkPass/Pass>

#include "itinerarymemento.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>

using namespace MimeTreeParser::Itinerary;
using namespace KItinerary;

namespace
{

template<typename T>
const T *findHeader(KMime::Content *content)
{
    auto header = content->header<T>();
    if (header || !content->parent()) {
        return header;
    }
    return findHeader<T>(content->parent());
}

const KMime::Headers::Base *findHeader(KMime::Content *content, const char *headerType)
{
    const auto header = content->headerByType(headerType);
    if (header || !content->parent()) {
        return header;
    }
    return findHeader(content->parent(), headerType);
}

bool isPkPassContent(KMime::Content *content)
{
    const auto ct = content->contentType();
    const QByteArray mimetype = ct->mimeType();
    if (mimetype == "application/vnd.apple.pkpass") {
        return true;
    }
    if (mimetype != "application/octet-stream" && mimetype != "application/zip") {
        return false;
    }
    if (ct->name().endsWith(QLatin1String("pkpass"))) {
        return true;
    }
    const auto cd = content->contentDisposition(false);
    return cd && cd->filename().endsWith(QLatin1String("pkpass"));
}

bool isCalendarContent(KMime::Content *content)
{
    const auto ct = content->contentType();
    const QByteArray mimetype = ct ? ct->mimeType() : QByteArray();
    if (mimetype == "text/calendar" || mimetype == "application/ics") {
        return true;
    }
    if (mimetype != "text/plain" && mimetype != "application/octet-stream") {
        return false;
    }
    if (ct && ct->name().endsWith(QLatin1String(".ics"))) {
        return true;
    }
    const auto cd = content->contentDisposition(false);
    return cd && cd->filename().endsWith(QLatin1String(".ics"));
}

KMime::Content *findMultipartRelatedParent(KMime::Content *node)
{
    while (node) {
        if (node->contentType()->mimeType() == "multipart/related") {
            return node;
        }
        node = node->parent();
    }
    return nullptr;
}
};

ItineraryMemento *ItineraryProcessor::process(std::shared_ptr<MimeTreeParser::ObjectTreeParser> parser)
{
    auto memento = new ItineraryMemento();
    const auto header = findHeader<KMime::Headers::Date>(parser->message().get());
    if (header) {
        memento->setMessageDate(header->dateTime());
    }

    bool contextIsToplevel = false;
    const auto parts = parser->collectContentParts() + parser->collectAttachmentParts();
    for (const auto &part : parts) {
        std::unique_ptr<KPkPass::Pass> pass;
        bool isPdf = false;

        ExtractorEngine engine;
        engine.setUseSeparateProcess(true);
        engine.setContext(QVariant::fromValue<KMime::Content *>(part->node()), u"message/rfc822");
        if (isPkPassContent(part->node())) {
            pass.reset(KPkPass::Pass::fromData(part->node()->decodedContent()));
            engine.setContent(QVariant::fromValue<KPkPass::Pass *>(pass.get()), u"application/vnd.apple.pkpass");
        } else if (part->node()->contentType()->isHTMLText()) {
            engine.setContent(part->node()->decodedText(), u"text/html");
            // find embedded images that belong to this HTML part, and create child-nodes for those
            // this is needed for finding barcodes in those images
            if (const auto rootNode = findMultipartRelatedParent(part->node())) {
                const auto children = rootNode->contents();
                for (const auto node : children) {
                    if (node->contentID(false) && node->contentType(false) && node->contentType()->mimeType() == "image/png") {
                        auto pngNode = engine.documentNodeFactory()->createNode(node->decodedContent(), {}, u"image/png");
                        engine.rootDocumentNode().appendChild(pngNode);
                    }
                }
            }
        } else if (part->node()->contentType()->mimeType() == "application/pdf"
                   || part->node()->contentType()->name().endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive)) {
            isPdf = true;
            engine.setData(part->node()->decodedContent());
        } else if (isCalendarContent(part->node())) {
            engine.setData(part->node()->decodedContent());
        } else if (part->node()->contentType()->isPlainText()) {
            engine.setContent(part->node()->decodedText(), u"text/plain");
        } else {
            continue;
        }

        const auto data = engine.extract();
        qWarning().noquote() << QJsonDocument(data).toJson();
        auto decodedData = JsonLdDocument::fromJson(data);

        for (auto it = decodedData.begin(); it != decodedData.end(); ++it) {
            if (JsonLd::isA<Event>(*it)) { // promote Event to EventReservation
                EventReservation res;
                res.setReservationFor(*it);
                *it = res;
            }
        }

        if (!decodedData.isEmpty()) {
            if (isPdf) {
                const auto docData = part->node()->decodedContent();
                const auto docId = DocumentUtil::idForContent(docData);
                DigitalDocument docInfo;
                docInfo.setEncodingFormat(QStringLiteral("application/pdf"));
                docInfo.setName(part->filename());
                memento->addDocument(docId, docInfo, docData);

                for (auto &res : decodedData) {
                    DocumentUtil::addDocumentId(res, docId);
                }
            }

            memento->appendData(decodedData);
        }

        if (pass) {
            memento->addPass(pass.get(), part->node()->decodedContent());
        }
    }

    return memento;
}
