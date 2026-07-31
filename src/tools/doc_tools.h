#pragma once

#include <QString>
#include <QStringList>

namespace DocTools {
    QString wordToPdf(const QString &docxPath, const QString &pdfPath = QString());
    QString pdfToWord(const QString &pdfPath, const QString &docxPath = QString());
    QString imageToPdf(const QStringList &imagePaths, const QString &pdfPath = "output.pdf");
    QStringList splitPdf(const QString &pdfPath, const QString &outputDir = QString());
    QString textToPdf(const QString &text, const QString &pdfPath = "output.pdf");
}
