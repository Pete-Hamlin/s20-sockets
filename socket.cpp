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

Socket::Socket ( QHostAddress IPaddress, QByteArray reply )
{
    ip = IPaddress;
    mac = reply.mid ( 7, 6 );
    rmac = mac;
    std::reverse ( rmac.begin(), rmac.end() );

    powered = reply.right ( 1 ) == one;
    // 68:64:00:06:71:61 initial detection ??
    // 68:64:00:1e:63:6c:ac:cf:23:35:f5:8c:20:20:20:20:20:20:38:38:38:38:38:38:20:20:20:20:20:20 // to 42.121.111.208

    commandID[Subscribe] = QByteArray::fromHex ( "63 6c" );
    commandID[PowerOn] = QByteArray::fromHex ( "73 66" );
    commandID[PowerOff] = commandID[PowerOn];
    commandID[TableData] = QByteArray::fromHex ( "72 74" );
    commandID[SocketData] = commandID[TableData];
    commandID[TimingData] = commandID[TableData];
    commandID[WriteSocketData] = QByteArray::fromHex ( "74 6d" );

    // 2 hex bytes are the total length of the message
    datagram[Subscribe] = magicKey + QByteArray::fromHex ( "00 1e" ) + commandID[Subscribe] + mac + twenties + rmac + twenties;
    datagram[PowerOn] = magicKey + QByteArray::fromHex ( "00 17 64 63" ) + mac + twenties + zeros + one;
    datagram[PowerOff] = magicKey + QByteArray::fromHex ( "00 17 64 63" ) + mac + twenties + zeros + zero;
    datagram[TableData] = magicKey + QByteArray::fromHex ( "00 1d" ) + commandID[TableData] + mac + twenties + zeros + QByteArray::fromHex ( "01 00 00" ) + zeros;

    tableData();

    datagram[WriteSocketData] = magicKey + QByteArray::fromHex ( "00 a5" ) + commandID[WriteSocketData] + mac + twenties + zeros + QByteArray::fromHex ( "04:00:01" ) /*table number and unknown*/ +  QByteArray::fromHex ( "8a:00" ) /* record length = 138 bytes*/ + QByteArray::fromHex ( ":01:00:43:25" ) + mac + twenties + rmac + twenties + QByteArray::fromHex ( "38:38:38:38:38:38:20:20:20:20:20:20" ) + QByteArray ( "Heater          " ) + QByteArray::fromHex ( "04:00:20:00:00:00:14:00:00:00:05:00:00:00:10:27" ) + fromIP ( 42,121,111,208 ) + QByteArray::fromHex ( "10:27" ) + "vicenter.orvibo.com" + "   " + twenties + twenties + twenties + fromIP ( 192,168,1,212 ) + QByteArray::fromHex ( "c0:a8:01:01:ff:ff:ff:00:01:01:00:08:00:ff:00:00" );

    subscribeTimer = new QTimer(this);
    subscribeTimer->setInterval(2*60*1000);
    connect(subscribeTimer, &QTimer::timeout, this, &Socket::subscribe);
    subscribeTimer->start();
    subscribe();
}

Socket::~Socket()
{
    delete subscribeTimer;
}

void Socket::subscribe()
{
    sendDatagram ( Subscribe );
}

void Socket::toggle()
{
    bool powerOld = powered;
    while ( powerOld == powered )
    {
        sendDatagram ( powerOld ? PowerOff : PowerOn );
    }
}

void Socket::changeSocketName ( /*QString name*/ )
{
    sendDatagram ( WriteSocketData );
}

void Socket::tableData()
{
    sendDatagram ( TableData );
    datagram[SocketData] = magicKey + QByteArray::fromHex ( "00 1d" ) + commandID[SocketData] + mac + twenties + zeros + QByteArray::fromHex ( "04 00 00" ) + zeros;
    datagram[TimingData] = magicKey + QByteArray::fromHex ( "00 1d" ) + commandID[TimingData] + mac + twenties + zeros + QByteArray::fromHex ( "03 00 00" ) + zeros;
    // table number + 00 + version number
    sendDatagram ( SocketData );
    sendDatagram ( TimingData );
}

void Socket::sendDatagram ( Datagram d )
{
    udpSocket = new QUdpSocket();
    udpSocket->connectToHost ( ip, 10000 );
    udpSocket->write ( datagram[d] );
    delete udpSocket;
}

bool Socket::parseReply ( QByteArray reply )
{
    if ( reply.left ( 2 ) != magicKey )
    {
        return false;
    }

    QByteArray id = reply.mid ( 4, 2 );
    unsigned int datagram = std::distance ( commandID, std::find ( commandID, commandID + MaxCommands, id ) ); // match commandID with enum
    if ( datagram == 3 ) // determine the table number
    {
        unsigned int table = reply[reply.indexOf ( zeros ) + 4]; // Table number immediately follows zeros
        switch ( table )
        {
        case 1:
            datagram = TableData;
            break;
        case 3:
            datagram = TimingData;
            break;
        case 4:
            datagram = SocketData;
            break;
        default:
            return false;
        }
    }
//     std::cout << reply.toHex().toStdString() << " " << datagram << std::endl; // for debugging purposes only
    switch ( datagram )
    {
    case Subscribe:
    case PowerOff:
    case PowerOn:
    {
        bool poweredOld = powered;
        powered = reply.right ( 1 ) == one;
        if (powered != poweredOld)
            Q_EMIT stateChanged();
        break;
    }
    case TableData:
//         FIXME: order might be swapped;
        socketTableVersion = reply.mid ( reply.indexOf ( QByteArray::fromHex ( "000100000600" ) ) + 6, 2 );
//      000100000600
        break;
    case SocketData:
        remotePassword = reply.mid ( reply.indexOf ( rmac + twenties ) + 12, 12 );
        name = reply.mid ( reply.indexOf ( rmac + twenties ) + 24, 16 );
        Q_EMIT stateChanged();
        break;
    case TimingData:
        break;
    case WriteSocketData:
        Q_EMIT stateChanged();
        break;
    default:
        return false;
    }
    return true;
}

QByteArray Socket::fromIP ( unsigned char a, unsigned char b, unsigned char c, unsigned char d )
{
    return QByteArray::number ( a ) + QByteArray::number ( b ) + QByteArray::number ( c ) + QByteArray::number ( d );
}
