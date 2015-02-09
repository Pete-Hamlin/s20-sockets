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
#include <QUdpSocket>

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

    udpSocket = new QUdpSocket();
    udpSocket->connectToHost ( ip, 10000 );

    connect (this, &Socket::datagramQueued, this, &Socket::listen);
    subscribed = false;
    subscribeTimer = new QTimer(this);
    subscribeTimer->setInterval(2*60*1000); // 2 min
    subscribeTimer->setSingleShot(false);
    connect(subscribeTimer, &QTimer::timeout, this, &Socket::subscribe);
    subscribeTimer->start();
    subscribe();
    tableData();
}

Socket::~Socket()
{
    delete subscribeTimer;
    delete udpSocket;
}

void Socket::sendDatagram ( Datagram d )
{
    commands.enqueue(d);
    Q_EMIT datagramQueued();
}

void Socket::run()
{
    while ( commands.size() > 0 )
    {
        udpSocket->write ( datagram[commands.head()] );
        QThread::msleep(100);
    }
}

void Socket::subscribe()
{
    sendDatagram ( Subscribe );
}

void Socket::toggle()
{
    sendDatagram ( powered ? PowerOff : PowerOn );
}

void Socket::changeSocketName ( QString newName )
{
    QByteArray name = newName.toLatin1().leftJustified(16, ' ', true);

    datagram[WriteSocketData] = magicKey + QByteArray::fromHex ( "00 a5" ) + commandID[WriteSocketData] + mac + twenties + zeros + QByteArray::fromHex ( "04:00:01" ) /*table number and unknown*/ +  QByteArray::fromHex ( "8a:00" ) /* record length = 138 bytes*/ + QByteArray::fromHex ( "01:00" ) /* record number = 1*/ + versionID + mac + twenties + rmac + twenties + remotePassword + name + icon + hardwareVersion + firmwareVersion + wifiFirmwareVersion + port + fromIP ( 42,121,111,208 ) + port + QStringLiteral("vicenter.orvibo.com   ").toLatin1() + twenties + twenties + twenties + ip.toString().toLatin1() + localGatewayIP + QByteArray::fromHex ( "ff:ff:ff:00:01:01:00:08:00:ff:00:00" );
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
    std::cout << reply.toHex().toStdString() << " " << datagram << std::endl; // for debugging purposes only
    switch ( datagram )
    {
    case Subscribe:
        subscribed = true;
        subscribeTimer->setInterval(2*60*1000); // 2min
    case PowerOff:
    case PowerOn:
    {
        bool poweredOld = powered;
        powered = reply.right ( 1 ) == one;
        if (powered != poweredOld)
            Q_EMIT stateChanged();
        if ( datagram == PowerOff && powered == true )
        {
            datagram = PowerOn;
        }
        break;
    }
    case TableData:
//         FIXME: order might be swapped;
        socketTableVersion = reply.mid ( reply.indexOf ( QByteArray::fromHex ( "000100000600" ) ) + 6, 2 );
//      000100000600
        break;
    case SocketData:
    {
        unsigned short int index = reply.indexOf ( rmac + twenties );
        versionID = reply.mid ( index - 14, 2 );
        index += 12; // length of rmac + padding
        remotePassword = reply.mid ( index, 12 ); // max 12 symbols
        index += 12;
        name = reply.mid ( index, 16 ); // max 16 symbols
        index += 16;
        icon = reply.mid ( index, 2 );
        index += 2;
        hardwareVersion = reply.mid ( index, 4 );
        index += 4;
        firmwareVersion = reply.mid ( index, 4 );
        index += 4;
        wifiFirmwareVersion = reply.mid (index, 4);
        index += 56;
        localGatewayIP = reply.mid (index, 4);
        Q_EMIT stateChanged();
        break;
    }
    case TimingData:
        break;
    case WriteSocketData:
        sendDatagram ( SocketData );
//         Q_EMIT stateChanged();
        break;
    default:
        return false;
    }

    if (commands.size() > 0)
    {
        if ( datagram == commands.head() )
        {
            commands.dequeue();
        }
    }

    return true;
}

QByteArray Socket::fromIP ( unsigned char a, unsigned char b, unsigned char c, unsigned char d )
{
    qWarning() << QByteArray::number ( a, 16 ) + QByteArray::number ( b, 16 ) + QByteArray::number ( c, 16 ) + QByteArray::number ( d, 16 );
    return QByteArray::number ( a, 16 ) + QByteArray::number ( b, 16 ) + QByteArray::number ( c, 16 ) + QByteArray::number ( d, 16 );
}
