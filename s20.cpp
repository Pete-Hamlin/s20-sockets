/*************************************************************************
 *  Copyright (C) 2015 by Andrius Štikonas <andrius@stikonas.eu>         *
 *                                                                       *
 *  This program is free software; you can redistribute it and/or modify *
 *  it under the terms of the GNU General Public License as published by *
 *  the Free Software Foundation; either version 3 of the License, or    *
 *  (at your option) any later version.                                  *
 *                                                                       *
 *  This program is distributed in the hope that it will be useful,      *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *  GNU General Public License for more details.                         *
 *                                                                       *
 *  You should have received a copy of the GNU General Public License    *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 *************************************************************************/

#include <QCoreApplication>
#include <QByteArray>
#include <QHostAddress>
#include <QUdpSocket>

class Socket : public QObject
{
public:
    Socket();

private:
    void readPendingDatagrams();
    QByteArray* makeNewDatagram();

    QUdpSocket *udpSocketSend;
    QUdpSocket *udpSocketGet;
    QHostAddress socketIPAddress;

    enum {Discover, Subscribe, PowerOff, PowerOn};
    QByteArray datagram[4] = {
        QByteArray::fromHex("68 64 00 06 71 61"), // global discovery
        QByteArray::fromHex("68 64 00 1e 63 6c ac cf 23 35 f5 8c 20 20 20 20 20 20 8c f5 35 23 cf ac 20 20 20 20 20 20"), // subscribe
        QByteArray::fromHex("68 64 00 17 64 63 ac cf 23 35 f5 8c 20 20 20 20 20 20 00 00 00 00 00"), // power off
        QByteArray::fromHex("68 64 00 17 64 63 ac cf 23 35 f5 8c 20 20 20 20 20 20 00 00 00 00 01") // power on
    };
};


Socket::Socket() {
    udpSocketSend = new QUdpSocket();
    udpSocketGet  = new QUdpSocket();
// 	QHostAddress *host  = new QHostAddress("192.168.1.212");
    QHostAddress *bcast = new QHostAddress("192.168.1.255");

    udpSocketSend->connectToHost(*bcast, 10000);
    udpSocketGet->bind(QHostAddress::Any, 10000);
//     QObject::connect(udpSocketGet, &QIODevice::readyRead, this, &Socket::readPendingDatagrams);

    udpSocketSend->write(datagram[Discover]);
    readPendingDatagrams();
}

void Socket::readPendingDatagrams()
{
    while (udpSocketGet->waitForReadyRead(1000)) // 1s
    {
        while (udpSocketGet->hasPendingDatagrams())
        {
            QByteArray datagramGet;
            datagramGet.resize(udpSocketGet->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;

            udpSocketGet->readDatagram(datagramGet.data(), datagramGet.size(), &sender, &senderPort);

            if (datagramGet != datagram[Discover])
            {
                socketIPAddress = sender;
            }
        }
    }

}

int main(int argc, char *argv[])
{
    QCoreApplication s20(argc, argv);
    Socket socket;
    return 0;
}
