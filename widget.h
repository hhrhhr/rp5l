#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#ifdef Q_WS_WIN
  #include <QSettings>
#endif

#include <QFileDialog>
#include <QListWidget>
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
    void on_selectInput_clicked();
    void on_selectOutput_clicked();
    void on_rpackList_itemClicked(QListWidgetItem *item);
    void on_rpackList_itemActivated(QListWidgetItem *item);
    void on_scanRpack_clicked();
    void on_unpackRpack_clicked();

private:
    Ui::Widget *ui;
    QString inPath;
    QString outPath;
    QString currentRpack;
    void fillRpacksList();
    void fillHeadersList();
    void scanRpack();

    header      h;
    section     s[];
    filepart    f[];
    filemap     m[];
    quint32     fp[];
    QStringList fn;

    void unpackBlock(quint32 offs, quint32 pack, quint32 unpk);

    // tools
    QString addSpace(QString str);

};

#endif // WIDGET_H
