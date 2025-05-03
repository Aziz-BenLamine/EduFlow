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
        qDebug() << "Connecté à l'Arduino sur" << portName;
        return true;
    } else {
        qDebug() << "Échec de la connexion à l'Arduino sur" << portName;
        return false;
    }
}

void Arduino::disconnectArduino()
{
    if (serialPort->isOpen()) {
        serialPort->close();
        qDebug() << "Déconnecté de l'Arduino";
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
        qDebug() << "Données envoyées à l'Arduino :" << data;
    } else {
        qDebug() << "Impossible d'envoyer des données : le port série n'est pas ouvert";
    }
}

void Arduino::readSerialData()
{
    buffer += serialPort->readAll(); // Ajoute les données reçues au buffer
    qDebug() << "Données série brutes reçues :" << buffer;

    // Vérifie si le buffer contient une nouvelle ligne
    while (buffer.contains('\n')) {
        // Trouve l'index du premier '\n'
        int newlineIndex = buffer.indexOf('\n');
        QString line = buffer.left(newlineIndex).trimmed(); // Extrait la ligne
        buffer.remove(0, newlineIndex + 1); // Supprime la ligne traitée du buffer

        if (line.isEmpty()) {
            continue; // Ignore les lignes vides
        }

        qDebug() << "Traitement de la ligne :" << line;

        // Vérifie si la ligne indique un mouvement
        if (line.startsWith("Mouvement détecté ! Compteur : ")) {
            QString countStr = line.mid(30).trimmed(); // Extrait le nombre après "Mouvement détecté ! Compteur : "
            bool ok;
            int motionCount = countStr.toInt(&ok); // Convertit en entier
            if (ok) {
                qDebug() << "Mouvement détecté, compteur :" << motionCount;
                emit motionDetected(motionCount); // Émet le signal avec le compteur
            } else {
                qDebug() << "Erreur de conversion du compteur :" << countStr;
            }
        }
        // Vérifie si aucun mouvement n'est détecté
        else if (line == "Aucun mouvement détecté") {
            qDebug() << "Aucun mouvement détecté reçu";
            // Optionnel : émettre un signal si nécessaire, par exemple emit noMotionDetected();
        }
        // Ligne non reconnue
        else {
            qDebug() << "Ligne non reconnue :" << line;
        }
    }
}
