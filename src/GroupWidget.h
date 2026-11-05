#pragma once

#include "WindowInfo.h"

#include <QIcon>
#include <QStringList>
#include <QVector>
#include <QWidget>

/// A single application group: header plus window rows.
class QKeyEvent;

class GroupWidget : public QWidget
{
    Q_OBJECT

public:
    GroupWidget(QString applicationKey, QString title, QIcon icon,
                QVector<WindowInfo> windows, QWidget *parent = nullptr);

    static int heightForRows(int rows);

    QString applicationKey() const;
    QStringList identityOrder() const;

signals:
    void windowActivated(quintptr handle);
    void orderChanged(const QString &applicationKey, const QStringList &identityOrder);
    void selectionChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int rowAt(const QPoint &position) const;
    void rebuildLayout();
    void selectRow(int row);
    void moveRow(int from, int to);

    QString m_applicationKey;
    QString m_title;
    QIcon m_icon;
    QVector<WindowInfo> m_windows;
    QVector<int> m_rowOffsets;

    int m_selectedRow = -1;
    int m_hoverRow = -1;
    int m_pressRow = -1;
    int m_pressY = 0;
    int m_dropRow = -1;
    bool m_dragging = false;
};




