#ifndef RP5L_STRUCTURE_H
#define RP5L_STRUCTURE_H

#include <QMap>
#include <QList>
#include <QString>

struct header {
    quint32 magic;
    quint32 version;
    quint32 compression;
    quint32 dataSize;
    quint32 sectionCount;
    quint32 filesCount;
    quint32 filenamesSize;
    quint32 filenamesCount;
    quint32 blockSize;
};

struct section {
    quint8 type1;
    quint16 unk1;
    quint8 type2;
    quint32 offset;
    quint32 unpackedSize;
    quint32 packedSize;
    quint32 packsCount;
};

struct files {
    quint32 sectionNumber;
    quint32 fileIndex;
    quint32 offset;
    quint32 unpackedSize;
    quint32 packedSize;
};

struct unknown1 {
    quint8 type1;
    quint16 unk1;
    quint8 type2;
    quint32 fileIndex;
};

struct filenames {
    quint32 fnPtr;
    QString filename;
};

#endif // RP5L_STRUCTURE_H
