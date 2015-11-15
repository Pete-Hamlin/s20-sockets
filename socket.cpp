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

#include <QDataStream>
#include <QMutex>
#include <QThread>
#include <QUdpSocket>

Socket::Socket(QHostAddress IPaddress, QByteArray reply)
{
    ip = IPaddress;
    mac = reply.mid(7, 6);
    rmac = mac;
    std::reverse(rmac.begin(), rmac.end());
    powered = reply.right(1) == one;

    commandID[QueryAll] = QStringLiteral("qa").toLatin1();
    commandID[Discover] = QStringLiteral("qg").toLatin1();   // qg
    commandID[Subscribe] = QStringLiteral("cl").toLatin1();   // Login Command
    commandID[PowerOn] = QStringLiteral("sf").toLatin1();   // State Flip (change of power state)
    commandID[PowerOff] = commandID[PowerOn];
    commandID[ReadTable] = QStringLiteral("rt").toLatin1();
    commandID[SocketData] = commandID[ReadTable];
    commandID[TimingData] = commandID[ReadTable];
    commandID[TableModify] = QStringLiteral("tm").toLatin1();
    QByteArray commandIDPower = QStringLiteral("dc").toLatin1();   // Socket change responce

    // 2 hex bytes are the total length of the message
    datagram[Discover] = commandID[Discover] + mac + twenties;
    datagram[Subscribe] = commandID[Subscribe] + mac + twenties + rmac + twenties;
    datagram[PowerOn] = commandIDPower + mac + twenties + zeros + one;
    datagram[PowerOff] = commandIDPower + mac + twenties + zeros + zero;
    datagram[ReadTable] = commandID[ReadTable] + mac + twenties + /*zeros*/QByteArray::fromHex("72 00 00 00") + QByteArray::fromHex("01 00 00") + zeros;

    udpSocket = new QUdpSocket();
    udpSocket->connectToHost(ip, 10000);

    connect(this, &Socket::datagramQueued, this, &Socket::processQueue);
    subscribeTimer = new QTimer(this);
    subscribeTimer->setInterval(2 * 60 * 1000); // 2 min
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

void Socket::sendDatagram(Datagram d)
{
    commands.enqueue(d);
    Q_EMIT datagramQueued();
}

void Socket::run()
{
    QMutex mutex;
    if (!mutex.tryLock()) {
        return;
    }

    unsigned short retryCount = 0;
    QByteArray currentDatagram, previousDatagram = 0, recordLength;
    while (commands.size() > 0) {
        currentDatagram = datagram[commands.head()];
        if (previousDatagram == currentDatagram) {
            ++retryCount;
        }
        if (retryCount == 5) {
            std::cout << "Stop retrying: " << currentDatagram.toHex().toStdString() << std::endl;
            commands.dequeue();
            retryCount = 0;
        }

        uint16_t length = currentDatagram.length() + 4; // +4 for magicKey and total message length
        recordLength = intToHex(length, 2, false);

        udpSocket->write(magicKey + recordLength + currentDatagram);
        previousDatagram = currentDatagram;
        QThread::msleep(100);
    }
    mutex.unlock();
}

void Socket::subscribe()
{
    sendDatagram(Subscribe);
}

void Socket::discover()
{
    sendDatagram(Discover);
}

void Socket::toggle()
{
    sendDatagram(powered ? PowerOff : PowerOn);
}

void Socket::powerOff()
{
    if (powered)
        sendDatagram(PowerOff);
    Q_EMIT stateChanged();
}

void Socket::powerOn()
{
    if (!powered)
        sendDatagram(PowerOn);
    Q_EMIT stateChanged();
}

void Socket::changeSocketName(QString newName)
{
    QByteArray name = newName.toLatin1().leftJustified(16, ' ', true);
    writeSocketData(name, remotePassword, timezone, countdown);
}

void Socket::changeSocketPassword(QString newPassword)
{
    QByteArray password = newPassword.toLatin1().leftJustified(12, ' ', true);
    writeSocketData(socketName, password, timezone, countdown);
}

void Socket::changeTimezone(int8_t newTimezone)
{
    writeSocketData(socketName, remotePassword, newTimezone, countdown);
}

void Socket::setCountDown(uint16_t countdown)
{
    writeSocketData(socketName, remotePassword, timezone, countdown);
}

void Socket::toggleCountDown()
{
    countdownEnabled=!countdownEnabled;
    writeSocketData(socketName, remotePassword, timezone, countdown);
}

void Socket::writeSocketData(QByteArray socketName, QByteArray remotePassword, int8_t timezone, uint16_t countdown)
{
    QByteArray countDown = intToHex(countdown, 2); // 2 bytes

    QByteArray record = QByteArray::fromHex("01:00") /* record number = 1*/ + versionID + mac + twenties + rmac + twenties + remotePassword + socketName + icon + hardwareVersion + firmwareVersion + wifiFirmwareVersion + port + staticServerIP + port + domainServerName + localIP + localGatewayIP + localNetMask + dhcpNode + discoverable + timeZoneSet + intToHex(timezone, 1) + (countdownEnabled ? QByteArray::fromHex("01:00") : QByteArray::fromHex("00:ff")) + countDown + zeros + zeros + zeros + QStringLiteral("000000000000000000000000000000").toLocal8Bit();

    QByteArray recordLength = intToHex(record.length(), 2); // 2 bytes

    datagram[TableModify] = commandID[TableModify] + mac + twenties + zeros + QByteArray::fromHex("04:00:01") /*table number and unknown*/ + recordLength + record;
    sendDatagram(TableModify);
}

void Socket::tableData()
{
    sendDatagram(ReadTable);
    datagram[SocketData] = commandID[SocketData] + mac + twenties + zeros + QByteArray::fromHex("04 00 00") + zeros;
    datagram[TimingData] = commandID[TimingData] + mac + twenties + zeros + QByteArray::fromHex("03 00 00") + zeros;
    // table number + 00 + version number
    sendDatagram(SocketData);
    sendDatagram(TimingData);
}

bool Socket::parseReply(QByteArray reply)
{
    if (reply.left(2) != magicKey) {
        return false;
    }

    QByteArray id = reply.mid(4, 2);
    unsigned int datagram = std::distance(commandID, std::find(commandID, commandID + MaxCommands, id));       // match commandID with enum
    if (datagram == ReadTable) { // determine the table number
        unsigned int table = reply[reply.indexOf(twenties) + 11];
        switch (table) {
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
            datagram = ReadTable;
            return false;
        }
    }
    switch (datagram) {
    case QueryAll:
    case Discover: {
        uint32_t time = hexToInt(reply.right(5).left(4));
        socketDateTime.setDate(QDate(1900, 01, 01)); // midnight 1900-01-01
        socketDateTime.setTime(QTime(0, 0, 0));
        socketDateTime = socketDateTime.addSecs(time);
    }
    case Subscribe:
    case PowerOff:
    case PowerOn: {
        bool poweredOld = powered;
        powered = reply.right(1) == one;
        if (powered != poweredOld) {
            Q_EMIT stateChanged();
        }
        if (datagram == PowerOff && powered == true) { // Required to deque
            datagram = PowerOn;
        }
        break;
    }
    case ReadTable:
//         FIXME: order might be swapped;
        socketTableVersion = reply.mid(reply.indexOf(QByteArray::fromHex("000100000600")) + 6, 2);
//      000100000600
        break;
    case SocketData: {
//         std::cout << reply.toHex().toStdString() << " " << datagram << std::endl; // for debugging purposes only
        unsigned short int index = reply.indexOf(rmac + twenties);
        versionID = reply.mid(index - 14, 2);
        index += 12; // length of rmac + padding
        remotePassword = reply.mid(index, 12);    // max 12 symbols
        index += 12;
        socketName = reply.mid(index, 16);    // max 16 symbols
        index += 16;
        icon = reply.mid(index, 2);
        index += 2;
        hardwareVersion = reply.mid(index, 4);
        index += 4;
        firmwareVersion = reply.mid(index, 4);
        index += 4;
        wifiFirmwareVersion = reply.mid(index, 4);
        index += 6;
        staticServerIP = reply.mid(index, 4);     // 42.121.111.208 is used
        index += 6;
        domainServerName = reply.mid(index, 40);
        index += 40;
        localIP = reply.mid(index, 4);
        index += 4;
        localGatewayIP = reply.mid(index, 4);
        index += 4;
        localNetMask = reply.mid(index, 4);
        index += 4;
        dhcpNode = reply.mid(index, 1);
        ++index;
        discoverable = reply.mid(index, 1);
        ++index;
        timeZoneSet = reply.mid(index, 1);
        ++index;
        timezone = hexToInt(reply.mid(index, 1));
	socketDateTime = socketDateTime.addSecs(timezone *3600);
        ++index;
        countdownEnabled = reply.mid(index, 2) == QByteArray::fromHex("01:00");
        index += 2;
        countdown = hexToInt(reply.mid(index, 2));
        Q_EMIT stateChanged();
        break;
    }
    case TimingData:
//         std::cout << reply.toHex().toStdString() << " " << datagram << std::endl; // for debugging purposes only
        break;
    case TableModify:
        sendDatagram(SocketData);
        break;
    default:
        return false;
    }

    if (commands.size() > 0) {
        if (datagram == commands.head()) {
            commands.dequeue();
        }
    }

    return true;
}

// length in bytes
QByteArray Socket::intToHex(unsigned int decimal, unsigned int length, bool littleEndian) {
    QByteArray hex;
    QDataStream stream(&hex, QIODevice::WriteOnly);
    littleEndian ? stream.setByteOrder(QDataStream::LittleEndian) : stream.setByteOrder(QDataStream::BigEndian);
    stream << decimal;
    return littleEndian ? hex.left(length) : hex.right(length);
}

int Socket::hexToInt(QByteArray hex, bool littleEndian) {
    QDataStream stream(&hex, QIODevice::ReadOnly);
    littleEndian ? stream.setByteOrder(QDataStream::LittleEndian) : stream.setByteOrder(QDataStream::BigEndian);
    switch (hex.length()) {
    case 1:
        uint8_t value;
        stream >> value;
        return value;
    case 2:
        uint16_t value2;
        stream >> value2;
        return value2;
    case 4:
        uint32_t value4;
        stream >> value4;
        return value4;
    default:
        return 0;
    }
}
