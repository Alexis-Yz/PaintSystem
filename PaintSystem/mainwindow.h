#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QGraphicsScene>
#include <QRubberBand>
#include <QGraphicsItemGroup>
#include "translatedialog.h"
#include "helpdialog.h"

namespace Ui {
class MainWindow;
}

enum Status {
    Select,
    Fill,
    Translate,
    Delete,
    DrawLine,
    DrawRect,
    DrawEllipse,
    DrawPolygon,
    Files // 文件操作
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool save(QString &FileName);
    bool open(QString &FileName);

public slots:
    void on_fillButton_clicked();

    // 鼠标槽函数
    void mousePressed_slot(QMouseEvent *);
    void mouseMove_slot(QMouseEvent *);
    void mouseReleased_slot(QMouseEvent *);
    void mouseDoubleClick_slot(QMouseEvent *);


    // 保存，新建，打开
    void actNew_triggered_slot();
    void actSave_triggered_slot();
    void actOpen_triggered_slot();

    // 关闭
    void closeEvent(QCloseEvent *);

private slots:
    void on_actionHelp_triggered(); // 帮助

private:
    const int areaORlen_key = 1;  // 图形项长度/面积关键字
    const int shape_key = 2;  // 图形项类型关键字

    int group_id = 0;
    const int group_id_key = 3;  // 组合id关键字

    bool isSaved; // 是否保存


private:
    Ui::MainWindow *ui;
    Status cur_status;  // 当前状态
    QLabel shapeinfo_label; // 图形信息标签
    QLabel coordinate_label;  // 坐标标签
    QGraphicsScene  *scene;  // 场景
    QPointF start_point; // 绘制图形时的起点
    QPointF end_point;
    QVector<QPointF> points; // 绘制多边形时使用
    TranslateDialog *tDialog; // 平移对话框
    HelpDialog *hDialog;    // 帮助对话框

//    bool multiSelectMode;
//    QPointF multiSelectStartPoint;
//    QRectF multiSelectRect;
    QPoint rubberBandStart; // 用于存储橡皮筋矩形框的起点位置
    QRubberBand *rubberBand;
    QColor fill_color;
};

#endif // MAINWINDOW_H
