#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QSettings>

#include "SerialHandler.h"
#include "MorseDecoder.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onRefreshPortsClicked();
    void onClearClicked();
    void onCopyClicked();

    void onSerialConnected();
    void onSerialDisconnected();
    void onSerialError(const QString& error);

    void onElementDecoded(const QString& element);
    void onCharacterDecoded(QChar character);
    void onWordSpaceDetected();
    void onDecodingError(const QString& pattern);

    void onWpmChanged(int value);
    void onSidetoneToggled(bool enabled);
    void onSidetoneFreqChanged(int value);
    void onSidetoneVolumeChanged(int value);

private:
    void setupUi();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void refreshPorts();
    void updateConnectionState(bool connected);
    void trimDecodedText();

    // Serial and decoder
    SerialHandler *m_serialHandler;
    MorseDecoder *m_morseDecoder;

    // Constants for performance limits
    static constexpr int MAX_DISPLAY_LINES = 1000;
    static constexpr int TRIM_TO_LINES = 800;

    // UI Components
    QComboBox *m_portCombo;
    QComboBox *m_baudCombo;
    QPushButton *m_connectBtn;
    QPushButton *m_refreshBtn;

    QTextEdit *m_decodedText;
    QLabel *m_currentMorse;
    QLabel *m_statusLabel;

    QSpinBox *m_wpmSpin;
    QCheckBox *m_sidetoneCheck;
    QSpinBox *m_sidetoneFreqSpin;
    QSlider *m_volumeSlider;

    QPushButton *m_clearBtn;
    QPushButton *m_copyBtn;

    QSettings *m_settings;
};

#endif // MAINWINDOW_H
