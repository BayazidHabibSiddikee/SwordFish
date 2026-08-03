// utils/integrity.cpp — SHA-256 file integrity checking
// Equivalent of the Python utils/integrity.py

#include "integrity.h"

#include <QFile>
#include <QCryptographicHash>

namespace Integrity {

QString generateFileHash(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QString();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    constexpr qint64 chunkSize = 8192;
    QByteArray chunk;
    while (!(chunk = file.read(chunkSize)).isEmpty()) {
        hash.addData(chunk);
    }
    return QString::fromLatin1(hash.result().toHex());
}

bool checkIntegrity(const QStringList &filePaths, const QStringList &expectedHashes) {
    if (filePaths.size() != expectedHashes.size()) return false;
    for (int i = 0; i < filePaths.size(); ++i) {
        QString actual = generateFileHash(filePaths.at(i));
        if (actual.isEmpty() || actual != expectedHashes.at(i))
            return false;
    }
    return true;
}

}  // namespace Integrity
