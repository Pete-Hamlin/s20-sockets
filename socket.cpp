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

Socket::Socket(QHostAddress IPaddress, QByteArray reply)
{
    ip = IPaddress;
    mac = reply.mid(7, 6);
    rmac = mac;
    std::reverse(rmac.begin(), rmac.end());
    powered = reply.right(1) == QByteArray::fromHex("01");
    QByteArray twenties = QByteArray::fromHex("20 20 20 20 20 20");
    QByteArray zeros = QByteArray::fromHex("00 00 00 00");

    datagram[Subscribe] = QByteArray::fromHex("68 64 00 1e 63 6c") + mac + twenties + rmac + twenties;
    datagram[PowerOn] = QByteArray::fromHex("68 64 00 17 64 63") + mac + twenties + zeros + QByteArray::fromHex("01");
    datagram[PowerOff] = QByteArray::fromHex("68 64 00 17 64 63") + mac + twenties + zeros + QByteArray::fromHex("00");
}

bool Socket::toggle()
{
    sendDatagram(datagram[Subscribe]); // TODO: process replies
    sendDatagram(datagram[powered ? PowerOff : PowerOn]);
}

void Socket::sendDatagram(QByteArray datagram)
{
    udpSocketSend = new QUdpSocket();
    udpSocketSend->connectToHost(ip, 10000);
    udpSocketSend->write(datagram);
    udpSocketSend->disconnectFromHost();
    delete udpSocketSend;
}
