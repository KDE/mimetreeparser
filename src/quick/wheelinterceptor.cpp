// SPDX-FileCopyrightText: 2025 Claudio Cambra <claudio.cambra@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "wheelinterceptor.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QStyleHints>
#include <QWheelEvent>

WheelInterceptor::WheelInterceptor(QObject *parent)
    : QObject(parent)
{
}

QQuickItem *WheelInterceptor::source() const
{
    return m_source;
}

void WheelInterceptor::setSource(QQuickItem *source)
{
    if (m_source == source) {
        return;
    }
    if (m_source) {
        QCoreApplication::instance()->removeEventFilter(this);
    }
    m_source = source;
    if (m_source) {
        QCoreApplication::instance()->installEventFilter(this);
    }
    Q_EMIT sourceChanged();
}

QQuickItem *WheelInterceptor::target() const
{
    return m_target;
}

void WheelInterceptor::setTarget(QQuickItem *target)
{
    if (m_target == target) {
        return;
    }
    m_target = target;
    Q_EMIT targetChanged();
}

static QQuickItem *findTopmostFlickable(QQuickItem *item)
{
    QQuickItem *topmost = nullptr;
    auto current = item;
    while (current) {
        if (current->inherits("QQuickFlickable")) {
            topmost = current;
        }
        current = current->parentItem();
    }
    return topmost;
}

bool WheelInterceptor::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    if (event->type() == QEvent::Wheel && m_source && m_target) {
        auto wheelEvent = static_cast<QWheelEvent *>(event);
        const auto localPos = m_source->mapFromGlobal(wheelEvent->globalPosition().toPoint());
        if (m_source->contains(localPos)) {
            auto flickable = findTopmostFlickable(m_source);
            if (!flickable) {
                flickable = findTopmostFlickable(m_target);
            }
            if (flickable) {
                const auto viewH = flickable->height();
                const auto contentH = flickable->property("contentHeight").toReal();
                const auto currentY = flickable->property("contentY").toReal();
                const auto maxScroll = qMax(0.0, contentH - viewH);
                const auto delta = wheelEvent->angleDelta().y();
                const auto scrollLines = QGuiApplication::styleHints()->wheelScrollLines();
                const auto pixels = -(delta / 120.0) * scrollLines * 20.0;
                flickable->setProperty("contentY", qBound(0.0, currentY + pixels, maxScroll));
                return true;
            }
        }
    }
    return false;
}
