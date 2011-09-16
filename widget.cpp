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
    section s[h.sectionCount];
    quint32 i = 0;
    quint32 allPacked = 0;
    quint32 allUnpacked = 0;
    do {
        in >> s[i].filetype >> s[i].type2 >> s[i].type3 >> s[i].type4;
        in >> s[i].offset >> s[i].unpackedSize >> s[i].packedSize >> s[i].partsCount;
        allPacked += s[i].packedSize;
        allUnpacked += s[i].unpackedSize;
    } while (++i < h.sectionCount);

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
    if (h.compression != 1) {
        ui->headerList->item(2)->setBackgroundColor(Qt::red);
    }

    rpack.close();
}

void Widget::scanRpack()
{
    qDebug() << this << "scanRpack";
    ui->filesList->clear();

    QFile rpack(inPath + "\\" + currentRpack);
    if (!rpack.open(QIODevice::ReadOnly))
        return;
    QDataStream in(&rpack);
    in.setByteOrder(QDataStream::LittleEndian);
    rpack.seek(9*4 + h.sectionCount*4*5);   // jump to fileparts block

    qDebug() << "fileparts" << rpack.pos();
    filepart f[h.partsCount];
    quint32 i = 0;
    do {
        in >> f[i].sectionIndex >> f[i].unk1 >> f[i].fileIndex;
        in >> f[i].offset >> f[i].unpackedSize >> f[i].packedSize;
    } while (++i < h.partsCount);

    qDebug() << "filemaps" << rpack.pos();
    filemap m[h.filenamesCount];
    i = 0;
    do {
        in >> m[i].partsCount >> m[i].unk1 >> m[i].filetype;
        in >> m[i].unk2 >> m[i].fileIndex >> m[i].firstPart;
    } while (++i < h.filenamesCount);

    qDebug() << "fnPtrs" << rpack.pos();
    quint32 fp[h.filenamesCount];
    i = 0;
    do {
        in >> fp[i];
    } while (++i < h.filenamesCount);

    QByteArray ba;
    ba.resize(h.filenamesSize);
    char *data = ba.data();
    in.readRawData(data, h.filenamesSize);
    fn.clear();
    i = 0;
    do {
        fn << data + fp[i];
    } while (++i < h.filenamesCount);

    // fill filenames list
    QString filename;
    i = 0;
    do {
        if (m[i].filetype == 32)
            filename = "tex";
        else if (m[i].filetype == 48)
            filename = "shd";
        else
            filename = "unk";
        filename = "(" + filename + ") " + fn.at(i);
        ui->filesList->addItem(filename);
    } while (++i < h.filenamesCount);
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

