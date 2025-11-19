#ifndef MORSEDECODER_H
#define MORSEDECODER_H

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include "MorseTable.h"

class MorseDecoder : public QObject {
    Q_OBJECT

public:
    explicit MorseDecoder(QObject *parent = nullptr);

    void setWpm(int wpm);
    int wpm() const { return m_wpm; }

    void reset();

public slots:
    void keyDown();
    void keyUp();
    void processElement(bool isDit); // For character mode

signals:
    void elementDecoded(const QString& element); // "." or "-"
    void characterDecoded(QChar character);
    void wordSpaceDetected();
    void decodingError(const QString& pattern);

private slots:
    void onCharacterTimeout();
    void onWordTimeout();

private:
    void processKeyDuration(qint64 duration);
    void finalizeCharacter();
    void updateTimingAverages(qint64 duration, bool isDit);
    qint64 calculateUnitTime() const;

    MorseTable m_morseTable;
    QString m_currentPattern;

    QElapsedTimer m_keyTimer;
    QTimer m_characterTimer;
    QTimer m_wordTimer;

    bool m_keyIsDown;
    int m_wpm;

    // Adaptive timing
    qint64 m_ditAvg;
    qint64 m_dahAvg;
    int m_sampleCount;
};

#endif // MORSEDECODER_H
