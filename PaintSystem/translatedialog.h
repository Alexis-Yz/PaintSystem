#ifndef TRANSLATEDIALOG_H
#define TRANSLATEDIALOG_H

#include <QDialog>

namespace Ui {
class TranslateDialog;
}

class TranslateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TranslateDialog(QWidget *parent = 0);
    ~TranslateDialog();
signals:
    void translate_value(float x, float y);

private:
    Ui::TranslateDialog *ui;
};

#endif // TRANSLATEDIALOG_H

