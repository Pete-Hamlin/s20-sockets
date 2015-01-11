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

class Server : public QObject
{
Q_OBJECT
public:
	Server();

	bool received;

private slots:
	void readPendingDatagrams();

private:
	QByteArray* makeNewDatagram();

	QUdpSocket *udpSocketSend;
	QUdpSocket *udpSocketGet;
};


Server::Server(){
	received = false;
	udpSocketSend = new QUdpSocket();
	udpSocketGet  = new QUdpSocket();
	QHostAddress *host  = new QHostAddress("192.168.1.212");
	QHostAddress *bcast = new QHostAddress("192.168.1.255");

	udpSocketSend->connectToHost(*bcast, 10000);
	udpSocketGet->bind(*host, 10000);
	connect(udpSocketGet, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

	QByteArray *datagram = makeNewDatagram(); // data from external function
	udpSocketSend->write(*datagram);
	qWarning() << "sent";
}

QByteArray* Server::makeNewDatagram()
{
	 QByteArray *datagram = new QByteArray;
	 *datagram = QByteArray::fromHex("68 64 00 06 71 61"); // global discovery
// 	 *datagram = QByteArray::fromHex("68 64 00 1e 63 6c ac cf 23 35 f5 8c 20 20 20 20 20 20 8c f5 35 23 cf ac 20 20 20 20 20 20"); // subscribe
// 	 *datagram = QByteArray::fromHex("68 64 00 17 64 63 ac cf 23 35 f5 8c 20 20 20 20 20 20 00 00 00 00 00"); // power off
// 	 *datagram = QByteArray::fromHex("68 64 00 17 64 63 ac cf 23 35 f5 8c 20 20 20 20 20 20 00 00 00 00 01"); // power on
	 return datagram;
}

void Server::readPendingDatagrams()
{
    qWarning() << "read0";
    while (udpSocketGet->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocketGet->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpSocketGet->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

	qWarning() << "read";
	qWarning() << QString(datagram);
//         processTheDatagram(datagram);
	received = true;
    }
}

int main(int argc, char *argv[])
{
	QCoreApplication s20(argc, argv);
	Server server;
	while (!server.received)
	{
		true;
	}
	return 0;
}

#include "s20.moc"
