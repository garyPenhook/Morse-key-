#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QAudioSink>
#include <QAudioFormat>
#include <QBuffer>
#include <QTimer>

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
    void pollControlLines();
    void writeAudioData();

private:
    void parseData(const QByteArray& data);
    void initializeAudio();
    void startTone();
    void stopTone();
    void generateToneData();

    QSerialPort *m_serialPort;
    QByteArray m_buffer;

    // Control line polling
    QTimer *m_pollTimer;
    bool m_lastKeyState;
    int m_debounceCount;

    // Audio/sidetone
    QAudioSink *m_audioSink;
    QIODevice *m_audioIO;
    QByteArray m_toneData;
    QTimer *m_audioTimer;
    int m_tonePos;
    bool m_sidetoneEnabled;
    int m_sidetoneFreq;
    float m_sidetoneVolume;
    bool m_toneActive;
};

#endif // SERIALHANDLER_H
