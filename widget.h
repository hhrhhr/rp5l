#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#ifdef Q_WS_WIN
  #include <QSettings>
#endif

#include <QFileDialog>
#include <QListWidget>
#include "rp5l_structure.h"
#include <QTimer>

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
    void on_selectInput_clicked();
    void on_selectOutput_clicked();
    void on_rpackList_itemClicked(QListWidgetItem *item);
    void on_rpackList_itemActivated(QListWidgetItem *item);
    void on_scanRpack_clicked();
    void on_unpackRpack_clicked();
    void unpackRpack();

private:
    Ui::Widget *ui;
    QString inPath;
    QString outPath;
    QString currentRpack;
    void fillRpacksList();
    void fillHeadersList();
    void scanRpack();

    header h;
    QList<section> s;
    QList<filepart> p;
    QList<filemap> m;
    QList<quint32> fp;
    QList<QString> fn;


    // tools
    QString addSpace(QString str);
    void unpackBlock(QDataStream &in, quint32 offs, quint32 pack, quint32 unpk, QDataStream &out);

};

#endif // WIDGET_H
