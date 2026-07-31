#include "doc_tools.h"
#include "tool_check.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <stdexcept>

namespace DocTools {

QString wordToPdf(const QString &docxPath, const QString &pdfPath) {
    if (!sfToolExists("libreoffice"))
        throw std::runtime_error("libreoffice not installed. Install: sudo apt install libreoffice");
    QString out = pdfPath.isEmpty() ?
        QFileInfo(docxPath).completeBaseName() + ".pdf" : pdfPath;

    // Use LibreOffice for conversion
    QProcess proc;
    proc.start("libreoffice", QStringList() << "--headless" << "--convert-to" << "pdf"
               << "--outdir" << QFileInfo(out).absolutePath() << docxPath);
    proc.waitForFinished(60000);

    if (proc.exitCode() != 0) {
        throw std::runtime_error("Word to PDF conversion failed");
    }
    return out;
}

QString pdfToWord(const QString &pdfPath, const QString &docxPath) {
    if (!sfToolExists("libreoffice"))
        throw std::runtime_error("libreoffice not installed. Install: sudo apt install libreoffice");
    QString out = docxPath.isEmpty() ?
        QFileInfo(pdfPath).completeBaseName() + ".docx" : docxPath;

    QProcess proc;
    proc.start("libreoffice", QStringList() << "--headless" << "--convert-to" << "docx"
               << "--outdir" << QFileInfo(out).absolutePath() << pdfPath);
    proc.waitForFinished(60000);

    if (proc.exitCode() != 0) {
        throw std::runtime_error("PDF to Word conversion failed");
    }
    return out;
}

QString imageToPdf(const QStringList &imagePaths, const QString &pdfPath) {
    if (!sfToolExists("img2pdf") && !sfToolExists("convert"))
        throw std::runtime_error("img2pdf or ImageMagick not installed. Install: sudo apt install img2pdf");
    QStringList args;
    args << "--layout" << "--pagesize" << "A4";
    for (const auto &img : imagePaths) {
        args << img;
    }
    args << pdfPath;

    QProcess proc;
    proc.start("img2pdf", args);
    proc.waitForFinished(60000);

    // Fallback: use ImageMagick convert
    if (proc.exitCode() != 0) {
        QStringList imgArgs;
        for (const auto &img : imagePaths) {
            imgArgs << img;
        }
        imgArgs << pdfPath;
        QProcess proc2;
        proc2.start("convert", imgArgs);
        proc2.waitForFinished(120000);
    }
    return pdfPath;
}

QStringList splitPdf(const QString &pdfPath, const QString &outputDir) {
    if (!sfToolExists("qpdf"))
        throw std::runtime_error("qpdf not installed. Install: sudo apt install qpdf");
    QString dir = outputDir.isEmpty() ? QFileInfo(pdfPath).absolutePath() : outputDir;
    QDir().mkpath(dir);
    QString base = QFileInfo(pdfPath).baseName();
    QStringList paths;

    // Use qpdf to split
    QString pattern = dir + "/" + base + "_page_%d.pdf";
    QProcess proc;
    proc.start("qpdf", QStringList() << pdfPath
               << "--pages" << pdfPath << "--" << pattern);
    proc.waitForFinished(60000);

    // Collect generated files
    QDir d(dir);
    QStringList filters;
    filters << base + "_page_*.pdf";
    QFileInfoList files = d.entryInfoList(filters, QDir::Files);
    for (const auto &f : files) {
        paths.append(f.absoluteFilePath());
    }

    return paths;
}

QString textToPdf(const QString &text, const QString &pdfPath) {
    if (!sfToolExists("enscript") && !sfToolExists("ps2pdf"))
        throw std::runtime_error("enscript/ghostscript not installed. Install: sudo apt install enscript ghostscript");
    // Use enscript + ps2pdf, or write a simple text file
    QString tmpTxt = QDir::tempPath() + "/sf_text_to_pdf.txt";
    QFile tmpFile(tmpTxt);
    if (tmpFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        tmpFile.write(text.toUtf8());
        tmpFile.close();
    }

    QProcess proc;
    proc.start("enscript", QStringList() << "-p" << tmpTxt + ".ps" << tmpTxt);
    proc.waitForFinished(30000);

    if (proc.exitCode() == 0) {
        QProcess ps2pdf;
        ps2pdf.start("ps2pdf", QStringList() << tmpTxt + ".ps" << pdfPath);
        ps2pdf.waitForFinished(30000);
        QFile::remove(tmpTxt + ".ps");
    } else {
        // Fallback: write plain text as PDF using ps2pdf
        QStringList args;
        args << "-dNOPAUSE" << "-dBATCH" << "-sDEVICE=pdfwrite"
             << QString("-sOutputFile=%1").arg(pdfPath)
             << tmpTxt;
        QProcess::startDetached("enscript", args);
    }

    QFile::remove(tmpTxt);
    return pdfPath;
}

} // namespace DocTools
