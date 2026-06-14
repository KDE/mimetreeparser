// SPDX-FileCopyrightText: 2025 Claudio Cambra <claudio.cambra@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QObject>
#include <QQuickItem>

class WheelInterceptor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickItem *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)

public:
    explicit WheelInterceptor(QObject *parent = nullptr);

    QQuickItem *source() const;
    void setSource(QQuickItem *source);

    QQuickItem *target() const;
    void setTarget(QQuickItem *target);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

Q_SIGNALS:
    void sourceChanged();
    void targetChanged();

private:
    void updateScrollTarget();
    QQuickItem *m_source = nullptr;
    QQuickItem *m_target = nullptr;
    QQuickItem *m_scrollTarget = nullptr;
};
