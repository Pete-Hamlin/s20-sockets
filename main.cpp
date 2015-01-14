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

#include <iostream>
#include <vector>

#include "socket.h"

void readDiscoverDatagrams(QUdpSocket *udpSocketGet, std::vector<Socket> &sockets);
void listSockets(std::vector<Socket> const &sockets);

QByteArray discover = QByteArray::fromHex("68 64 00 06 71 61");

int main(int argc, char *argv[])
{
    QUdpSocket *udpSocketSend = new QUdpSocket();
    QUdpSocket *udpSocketGet = new QUdpSocket();

    udpSocketSend->connectToHost(QHostAddress::Broadcast, 10000);
    udpSocketGet->bind(QHostAddress::Any, 10000);

    udpSocketSend->write(discover);
    udpSocketSend->disconnectFromHost();
    delete udpSocketSend;
    std::vector<Socket> sockets;

    readDiscoverDatagrams(udpSocketGet, sockets);
    listSockets(sockets);

    char command;
    std::cin >> command;
    switch(command)
    {
        case 'q':
            break;
        case 't':
            sockets.begin()->toggle();
    }
    return 0;
}

void readDiscoverDatagrams(QUdpSocket *udpSocketGet, std::vector<Socket> &sockets)
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

            if (datagramGet != discover && datagramGet.left(2) == QByteArray::fromHex("68 64"))
            {
                bool duplicate = false;
                for(std::vector<Socket>::const_iterator i = sockets.begin() ; i != sockets.end(); ++i)
                {
                    if (i->ip == sender)
                        duplicate = true;
                }
                if(!duplicate)
                {
                    const Socket socket(sender, datagramGet);
                    sockets.push_back(socket);
                }
            }
        }
    }
}

void listSockets(std::vector<Socket> const &sockets)
{
    for (std::vector<Socket>::const_iterator i = sockets.begin() ; i != sockets.end(); ++i)
    {
        std::cout << "IP Address: " << i->ip.toString().toStdString() << "\t MAC Address: " << i->mac.toHex().toStdString()  << "\t Powered: " << i->powered << std::endl;
    }
}
