#ifndef ARDUINO_H
#define ARDUINO_H
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

class Arduino
{
public:
    Arduino();
    ~Arduino(); // Added destructor
    int connect_arduino();
    int close_arduino();
    void write_to_arduino(QByteArray data);
    QByteArray read_from_arduino();
    QSerialPort* getserial();
    QString getarduino_port_name();

private:
    QSerialPort* serial;
    static const quint16 arduino_vendor_id = 9025; // Arduino Uno (adjust for clones, e.g., 6790 for CH340)
    static const quint16 arduino_product_id = 67; // Arduino Uno (adjust for clones, e.g., 29987 for CH340)
    QString arduino_port_name;
    bool arduino_is_available;
    QByteArray data;
};

#endif // ARDUINO_H
