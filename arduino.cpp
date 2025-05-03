#include "arduino.h"
#include <QDebug>

Arduino::Arduino() {
    serial = nullptr;
    arduino_is_available = false;
    arduino_port_name = "";
}

Arduino::~Arduino() {
    close_arduino();
    delete serial;
}

int Arduino::connect_arduino() {
    serial = new QSerialPort;
    arduino_is_available = false;

    // Hardcode COM5 for consistency with your setup
    arduino_port_name = "COM5";
    serial->setPortName(arduino_port_name);

    // Configure serial port
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    // Try to open the port
    if (serial->open(QIODevice::ReadWrite)) {
        arduino_is_available = true;
        qDebug() << "Connected to Arduino on COM5";
        return 0; // Success
    } else {
        qDebug() << "Failed to connect to Arduino on COM5:" << serial->errorString();
        delete serial;
        serial = nullptr;
        return 1; // Failure
    }
}

int Arduino::close_arduino() {
    if (serial && serial->isOpen()) {
        serial->close();
        qDebug() << "Arduino COM5 closed";
        return 0; // Success
    }
    return 1; // Failure
}

QByteArray Arduino::read_from_arduino() {
    if (serial && serial->isReadable()) {
        data = serial->readAll();
        return data;
    }
    return QByteArray();
}

void Arduino::write_to_arduino(QByteArray data) {
    if (serial && serial->isWritable()) {
        serial->write(data);
        serial->waitForBytesWritten(1000); // Ensure data is sent
        qDebug() << "Wrote to Arduino:" << data;
    } else {
        qDebug() << "Serial port not writable";
    }
}

QSerialPort* Arduino::getserial() {
    return serial;
}

QString Arduino::getarduino_port_name() {
    return arduino_port_name;
}
