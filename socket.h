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

#include <QByteArray>
#include <QHostAddress>
#include <QUdpSocket>

class Socket
{
public:
    Socket(QHostAddress, QByteArray);
    void toggle();
    void tableData();

    QHostAddress ip;
    QByteArray mac;
    bool powered;
    QString name, remotePassword;

private:
    enum Datagram {Subscribe, PowerOff, PowerOn, TableData, SocketData, TimingData, MaxCommands};

    void sendDatagram(Datagram);
    void readDatagrams(QUdpSocket *udpSocketGet, Datagram d);

    QByteArray commandID[MaxCommands];
    QByteArray datagram[MaxCommands];
    QByteArray rmac; // Reveresed mac

    const QByteArray magicKey = QByteArray::fromHex("68 64"); // recognize datagrams from the socket
    const QByteArray twenties = QByteArray::fromHex("20 20 20 20 20 20"); // mac address padding
    const QByteArray zeros = QByteArray::fromHex("00 00 00 00");
    const QByteArray zero =  QByteArray::fromHex("00");
    const QByteArray one =  QByteArray::fromHex("01");

    QUdpSocket *udpSocketSend, *udpSocketGet;
};
