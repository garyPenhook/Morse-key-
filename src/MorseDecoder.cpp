#include "MorseDecoder.h"

MorseDecoder::MorseDecoder(QObject *parent)
    : QObject(parent)
    , m_keyIsDown(false)
    , m_wpm(20)
    , m_ditAvg(60)  // Initial estimate at 20 WPM
    , m_dahAvg(180)
    , m_sampleCount(0)
{
    connect(&m_characterTimer, &QTimer::timeout, this, &MorseDecoder::onCharacterTimeout);
    connect(&m_wordTimer, &QTimer::timeout, this, &MorseDecoder::onWordTimeout);

    m_characterTimer.setSingleShot(true);
    m_wordTimer.setSingleShot(true);
}

void MorseDecoder::setWpm(int wpm) {
    m_wpm = qBound(5, wpm, 50);
    qint64 unit = calculateUnitTime();
    m_ditAvg = unit;
    m_dahAvg = unit * 3;
}

qint64 MorseDecoder::calculateUnitTime() const {
    // PARIS standard: "PARIS" = 50 units
    // At X WPM, we send X times "PARIS" per minute
    // So unit time = 60000ms / (50 * WPM)
    return 60000 / (50 * m_wpm);
}

void MorseDecoder::reset() {
    m_currentPattern.clear();
    m_characterTimer.stop();
    m_wordTimer.stop();
    m_keyIsDown = false;
    m_sampleCount = 0;

    qint64 unit = calculateUnitTime();
    m_ditAvg = unit;
    m_dahAvg = unit * 3;
}

void MorseDecoder::keyDown() {
    if (m_keyIsDown) return;

    m_keyIsDown = true;
    m_keyTimer.start();
    m_characterTimer.stop();
    m_wordTimer.stop();
}

void MorseDecoder::keyUp() {
    if (!m_keyIsDown) return;

    m_keyIsDown = false;
    qint64 duration = m_keyTimer.elapsed();
    processKeyDuration(duration);

    // Start timers for character and word boundaries
    qint64 unit = (m_ditAvg + m_dahAvg / 3) / 2;
    m_characterTimer.start(unit * 3);  // Character gap = 3 units
    m_wordTimer.start(unit * 7);       // Word gap = 7 units
}

void MorseDecoder::processKeyDuration(qint64 duration) {
    // Determine if dit or dah based on threshold
    qint64 threshold = (m_ditAvg + m_dahAvg) / 2;
    bool isDit = duration < threshold;

    // Update adaptive timing
    updateTimingAverages(duration, isDit);

    // Add to pattern
    if (isDit) {
        m_currentPattern += ".";
        emit elementDecoded(".");
    } else {
        m_currentPattern += "-";
        emit elementDecoded("-");
    }
}

void MorseDecoder::processElement(bool isDit) {
    // For character mode where device sends elements directly
    if (isDit) {
        m_currentPattern += ".";
        emit elementDecoded(".");
    } else {
        m_currentPattern += "-";
        emit elementDecoded("-");
    }

    // Reset character timeout
    qint64 unit = calculateUnitTime();
    m_characterTimer.start(unit * 3);
    m_wordTimer.start(unit * 7);
}

void MorseDecoder::updateTimingAverages(qint64 duration, bool isDit) {
    // Exponential moving average for adaptive timing
    const double alpha = (m_sampleCount < 10) ? 0.5 : 0.2;

    if (isDit) {
        m_ditAvg = static_cast<qint64>(alpha * duration + (1 - alpha) * m_ditAvg);
    } else {
        m_dahAvg = static_cast<qint64>(alpha * duration + (1 - alpha) * m_dahAvg);
    }

    m_sampleCount++;
}

void MorseDecoder::onCharacterTimeout() {
    finalizeCharacter();
}

void MorseDecoder::onWordTimeout() {
    m_characterTimer.stop();

    if (!m_currentPattern.isEmpty()) {
        finalizeCharacter();
    }

    emit wordSpaceDetected();
}

void MorseDecoder::finalizeCharacter() {
    if (m_currentPattern.isEmpty()) return;

    QChar decoded = m_morseTable.decode(m_currentPattern);

    if (!decoded.isNull()) {
        emit characterDecoded(decoded);
    } else {
        emit decodingError(m_currentPattern);
    }

    m_currentPattern.clear();
}
