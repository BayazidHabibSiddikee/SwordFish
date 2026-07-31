#include "office_tools.h"
#include "tool_check.h"
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <stdexcept>

namespace OfficeTools {

QString xlsxToPdf(const QString &xlsxPath, const QString &pdfPath) {
    if (!sfToolExists("libreoffice"))
        throw std::runtime_error("libreoffice not installed. Install: sudo apt install libreoffice");
    QString out = pdfPath.isEmpty() ?
        QFileInfo(xlsxPath).completeBaseName() + ".pdf" : pdfPath;

    QProcess proc;
    proc.start("libreoffice", QStringList() << "--headless" << "--convert-to" << "pdf"
               << "--outdir" << QFileInfo(out).absolutePath() << xlsxPath);
    proc.waitForFinished(60000);
    return out;
}

QString pdfToXlsx(const QString &pdfPath, const QString &xlsxPath) {
    QString out = xlsxPath.isEmpty() ?
        QFileInfo(pdfPath).completeBaseName() + ".xlsx" : xlsxPath;

    // Use tabula-py or python script for table extraction
    QProcess proc;
    proc.start("python3", QStringList() << "-c"
        << QString("import pdfplumber, openpyxl; "
                   "wb = openpyxl.Workbook(); ws = wb.active; "
                   "with pdfplumber.open('%1') as pdf: "
                   "  for i, page in enumerate(pdf.pages): "
                   "    table = page.extract_table(); "
                   "    if table: "
                   "      for row in table: "
                   "        ws.append([str(c) if c else '' for c in row]); "
                   "wb.save('%2')")
        .arg(pdfPath).arg(out));
    proc.waitForFinished(120000);
    return out;
}

QString pptxToPdf(const QString &pptxPath, const QString &pdfPath) {
    if (!sfToolExists("libreoffice"))
        throw std::runtime_error("libreoffice not installed. Install: sudo apt install libreoffice");
    QString out = pdfPath.isEmpty() ?
        QFileInfo(pptxPath).completeBaseName() + ".pdf" : pdfPath;

    QProcess proc;
    proc.start("libreoffice", QStringList() << "--headless" << "--convert-to" << "pdf"
               << "--outdir" << QFileInfo(out).absolutePath() << pptxPath);
    proc.waitForFinished(60000);
    return out;
}

QString pdfToPptx(const QString &pdfPath, const QString &pptxPath) {
    if (!sfToolExists("libreoffice"))
        throw std::runtime_error("libreoffice not installed. Install: sudo apt install libreoffice");
    QString out = pptxPath.isEmpty() ?
        QFileInfo(pdfPath).completeBaseName() + ".pptx" : pptxPath;

    QProcess proc;
    proc.start("libreoffice", QStringList() << "--headless" << "--convert-to" << "pptx"
               << "--outdir" << QFileInfo(out).absolutePath() << pdfPath);
    proc.waitForFinished(60000);
    return out;
}

QString csvToXlsx(const QString &csvPath, const QString &xlsxPath) {
    if (!sfToolExists("python3"))
        throw std::runtime_error("python3 not installed");
    QString out = xlsxPath.isEmpty() ?
        QFileInfo(csvPath).completeBaseName() + ".xlsx" : xlsxPath;

    QProcess proc;
    proc.start("python3", QStringList() << "-c"
        << QString("import pandas as pd; "
                   "pd.read_csv('%1').to_excel('%2', index=False)")
        .arg(csvPath).arg(out));
    proc.waitForFinished(30000);
    return out;
}

QString xlsxToCsv(const QString &xlsxPath, const QString &csvPath) {
    if (!sfToolExists("python3"))
        throw std::runtime_error("python3 not installed");
    QString out = csvPath.isEmpty() ?
        QFileInfo(xlsxPath).completeBaseName() + ".csv" : csvPath;

    QProcess proc;
    proc.start("python3", QStringList() << "-c"
        << QString("import pandas as pd; "
                   "pd.read_excel('%1').to_csv('%2', index=False)")
        .arg(xlsxPath).arg(out));
    proc.waitForFinished(30000);
    return out;
}

QStringList pdfToImage(const QString &pdfPath, const QString &outputDir,
                       const QString &fmt, int dpi) {
    if (!sfToolExists("pdftoppm") && !sfToolExists("convert"))
        throw std::runtime_error("pdftoppm or ImageMagick not installed. Install: sudo apt install poppler-utils");
    QString dir = outputDir.isEmpty() ? QFileInfo(pdfPath).absolutePath() : outputDir;
    QDir().mkpath(dir);
    QString base = QFileInfo(pdfPath).baseName();
    QStringList paths;

    // Use pdftoppm for conversion
    QString prefix = dir + "/" + base + "_page_";
    QProcess proc;
    proc.start("pdftoppm", QStringList() << "-r" << QString::number(dpi)
               << "-png" << pdfPath << prefix);
    proc.waitForFinished(120000);

    // Collect generated files
    QDir d(dir);
    QFileInfoList files = d.entryInfoList(QStringList() << base + "_page_*." + fmt, QDir::Files);
    for (const auto &f : files) {
        paths.append(f.absoluteFilePath());
    }

    // Fallback: use ImageMagick
    if (paths.isEmpty()) {
        QProcess proc2;
        proc2.start("convert", QStringList() << "-density" << QString::number(dpi)
                    << pdfPath << prefix + "%d." + fmt);
        proc2.waitForFinished(120000);
        files = d.entryInfoList(QStringList() << base + "_page_*." + fmt, QDir::Files);
        for (const auto &f : files) {
            paths.append(f.absoluteFilePath());
        }
    }

    return paths;
}

QString pdfToText(const QString &pdfPath, const QString &outputPath) {
    QString out = outputPath.isEmpty() ?
        QFileInfo(pdfPath).completeBaseName() + ".txt" : outputPath;

    if (!sfToolExists("pdftotext") && !sfToolExists("python3"))
        throw std::runtime_error("pdftotext not installed. Install: sudo apt install poppler-utils");

    QProcess proc;
    proc.start("pdftotext", QStringList() << pdfPath << out);
    proc.waitForFinished(30000);

    if (proc.exitCode() != 0) {
        // Fallback: use python
        QProcess proc2;
        proc2.start("python3", QStringList() << "-c"
            << QString("import fitz; doc = fitz.open('%1'); "
                       "text = ''.join(page.get_text() for page in doc); "
                       "doc.close(); "
                       "with open('%2', 'w', encoding='utf-8') as f: f.write(text)")
            .arg(pdfPath).arg(out));
        proc2.waitForFinished(30000);
    }

    return out;
}

} // namespace OfficeTools
