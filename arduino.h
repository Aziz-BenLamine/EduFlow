#ifndef ARDUINO_H
#define ARDUINO_H

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QObject>

class Arduino : public QObject
{
    Q_OBJECT

public:
    Arduino(QObject *parent = nullptr);
    ~Arduino();
    bool connectArduino(const QString &portName); // Connect to Arduino (from HEAD)
    int connect_arduino(); // Automatic port detection (from gestion_colis)
    void disconnectArduino(); // Disconnect from Arduino (from HEAD)
    int close_arduino(); // Close serial port (from gestion_colis)
    QStringList availablePorts(); // Get available serial ports (from HEAD)
    void sendData(const QString &data); // Send data to Arduino (from HEAD)
    void write_to_arduino(QByteArray data); // Write QByteArray to Arduino (from gestion_colis)
    QByteArray read_from_arduino(); // Read from Arduino (from gestion_colis)
    QSerialPort* getserial(); // Get serial port (from gestion_colis)
    QString getarduino_port_name(); // Get Arduino port name (from gestion_colis)

signals:
    void uidReceived(const QString &uid); // Signal emitted when UID is received (from HEAD)
    void motionDetected(int motionCount); // Signal emitted when motion is detected with counter (from HEAD)

private slots:
    void readSerialData(); // Slot to read data from Arduino (from HEAD)

private:
    QSerialPort *serialPort; // Serial port for communication (from HEAD)
    QString buffer; // Buffer to store incoming data (from HEAD)
    static const quint16 arduino_vendor_id = 9025; // Arduino Uno vendor ID (from gestion_colis)
    static const quint16 arduino_product_id = 67; // Arduino Uno product ID (from gestion_colis)
    QString arduino_port_name; // Port name for Arduino (from gestion_colis)
    bool arduino_is_available; // Availability flag (from gestion_colis)
    QByteArray data; // Data buffer for reading (from gestion_colis)
};

#endif // ARDUINO_H
