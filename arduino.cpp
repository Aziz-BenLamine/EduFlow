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

    while (buffer.contains('\n')) {
        int newlineIndex = buffer.indexOf('\n');
        QString line = buffer.left(newlineIndex).trimmed();
        buffer.remove(0, newlineIndex + 1);

        if (line.isEmpty()) {
            continue;
        }

        qDebug() << "Processing line:" << line;

        // Handle UID data (from HEAD)
        if (line.startsWith("UID: ")) {
            QString uid = line.mid(5).trimmed();
            qDebug() << "Extracted UID:" << uid;
            emit uidReceived(uid);
        }
        // Handle motion detection data (from Gestion-Etablissement)
        else if (line.startsWith("Mouvement détecté ! Compteur : ")) {
            QString countStr = line.mid(30).trimmed();
            bool ok;
            int motionCount = countStr.toInt(&ok);
            if (ok) {
                qDebug() << "Motion detected, counter:" << motionCount;
                emit motionDetected(motionCount);
            } else {
                qDebug() << "Error converting counter:" << countStr;
            }
        }
        // Handle no motion detected (from Gestion-Etablissement)
        else if (line == "Aucun mouvement détecté") {
            qDebug() << "No motion detected received";
            // Optional: emit a signal if needed
        }
        // Unrecognized line
        else {
            qDebug() << "Unrecognized line:" << line;
        }
    }
}
