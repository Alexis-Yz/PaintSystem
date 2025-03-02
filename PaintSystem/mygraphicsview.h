#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>

enum STATUS{ Edit,
              Draw,
              File
};

class MyGraphicsview : public QGraphicsView
{
    Q_OBJECT
public:
    explicit MyGraphicsview(QWidget *parent = 0);
    void set_cur_status(STATUS s) { status = s; }

private:
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);

signals:
    void mouseMove(QMouseEvent *); //鼠标移动
    void mousePressed(QMouseEvent *); //鼠标单击
    void mouseReleased(QMouseEvent *);
    void mouseDoubleClick(QMouseEvent *); //双击事件
    void keyPress(QKeyEvent *); //按键事件


public slots:

private:
    STATUS status;
};

#endif // MYGRAPHICSVIEW_H
