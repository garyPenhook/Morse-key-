#ifndef MORSETABLE_H
#define MORSETABLE_H

#include <QString>
#include <QMap>

class MorseTable {
public:
    MorseTable();

    // Decode morse pattern (e.g., ".-") to character
    QChar decode(const QString& pattern) const;

    // Encode character to morse pattern
    QString encode(QChar character) const;

    // Check if pattern exists
    bool isValidPattern(const QString& pattern) const;

private:
    void initializeTables();

    QMap<QString, QChar> m_morseToChar;
    QMap<QChar, QString> m_charToMorse;
};

#endif // MORSETABLE_H
