#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStatusBar>
#include <QLabel>
#include <QPoint>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QDebug>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsItemGroup>
#include <QLine>
#include <QGraphicsView>
#include <QColor>
#include <QColorDialog>
#include <QMessageBox>
#include <QVector>
#include <QPointF>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QDataStream>
#include <QList>
#include <QMap>
#include <QToolBar>
#include <QtMath>
#include "mygraphicsview.h"

#define PI 3.1415926


template<class T>
void setBrushColor(T *item)
{//函数模板
    QColor color = item->brush().color();
    color = QColorDialog::getColor(color,nullptr,"选择填充颜色");
    if (color.isValid())//确保选择了有效地颜色，即是否点击确定
        item->setBrush(QBrush(color));
}

//计算任意多边形的面积，顶点按照顺时针或者逆时针方向排列
double ComputePolygonArea(const QVector<QPointF> &points)//用QVector储存多边形顶点坐标
{
    int point_num = points.size();
    if (point_num < 3) return 0.0;//顶点个数小于三直接返回面积为0
    double s = points[0].y() * (points[point_num-1].x() - points[1].x());
    for(int i = 1; i < point_num; ++i)
        s += points[i].y() * (points[i-1].x() - points[(i+1)%point_num].x());
    return fabs(s/2.0);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    hDialog(nullptr),
    rubberBand(nullptr)
{
    ui->setupUi(this);
    // 设置初始状态
    cur_status = Files;
    isSaved = false;
    // 设置状态栏
    QStatusBar *sBar = statusBar();
    coordinate_label.setParent(this);//鼠标坐标
    coordinate_label.setMinimumWidth(150);//将标签最小宽度设置为不同像素
    shapeinfo_label.setParent(this);//图形信息
    shapeinfo_label.setMinimumWidth(200);
    sBar->addPermanentWidget(&coordinate_label);//设置为永久部件，提供辅助
    sBar->addPermanentWidget(&shapeinfo_label);

    // 创建场景
    scene = new QGraphicsScene(-300,-200,600,200);
    ui->graphicsView->setScene(scene);

    // 设置鼠标样式，启用鼠标追踪
    ui->graphicsView->setMouseTracking(true);

    // 信号与槽
    connect(ui->graphicsView, &MyGraphicsview::mouseMove, this, &MainWindow::mouseMove_slot);
    connect(ui->graphicsView, &MyGraphicsview::mousePressed, this, &MainWindow::mousePressed_slot);
    connect(ui->graphicsView, &MyGraphicsview::mouseReleased, this, &MainWindow::mouseReleased_slot);
    connect(ui->graphicsView, &MyGraphicsview::mouseDoubleClick, this, &MainWindow::mouseDoubleClick_slot);
    // 画直线
    connect(ui->actionLine, &QAction::triggered,[=](){
        cur_status = DrawLine;//设定状态
        shapeinfo_label.setText("");//清空之前绘制时的图形文字信息
        start_point = end_point = QPointF();
    });
    connect(ui->actionLine, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(Draw);
    });
    // 画长方形
    connect(ui->actionRectangle, &QAction::triggered,[=](){
        cur_status = DrawRect;
        shapeinfo_label.setText("");
        start_point = end_point = QPointF();
    });
    connect(ui->actionRectangle, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(Draw);
    });
    // 画椭圆
    connect(ui->actionEllipse, &QAction::triggered,[=](){
        cur_status = DrawEllipse;
        shapeinfo_label.setText("");
        start_point = end_point = QPointF();
    });
    connect(ui->actionEllipse, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(Draw);
    });
    // 画多边形
    connect(ui->actionPolygon, &QAction::triggered,[=](){
        cur_status = DrawPolygon;
        shapeinfo_label.setText("");
        start_point = end_point = QPointF();
        points.clear();
    });
    connect(ui->actionPolygon, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(Draw);
    });
    // 选择
    connect(ui->actionSelect, &QAction::triggered,[=](){
        cur_status = Select;
    });
    connect(ui->actionSelect, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(Edit);
    });
    // 填充
    connect(ui->actionFill, &QAction::triggered,[=](){
        cur_status = Fill;
        if(!scene->selectedItems().isEmpty())
            on_fillButton_clicked(); // 在选择颜色后直接进行填充操作
    });
    connect(ui->actionFill, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(Edit);
    });
    // 平移
    connect(ui->actionTranslate, &QAction::triggered,[=](){
        cur_status = Translate;
    });
    connect(ui->actionTranslate, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(Edit);
    });
    // 删除
    connect(ui->actionDelete, &QAction::triggered,[=](){
        cur_status = Delete;
    });
    connect(ui->actionDelete, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(Edit);
    });
    // 新建
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::actNew_triggered_slot);
    connect(ui->actionNew, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(File);
    });
    // 保存
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::actSave_triggered_slot);
    connect(ui->actionSave, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(File);
    });
    // 打开
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::actOpen_triggered_slot);
    connect(ui->actionOpen, &QAction::triggered,ui->graphicsView,[=](){
        ui->graphicsView->set_cur_status(File);
    });
}

//xigou
MainWindow::~MainWindow()
{
    delete scene;
    delete ui;
}


void MainWindow::mousePressed_slot(QMouseEvent *e) {\


    // 鼠标左击画直线、椭圆、长方形
    if (e->button() == Qt::LeftButton) {

        if (cur_status == Select) {
            // 记录鼠标点击位置作为橡皮筋矩形框的起点
            rubberBandStart = e->pos();
            if (!rubberBand) {
                rubberBand = new QRubberBand(QRubberBand::Rectangle, ui->graphicsView);
            }
            rubberBand->setGeometry(QRect(rubberBandStart, QSize()));
            rubberBand->show();
        }



        // 绘图时
        if (cur_status == DrawLine
            || cur_status == DrawRect
            || cur_status == DrawEllipse
            || cur_status == DrawPolygon
        )
        {
            isSaved = false;//将保存状态设为未保存
            start_point = ui->graphicsView->mapToScene(e->pos());//将当前鼠标点击位置储存
            if (cur_status == DrawPolygon) {//若绘制多边形，将start_point储存在points容器中
                points.append(start_point);
            }
        }


        // 编辑时
        else if (cur_status == Select
                 || cur_status == Fill
                 || cur_status == Translate
                 || cur_status == Delete)
        {
            isSaved = false;
            // 先选中
            QPointF pointScene = ui->graphicsView->mapToScene(e->pos()); //转换到Scene坐标，视图坐标转换为场景坐标
            QGraphicsItem *item = nullptr;
            item = scene->itemAt(pointScene, ui->graphicsView->transform()); //获取光标下的绘图项
            if (item) {
                if (item->group()) {
                    item = item->group();
                }//方便对多个图形同时进行操作，视为一个组
                // 状态栏显示长度/面积
                item->setCursor(Qt::SizeAllCursor); // 当鼠标移动到选定的图形项时，变为一个箭头
                QString shapeinfo = "面积";
                if (item->data(shape_key).toString() == "直线")//若图形项中有直线，则改为长度
                    shapeinfo = "长度";
                QString str;
                if (item->group()) str = item->group()->data(areaORlen_key).toString();
                else str = item->data(areaORlen_key).toString();
                shapeinfo_label.setText(QString("选中对象: %1   " + shapeinfo + ": %2    ").arg(item->data(shape_key).toString()).arg(str));
            } else {
                shapeinfo_label.setText("");
                return;
            }
            // 填充
            if (cur_status == Fill) {
                switch (item->type())  //绘图项的类型，执行不同功能
                {
                    case QGraphicsRectItem::Type: //矩形框
                    {
                        //强制转换为矩形类型
                        QGraphicsRectItem *theItem = qgraphicsitem_cast<QGraphicsRectItem*>(item);
                        setBrushColor(theItem);//设置填充颜色
                        break;
                    }
                    case QGraphicsEllipseItem::Type: // 椭圆
                    {

                        QGraphicsEllipseItem *theItem;
                        theItem = qgraphicsitem_cast<QGraphicsEllipseItem*>(item);
                        setBrushColor(theItem);
                        break;
                    }
                    case QGraphicsPolygonItem::Type: //多边形
                    {
                        QGraphicsPolygonItem *theItem = qgraphicsitem_cast<QGraphicsPolygonItem*>(item);
                        setBrushColor(theItem);
                        break;
                    }
                    case QGraphicsLineItem::Type: //直线，设置线条颜色
                    {
                        QGraphicsLineItem *theItem = qgraphicsitem_cast<QGraphicsLineItem*>(item);
                        QPen pen = theItem->pen();
                        QColor color = theItem->pen().color();
                        color = QColorDialog::getColor(color,this,"选择线条颜色");//弹出对话框，选择颜色
                        if (color.isValid())
                        {
                            pen.setColor(color);
                            theItem->setPen(pen);
                        }
                        break;
                    }
                }
            }

            else if (cur_status == Translate) {  // 平移
                // 平移模态对话框
                tDialog = new TranslateDialog(this);
                // 接受信号，包含输入的平移量
                connect(tDialog, &TranslateDialog::translate_value,[=](float x, float y){
                    item->setPos(QPointF(item->x() + x, item->y() + y));
                    //item->rect().setX(item->x() + x);
                });
                tDialog->exec();
            }
            else if (cur_status == Delete) {  // 删除
                // 删除所选中的绘图项
                int cnt = scene->selectedItems().count();
                if (cnt > 0)
                for (int i = 0; i < cnt; i++) {
                    QGraphicsItem *item = scene->selectedItems().at(0);
                    scene->removeItem(item);
                }
            }
            scene->clearSelection();//清空选中状态
        } else {  // 其他选项
            shapeinfo_label.setText("");
        }
    } else if (e->button() == Qt::RightButton) { // 鼠标右击 画多边形
        if (cur_status == DrawPolygon) {
            start_point = ui->graphicsView->mapToScene(e->pos());//坐标转换
            points.append(start_point);
            for (auto e : points) qDebug() << e.x() << e.y() << endl;
            // 画多边形
            QGraphicsPolygonItem *item = new QGraphicsPolygonItem(QPolygonF(points));
            item->setFlags(//设置其可移动，可选中，可获得焦点
                QGraphicsItem::ItemIsMovable
                | QGraphicsItem::ItemIsSelectable
                | QGraphicsItem::ItemIsFocusable
                );
            double area = ComputePolygonArea(points);//得到面积
            item->setData(areaORlen_key, area);//添加数据，方便后续操作
            item->setData(shape_key, "多边形");
            scene->addItem(item);//添加到场景中
            // 清空points容器，便于下一个多边形绘制
            points.clear();
        }
    }
}


// 鼠标移动过程中状态栏显示坐标
void MainWindow::mouseMove_slot(QMouseEvent *e) {
    QPointF point = ui->graphicsView->mapToScene(e->pos());
    coordinate_label.setText(QString("坐标：(%1, %2)").arg(point.x()).arg(point.y()));
    if (cur_status == DrawLine
            || cur_status == DrawRect
            || cur_status == DrawEllipse
            || cur_status == DrawPolygon
        )
    {
        // 设置鼠标样式
        ui->graphicsView->setCursor(Qt::CrossCursor);//此时鼠标显示为十字线，方便用户绘制
    }

    if (rubberBand) {
        // 更新橡皮筋矩形框的大小，形成动态效果
        rubberBand->setGeometry(QRect(rubberBandStart, e->pos()).normalized());
    }
}

// 鼠标释放时绘制图形
void MainWindow::mouseReleased_slot(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {

        if (cur_status == Select) {
            // 计算橡皮筋矩形框的最终大小
            QRect rubberRect = QRect(rubberBandStart, e->pos()).normalized();
            rubberRect = ui->graphicsView->mapToScene(rubberRect).boundingRect().toRect();

            // 遍历场景中的所有形状，检查是否与橡皮筋矩形框相交
            QList<QGraphicsItem*> items = scene->items(rubberRect, Qt::IntersectsItemBoundingRect);
            for (QGraphicsItem* item : items) {
                item->setSelected(true);
                // 在这里根据需要执行选择、填充或删除操作
                // 例如，选择：item->setSelected(true);
                // 填充：setBrushColor(item);
                // 删除：scene->removeItem(item);
            }

            // 隐藏并销毁橡皮筋矩形框
            rubberBand->hide();
            rubberBand->deleteLater();
            rubberBand = nullptr;
        }
        // 获取鼠标释放时的坐标
        end_point = ui->graphicsView->mapToScene(e->pos());
        switch(cur_status) {
        case DrawLine:  // 绘制直线
        {
            QGraphicsLineItem *line_item = new QGraphicsLineItem(QLineF(start_point, end_point));//x,y 为左上角的图元局部坐标，图元中心点为0,0
            line_item->setFlags(
                QGraphicsItem::ItemIsMovable
                | QGraphicsItem::ItemIsSelectable
                | QGraphicsItem::ItemIsFocusable
                );

            QPen pen;
            pen.setWidth(2);//设置宽度
            line_item->setPen(pen);
            // 计算长度
            double len = sqrt(pow((start_point.x() - end_point.x()), 2) + pow((start_point.y() - end_point.y()), 2));
            line_item->setData(areaORlen_key, QVariant(len));
            line_item->setData(shape_key, "直线");
            scene->addItem(line_item);
            break;
        }
        case DrawRect:  // 绘制矩形
        {
            // 保持start_point在左上方，便于后续操作
            if (start_point.x() > end_point.x()) {
                QPointF tmp = start_point;
                start_point = end_point;
                end_point = tmp;
            }
            QGraphicsRectItem *rect_item = new QGraphicsRectItem(QRectF(start_point, end_point));//x,y 为左上角的图元局部坐标，图元中心点为0,0
            rect_item->setFlags(
                QGraphicsItem::ItemIsMovable
                | QGraphicsItem::ItemIsSelectable
                | QGraphicsItem::ItemIsFocusable
                );
            double area = (end_point.x() - start_point.x()) * (end_point.y() - start_point.y());
            rect_item->setData(areaORlen_key, QVariant(area));//获得并储存面积
            rect_item->setData(shape_key, "矩形");//设置图形类型
            scene->addItem(rect_item);//显示矩形
            break;
        }
        case DrawEllipse:  // 绘制椭圆
        {
            // 保持start_point在左上方
            if (start_point.x() > end_point.x()) {
                QPointF tmp = start_point;
                start_point = end_point;
                end_point = tmp;
            }
            QGraphicsEllipseItem *item=new QGraphicsEllipseItem(QRectF(start_point, end_point));
            item->setFlags(
                QGraphicsItem::ItemIsMovable
                | QGraphicsItem::ItemIsSelectable
                | QGraphicsItem::ItemIsFocusable
                );
            double area = PI * (end_point.x() - start_point.x()) * 0.5 * (end_point.y() - start_point.y()) * 0.5;
            item->setData(areaORlen_key, area);
            item->setData(shape_key, "椭圆");
            scene->addItem(item);
            break;
        }
        default:
            break;
        }
    }

}

// 鼠标双击时显示长度/面积对话框
void MainWindow::mouseDoubleClick_slot(QMouseEvent *e) {
    if (cur_status == Select) {
        QPointF pointScene = ui->graphicsView->mapToScene(e->pos()); //转换到Scene坐标
        QGraphicsItem *item = nullptr;
        item = scene->itemAt(pointScene, ui->graphicsView->transform()); //获取光标下的绘图项
        if (item) {
            QString shapeinfo = "面积";
            if (item->data(shape_key).toString() == "直线")
                shapeinfo = "长度";
            QString str;
            if (item->group()) str = item->group()->data(areaORlen_key).toString();
            else str = item->data(areaORlen_key).toString();
            QMessageBox::information(this, shapeinfo, str);
        }
    }
}

// 保存文件
void MainWindow::actSave_triggered_slot() {
    cur_status = Files;//表明当前状态为文件操作状态
    //以Qt预定义编码保存数据文件
    QString curPath = QDir::currentPath();//获取当前运行的文件路径
    QString aFileName = QFileDialog::getSaveFileName(this,tr("选择保存文件"),curPath,"Qt预定义编码数据文件(*.stm)");
    if(aFileName.isEmpty())//选择文件路径和文件名，为空则取消
        return;

    if(save(aFileName)){//保存为流数据文件
        QMessageBox::information(this, "提示消息", "文件已经成功保存!");
        isSaved = true;
    }
}
bool MainWindow::save(QString &FileName) {
    QFile file(FileName);
    if (!(file.open(QIODevice::WriteOnly | QIODevice::Truncate)))//若已存在则取消
        return false;
    QDataStream stream(&file);//设置数据流对象
    stream.setVersion(QDataStream::Qt_5_3);
    // 获取sence中的图形
    QList<QGraphicsItem *>	items_list = scene->items();
    // 保存数据
    for (int i = 0; i < items_list.size(); i++) {
        switch(items_list[i]->type())
        {
            case QGraphicsRectItem::Type: //矩形框
            {
                QGraphicsRectItem *theItem = qgraphicsitem_cast<QGraphicsRectItem*>(items_list[i]);
                stream << QString("矩形");
                stream << theItem->rect();
                stream << theItem->data(areaORlen_key).toDouble();
                if (theItem->brush() != QBrush()) stream << theItem->brush().color();
                else stream << QColor();
                break;
            }
            case QGraphicsEllipseItem::Type: // 椭圆
            {
                QGraphicsEllipseItem *theItem = qgraphicsitem_cast<QGraphicsEllipseItem*>(items_list[i]);
                stream << QString("椭圆");
                stream << theItem->rect();
                stream << theItem->data(areaORlen_key).toDouble();//获取面积并以双精度保存
                if (theItem->brush() != QBrush()) stream << theItem->brush().color();
                else stream << QColor();
                break;
            }
            case QGraphicsPolygonItem::Type: //多边形
            {
                QGraphicsPolygonItem *theItem = qgraphicsitem_cast<QGraphicsPolygonItem*>(items_list[i]);
                stream << QString("多边形");
                stream << theItem->polygon();
                stream << theItem->data(areaORlen_key).toDouble();
                if (theItem->brush() != QBrush()) stream << theItem->brush().color();
                else stream << QColor();
                break;
            }
            case QGraphicsLineItem::Type: //直线
            {
                QGraphicsLineItem *theItem = qgraphicsitem_cast<QGraphicsLineItem*>(items_list[i]);
                stream << QString("直线");
                stream << theItem->line();
                stream << theItem->data(areaORlen_key).toDouble();
                if (theItem->group()) {//检查图形是否处于组中，处理group-id
                    stream << theItem->group()->data(group_id_key).toInt();
                } else {
                    stream << -1;
                }
                stream << theItem->data(shape_key).toString();
                stream << theItem->pen().color();
                break;
            }
        }
    }
    file.close();
    return true;
}

// 打开文件
void MainWindow::actOpen_triggered_slot() {
    cur_status = Files;
    QString curPath=QDir::currentPath();
    //调用打开文件对话框打开一个文件
    QString aFileName=QFileDialog::getOpenFileName(this,tr("打开一个文件"),curPath,"流数据文件(*.stm)");
    if(aFileName.isEmpty())
        return;
    if(open(aFileName)) //保存为流数据文件
        QMessageBox::information(this,"提示消息","文件已经打开!");
}
bool MainWindow::open(QString &FileName) {
    QFile file(FileName);
    if (!(file.open(QIODevice::ReadOnly)))
        return false;
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_3);
    scene->clear();
    group_id = 0;
    while (!file.atEnd())
    {
        QString type;
        stream >> type;
        if (type == "矩形")
        {
            QRectF rect;
            stream >> rect;
            QGraphicsRectItem *rect_item = new QGraphicsRectItem(rect);//x,y 为左上角的图元局部坐标，图元中心点为0,0
            rect_item->setFlags(
                QGraphicsItem::ItemIsMovable
                | QGraphicsItem::ItemIsSelectable
                | QGraphicsItem::ItemIsFocusable
                );
            double area;
            stream >> area;
            rect_item->setData(areaORlen_key, QVariant(area));
            rect_item->setData(shape_key, "矩形");
            scene->addItem(rect_item);

            QColor color;
            stream >> color;//先读取颜色，然后检查颜色是否为空
            if (color.isValid())
                rect_item->setBrush(QBrush(color));
        }
        else if (type == "椭圆")
        {
            QRectF rect;
            stream >> rect;
            QGraphicsEllipseItem *item = new QGraphicsEllipseItem(rect);
            item->setFlags(
                QGraphicsItem::ItemIsMovable
                | QGraphicsItem::ItemIsSelectable
                | QGraphicsItem::ItemIsFocusable
                );
            double area;
            stream >> area;
            item->setData(areaORlen_key, area);
            item->setData(shape_key, "椭圆");
            scene->addItem(item);
            QColor color;
            stream >> color;
            if (color.isValid())
                item->setBrush(QBrush(color));
        }
        else if (type == "多边形")
        {
            QPolygonF polygon;
            stream >> polygon;
            QGraphicsPolygonItem *item = new QGraphicsPolygonItem(polygon);
            item->setFlags(
                QGraphicsItem::ItemIsMovable
                | QGraphicsItem::ItemIsSelectable
                | QGraphicsItem::ItemIsFocusable
                );
            double area;
            stream >> area;
            item->setData(areaORlen_key, area);
            item->setData(shape_key, "多边形");
            scene->addItem(item);
            QColor color;
            stream >> color;
            if (color.isValid())
                item->setBrush(QBrush(color));
        }
        else if (type == "直线")
        {
            QLineF line;
            stream >> line;
            QGraphicsLineItem *line_item = new QGraphicsLineItem(line);//x,y 为左上角的图元局部坐标，图元中心点为0,0
            line_item->setFlags(
                QGraphicsItem::ItemIsMovable
                | QGraphicsItem::ItemIsSelectable
                | QGraphicsItem::ItemIsFocusable
                );
            QPen pen;
            pen.setWidth(2);
            line_item->setPen(pen);
            line_item->setSelected(false);
            line_item->clearFocus();
            // 计算长度
            double len;
            stream >> len;
            line_item->setData(areaORlen_key, QVariant(len));
            //line_item->setData(shape_key, "直线");
            int id;
            stream >> id;
            line_item->setData(group_id_key, id);
            QString shape;
            stream >> shape;
            line_item->setData(shape_key, shape);
            scene->addItem(line_item);
            QColor color;
            stream >> color;
            if (color.isValid())
                line_item->setPen(QPen(color));
        }
    }
    file.close();
    return true;
}

void MainWindow::on_fillButton_clicked()
{
    QColor fill_color = QColorDialog::getColor(Qt::white, this, "选择填充颜色");

    if (!fill_color.isValid()) {
        // 用户取消了颜色选择，不执行填充操作
        return;
    }

    QList<QGraphicsItem *> selectedItems = scene->selectedItems();
    foreach (QGraphicsItem *item, selectedItems) {
        switch (item->type()) {
        case QGraphicsRectItem::Type: {
            QGraphicsRectItem *rectItem = qgraphicsitem_cast<QGraphicsRectItem*>(item);
            rectItem->setBrush(fill_color);
            break;
        }
        case QGraphicsEllipseItem::Type: {
            QGraphicsEllipseItem *ellipseItem = qgraphicsitem_cast<QGraphicsEllipseItem*>(item);
            ellipseItem->setBrush(fill_color);
            break;
        }
        case QGraphicsPolygonItem::Type: {
            QGraphicsPolygonItem *polygonItem = qgraphicsitem_cast<QGraphicsPolygonItem*>(item);
            polygonItem->setBrush(fill_color);
            break;
        }
            // Add other shape types as needed
        }
    }
}


// 新建
void MainWindow::actNew_triggered_slot() {
    if (scene->items().size() && !isSaved) {
        // 先询问是否保存
        int ans = QMessageBox::question(this, "提示", "当前文件未保存，是否先保存？");
        if (ans == QMessageBox::Yes) {
            //以Qt预定义编码保存数据文件
            QString curPath = QDir::currentPath();
            QString aFileName = QFileDialog::getSaveFileName(this, tr("选择保存文件"), curPath,"Qt预定义编码数据文件(*.stm)");
            if (aFileName.isEmpty())
                return;
            if  (save(aFileName)) {//保存为流数据文件
                QMessageBox::information(this, "提示消息", "文件已经成功保存!");
                isSaved = true;
            }
        }
    }
    scene->clear();
    group_id = 0;
}

// 关闭文件
void MainWindow::closeEvent(QCloseEvent *) {
    if (scene->items().size() && !isSaved) {
        // 先询问是否保存
        int ans = QMessageBox::question(this, "提示", "当前文件未保存，是否先保存？");
        if (ans == QMessageBox::Yes) {
            //以Qt预定义编码保存数据文件
            QString curPath = QDir::currentPath();
            QString aFileName = QFileDialog::getSaveFileName(this, tr("选择保存文件"), curPath,"Qt预定义编码数据文件(*.stm)");
            if (aFileName.isEmpty())
                return;
            if  (save(aFileName)) {//保存为流数据文件
                QMessageBox::information(this, "提示消息", "文件已经成功保存!");
                isSaved = true;
            }
        }
    }
}

// 帮助
void MainWindow::on_actionHelp_triggered()
{
    if(!hDialog)
    {
        hDialog=new HelpDialog(this);
    }
    hDialog->show();
}

