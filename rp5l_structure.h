#ifndef RP5L_STRUCTURE_H
#define RP5L_STRUCTURE_H

#include <Qt>
#include <QByteArray>

struct header {
    quint32 magic;              // RP5L
    quint32 version;            // 36
    quint32 compression;        // 1-zlib, 2-???
    quint32 partsCount;         //
    quint32 sectionCount;       //
    quint32 filesCount;         //
    quint32 filenamesSize;      //
    quint32 filenamesCount;     //
    quint32 blockSize;          // 2048
};

struct section {
    quint8 filetype;            //
    quint8 type2;               // 0 (sometime 6)
    quint8 type3;               // 0
    quint8 type4;               //
    quint32 offset;             // absolute
    quint32 unpackedSize;       // (without 0x00)
    quint32 packedSize;         // (without 0x00)
    quint32 partsCount;         //
};

struct filepart {
    quint8 sectionIndex;        // 0 ... header.sectionCount-1
    quint8 unk1;                //
    quint16 fileIndex;          //
    quint32 offset;             // relative to section.offset
    quint32 unpackedSize;       //
    quint32 packedSize;         // 0-not packed
};

struct filemap {
    quint8 partsCount;          // parts count
    quint8 unk1;                // 0
    quint8 filetype;            // file type
    quint8 unk2;                // ???
    quint32 fileIndex;          //
    quint32 firstPart;          // start part
};

// not used
struct rp5l {
    header      head;
    section     sect[];
    filepart    part[];
    filemap     map[];
    quint32     fnptr[];
    QString     name[];
};

#endif // RP5L_STRUCTURE_H
