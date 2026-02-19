# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

find_package(KF6KIO ${KF_MIN_VERSION} CONFIG)

ecm_add_qml_module(mimetreeparser_plugin
    URI "org.kde.pim.mimetreeparser"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES QtQuick
)

target_sources(mimetreeparser_plugin PRIVATE
    itinerarykdeconnecthandler.h
    itinerarykdeconnecthandler.cpp
    itinerarymodel.cpp
    itinerarymodel.h
    types.h
)

target_link_libraries(mimetreeparser_plugin PRIVATE
    Qt::Qml
    KPim6::MimeTreeParserCore
    KF6::KIOWidgets
)

ecm_target_qml_sources(mimetreeparser_plugin
    SOURCES
        qml/MailViewer.qml
    RESOURCES
        images/bike.svg
        images/bus.svg
        images/cablecar.svg
        images/car.svg
        images/coach.svg
        images/couchettecar.svg
        images/elevator.svg
        images/escalator.svg
        images/ferry.svg
        images/flight.svg
        images/foodestablishment.svg
        images/funicular.svg
        images/longdistancetrain.svg
        images/rapidtransit.svg
        images/seat.svg
        images/shuttle.svg
        images/sleepingcar.svg
        images/stairs.svg
        images/subway.svg
        images/taxi.svg
        images/train.svg
        images/tramway.svg
        images/transfer.svg
        images/wait.svg
        images/walk.svg
)

configure_file(config-mimetreeparser-quick.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-mimetreeparser-quick.h)

ecm_target_qml_sources(mimetreeparser_plugin PRIVATE
    PATH private
    SOURCES
        qml/private/AttachmentDelegate.qml
        qml/private/Banner.qml
        qml/private/ErrorPart.qml
        qml/private/HtmlPart.qml
        qml/private/ICalPart.qml
        qml/private/MailPart.qml
        qml/private/MailPartModel.qml
        qml/private/MailPartView.qml
        qml/private/TextPart.qml
)

ecm_target_qml_sources(mimetreeparser_plugin PRIVATE
    PATH itinerary
    SOURCES
        qml/itinerary/FlightReservationComponent.qml
        qml/itinerary/FoodReservationComponent.qml
        qml/itinerary/HotelReservationComponent.qml
        qml/itinerary/ItineraryReservationComponent.qml
        qml/itinerary/TrainReservationComponent.qml
        qml/itinerary/JourneySectionStopDelegateLineSegment.qml
        qml/itinerary/ItineraryComponent.qml
        qml/itinerary/TransportIcon.qml
        qml/itinerary/TripMenu.qml
)

ecm_finalize_qml_module(mimetreeparser_plugin DESTINATION ${KDE_INSTALL_QMLDIR})
