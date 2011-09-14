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
    diPath = reg.value("SteamPath").toString();
    diPath += "\\steamapps\\common\\dead island";
    if (QFile::exists(diPath + "\\DeadIslandGame.exe")) {
        diPath += "\\DI\\Data";
    } else {
        diPath = "";
    }
    outPath = "i:\\tmp_DI\\unpack2";
#endif
    fillRpacksList();
}

Widget::~Widget()
{
    qDebug() << this << "~Widget";
    delete ui;
}

// private slots //////////////////////////////////////////////////////////////////////////////////

void Widget::on_selectRpacks_clicked()
{
    qDebug() << this << "on_selectRpacks_clicked";
    QString oldDiPath = diPath;
    diPath = QFileDialog::getExistingDirectory(this, "Select rpacks catalog", oldDiPath);
    qDebug() << diPath;
    if (diPath.isEmpty()) {
        qDebug() << "no select";
        diPath = oldDiPath;
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

void Widget::on_rpacksList_currentRowChanged(int currentRow)
{
    qDebug() << this << "on_rpacksList_currentRowChanged" << currentRow;
    if (currentRow < 0)
        return;
    currentRpack = ui->rpacksList->item(currentRow)->text();
    currentRpack = diPath + "/" + currentRpack;
    QFile rpack(currentRpack);
    if (!rpack.open(QIODevice::ReadOnly)) {
        qWarning() << "rpack not open";
        return;
    }
    QDataStream in(&rpack);
    in.setByteOrder(QDataStream::LittleEndian);

    ui->rpacksFiles->clear();
    QListWidget *c = ui->rpacksContent;
    c->clear();

    // header /////////////////////////////
    qDebug() << "header" << rpack.pos();
    in >> h.magic;
    if (h.magic != 1278562386) /*RP5L*/ {
        c->addItem("magic not match");
        return;
    }
    in >> h.version >> h.compression >> h.dataSize >> h.sectionCount;
    in >> h.filesCount >> h.filenamesSize >> h.filenamesCount >> h.blockSize;

    QString txt = "RP5L, vesrsion: ";
    txt.append(QString::number(h.version));
    c->addItem(txt);
    if (h.compression == 1) {
        c->addItem("compression: zlib");
    } else {
        c->addItem("compression: unknown");
    }
    txt = "sections count: ";
    txt.append(QString::number(h.sectionCount));
    c->addItem(txt);
    txt = "files count: ";
    txt.append(QString::number(h.filesCount));
    c->addItem(txt);
    txt = "filenames count: ";
    txt.append(QString::number(h.filenamesCount));
    c->addItem(txt);
    c->addItem("---");

    // sections ///////////////////////////
    qDebug() << "sections" << rpack.pos();
    sections.clear();
    sections.reserve(h.sectionCount);

    for (quint32 i = 0; i < h.sectionCount; ++i) {
        section s;
        in >> s.type1 >> s.unk1 >> s.type2 >> s.offset
           >> s.unpackedSize >> s.packedSize >> s.packsCount;

        if (s.unk1 != 0)
            qWarning() << "unk1 <> 0!!!";

        QString type;
        if (s.type1 == 32 && s.type2 == 2) {
            type = "tex headers";
        } else if (s.type1 == 33 && s.type2 == 3) {
            type = "textures";
        } else if ((s.type1 == 48 || s.type1 == 50 || s.type1 == 51) && s.type2 == 3) {
            type = "shaders???";
        } else {
            type = "unknown";
        }

        QStringList l;
        l << type
             + "\t" + QString::number(s.type1) + " " + QString::number(s.unk1)
             + " " + QString::number(s.type2) + " " + QString::number(s.offset)
             + " " + QString::number(s.unpackedSize) + " " + QString::number(s.packedSize)
             + " " + QString::number(s.packsCount);
        ui->rpacksContent->addItems(l);

        sections << s;
    }

    // filesets /////////////////////////////
    qDebug() << "filesets" << rpack.pos();
    filesets.clear();
    filesets.reserve(h.filesCount);

    for (quint32 i = 0; i < h.dataSize; ++i) {
        fileset f;
        in >> f.sectionNumber >> f.fileIndex >> f.offset >> f.unpackedSize >> f.packedSize;

        filesets << f;
    }

    // unknown //////////////////////////////
    qDebug() << "unknown" << rpack.pos();
    unknowns.clear();
    unknowns.reserve(h.filesCount);

    for (quint32 i = 0; i < h.filesCount; ++i) {
        unknown u;
        in >> u.type1 >> u.type2 >> u.fileIndex >> u.unk1;

        unknowns << u;
    }

    // filenames pointers //////////////////////////
    qDebug() << "fn pointers" << rpack.pos();
    fnPtrs.clear();
    fnPtrs.reserve(h.filenamesCount);

    for (quint32 i = 0; i < h.filenamesCount; ++i) {
        fnPtr fp;
        in >> fp.fnPtr;

        fnPtrs << fp;
    }

    // filenames //////////////////////////
    qDebug() << "filenames" << rpack.pos();
    filenames.clear();
    filenames.reserve(h.filenamesCount);

    QByteArray ba;
    ba.resize(h.filenamesSize);
    char *data = ba.data();
    in.readRawData(data, h.filenamesSize);
    qDebug() << "all data read" << rpack.pos();

    QList<QByteArray> fba;
    fba = ba.split(0);
    if (!fba.isEmpty())
        fba.removeLast();

    for (quint32 i = 0; i < h.filenamesCount; ++i) {
        filename f;
        f.name = QString(fba.at(i));

        filenames << f;
    }

    rpack.close();

    ui->scanRpack->setEnabled(1);
    if (h.compression == 1) {
        ui->unpackPrpack->setText("unpack");
    } else {
        ui->unpackPrpack->setText("unknown compression");
    }
    ui->unpackPrpack->setDisabled(1);
}

void Widget::on_scanRpack_clicked()
{
    qDebug() << this << "on_scanRpack_clicked";
    ui->rpacksFiles->clear();

    for (quint32 i = 0; i < h.filenamesCount; ++i) {
        ui->rpacksFiles->addItem(filenames.at(i).name);
    }

    if (h.compression == 1) {
        ui->unpackPrpack->setText("unpack");
        ui->unpackPrpack->setEnabled(1);
    } else {
        ui->unpackPrpack->setText("unknown compression");
    }
}

void Widget::on_unpackPrpack_clicked()
{
    qDebug() << this << "on_unpackPrpack_clicked";

    quint32 offset = sections.at(0).offset;
    quint32 size_unpacked = sections.at(0).unpackedSize;
    quint32 size_packed = sections.at(0).packedSize;

    QFile rpack(currentRpack);
    if (!rpack.open(QIODevice::ReadOnly))
        return;
    QDataStream in(&rpack);

    // prepare packed
    QByteArray packed;
    packed.resize(size_packed + 4);
    char *data = packed.data();
    rpack.seek(offset - 4);
    in.readRawData(data, size_packed + 4);

    // write unpacked size in header
    QByteArray size;
    size.resize(4);
    QDataStream s(&size, QIODevice::WriteOnly);
    s << size_unpacked;
    packed.replace(0, 4, size);

    // unpack
    QByteArray unpacked;
    unpacked.resize(size_unpacked);
    unpacked = qUncompress(packed);
    data = unpacked.data();

    // save file
    QString fn;
    fn = outPath + "\\header.bin";
    QFile file(fn);
    if (!file.open(QIODevice::WriteOnly))
        return;
    QDataStream out(&file);
    out.writeRawData(data, size_unpacked);
}

// private

void Widget::fillRpacksList()
{
    qDebug() << this << "fillRpacksList";
    ui->lbDiCatalog->setText(diPath);
    ui->lbOutDirectory->setText(outPath);
    ui->rpacksList->clear();
    ui->rpacksContent->clear();
    ui->rpacksFiles->clear();

    if (!diPath.isEmpty()) {
        QDir dir;
        dir.setCurrent(diPath);
        QStringList rpacks;
        rpacks << dir.entryList(QStringList("*.rpack"), QDir::Files);
        ui->rpacksList->addItems(rpacks);
    }

    ui->scanRpack->setDisabled(1);
    if (h.compression == 1) {
        ui->unpackPrpack->setText("unpack");
    } else {
        ui->unpackPrpack->setText("unknown compression");
    }
    ui->unpackPrpack->setDisabled(1);
}
