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

#include "socket.h"

#include <algorithm>
#include <iostream>
#include <QThread>

Socket::Socket(QHostAddress IPaddress, QByteArray reply)
{
    ip = IPaddress;
    mac = reply.mid(7, 6);
    rmac = mac;
    std::reverse(rmac.begin(), rmac.end());

    powered = reply.right(1) == one;

    commandID[Subscribe] = QByteArray::fromHex("63 6c");
    commandID[PowerOn] = QByteArray::fromHex("73 66");
    commandID[PowerOff] = commandID[PowerOn];
    commandID[TableData] = QByteArray::fromHex("72 74");
    commandID[SocketData] = commandID[TableData];
    commandID[TimingData] = commandID[TableData];

    // 2 hex bytes are the total length of the message
    datagram[Subscribe] = magicKey + QByteArray::fromHex("00 1e") + commandID[Subscribe] + mac + twenties + rmac + twenties;
    datagram[PowerOn] = magicKey + QByteArray::fromHex("00 17 64 63") + mac + twenties + zeros + one;
    datagram[PowerOff] = magicKey + QByteArray::fromHex("00 17 64 63") + mac + twenties + zeros + zero;
    datagram[TableData] = magicKey + QByteArray::fromHex("00 1d") + commandID[TableData] + mac + twenties + zeros + QByteArray::fromHex("01 00 00") + zeros;
    // FIXME: parse table versions and numbers
    datagram[SocketData] = magicKey + QByteArray::fromHex("00 1d") + commandID[SocketData] + mac + twenties + zeros + QByteArray::fromHex("04 00 02") + zeros;
    datagram[TimingData] = magicKey + QByteArray::fromHex("00 1d") + commandID[TimingData] + mac + twenties + zeros + QByteArray::fromHex("03 00 03") + zeros;
    // table number 00 version number
}

void Socket::toggle()
{
    bool powerOld = powered;
    while (powerOld == powered)
    {
        sendDatagram(Subscribe);
        sendDatagram(powerOld ? PowerOff : PowerOn);
    }
}

void Socket::tableData()
{
    sendDatagram(Subscribe);
    sendDatagram(TableData);
    sendDatagram(SocketData);
    sendDatagram(TimingData);
    std::cout << name.toStdString() << std::endl;
}

void Socket::sendDatagram(Datagram d)
{
    udpSocketGet = new QUdpSocket();
    udpSocketGet->bind(QHostAddress::Any, 10000);

    udpSocketSend = new QUdpSocket();
    udpSocketSend->connectToHost(ip, 10000);
    udpSocketSend->write(datagram[d]);
    delete udpSocketSend;
    readDatagrams(udpSocketGet, d);
    delete udpSocketGet;
}

void Socket::readDatagrams(QUdpSocket *udpSocketGet, Datagram d)
{
    while (udpSocketGet->waitForReadyRead(300)) // 300ms
    {
        while (udpSocketGet->hasPendingDatagrams())
        {
            QByteArray datagramGet;
            datagramGet.resize(udpSocketGet->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;

            udpSocketGet->readDatagram(datagramGet.data(), datagramGet.size(), &sender, &senderPort);

            if (datagramGet.left(2) == magicKey && datagramGet.mid(4,2) == commandID[d])
            {
                std::cout << datagramGet.toHex().toStdString() << " " << d << std::endl;
                switch (d)
                {
                    case Subscribe:
                    case PowerOff:
                    case PowerOn:
                        powered = datagramGet.right(1) == one;
                        break;
                    case SocketData:
                        remotePassword = datagramGet.mid(58, 12);
                        name = datagramGet.mid(70, 16);
                        break;
                }
            }
        }
    }
}
