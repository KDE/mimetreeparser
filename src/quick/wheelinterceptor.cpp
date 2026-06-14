// SPDX-FileCopyrightText: 2025 Claudio Cambra <claudio.cambra@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "wheelinterceptor.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QMetaObject>
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
}

static void applyScroll(QQuickItem *target, qreal pixelDelta)
{
    const auto viewH = target->height();
    const auto contentH = target->property("contentHeight").toReal();
    const auto currentY = target->property("contentY").toReal();
    const auto maxScrollY = qMax(0.0, contentH - viewH);
    target->setProperty("contentY", qBound(0.0, currentY + pixelDelta, maxScrollY));
}

static void applyHScroll(QQuickItem *target, qreal pixelDelta)
{
    const auto viewW = target->width();
    const auto contentW = target->property("contentWidth").toReal();
    const auto currentX = target->property("contentX").toReal();
    const auto maxScrollX = qMax(0.0, contentW - viewW);
    target->setProperty("contentX", qBound(0.0, currentX + pixelDelta, maxScrollX));
}

bool WheelInterceptor::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    if (event->type() != QEvent::Wheel || !m_source) {
        return false;
    }
    auto wheelEvent = static_cast<QWheelEvent *>(event);
    const auto localPos = m_source->mapFromGlobal(wheelEvent->globalPosition().toPoint());
    if (!m_source->contains(localPos)) {
        return false;
    }

    const auto scrollLines = QGuiApplication::styleHints()->wheelScrollLines();
    const auto factor = scrollLines * 20.0 / 120.0;
    const auto deltaY = -wheelEvent->angleDelta().y() * factor;
    const auto deltaX = -wheelEvent->angleDelta().x() * factor;

    if (m_target) {
        applyScroll(m_target, deltaY);
        applyHScroll(m_target, deltaX);
    }
    if (m_scrollTarget) {
        applyScroll(m_scrollTarget, deltaY);
    }
    return true;
}
