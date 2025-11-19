#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_serialHandler(new SerialHandler(this))
    , m_morseDecoder(new MorseDecoder(this))
    , m_settings(new QSettings("MorseDecoder", "MorseKeyDecoder", this))
{
    setupUi();
    setupConnections();
    loadSettings();
    refreshPorts();
}

MainWindow::~MainWindow() {
    saveSettings();
}

void MainWindow::setupUi() {
    setWindowTitle("Morse Key Decoder");
    setMinimumSize(600, 500);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Top section: Serial config and Morse settings
    QHBoxLayout *topLayout = new QHBoxLayout();

    // Serial Configuration Group
    QGroupBox *serialGroup = new QGroupBox("Serial Port", this);
    QFormLayout *serialLayout = new QFormLayout(serialGroup);

    m_portCombo = new QComboBox(this);
    m_baudCombo = new QComboBox(this);
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200"});

    QHBoxLayout *portLayout = new QHBoxLayout();
    portLayout->addWidget(m_portCombo, 1);
    m_refreshBtn = new QPushButton("Refresh", this);
    portLayout->addWidget(m_refreshBtn);

    serialLayout->addRow("Port:", portLayout);
    serialLayout->addRow("Baud:", m_baudCombo);

    m_connectBtn = new QPushButton("Connect", this);
    serialLayout->addRow(m_connectBtn);

    topLayout->addWidget(serialGroup);

    // Morse Settings Group
    QGroupBox *morseGroup = new QGroupBox("Morse Settings", this);
    QFormLayout *morseLayout = new QFormLayout(morseGroup);

    m_wpmSpin = new QSpinBox(this);
    m_wpmSpin->setRange(5, 50);
    m_wpmSpin->setValue(20);
    m_wpmSpin->setSuffix(" WPM");
    morseLayout->addRow("Speed:", m_wpmSpin);

    m_sidetoneCheck = new QCheckBox("Enable Sidetone", this);
    m_sidetoneCheck->setChecked(true);
    morseLayout->addRow(m_sidetoneCheck);

    m_sidetoneFreqSpin = new QSpinBox(this);
    m_sidetoneFreqSpin->setRange(200, 1500);
    m_sidetoneFreqSpin->setValue(600);
    m_sidetoneFreqSpin->setSuffix(" Hz");
    morseLayout->addRow("Frequency:", m_sidetoneFreqSpin);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    morseLayout->addRow("Volume:", m_volumeSlider);

    topLayout->addWidget(morseGroup);

    mainLayout->addLayout(topLayout);

    // Current Morse display
    QGroupBox *currentGroup = new QGroupBox("Current Input", this);
    QVBoxLayout *currentLayout = new QVBoxLayout(currentGroup);
    m_currentMorse = new QLabel(this);
    m_currentMorse->setAlignment(Qt::AlignCenter);
    m_currentMorse->setStyleSheet("font-size: 24px; font-family: monospace; padding: 10px;");
    m_currentMorse->setMinimumHeight(50);
    currentLayout->addWidget(m_currentMorse);
    mainLayout->addWidget(currentGroup);

    // Decoded text display
    QGroupBox *decodedGroup = new QGroupBox("Decoded Text", this);
    QVBoxLayout *decodedLayout = new QVBoxLayout(decodedGroup);

    m_decodedText = new QTextEdit(this);
    m_decodedText->setReadOnly(true);
    m_decodedText->setStyleSheet("font-size: 16px; font-family: monospace;");
    decodedLayout->addWidget(m_decodedText);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_clearBtn = new QPushButton("Clear", this);
    m_copyBtn = new QPushButton("Copy", this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_clearBtn);
    buttonLayout->addWidget(m_copyBtn);
    decodedLayout->addLayout(buttonLayout);

    mainLayout->addWidget(decodedGroup, 1);

    // Status bar
    m_statusLabel = new QLabel("Disconnected", this);
    statusBar()->addWidget(m_statusLabel);
}

void MainWindow::setupConnections() {
    // UI connections
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshPortsClicked);
    connect(m_clearBtn, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(m_copyBtn, &QPushButton::clicked, this, &MainWindow::onCopyClicked);

    connect(m_wpmSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onWpmChanged);
    connect(m_sidetoneCheck, &QCheckBox::toggled, this, &MainWindow::onSidetoneToggled);
    connect(m_sidetoneFreqSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onSidetoneFreqChanged);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onSidetoneVolumeChanged);

    // Serial handler connections
    connect(m_serialHandler, &SerialHandler::connected, this, &MainWindow::onSerialConnected);
    connect(m_serialHandler, &SerialHandler::disconnected, this, &MainWindow::onSerialDisconnected);
    connect(m_serialHandler, &SerialHandler::errorOccurred, this, &MainWindow::onSerialError);

    connect(m_serialHandler, &SerialHandler::keyDown, m_morseDecoder, &MorseDecoder::keyDown);
    connect(m_serialHandler, &SerialHandler::keyUp, m_morseDecoder, &MorseDecoder::keyUp);
    connect(m_serialHandler, &SerialHandler::elementReceived, m_morseDecoder, &MorseDecoder::processElement);

    // Decoder connections
    connect(m_morseDecoder, &MorseDecoder::elementDecoded, this, &MainWindow::onElementDecoded);
    connect(m_morseDecoder, &MorseDecoder::characterDecoded, this, &MainWindow::onCharacterDecoded);
    connect(m_morseDecoder, &MorseDecoder::wordSpaceDetected, this, &MainWindow::onWordSpaceDetected);
    connect(m_morseDecoder, &MorseDecoder::decodingError, this, &MainWindow::onDecodingError);
}

void MainWindow::loadSettings() {
    m_wpmSpin->setValue(m_settings->value("wpm", 20).toInt());
    m_sidetoneCheck->setChecked(m_settings->value("sidetone_enabled", true).toBool());
    m_sidetoneFreqSpin->setValue(m_settings->value("sidetone_freq", 600).toInt());
    m_volumeSlider->setValue(m_settings->value("sidetone_volume", 50).toInt());
    m_baudCombo->setCurrentText(m_settings->value("baud_rate", "9600").toString());

    QString lastPort = m_settings->value("last_port").toString();
    if (!lastPort.isEmpty()) {
        int idx = m_portCombo->findText(lastPort);
        if (idx >= 0) {
            m_portCombo->setCurrentIndex(idx);
        }
    }
}

void MainWindow::saveSettings() {
    m_settings->setValue("wpm", m_wpmSpin->value());
    m_settings->setValue("sidetone_enabled", m_sidetoneCheck->isChecked());
    m_settings->setValue("sidetone_freq", m_sidetoneFreqSpin->value());
    m_settings->setValue("sidetone_volume", m_volumeSlider->value());
    m_settings->setValue("baud_rate", m_baudCombo->currentText());
    m_settings->setValue("last_port", m_portCombo->currentText());
}

void MainWindow::refreshPorts() {
    m_portCombo->clear();
    QStringList ports = m_serialHandler->availablePorts();
    m_portCombo->addItems(ports);

    if (ports.isEmpty()) {
        m_portCombo->addItem("No ports found");
        m_connectBtn->setEnabled(false);
    } else {
        m_connectBtn->setEnabled(true);
    }
}

void MainWindow::updateConnectionState(bool connected) {
    m_connectBtn->setText(connected ? "Disconnect" : "Connect");
    m_portCombo->setEnabled(!connected);
    m_baudCombo->setEnabled(!connected);
    m_refreshBtn->setEnabled(!connected);
}

void MainWindow::onConnectClicked() {
    if (m_serialHandler->isConnected()) {
        m_serialHandler->disconnect();
    } else {
        QString port = m_portCombo->currentText();
        qint32 baud = m_baudCombo->currentText().toInt();

        if (!port.isEmpty() && port != "No ports found") {
            m_serialHandler->connectToPort(port, baud);
        }
    }
}

void MainWindow::onRefreshPortsClicked() {
    refreshPorts();
}

void MainWindow::onClearClicked() {
    m_decodedText->clear();
    m_currentMorse->clear();
    m_morseDecoder->reset();
}

void MainWindow::onCopyClicked() {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_decodedText->toPlainText());
    statusBar()->showMessage("Copied to clipboard", 2000);
}

void MainWindow::onSerialConnected() {
    updateConnectionState(true);
    m_statusLabel->setText("Connected to " + m_portCombo->currentText());
    m_morseDecoder->reset();
}

void MainWindow::onSerialDisconnected() {
    updateConnectionState(false);
    m_statusLabel->setText("Disconnected");
}

void MainWindow::onSerialError(const QString& error) {
    m_statusLabel->setText("Error: " + error);
    QMessageBox::warning(this, "Serial Error", error);
}

void MainWindow::onElementDecoded(const QString& element) {
    m_currentMorse->setText(m_currentMorse->text() + element);
}

void MainWindow::onCharacterDecoded(QChar character) {
    m_decodedText->moveCursor(QTextCursor::End);
    m_decodedText->insertPlainText(QString(character));
    m_currentMorse->clear();
}

void MainWindow::onWordSpaceDetected() {
    m_decodedText->moveCursor(QTextCursor::End);
    m_decodedText->insertPlainText(" ");
}

void MainWindow::onDecodingError(const QString& pattern) {
    m_decodedText->moveCursor(QTextCursor::End);
    m_decodedText->insertPlainText("[" + pattern + "?]");
    m_currentMorse->clear();
}

void MainWindow::onWpmChanged(int value) {
    m_morseDecoder->setWpm(value);
}

void MainWindow::onSidetoneToggled(bool enabled) {
    m_serialHandler->setSidetoneEnabled(enabled);
    m_sidetoneFreqSpin->setEnabled(enabled);
    m_volumeSlider->setEnabled(enabled);
}

void MainWindow::onSidetoneFreqChanged(int value) {
    m_serialHandler->setSidetoneFrequency(value);
}

void MainWindow::onSidetoneVolumeChanged(int value) {
    m_serialHandler->setSidetoneVolume(value / 100.0f);
}
