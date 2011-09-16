#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    qDebug() << this << "Widget";
    ui->setupUi(this);

#ifdef Q_WS_WIN
    QSettings reg("HKEY_CURRENT_USER\\Software\\Valve\\Steam", QSettings::NativeFormat);
    inPath = reg.value("SteamPath").toString();
    inPath += "\\steamapps\\common\\dead island";
    if (QFile::exists(inPath + "\\DeadIslandGame.exe")) {
        inPath += "\\DI\\Data";
    } else {
        inPath = "";
    }
//    TODO: fix it
    outPath = "i:\\tmp_DI\\unpack2";
    fillRpacksList();
#endif

}

Widget::~Widget()
{
    qDebug() << this << "~Widget";
    delete ui;
}

// private slots //////////////////////////////////////////////////////////////////////////////////

void Widget::on_selectInput_clicked()
{
    qDebug() << this << "on_selectRpacks_clicked";
    QString oldInPath = inPath;
    inPath = QFileDialog::getExistingDirectory(this, "Select rpacks catalog", oldInPath);
    qDebug() << inPath;
    if (inPath.isEmpty()) {
        qDebug() << "no select";
        inPath = oldInPath;
        return;
    }
    fillRpacksList();
}

void Widget::on_selectOutput_clicked()
{
    qDebug() << this << "on_selectOutput_clicked";
    QString oldOutPath = outPath;
    outPath = QFileDialog::getExistingDirectory(this, "Select output directory", oldOutPath);
    qDebug() << outPath;
    if (outPath.isEmpty()) {
        qDebug() << "no select";
        outPath = oldOutPath;
        return;
    }
    ui->lbOutDirectory->setText(outPath);
}

void Widget::on_rpackList_itemClicked(QListWidgetItem *item)
{
    qDebug() << this << "on_rpackList_itemClicked";

    currentRpack = item->text();
    fillHeadersList();
}

void Widget::on_rpackList_itemActivated(QListWidgetItem *item)
{
    qDebug() << this << "on_rpackList_itemActivated";

    currentRpack = item->text();
    fillHeadersList();
}

void Widget::on_scanRpack_clicked()
{
    qDebug() << this << "on_scanRpack_clicked";

    if (!ui->headerList->count())
        return;

    scanRpack();
}

void Widget::on_unpackRpack_clicked()
{
    qDebug() << this << "on_unpackRpack_clicked";
    unpackRpack();
}

// private ///////////////////////////////////////////////////////////////////////////////////////

void Widget::fillRpacksList()
{
    qDebug() << this << "fillRpacksList";
    ui->lbInDirectory->setText(inPath);
    ui->lbOutDirectory->setText(outPath);
    ui->tabWidget->setTabText(0, "rpack not selected");
    ui->headerList->clear();
    ui->rpackList->clear();

    if (!inPath.isEmpty()) {
        QDir dir;
        dir.setCurrent(inPath);
        QStringList rpacks;
        rpacks << dir.entryList(QStringList("*.rpack"), QDir::Files);
        ui->rpackList->addItems(rpacks);
    }
}

void Widget::fillHeadersList()
{
    qDebug() << this << "fillHeadersList";
    ui->tabWidget->setTabText(0, "rpack not selected");
    ui->filesList->clear();
    ui->headerList->clear();

    QFile rpack(inPath + "\\" + currentRpack);
    if (!rpack.open(QIODevice::ReadOnly))
        return;
    ui->tabWidget->setTabText(0, currentRpack);
    QDataStream in(&rpack);
    in.setByteOrder(QDataStream::LittleEndian);

//    read header
    in >> h.magic >> h.version >> h.compression >> h.partsCount >> h.sectionCount
       >> h.filesCount >> h.filenamesSize >> h.filenamesCount >> h.blockSize;

//    read sections
    s.reserve(h.sectionCount);
    quint32 i = 0;
    quint32 allPacked = 0;
    quint32 allUnpacked = 0;
    do {
        section q;
        in >> q.filetype >> q.type2 >> q.type3 >> q.type4;
        in >> q.offset >> q.unpackedSize >> q.packedSize >> q.partsCount;
        s << q;
        allPacked += q.packedSize;
        allUnpacked += q.unpackedSize;
    } while (++i < h.sectionCount);

    rpack.close();

//    fill headers list
    QString magic("unknown");
    if (h.magic == 1278562386)
        magic = "RP5L";

    QString compression("unknown");
    if (h.compression == 1)
        compression = "zlib";

    QStringList headAndSect;
    headAndSect << "magic:\t" + magic;
    headAndSect << "version:\t" + QString::number(h.version);
    headAndSect << "compression:\t" + compression;
    headAndSect << "parts:\t" + QString::number(h.partsCount);
    headAndSect << "sections:\t" + QString::number(h.sectionCount);
    headAndSect << "files:\t" + QString::number(h.filesCount);
    headAndSect << "names size:\t" + QString::number(h.filenamesSize);
    headAndSect << "filenames:\t" + QString::number(h.filenamesCount);
    headAndSect << "block size:\t" + QString::number(h.blockSize);
    headAndSect << "";
    headAndSect << "rpack size:\t" + addSpace(QString::number(rpack.size())) + " bytes";
    headAndSect << "packed size:\t" + addSpace(QString::number(allPacked)) + " bytes";
    headAndSect << "unpacked size:\t" + addSpace(QString::number(allUnpacked)) + " bytes";
    ui->headerList->addItems(headAndSect);

    if (h.magic != 1278562386)
        ui->headerList->item(0)->setBackgroundColor(Qt::red);

    if (h.compression != 1)
        ui->headerList->item(2)->setBackgroundColor(Qt::red);

}

void Widget::scanRpack()
{
    qDebug() << this << "scanRpack";
    ui->filesList->clear();
    s.clear();
    p.clear();
    m.clear();
    fp.clear();
    fn.clear();

    QFile rpack(inPath + "\\" + currentRpack);
    if (!rpack.open(QIODevice::ReadOnly))
        return;
    QDataStream in(&rpack);
    in.setByteOrder(QDataStream::LittleEndian);
    rpack.seek(9*4 + h.sectionCount*5*4);

    qDebug() << "fileparts" << rpack.pos();
    p.reserve(h.partsCount);
    quint32 i = 0;
    do {
        filepart q;
        in >> q.sectionIndex >> q.unk1 >> q.fileIndex;
        in >> q.offset >> q.unpackedSize >> q.packedSize;
        p << q;
    } while (++i < h.partsCount);

    qDebug() << "filemaps" << rpack.pos();
    m.reserve(h.filenamesCount);
    i = 0;
    do {
        filemap q;
        in >> q.partsCount >> q.unk1 >> q.filetype;
        in >> q.unk2 >> q.fileIndex >> q.firstPart;
        m << q;
    } while (++i < h.filenamesCount);

    qDebug() << "fnPtrs" << rpack.pos();
    fp.reserve(h.filenamesCount);
    i = 0;
    do {
        quint32 q;
        in >> q;
        fp << q;
    } while (++i < h.filenamesCount);

    QByteArray ba;
    ba.resize(h.filenamesSize);
    char *data = ba.data();
    in.readRawData(data, h.filenamesSize);

    rpack.close();

    fn.reserve(h.filenamesCount);
    i = 0;
    do {
        QString q;
        q = data + fp.at(i);
        fn << q ;
    } while (++i < h.filenamesCount);

    // fill filenames list
    QString filename;
    i = 0;
    do {
        if (m.at(i).filetype == 32)
            filename = "tex";
        else if (m.at(i).filetype == 48)
            filename = "shd";
        else
            filename = "unk";
        filename = "(" + filename + ") " + fn.at(i);
        ui->filesList->addItem(filename);
    } while (++i < h.filenamesCount);
}

void Widget::unpackRpack()
{
    //
}

void Widget::unpackBlock(quint32 offs, quint32 pack, quint32 unpk)
{

    // unpack
//    unpacked.resize(size_unpacked);
//    unpacked = qUncompress(packed);
}

// tools

QString Widget::addSpace(QString str)
{
    int size = str.size();
    int i = 0;
    do {
        str.insert(size - 3 - i, " ");
        i += 3;
    } while (i < size - 2);

    return str;
}

