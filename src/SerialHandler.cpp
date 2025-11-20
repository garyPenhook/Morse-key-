#include "SerialHandler.h"
#include "ToneGenerator.h"
#include <QMediaDevices>
#include <QDebug>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

// KeyWatcher implementation - uses TIOCMIWAIT for interrupt-driven detection
KeyWatcher::KeyWatcher(int fd, QObject *parent)
    : QThread(parent)
    , m_fd(fd)
    , m_running(true)
{
}

void KeyWatcher::stop()
{
    m_running = false;
}

void KeyWatcher::run()
{
    int state = 0;
    ioctl(m_fd, TIOCMGET, &state);
    bool lastKeyDown = (state & TIOCM_CTS) || (state & TIOCM_DSR);

    while (m_running) {
        // Tight polling loop - no sleep for maximum responsiveness
        ioctl(m_fd, TIOCMGET, &state);
        bool keyDown = (state & TIOCM_CTS) || (state & TIOCM_DSR);

        if (keyDown != lastKeyDown) {
            lastKeyDown = keyDown;
            emit keyStateChanged(keyDown);
        }
    }
}

SerialHandler::SerialHandler(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_keyWatcher(nullptr)
    , m_audioSink(nullptr)
    , m_toneGenerator(nullptr)
    , m_audioIO(nullptr)
    , m_audioTimer(new QTimer(this))
    , m_sidetoneEnabled(true)
    , m_sidetoneFreq(600)
    , m_sidetoneVolume(0.5f)
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialHandler::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialHandler::onErrorOccurred);
    connect(m_audioTimer, &QTimer::timeout, this, &SerialHandler::writeAudioData);

    initializeAudio();
}

SerialHandler::~SerialHandler() {
    if (m_keyWatcher) {
        m_keyWatcher->stop();
        m_keyWatcher->wait(100);
        delete m_keyWatcher;
    }
    m_audioTimer->stop();
    if (m_audioSink) {
        m_audioSink->stop();
    }
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }
}

void SerialHandler::initializeAudio() {
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {
        qWarning() << "No audio output device found";
        return;
    }
    if (!device.isFormatSupported(format)) {
        qWarning() << "Audio format not supported, trying preferred format";
        format = device.preferredFormat();
    }

    m_toneGenerator = new ToneGenerator(format, this);
    m_toneGenerator->setFrequency(m_sidetoneFreq);
    m_toneGenerator->setVolume(m_sidetoneVolume);
    m_toneGenerator->start();

    m_audioSink = new QAudioSink(device, format, this);
    m_audioSink->setBufferSize(512);  // Minimum buffer for fastest response

    // Start in push mode - we write data via timer
    m_audioIO = m_audioSink->start();
    if (m_audioIO) {
        m_audioTimer->start(1);  // Write every 1ms for low latency
    } else {
        qWarning() << "Failed to start audio";
    }
}

void SerialHandler::startTone() {
    if (!m_sidetoneEnabled || !m_toneGenerator) return;
    m_toneGenerator->setActive(true);
}

void SerialHandler::stopTone() {
    if (!m_toneGenerator) return;
    m_toneGenerator->setActive(false);
}

void SerialHandler::writeAudioData() {
    if (!m_audioIO || !m_toneGenerator) return;

    int bytesFree = m_audioSink->bytesFree();
    if (bytesFree > 0) {
        QByteArray buffer(bytesFree, 0);
        qint64 bytesRead = m_toneGenerator->read(buffer.data(), bytesFree);
        if (bytesRead > 0) {
            m_audioIO->write(buffer.data(), bytesRead);
        }
    }
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
        m_serialPort->setDataTerminalReady(true);

        // Start interrupt-driven key watcher
        int fd = m_serialPort->handle();
        m_keyWatcher = new KeyWatcher(fd, this);
        connect(m_keyWatcher, &KeyWatcher::keyStateChanged,
                this, &SerialHandler::onKeyStateChanged, Qt::DirectConnection);
        m_keyWatcher->start();

        emit connected();
        return true;
    } else {
        emit errorOccurred(m_serialPort->errorString());
        return false;
    }
}

void SerialHandler::disconnect() {
    if (m_keyWatcher) {
        m_keyWatcher->stop();
        m_keyWatcher->wait(100);
        delete m_keyWatcher;
        m_keyWatcher = nullptr;
    }
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit disconnected();
    }
}

void SerialHandler::onKeyStateChanged(bool down) {
    if (down) {
        startTone();
        emit keyDown();
    } else {
        stopTone();
        emit keyUp();
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
    if (m_toneGenerator) {
        m_toneGenerator->setFrequency(m_sidetoneFreq);
    }
}

void SerialHandler::setSidetoneVolume(float volume) {
    m_sidetoneVolume = qBound(0.0f, volume, 1.0f);
    if (m_toneGenerator) {
        m_toneGenerator->setVolume(m_sidetoneVolume);
    }
}

void SerialHandler::onReadyRead() {
    QByteArray data = m_serialPort->readAll();
    m_buffer.append(data);
    emit dataReceived(data);
    parseData(data);
}

void SerialHandler::parseData(const QByteArray& data) {
    for (char c : data) {
        if (m_parseState == ParseState::WaitingForK && (c == '\n' || c == '\r')) {
            continue;
        }

        switch (m_parseState) {
        case ParseState::WaitingForK:
            if (c == 'K') {
                m_parseState = ParseState::WaitingForDigit;
            } else if (c == '.') {
                emit elementReceived(true);
            } else if (c == '-') {
                emit elementReceived(false);
            }
            break;

        case ParseState::WaitingForDigit:
            if (c == '1') {
                startTone();
                emit keyDown();
                m_parseState = ParseState::WaitingForNewline;
            } else if (c == '0') {
                stopTone();
                emit keyUp();
                m_parseState = ParseState::WaitingForNewline;
            } else {
                m_parseState = ParseState::WaitingForK;
            }
            break;

        case ParseState::WaitingForNewline:
            if (c == '\n' || c == '\r') {
                m_parseState = ParseState::WaitingForK;
            }
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
