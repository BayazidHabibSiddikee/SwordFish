#pragma once

#include <QString>
#include <QMap>

namespace TranslateTools {
    QString translateText(const QString &text, const QString &destLang);
    QMap<QString, QString> languageCodes();
}
