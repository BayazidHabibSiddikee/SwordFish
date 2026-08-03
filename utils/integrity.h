#pragma once
// utils/integrity.h — SHA-256 file integrity checking
// Equivalent of the Python utils/integrity.py

#include <QString>
#include <QStringList>

namespace Integrity {

    /**
     * Generate the SHA-256 hex digest of the file at filePath.
     * Returns an empty string if the file cannot be read.
     */
    QString generateFileHash(const QString &filePath);

    /**
     * Check that every file in filePaths matches the corresponding entry
     * in expectedHashes (SHA-256 hex strings).
     * Returns true only if ALL files match their expected hash.
     */
    bool checkIntegrity(const QStringList &filePaths, const QStringList &expectedHashes);

}  // namespace Integrity
