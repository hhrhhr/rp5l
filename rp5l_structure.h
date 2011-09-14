#ifndef RP5L_STRUCTURE_H
#define RP5L_STRUCTURE_H

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

struct fileset {
    quint16 sectionNumber;
    quint16 fileIndex;
    quint32 offset;
    quint32 unpackedSize;
    quint32 packedSize;
};

struct unknown {
    quint16 type1;
    quint16 type2;
    quint32 fileIndex;
    quint32 unk1;
};

struct fnPtr {
    quint32 fnPtr;
};

struct filename {
    QString name;
};

#endif // RP5L_STRUCTURE_H
