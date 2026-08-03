#include "translate.h"
#include "tool_check.h"
#include <QProcess>

namespace TranslateTools {

QMap<QString, QString> languageCodes() {
    return {
        {"english", "en"}, {"chinese", "zh"}, {"spanish", "es"},
        {"french", "fr"}, {"japanese", "ja"}, {"portuguese", "pt"},
        {"russian", "ru"}, {"korean", "ko"}, {"german", "de"},
        {"italian", "it"}, {"bangla", "bn"}, {"arabic", "ar"},
        {"hindi", "hi"}, {"turkish", "tr"}, {"dutch", "nl"},
    };
}

QString translateText(const QString &text, const QString &destLang) {
    if (!sfToolExists("python3"))
        return "Translation requires python3. Install python3 to use this feature.";
    // Use python deep-translator or googletrans
    QProcess proc;
    QString escapedText = text;
    escapedText.replace("'", "\\'");
    escapedText.replace("\n", " ");
    proc.start("python3", QStringList() << "-c"
        << QString("from deep_translator import GoogleTranslator; "
                   "print(GoogleTranslator(source='en', target='%1').translate('%2'))")
        .arg(destLang).arg(escapedText));
    proc.waitForFinished(15000);

    if (proc.exitCode() == 0) {
        QString result = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!result.isEmpty()) return result;
    }

    // Fallback: return error message
    return "Translation service unavailable.";
}

} // namespace TranslateTools
