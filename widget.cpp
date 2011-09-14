#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

#ifdef Q_WS_WIN
    QSettings reg("HKEY_CURRENT_USER\\Software\\Valve\\Steam", QSettings::NativeFormat);
    diPath = reg.value("SteamPath").toString();
    diPath += "/steamapps/common/dead island";
    if (QFile::exists(diPath + "/DeadIslandGame.exe")) {
        diPath += "/DI/Data";
    } else {
        diPath = "";
    }
#endif
    fillRpacksList();
}

Widget::~Widget()
{
    delete ui;
}

// private slots

void Widget::on_toolButton_clicked()
{
    QString oldDiPath = diPath;
    diPath = QFileDialog::getExistingDirectory(this, "Select rpacks catalog", oldDiPath);
    fillRpacksList();
}

void Widget::on_rpacksList_currentRowChanged(int currentRow)
{
    currentRpack = ui->rpacksList->item(currentRow)->text();
    currentRpack = diPath + "/" + currentRpack;
    QFile rpack(currentRpack);
    if (!rpack.open(QIODevice::ReadOnly))
        return;
    QDataStream in(&rpack);
    in.setByteOrder(QDataStream::LittleEndian);

    ui->rpacksFiles->clear();
    QListWidget *c = ui->rpacksContent;
    c->clear();

    in >> h.magic;
    if (h.magic != 1278562386) {    // RP5L
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

    section s;
    for (quint32 i = 0; i < h.sectionCount; ++i) {
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
    }

    rpack.close();
}

void Widget::on_scanRpack_clicked()
{
    QFile rpack(currentRpack);
    if (!rpack.open(QIODevice::ReadOnly))
        return;
    QDataStream in(&rpack);
    in.setByteOrder(QDataStream::LittleEndian);

    rpack.seek(36);                                     // size of header
    rpack.seek(rpack.pos() + (h.sectionCount * 4 * 5));     // size of sections
    rpack.seek(rpack.pos() + (h.dataSize * 4 * 4));     // skip files section
    rpack.seek(rpack.pos() + (h.filesCount * 4 * 3));   // skip unknown section
    rpack.seek(rpack.pos() + (h.filesCount * 4 * 1));   // skip filename size section

    // read filenames
    QByteArray ba;
    ba.resize(h.filenamesSize);
    char *data = ba.data();
    in.readRawData(data, h.filenamesSize);
    QList<QByteArray> fn;
    fn = ba.split(0);
    if (!fn.isEmpty())
        fn.removeLast();

    ui->rpacksFiles->clear();
    for (quint32 i = 0; i < h.filenamesCount; ++i) {
        ui->rpacksFiles->addItem(QString(fn.at(i)));
    }

    rpack.close();
}

// private

void Widget::fillRpacksList()
{
    ui->lbDiCatalog->setText(diPath);
    ui->rpacksList->clear();
    QDir dir;
    dir.setCurrent(diPath);
    QStringList rpacks;
    rpacks << dir.entryList(QStringList("*.rpack"), QDir::Files);
    ui->rpacksList->addItems(rpacks);
}

