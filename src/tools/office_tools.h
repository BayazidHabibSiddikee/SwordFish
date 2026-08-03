#pragma once

#include <QString>
#include <QStringList>

namespace OfficeTools {
    QString xlsxToPdf(const QString &xlsxPath, const QString &pdfPath = QString());
    QString pdfToXlsx(const QString &pdfPath, const QString &xlsxPath = QString());
    QString pptxToPdf(const QString &pptxPath, const QString &pdfPath = QString());
    QString pdfToPptx(const QString &pdfPath, const QString &pptxPath = QString());
    QString csvToXlsx(const QString &csvPath, const QString &xlsxPath = QString());
    QString xlsxToCsv(const QString &xlsxPath, const QString &csvPath = QString());
    QStringList pdfToImage(const QString &pdfPath, const QString &outputDir = QString(),
                           const QString &fmt = "png", int dpi = 200);
    QString pdfToText(const QString &pdfPath, const QString &outputPath = QString());
}
