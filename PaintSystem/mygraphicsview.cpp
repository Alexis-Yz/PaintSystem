#include "mygraphicsview.h"
#include <QMouseEvent>
#include <QPoint>
#include <QDebug>

MyGraphicsview::MyGraphicsview(QWidget *parent) :
    QGraphicsView(parent)
{
    status = File;
}

void MyGraphicsview::mousePressEvent(QMouseEvent *e) {
    emit mousePressed(e);
    if (status == Edit) { // 事件继续传递该父类QGraphicsView
        QGraphicsView::mousePressEvent(e);
    }
}

void MyGraphicsview::mouseMoveEvent(QMouseEvent *e) {
    emit mouseMove(e);
    if (status == Edit) {
        QGraphicsView::mouseMoveEvent(e);
    }
}

void MyGraphicsview::mouseReleaseEvent(QMouseEvent *e) {
    emit mouseReleased(e);
    if (status == Edit) {
        QGraphicsView::mouseReleaseEvent(e);
    }
}

void MyGraphicsview::mouseDoubleClickEvent(QMouseEvent *e) {
    emit mouseDoubleClick(e);
    QGraphicsView::mouseDoubleClickEvent(e);
}
