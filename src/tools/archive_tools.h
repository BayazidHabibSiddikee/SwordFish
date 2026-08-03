#pragma once

#include <QString>
#include <QStringList>

namespace ArchiveTools {
    QString zipFiles(const QStringList &filePaths, const QString &outputPath);
    QString unzipFile(const QString &zipPath, const QString &extractTo);
    QString tarFiles(const QStringList &filePaths, const QString &outputPath);
    QString untarFile(const QString &tarPath, const QString &extractTo);
    QString sevenZipFiles(const QStringList &filePaths, const QString &outputPath);
    QString unSevenZipFile(const QString &sevenZipPath, const QString &extractTo);
}
