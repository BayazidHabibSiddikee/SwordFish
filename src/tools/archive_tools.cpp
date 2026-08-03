#include "archive_tools.h"
#include "tool_check.h"
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <stdexcept>

namespace ArchiveTools {

QString zipFiles(const QStringList &filePaths, const QString &outputPath) {
    if (!sfToolExists("zip"))
        throw std::runtime_error("zip not installed. Install: sudo apt install zip");
    QStringList args;
    args << "-j" << outputPath;
    for (const auto &f : filePaths) {
        args << f;
    }

    QProcess proc;
    proc.start("zip", args);
    proc.waitForFinished(60000);

    if (proc.exitCode() != 0) {
        throw std::runtime_error("zip failed: " + proc.readAllStandardError().toStdString());
    }
    return outputPath;
}

QString unzipFile(const QString &zipPath, const QString &extractTo) {
    if (!sfToolExists("unzip"))
        throw std::runtime_error("unzip not installed. Install: sudo apt install unzip");
    QDir().mkpath(extractTo);

    QProcess proc;
    proc.start("unzip", QStringList() << "-o" << zipPath << "-d" << extractTo);
    proc.waitForFinished(60000);

    if (proc.exitCode() != 0) {
        throw std::runtime_error("unzip failed: " + proc.readAllStandardError().toStdString());
    }
    return extractTo;
}

QString tarFiles(const QStringList &filePaths, const QString &outputPath) {
    QStringList args;
    args << "-czf" << outputPath;
    for (const auto &f : filePaths) {
        args << QFileInfo(f).fileName();
    }

    QProcess proc;
    proc.setWorkingDirectory(QFileInfo(filePaths.first()).absolutePath());
    proc.start("tar", args);
    proc.waitForFinished(60000);

    if (proc.exitCode() != 0) {
        throw std::runtime_error("tar failed: " + proc.readAllStandardError().toStdString());
    }
    return outputPath;
}

QString untarFile(const QString &tarPath, const QString &extractTo) {
    QDir().mkpath(extractTo);

    QProcess proc;
    proc.start("tar", QStringList() << "-xzf" << tarPath << "-C" << extractTo);
    proc.waitForFinished(60000);

    if (proc.exitCode() != 0) {
        throw std::runtime_error("untar failed: " + proc.readAllStandardError().toStdString());
    }
    return extractTo;
}

QString sevenZipFiles(const QStringList &filePaths, const QString &outputPath) {
    if (!sfToolExists("7z"))
        throw std::runtime_error("7z not installed. Install: sudo apt install p7zip-full");
    QStringList args;
    args << "a" << outputPath;
    for (const auto &f : filePaths) {
        args << f;
    }

    QProcess proc;
    proc.start("7z", args);
    proc.waitForFinished(60000);

    if (proc.exitCode() != 0) {
        throw std::runtime_error("7z failed: " + proc.readAllStandardError().toStdString());
    }
    return outputPath;
}

QString unSevenZipFile(const QString &sevenZipPath, const QString &extractTo) {
    if (!sfToolExists("7z"))
        throw std::runtime_error("7z not installed. Install: sudo apt install p7zip-full");
    QDir().mkpath(extractTo);

    QProcess proc;
    proc.start("7z", QStringList() << "x" << sevenZipPath << "-o" + extractTo);
    proc.waitForFinished(60000);

    if (proc.exitCode() != 0) {
        throw std::runtime_error("7z extract failed: " + proc.readAllStandardError().toStdString());
    }
    return extractTo;
}

} // namespace ArchiveTools
