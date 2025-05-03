#ifndef ARDUINO_H
#define ARDUINO_H

#include <QSerialPort>
#include <QObject>

class Arduino : public QObject
{
    Q_OBJECT

public:
    Arduino(QObject *parent = nullptr);
    ~Arduino();
    bool connectArduino(const QString &portName); // Connect to Arduino
    void disconnectArduino(); // Disconnect from Arduino
    QStringList availablePorts(); // Get available serial ports
    void sendData(const QString &data); // Send data to Arduino

signals:
    void uidReceived(const QString &uid); // Signal emitted when UID is received

private slots:
    void readSerialData(); // Slot to read data from Arduino

private:
    QSerialPort *serialPort; // Serial port for communication
    QString buffer; // Buffer to store incoming data
};

#endif // ARDUINO_H
