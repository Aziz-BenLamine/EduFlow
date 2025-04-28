#include "arduino.h"
#include <QSerialPortInfo>
#include <QDebug>

Arduino::Arduino(QObject *parent) : QObject(parent), serialPort(new QSerialPort(this))
{
    connect(serialPort, &QSerialPort::readyRead, this, &Arduino::readSerialData);
}

Arduino::~Arduino()
{
    disconnectArduino();
    delete serialPort;
}

bool Arduino::connectArduino(const QString &portName)
{
    if (serialPort->isOpen()) {
        serialPort->close();
    }

    serialPort->setPortName(portName);
    serialPort->setBaudRate(QSerialPort::Baud9600);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "Connected to Arduino on" << portName;
        return true;
    } else {
        qDebug() << "Failed to connect to Arduino on" << portName;
        return false;
    }
}

void Arduino::disconnectArduino()
{
    if (serialPort->isOpen()) {
        serialPort->close();
        qDebug() << "Disconnected from Arduino";
    }
}

QStringList Arduino::availablePorts()
{
    QStringList ports;
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        ports << info.portName();
    }
    return ports;
}

void Arduino::sendData(const QString &data)
{
    if (serialPort->isOpen()) {
        serialPort->write((data + "\n").toUtf8());
        serialPort->flush();
        qDebug() << "Sent to Arduino:" << data;
    } else {
        qDebug() << "Cannot send data: Serial port is not open";
    }
}

void Arduino::readSerialData()
{
    buffer += serialPort->readAll();
    qDebug() << "Raw serial data received:" << buffer;
    qDebug() << "Raw serial data (hex):" << buffer.toUtf8().toHex();
    if (buffer.contains('\n')) {
        QStringList lines = buffer.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            qDebug() << "Processing line:" << line;
            if (line.startsWith("UID: ")) {
                QString uid = line.mid(5).trimmed();
                qDebug() << "Extracted UID:" << uid;
                emit uidReceived(uid);
            } else {
                qDebug() << "Line does not start with 'UID: ': " << line;
            }
        }
        buffer.clear();
    }
}
