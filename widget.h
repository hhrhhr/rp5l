#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#ifdef Q_WS_WIN
  #include <QSettings>
#endif

#include <QFileDialog>
#include "rp5l_structure.h"
//#include <QByteArray>

#include <QDebug>

namespace Ui {
    class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_toolButton_clicked();
    void on_rpacksList_currentRowChanged(int currentRow);

    void on_scanRpack_clicked();

private:
    Ui::Widget *ui;
    QString diPath;
    void fillRpacksList();
    header h;
    QString currentRpack;
};

#endif // WIDGET_H
