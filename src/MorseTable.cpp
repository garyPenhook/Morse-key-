#include "MorseTable.h"

MorseTable::MorseTable() {
    initializeTables();
}

void MorseTable::initializeTables() {
    // Letters
    m_morseToChar[".-"] = 'A';
    m_morseToChar["-..."] = 'B';
    m_morseToChar["-.-."] = 'C';
    m_morseToChar["-.."] = 'D';
    m_morseToChar["."] = 'E';
    m_morseToChar["..-."] = 'F';
    m_morseToChar["--."] = 'G';
    m_morseToChar["...."] = 'H';
    m_morseToChar[".."] = 'I';
    m_morseToChar[".---"] = 'J';
    m_morseToChar["-.-"] = 'K';
    m_morseToChar[".-.."] = 'L';
    m_morseToChar["--"] = 'M';
    m_morseToChar["-."] = 'N';
    m_morseToChar["---"] = 'O';
    m_morseToChar[".--."] = 'P';
    m_morseToChar["--.-"] = 'Q';
    m_morseToChar[".-."] = 'R';
    m_morseToChar["..."] = 'S';
    m_morseToChar["-"] = 'T';
    m_morseToChar["..-"] = 'U';
    m_morseToChar["...-"] = 'V';
    m_morseToChar[".--"] = 'W';
    m_morseToChar["-..-"] = 'X';
    m_morseToChar["-.--"] = 'Y';
    m_morseToChar["--.."] = 'Z';

    // Numbers
    m_morseToChar["-----"] = '0';
    m_morseToChar[".----"] = '1';
    m_morseToChar["..---"] = '2';
    m_morseToChar["...--"] = '3';
    m_morseToChar["....-"] = '4';
    m_morseToChar["....."] = '5';
    m_morseToChar["-...."] = '6';
    m_morseToChar["--..."] = '7';
    m_morseToChar["---.."] = '8';
    m_morseToChar["----."] = '9';

    // Punctuation
    m_morseToChar[".-.-.-"] = '.';
    m_morseToChar["--..--"] = ',';
    m_morseToChar["..--.."] = '?';
    m_morseToChar[".----."] = '\'';
    m_morseToChar["-.-.--"] = '!';
    m_morseToChar["-..-."] = '/';
    m_morseToChar["-.--."] = '(';
    m_morseToChar["-.--.-"] = ')';
    m_morseToChar[".-..."] = '&';
    m_morseToChar["---..."] = ':';
    m_morseToChar["-.-.-."] = ';';
    m_morseToChar["-...-"] = '=';
    m_morseToChar[".-.-."] = '+';
    m_morseToChar["-....-"] = '-';
    m_morseToChar["..--.-"] = '_';
    m_morseToChar[".-..-."] = '"';
    m_morseToChar["...-..-"] = '$';
    m_morseToChar[".--.-."] = '@';

    // Build reverse mapping
    for (auto it = m_morseToChar.begin(); it != m_morseToChar.end(); ++it) {
        m_charToMorse[it.value()] = it.key();
    }
}

QChar MorseTable::decode(const QString& pattern) const {
    if (m_morseToChar.contains(pattern)) {
        return m_morseToChar[pattern];
    }
    return QChar(); // Invalid pattern
}

QString MorseTable::encode(QChar character) const {
    QChar upper = character.toUpper();
    if (m_charToMorse.contains(upper)) {
        return m_charToMorse[upper];
    }
    return QString(); // Invalid character
}

bool MorseTable::isValidPattern(const QString& pattern) const {
    return m_morseToChar.contains(pattern);
}
