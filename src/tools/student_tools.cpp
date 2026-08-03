#include "student_tools.h"
#include "tool_check.h"
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <cmath>
#include <stdexcept>
#include <sstream>

namespace StudentTools {

QString generateQr(const QString &text, const QString &outputPath, int boxSize) {
    if (!sfToolExists("qrencode"))
        throw std::runtime_error("qrencode not installed. Install: sudo apt install qrencode");
    Q_UNUSED(boxSize);
    QProcess proc;
    proc.start("qrencode", QStringList() << "-o" << outputPath
               << "-s" << "10" << "-m" << "4" << text);
    proc.waitForFinished(30000);

    if (proc.exitCode() != 0) {
        throw std::runtime_error("QR generation failed: " + proc.readAllStandardError().toStdString());
    }
    return outputPath;
}

double convertUnit(double value, const QString &fromUnit, const QString &toUnit,
                   const QString &category) {
    QMap<QString, QMap<QString, double>> conversions = {
        {"length", {
            {"meter", 1.0}, {"kilometer", 1000.0}, {"centimeter", 0.01},
            {"millimeter", 0.001}, {"mile", 1609.344}, {"yard", 0.9144},
            {"foot", 0.3048}, {"inch", 0.0254}
        }},
        {"weight", {
            {"kilogram", 1.0}, {"gram", 0.001}, {"milligram", 0.000001},
            {"pound", 0.453592}, {"ounce", 0.0283495}, {"ton", 1000.0}
        }},
        {"data", {
            {"byte", 1}, {"kilobyte", 1024}, {"megabyte", 1048576},
            {"gigabyte", 1073741824}, {"terabyte", 1099511627776.0}
        }},
        {"speed", {
            {"m/s", 1.0}, {"km/h", 0.277778}, {"mph", 0.44704},
            {"knot", 0.514444}
        }},
        {"area", {
            {"sq_meter", 1.0}, {"sq_kilometer", 1000000.0},
            {"sq_mile", 2589988.11}, {"sq_yard", 0.836127},
            {"sq_foot", 0.092903}, {"acre", 4046.86}, {"hectare", 10000.0}
        }},
        {"volume", {
            {"liter", 1.0}, {"milliliter", 0.001}, {"gallon", 3.78541},
            {"quart", 0.946353}, {"pint", 0.473176}, {"cup", 0.236588},
            {"cubic_meter", 1000.0}
        }},
    };

    // Temperature is special
    if (fromUnit == "celsius" || fromUnit == "fahrenheit" || fromUnit == "kelvin") {
        if (fromUnit == toUnit) return value;
        double kelvin;
        if (fromUnit == "celsius") kelvin = value + 273.15;
        else if (fromUnit == "fahrenheit") kelvin = (value - 32) * 5.0 / 9.0 + 273.15;
        else kelvin = value;

        if (toUnit == "celsius") return kelvin - 273.15;
        else if (toUnit == "fahrenheit") return (kelvin - 273.15) * 9.0 / 5.0 + 32;
        else return kelvin;
    }

    // Find category
    QString cat = category;
    if (cat.isEmpty()) {
        for (auto it = conversions.begin(); it != conversions.end(); ++it) {
            if (it.value().contains(fromUnit) && it.value().contains(toUnit)) {
                cat = it.key();
                break;
            }
        }
    }

    if (cat.isEmpty() || !conversions.contains(cat))
        throw std::runtime_error("Unknown units or category");

    auto &units = conversions[cat];
    if (!units.contains(fromUnit) || !units.contains(toUnit))
        throw std::runtime_error("Unknown unit in category");

    double base = value * units[fromUnit];
    return base / units[toUnit];
}

QString calculate(const QString &expression) {
    // Safe expression evaluator using QProcess + python
    QString escapedExpr = expression;
    escapedExpr.replace("'", "\\'");
    QProcess proc;
    proc.start("python3", QStringList() << "-c"
        << QString("expr = '%1'; "
                   "allowed = set('0123456789+-*/.()% '); "
                   "if not all(c in allowed for c in expr): "
                   "  print('Invalid characters') "
                   "else: "
                   "  try: print(eval(expr)) "
                   "  except Exception as e: print(f'Error: {e}')")
        .arg(escapedExpr));
    proc.waitForFinished(5000);
    return proc.readAllStandardOutput().trimmed();
}

QString programmerCalc(const QString &value, const QString &fromBase, const QString &toBase) {
    bool ok;
    long long val;
    if (fromBase == "dec") val = value.toLongLong(&ok);
    else if (fromBase == "bin") val = value.toLongLong(&ok, 2);
    else if (fromBase == "hex") val = value.toLongLong(&ok, 16);
    else if (fromBase == "oct") val = value.toLongLong(&ok, 8);
    else return "Invalid base";

    if (!ok) return "Error: invalid value";

    if (toBase == "dec") return QString::number(val);
    else if (toBase == "bin") return QString::number(val, 2);
    else if (toBase == "hex") return QString::number(val, 16).toUpper();
    else if (toBase == "oct") return QString::number(val, 8);
    return "Invalid base";
}

QString saveNote(const QString &text, const QString &filepath) {
    QString path = filepath;
    if (path.isEmpty()) {
        path = QString("note_%1.txt")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    }
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << text;
        file.close();
    }
    return path;
}

} // namespace StudentTools
