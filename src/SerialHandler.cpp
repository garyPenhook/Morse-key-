#include "SerialHandler.h"
#include <QMediaDevices>
#include <QtMath>

SerialHandler::SerialHandler(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_pollTimer(new QTimer(this))
    , m_lastKeyState(false)
    , m_audioSink(nullptr)
    , m_audioBuffer(nullptr)
    , m_sidetoneEnabled(true)
    , m_sidetoneFreq(600)
    , m_sidetoneVolume(0.5f)
    , m_toneActive(false)
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialHandler::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialHandler::onErrorOccurred);
    connect(m_pollTimer, &QTimer::timeout, this, &SerialHandler::pollControlLines);

    initializeAudio();
}

SerialHandler::~SerialHandler() {
    stopTone();
    disconnect();
    delete m_audioSink;
    delete m_audioBuffer;
}

void SerialHandler::initializeAudio() {
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format)) {
        qWarning() << "Audio format not supported";
        return;
    }

    m_audioSink = new QAudioSink(device, format, this);
    m_audioBuffer = new QBuffer(this);

    generateToneData();
}

void SerialHandler::generateToneData() {
    // Generate 1 second of tone data
    const int sampleRate = 44100;
    const int duration = 1; // seconds
    const int numSamples = sampleRate * duration;

    m_toneData.resize(numSamples * sizeof(qint16));
    qint16 *data = reinterpret_cast<qint16*>(m_toneData.data());

    for (int i = 0; i < numSamples; ++i) {
        qreal t = static_cast<qreal>(i) / sampleRate;
        qreal value = qSin(2.0 * M_PI * m_sidetoneFreq * t);
        data[i] = static_cast<qint16>(value * 32767 * m_sidetoneVolume);
    }
}

void SerialHandler::startTone() {
    if (!m_sidetoneEnabled || !m_audioSink || m_toneActive) return;

    m_audioBuffer->setData(m_toneData);
    m_audioBuffer->open(QIODevice::ReadOnly);
    m_audioSink->start(m_audioBuffer);
    m_toneActive = true;
}

void SerialHandler::stopTone() {
    if (!m_toneActive || !m_audioSink) return;

    m_audioSink->stop();
    m_audioBuffer->close();
    m_toneActive = false;
}

QStringList SerialHandler::availablePorts() const {
    QStringList ports;
    const auto serialPorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : serialPorts) {
        ports << info.portName();
    }
    return ports;
}

bool SerialHandler::connectToPort(const QString& portName, qint32 baudRate) {
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        // Enable DTR to power the device
        m_serialPort->setDataTerminalReady(true);
        m_lastKeyState = false;
        m_pollTimer->start(1); // Poll every 1ms for responsive keying
        emit connected();
        return true;
    } else {
        emit errorOccurred(m_serialPort->errorString());
        return false;
    }
}

void SerialHandler::disconnect() {
    m_pollTimer->stop();
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit disconnected();
    }
}

void SerialHandler::pollControlLines() {
    if (!m_serialPort->isOpen()) return;

    // Check CTS (Clear To Send) line - commonly used for key state
    // Also check DSR and DCD as fallbacks
    bool keyState = m_serialPort->pinoutSignals() & QSerialPort::ClearToSendSignal;

    // Some devices use DSR instead
    if (!keyState) {
        keyState = m_serialPort->pinoutSignals() & QSerialPort::DataSetReadySignal;
    }

    // Detect state change
    if (keyState != m_lastKeyState) {
        m_lastKeyState = keyState;

        if (keyState) {
            startTone();
            emit keyDown();
        } else {
            stopTone();
            emit keyUp();
        }
    }
}

bool SerialHandler::isConnected() const {
    return m_serialPort->isOpen();
}

void SerialHandler::setSidetoneEnabled(bool enabled) {
    m_sidetoneEnabled = enabled;
    if (!enabled) {
        stopTone();
    }
}

void SerialHandler::setSidetoneFrequency(int frequency) {
    m_sidetoneFreq = qBound(200, frequency, 1500);
    generateToneData();
}

void SerialHandler::setSidetoneVolume(float volume) {
    m_sidetoneVolume = qBound(0.0f, volume, 1.0f);
    generateToneData();
}

void SerialHandler::onReadyRead() {
    QByteArray data = m_serialPort->readAll();
    m_buffer.append(data);
    emit dataReceived(data);
    parseData(data);
}

void SerialHandler::parseData(const QByteArray& data) {
    // Parse different protocols
    for (char c : data) {
        switch (c) {
        case 'K':
            // Wait for next char
            break;
        case '1':
            // Key down - start sidetone
            startTone();
            emit keyDown();
            break;
        case '0':
            // Key up - stop sidetone
            stopTone();
            emit keyUp();
            break;
        case '.':
            // Dit in character mode
            emit elementReceived(true);
            break;
        case '-':
            // Dah in character mode
            emit elementReceived(false);
            break;
        default:
            break;
        }
    }
}

void SerialHandler::onErrorOccurred(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError) return;

    QString errorMessage;
    switch (error) {
    case QSerialPort::DeviceNotFoundError:
        errorMessage = "Device not found";
        break;
    case QSerialPort::PermissionError:
        errorMessage = "Permission denied. Add user to 'dialout' group.";
        break;
    case QSerialPort::OpenError:
        errorMessage = "Cannot open port";
        break;
    case QSerialPort::ResourceError:
        errorMessage = "Device disconnected";
        disconnect();
        break;
    default:
        errorMessage = m_serialPort->errorString();
        break;
    }

    emit errorOccurred(errorMessage);
}
