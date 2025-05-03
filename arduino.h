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
    bool connectArduino(const QString &portName); // Connecter à l'Arduino
    void disconnectArduino(); // Déconnecter de l'Arduino
    QStringList availablePorts(); // Obtenir les ports série disponibles
    void sendData(const QString &data); // Envoyer des données à l'Arduino

signals:
    void motionDetected(int motionCount); // Signal émis lorsqu'un mouvement est détecté avec le compteur

private slots:
    void readSerialData(); // Slot pour lire les données de l'Arduino

private:
    QSerialPort *serialPort; // Port série pour la communication
    QString buffer; // Buffer pour stocker les données entrantes
};

#endif // ARDUINO_H
