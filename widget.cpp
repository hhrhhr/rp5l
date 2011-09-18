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
    if (!ui->filesList->count())
        return;
    if (h.compression != 1)
        return;
    ui->tabWidget->setDisabled(1);
    QTimer::singleShot(1000, this, SLOT(startUnpackRpack()));
    ui->tabWidget->setEnabled(1);
}

void Widget::startUnpackRpack()
{
    unpackRpack();
}

void Widget::on_unpackAll_clicked()
{
    qDebug() << this << "on_unpackAll_clicked";
    int count = ui->rpackList->count();
    if (!count)
        return;
    int i = 0;
    do {
        currentRpack = ui->rpackList->item(i)->text();
        fillHeadersList();
        scanRpack();
        if (h.compression == 1) {
            qDebug() << currentRpack << "unpacking...";
//            unpackRpack();
            QFuture<void> future = QtConcurrent::run(this, &Widget::unpackRpack);
            future.waitForFinished();
        } else {
            qDebug() << currentRpack << "compression unknown";
        }
        i++;
    } while (i < count);

}

// private ///////////////////////////////////////////////////////////////////////////////////////

void Widget::fillRpacksList()
{
    qDebug() << this << "fillRpacksList";
    ui->lbInDirectory->setText(inPath);
    ui->lbOutDirectory->setText(outPath);
    ui->tabWidget->setTabText(0, "rpack not selected");
    ui->filesList->clear();
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
    s.clear();
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
    p.clear();
    m.clear();
    fp.clear();
    fn.clear();

    if (h.partsCount == 0)
        return;

    QFile rpack(inPath + "\\" + currentRpack);
    if (!rpack.open(QIODevice::ReadOnly))
        return;
    QDataStream in(&rpack);
    in.setByteOrder(QDataStream::LittleEndian);
    rpack.seek(9*4 + h.sectionCount*5*4);       // skip header and sections

//    qDebug() << "fileparts" << rpack.pos();
    p.reserve(h.partsCount);
    quint32 i = 0;
    do {
        filepart q;
        in >> q.sectionIndex >> q.unk1 >> q.fileIndex >> q.offset >> q.unpackedSize >> q.packedSize;
        p << q;
    } while (++i < h.partsCount);

//    qDebug() << "filemaps" << rpack.pos();
    m.reserve(h.filenamesCount);
    i = 0;
    do {
        filemap q;
        in >> q.partsCount >> q.unk1 >> q.filetype >> q.unk2 >> q.fileIndex >> q.firstPart;
        m << q;
    } while (++i < h.filenamesCount);

//    qDebug() << "fnPtrs" << rpack.pos();
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
        filename = "(";
        if (m.at(i).filetype == 32)
            filename += "tex";
        else if (m.at(i).filetype == 48)
            filename += "shd";
        else
            filename += QString::number(m.at(i).filetype);
        filename += ") " + fn.at(i);
        ui->filesList->addItem(filename);
    } while (++i < h.filenamesCount);
}

void Widget::unpackRpack()
{
    QDir outdir;
    outdir.mkpath(outPath + "\\" + currentRpack);
    outdir.setCurrent(outPath + "\\" + currentRpack);

    QFile rpack(inPath + "\\" + currentRpack);
    if (!rpack.open(QIODevice::ReadOnly)) return;
    QDataStream in(&rpack);

    QByteArray texHeader;
    QDataStream tex(&texHeader, QIODevice::ReadWrite);
    char *texdata = texHeader.data();

    for (quint32 i = 0; i < h.filenamesCount; ++i) {
        QString outname = QString("%1__%2.%3")
                          .arg(i)
                          .arg(fn.at(i))
                          .arg(m.at(i).filetype);

        QFile outfile(outname);
        if (!outfile.open(QIODevice::WriteOnly)) return;
        QDataStream out(&outfile);

        quint32 ptr = m.at(i).firstPart;
        quint32 count = m.at(i).partsCount;
// qDebug() << "start" << ptr << "count" << count;
        do {
            quint8 sidx = p.at(ptr).sectionIndex;
            quint32 offs = p.at(ptr).offset;
            quint32 pack = p.at(ptr).packedSize;
            quint32 unpk = p.at(ptr).unpackedSize;
            quint8 sect = s.at(sidx).filetype;
            quint32 soffs = s.at(sidx).offset;

            if (sect == 32) {
// texture header
                if (!texHeader.size()) {
// qDebug() << "\t\tunpack texture headers";
                    quint32 spack = s.at(sidx).packedSize;
                    quint32 sunpk = s.at(sidx).unpackedSize;
// qDebug() << "\t\tsect:" << soffs << spack << sunpk;
                    texHeader.resize(sunpk);
                    texdata = texHeader.data();
                    unpackBlock(in, soffs, spack, sunpk, tex);
                }
// qDebug() << "\thead:" << offs << unpk;
                out.writeRawData(texdata+offs, unpk);
            } else {
// all other files
                QByteArray tmpUnpack;
                QDataStream tmp(&tmpUnpack, QIODevice::ReadWrite);
                tmpUnpack.resize(unpk);
                char *tmpdata = tmpUnpack.data();
// qDebug() << "\tbody:" << soffs << offs << unpk << pack << tmpUnpack.size();
                unpackBlock(in, soffs + offs, pack, unpk, tmp);
                out.writeRawData(tmpdata, unpk);
            }
            ptr++;
        } while (--count);
        outfile.close();
    }
    rpack.close();

    // save structure
    QFile structure("..\\" + currentRpack + ".struct");
    if (!structure.open(QIODevice::WriteOnly)) return;
    QDataStream st(&structure);
//    st.setByteOrder(QDataStream::LittleEndian);

    st << h.magic << h.version << h.compression << h.partsCount << h.sectionCount
       << h.filesCount << h.filenamesSize << h.filenamesCount << h.blockSize;
    foreach (section i, s) {
        st << i.filetype << i.type2 << i.type3 << i.type4
           << i.offset << i.unpackedSize << i.packedSize << i.partsCount;
    }
    foreach (filepart i, p) {
        st << i.sectionIndex << i.unk1 << i.fileIndex << i.offset << i.unpackedSize << i.packedSize;
    }
    foreach (filemap i, m) {
        st << i.partsCount << i.unk1 << i.filetype << i.unk2 << i.fileIndex << i.firstPart;
    }
    foreach (quint32 i, fp) {
        st << i;
    }
    foreach (QString i, fn) {
        st << i;
    }
    structure.close();

}

void Widget::unpackBlock(QDataStream &in, quint32 offs, quint32 pack, quint32 unpk, QDataStream &out)
{
    // prepare array
    QByteArray compressed;
    compressed.resize(pack + 4);
    QDataStream t(&compressed, QIODevice::WriteOnly);
    char *cdata = compressed.data();

    // read compressed block
    in.device()->seek(offs - 4);
    in.readRawData(cdata, pack + 4);

    // insert size of uncompressed data
    QByteArray size;
    size.resize(4);
    QDataStream s(&size, QIODevice::WriteOnly);
    s << unpk;
    compressed.replace(0, 4, size);

    // unpack and write output block
    QByteArray uncompressed;
    uncompressed.resize(unpk);
    uncompressed = qUncompress(compressed);
    const char *udata = uncompressed.data();
    int written = out.writeRawData(udata, unpk);
    if (written == -1)
        qCritical() << "write failed!";

//    out.writeRawData(qUncompress(compressed).data(), unpk);
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

void Widget::on_scanTextures_clicked()
{
    qDebug() << "on_scanTextures_clicked()";

    ui->texturesList->clear();
    ui->detailsList->clear();

    QString texPath = outPath + "\\" + currentRpack;
    QDir dir(texPath);
    if (!dir.exists())
        return;
    dir.setCurrent(texPath);
    QStringList textures;
    textures << dir.entryList(QStringList("*.32"), QDir::Files);
    ui->texturesList->addItems(textures);
}

void Widget::on_texturesList_currentRowChanged(int index)
{
    if (!ui->texturesList->count())
        return;
    if (index == -1)
        return;
    currentTexture = ui->texturesList->item(index)->text();
    scanTexture();
}


void Widget::on_texturesList_itemClicked(QListWidgetItem *item)
{
    if (!ui->texturesList->count())
        return;
    currentTexture = item->text();
    scanTexture();
}

void Widget::scanTexture()
{
    ui->detailsList->clear();

    QFile texture(outPath + "\\" + currentRpack + "\\" + currentTexture);
    if (!texture.open(QIODevice::ReadOnly))
        return;
    QDataStream tex(&texture);
    tex.setByteOrder(QDataStream::LittleEndian);
    quint16 width, height, unk1, cubemaps;
    quint32 mips, dxt;
    tex >> width >> height >> unk1 >> cubemaps;
    tex >> mips >> dxt;
    quint32 mip[mips];
    for (uint i = 0; i < mips; ++i) {
        tex >> mip[i];
    };
    texture.close();

    QStringList td;
    td << "width\t" + QString::number(width);
    td << "height\t" + QString::number(height);
    td << "unk1\t" + QString::number(unk1);
    td << "cubemaps\t" + QString::number(cubemaps);
    td << "mips\t" + QString::number(mips);
    td << "dxt\t" + QString::number(dxt);
    for (uint i = 0; i < mips; ++i) {
        td << QString("mip%1\t").arg(i) + QString::number(mip[i]);
    };

    ui->detailsList->addItems(td);
}


void Widget::on_tabWidget_currentChanged(int index)
{
    if (index != 1)
        return;

    on_scanTextures_clicked();
}

void Widget::on_pushButton_2_clicked()
{

}


