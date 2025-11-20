#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QAudioSink>
#include <QAudioFormat>
#include <QTimer>
#include <QThread>
#include <atomic>

class ToneGenerator;

class KeyWatcher : public QThread {
    Q_OBJECT
public:
    explicit KeyWatcher(int fd, QObject *parent = nullptr);
    void stop();

signals:
    void keyStateChanged(bool down);

protected:
    void run() override;

private:
    int m_fd;
    std::atomic<bool> m_running;
};

class SerialHandler : public QObject {
    Q_OBJECT

public:
    explicit SerialHandler(QObject *parent = nullptr);
    ~SerialHandler();

    QStringList availablePorts() const;
    bool connectToPort(const QString& portName, qint32 baudRate = 9600);
    void disconnect();
    bool isConnected() const;

    // Sidetone settings
    void setSidetoneEnabled(bool enabled);
    bool sidetoneEnabled() const { return m_sidetoneEnabled; }
    void setSidetoneFrequency(int frequency);
    int sidetoneFrequency() const { return m_sidetoneFreq; }
    void setSidetoneVolume(float volume);

signals:
    void keyDown();
    void keyUp();
    void elementReceived(bool isDit); // For character mode
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
    void dataReceived(const QByteArray& data);

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void onKeyStateChanged(bool down);
    void writeAudioData();

private:
    void parseData(const QByteArray& data);
    void initializeAudio();
    void startTone();
    void stopTone();

    QSerialPort *m_serialPort;
    QByteArray m_buffer;

    // Serial parsing state machine
    enum class ParseState {
        WaitingForK,
        WaitingForDigit,
        WaitingForNewline
    };
    ParseState m_parseState = ParseState::WaitingForK;

    // Control line monitoring (interrupt-driven)
    KeyWatcher *m_keyWatcher;

    // Audio/sidetone (push-mode)
    QAudioSink *m_audioSink;
    ToneGenerator *m_toneGenerator;
    QIODevice *m_audioIO;
    QTimer *m_audioTimer;
    bool m_sidetoneEnabled;
    int m_sidetoneFreq;
    float m_sidetoneVolume;
};

#endif // SERIALHANDLER_H
