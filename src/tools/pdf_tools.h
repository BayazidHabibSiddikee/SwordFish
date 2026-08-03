#pragma once

#include <QString>
#include <QStringList>

namespace PdfTools {
    QString mergeDocuments(const QStringList &pdfList, const QString &outputFilename = "merged.pdf");
}
