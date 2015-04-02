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

#include <QMutex>
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

    QByteArray timeArray = reply.right(5).left(4);
    QDataStream stream(&timeArray, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    uint32_t time;
    stream >> time;
    socketDateTime.setDate(QDate(1900, 01, 01)); // midnight 1900-01-01
    socketDateTime = socketDateTime.addSecs(time);
    qWarning() << "Socket clock:" << socketDateTime.toString();

    commandID[Subscribe] = QByteArray::fromHex ( "63 6c" );
    commandID[PowerOn] = QByteArray::fromHex ( "73 66" );
    commandID[PowerOff] = commandID[PowerOn];
    commandID[TableData] = QByteArray::fromHex ( "72 74" );
    commandID[SocketData] = commandID[TableData];
    commandID[TimingData] = commandID[TableData];
    commandID[WriteSocketData] = QByteArray::fromHex ( "74 6d" );
    QByteArray commandIDPower = QByteArray::fromHex ( "64 63" );

    // 2 hex bytes are the total length of the message
    datagram[Subscribe] = commandID[Subscribe] + mac + twenties + rmac + twenties;
    datagram[PowerOn] = commandIDPower + mac + twenties + zeros + one;
    datagram[PowerOff] = commandIDPower + mac + twenties + zeros + zero;
    datagram[TableData] = commandID[TableData] + mac + twenties + zeros + QByteArray::fromHex ( "01 00 00" ) + zeros;

    udpSocket = new QUdpSocket();
    udpSocket->connectToHost ( ip, 10000 );

    connect (this, &Socket::datagramQueued, this, &Socket::processQueue);
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
    QMutex mutex;
    if ( !mutex.tryLock() )
    {
        return;
    }

    unsigned short retryCount = 0;
    QByteArray currentDatagram, previousDatagram = 0, recordLength;
    while ( commands.size() > 0 )
    {
        currentDatagram = datagram[commands.head()];
        if ( previousDatagram == currentDatagram )
        {
            ++retryCount;
        }
        if ( retryCount == 5 )
        {
            std::cout << "Stop retrying: " << currentDatagram.toHex().toStdString() << std::endl;
            commands.dequeue();
            retryCount = 0;
        }
        QDataStream stream(&recordLength, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        uint16_t length = currentDatagram.length() + 4; // +4 for magicKey and total message length
        stream << length;

        udpSocket->write ( magicKey + recordLength + currentDatagram );
        previousDatagram = currentDatagram;
        QThread::msleep(100);
    }
    mutex.unlock();
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
    writeSocketData(name, remotePassword, timeZone);
}

void Socket::changeSocketPassword ( QString newPassword )
{
    QByteArray password = newPassword.toLatin1().leftJustified(12, ' ', true);
    writeSocketData(socketName, password, timeZone);
}

void Socket::changeTimezone ( int8_t newTimezone )
{
    QByteArray timezone;
    QDataStream stream(&timezone, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << newTimezone;
    writeSocketData(socketName, remotePassword, timezone);
}

void Socket::writeSocketData(QByteArray name, QByteArray password, QByteArray timezone)
{
    QByteArray record = QByteArray::fromHex ( "01:00" ) /* record number = 1*/ + versionID + mac + twenties + rmac + twenties + password + name + icon + hardwareVersion + firmwareVersion + wifiFirmwareVersion + port + staticServerIP + port + domainServerName + localIP + localGatewayIP + localNetMask + dhcpNode + discoverable + timeZoneSet + timezone + QByteArray::fromHex ( "00:ff" ) + countdown;

    QByteArray recordLength;
    QDataStream stream(&recordLength, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    uint16_t length = record.length();
    stream << length;

    datagram[WriteSocketData] = commandID[WriteSocketData] + mac + twenties + zeros + QByteArray::fromHex ( "04:00:01" ) /*table number and unknown*/ + recordLength + record;
    sendDatagram ( WriteSocketData );
}

void Socket::tableData()
{
    sendDatagram ( TableData );
    datagram[SocketData] = commandID[SocketData] + mac + twenties + zeros + QByteArray::fromHex ( "04 00 00" ) + zeros;
    datagram[TimingData] = commandID[TimingData] + mac + twenties + zeros + QByteArray::fromHex ( "03 00 00" ) + zeros;
    // table number + 00 + version number
    sendDatagram ( SocketData );
    sendDatagram ( TimingData );
}

bool Socket::parseReply ( QByteArray reply )
{
    if ( reply.left(2) != magicKey )
    {
        return false;
    }

    QByteArray id = reply.mid ( 4, 2 );
    unsigned int datagram = std::distance ( commandID, std::find ( commandID, commandID + MaxCommands, id ) ); // match commandID with enum
    if ( datagram == TableData ) // determine the table number
    {
        unsigned int table = reply[reply.indexOf ( zeros ) + 4]; // Table number immediately follows zeros
        switch ( table )
        {
        case 1:
            break;
        case 3:
            datagram = TimingData;
            break;
        case 4:
            datagram = SocketData;
            break;
        case 0:
            qWarning() << "No table"; // FIXME: initial data query
        default:
            qWarning() << "Failed to identify data table.";
            datagram = TableData;
            return false;
        }
    }
    switch ( datagram )
    {
    case Subscribe:
    case PowerOff:
    case PowerOn:
    {
        bool poweredOld = powered;
        powered = reply.right(1) == one;
        if ( powered != poweredOld )
        {
            Q_EMIT stateChanged();
        }
        if ( datagram == PowerOff && powered == true ) // Required to deque
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
//         std::cout << reply.toHex().toStdString() << " " << datagram << std::endl; // for debugging purposes only
        unsigned short int index = reply.indexOf ( rmac + twenties );
        versionID = reply.mid ( index - 14, 2 );
        index += 12; // length of rmac + padding
        remotePassword = reply.mid ( index, 12 ); // max 12 symbols
        index += 12;
        socketName = reply.mid ( index, 16 ); // max 16 symbols
        index += 16;
        icon = reply.mid ( index, 2 );
        index += 2;
        hardwareVersion = reply.mid ( index, 4 );
        index += 4;
        firmwareVersion = reply.mid ( index, 4 );
        index += 4;
        wifiFirmwareVersion = reply.mid ( index, 4 );
        index += 6;
        staticServerIP = reply.mid ( index, 4 );  // 42.121.111.208 is used
        index += 6;
        domainServerName = reply.mid ( index, 40);
        index += 40;
        localIP = reply.mid ( index, 4 );
        index += 4;
        localGatewayIP = reply.mid ( index, 4 );
        index += 4;
        localNetMask = reply.mid ( index, 4 );
        index += 4;
        dhcpNode = reply.mid ( index, 1 );
        ++index;
        discoverable = reply.mid ( index, 1 );
        ++index;
        timeZoneSet = reply.mid ( index, 1 );
        ++index;
        timeZone = reply.mid ( index, 1 );
        ++index;
        countdown = reply.mid ( index, 2 );
        Q_EMIT stateChanged();
        break;
    }
    case TimingData:
        std::cout << reply.toHex().toStdString() << " " << datagram << std::endl; // for debugging purposes only
        break;
    case WriteSocketData:
        sendDatagram ( SocketData );
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
