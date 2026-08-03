#pragma once

#include <QString>
#include <QMap>

namespace StudentTools {
    QString generateQr(const QString &text, const QString &outputPath = "qrcode.png", int boxSize = 10);
    double convertUnit(double value, const QString &fromUnit, const QString &toUnit,
                       const QString &category = QString());
    QString calculate(const QString &expression);
    QString programmerCalc(const QString &value, const QString &fromBase, const QString &toBase);
    QString saveNote(const QString &text, const QString &filepath = QString());
}
