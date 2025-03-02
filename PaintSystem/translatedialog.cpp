#include "translatedialog.h"
#include "ui_translatedialog.h"
#include <QDialogButtonBox>
#include <QPushButton>

TranslateDialog::TranslateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TranslateDialog)
{
    ui->setupUi(this);
    // 对话框按键时发出信号，包括平移量
    connect(ui->buttonBox, &QDialogButtonBox::clicked,[=](QAbstractButton * button)
    {
        float x, y;
        if(ui->buttonBox->button(QDialogButtonBox::Ok) == static_cast<QPushButton *>(button))
        {  //判断按下的是否为"确定”按钮
            if(!ui->lineEdit->text().isEmpty() && !ui->lineEdit_2->text().isEmpty())  //判断lineEdit是否为空
            {
                x = ui->lineEdit->text().toFloat();//转化为浮点数
                y = ui->lineEdit_2->text().toFloat();
                emit translate_value(x, y);//发送信号
            }
        }
    });
}

TranslateDialog::~TranslateDialog()
{
    delete ui;
}

