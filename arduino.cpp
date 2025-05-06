#include "arduino.h"
#include <QSerialPortInfo>
#include <QDebug>

Arduino::Arduino(QObject *parent) : QObject(parent), serialPort(new QSerialPort(this)), arduino_is_available(false), arduino_port_name("")
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
        arduino_is_available = true;
        arduino_port_name = portName;
        qDebug() << "Connected to Arduino on" << portName;
        return true;
    } else {
        qDebug() << "Failed to connect to Arduino on" << portName << ":" << serialPort->errorString();
        return false;
    }
}

int Arduino::connect_arduino()
{
    if (serialPort->isOpen()) {
        serialPort->close();
    }

    // Try hardcoded COM5 (from gestion_colis)
    QString portName = "COM5";
    serialPort->setPortName(portName);
    serialPort->setBaudRate(QSerialPort::Baud9600);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (serialPort->open(QIODevice::ReadWrite)) {
        arduino_is_available = true;
        arduino_port_name = portName;
        qDebug() << "Connected to Arduino on COM5";
        return 0; // Success
    }

    // Fallback to automatic detection using vendor/product IDs
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        if (info.hasVendorIdentifier() && info.hasProductIdentifier() &&
            info.vendorIdentifier() == arduino_vendor_id &&
            info.productIdentifier() == arduino_product_id) {
            portName = info.portName();
            serialPort->setPortName(portName);
            if (serialPort->open(QIODevice::ReadWrite)) {
                arduino_is_available = true;
                arduino_port_name = portName;
                qDebug() << "Connected to Arduino on" << portName;
                return 0; // Success
            }
        }
    }

    qDebug() << "Failed to connect to Arduino:" << serialPort->errorString();
    return 1; // Failure
}

void Arduino::disconnectArduino()
{
    if (serialPort->isOpen()) {
        serialPort->close();
        arduino_is_available = false;
        arduino_port_name = "";
        qDebug() << "Disconnected from Arduino";
    }
}

int Arduino::close_arduino()
{
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
        arduino_is_available = false;
        arduino_port_name = "";
        qDebug() << "Arduino closed";
        return 0; // Success
    }
    return 1; // Failure
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

void Arduino::write_to_arduino(QByteArray data)
{
    if (serialPort->isOpen() && serialPort->isWritable()) {
        serialPort->write(data);
        serialPort->waitForBytesWritten(1000); // Ensure data is sent
        qDebug() << "Wrote to Arduino:" << data;
    } else {
        qDebug() << "Serial port not writable or not open";
    }
}

QByteArray Arduino::read_from_arduino()
{
    if (serialPort->isOpen() && serialPort->isReadable()) {
        data = serialPort->readAll();
        qDebug() << "Read from Arduino:" << data;
        return data;
    }
    return QByteArray();
}

QSerialPort* Arduino::getserial()
{
    return serialPort;
}

QString Arduino::getarduino_port_name()
{
    return arduino_port_name;
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

        // Handle UID data
        if (line.startsWith("UID: ")) {
            QString uid = line.mid(5).trimmed();
            qDebug() << "Extracted UID:" << uid;
            emit uidReceived(uid);
        }
        // Handle motion detection data
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
        // Handle no motion detected
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
