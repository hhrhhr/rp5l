#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#ifdef Q_WS_WIN
  #include <QSettings>
#endif

#include <QFileDialog>
#include "rp5l_structure.h"

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
    void on_selectRpacks_clicked();
    void on_selectOutput_clicked();
    void on_rpacksList_currentRowChanged(int currentRow);
    void on_scanRpack_clicked();
    void on_unpackPrpack_clicked();


private:
    Ui::Widget *ui;
    QString diPath;
    QString outPath;
    void fillRpacksList();
    QString currentRpack;

    header h;
    QList<section> sections;
    QList<fileset> filesets;
    QList<unknown> unknowns;
    QList<fnPtr> fnPtrs;
    QList<filename> filenames;

};

#endif // WIDGET_H
