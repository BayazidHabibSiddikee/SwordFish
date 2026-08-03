#include "pdf_tools.h"
#include "tool_check.h"
#include <QProcess>
#include <QTemporaryFile>
#include <stdexcept>

namespace PdfTools {

QString mergeDocuments(const QStringList &pdfList, const QString &outputFilename) {
    if (!sfToolExists("qpdf"))
        throw std::runtime_error("qpdf not installed. Install: sudo apt install qpdf");
    if (pdfList.size() < 2) {
        throw std::runtime_error("Need at least 2 PDFs to merge");
    }

    // Use qpdf for merging (widely available)
    QStringList args;
    args << "--empty" << "--pages";
    for (const auto &pdf : pdfList) {
        args << pdf;
    }
    args << "--" << outputFilename;

    QProcess proc;
    proc.start("qpdf", args);
    proc.waitForFinished(60000);

    if (proc.exitCode() != 0) {
        throw std::runtime_error("qpdf merge failed: " + proc.readAllStandardError().toStdString());
    }

    return outputFilename;
}

} // namespace PdfTools
