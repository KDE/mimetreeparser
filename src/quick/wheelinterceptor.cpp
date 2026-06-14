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
    updateScrollTarget();
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

void WheelInterceptor::updateScrollTarget()
{
    m_scrollTarget = nullptr;
    if (!m_source) {
        return;
    }

    auto current = m_source;
    while (current) {
        if (current->inherits("QQuickFlickable")) {
            m_scrollTarget = current;
        }
        current = current->parentItem();
    }

    if (!m_scrollTarget && m_target) {
        current = m_target;
        while (current) {
            if (current->inherits("QQuickFlickable")) {
                m_scrollTarget = current;
            }
            current = current->parentItem();
        }
    }
}

bool WheelInterceptor::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    if (event->type() == QEvent::Wheel && m_source && m_scrollTarget) {
        auto wheelEvent = static_cast<QWheelEvent *>(event);
        const auto localPos = m_source->mapFromGlobal(wheelEvent->globalPosition().toPoint());
        if (m_source->contains(localPos)) {
            const auto viewH = m_scrollTarget->height();
            const auto contentH = m_scrollTarget->property("contentHeight").toReal();
            const auto currentY = m_scrollTarget->property("contentY").toReal();
            const auto maxScroll = qMax(0.0, contentH - viewH);
            const auto delta = wheelEvent->angleDelta().y();
            const auto scrollLines = QGuiApplication::styleHints()->wheelScrollLines();
            const auto pixels = -(delta / 120.0) * scrollLines * 20.0;
            m_scrollTarget->setProperty("contentY", qBound(0.0, currentY + pixels, maxScroll));
            return true;
        }
    }
    return false;
}
