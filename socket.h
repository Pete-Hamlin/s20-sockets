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

#ifndef SOCKET_H
#define SOCKET_H

#include <QByteArray>
#include <QDateTime>
#include <QHostAddress>
#include <QQueue>
#include <QTimer>
#include <QThread>

class QUdpSocket;

const QByteArray magicKey = QByteArray::fromHex("68 64");    // recognize datagrams from the socket

class Socket : public QThread
{
    Q_OBJECT

Q_SIGNALS:
    void stateChanged();
    void datagramQueued();

public:
    Socket(QHostAddress, QByteArray);    // from discovery packet
    ~Socket();
    void toggle();
    void powerOff();
    void powerOn();
    void discover();
    void tableData();
    void changeSocketName(QString newName);
    void changeSocketPassword(QString newPassword);
    void changeTimezone(int8_t newTimezone);
    void setCountDown(uint16_t countdown);
    void toggleCountDown();
    bool parseReply(QByteArray);

    QHostAddress ip;
    QByteArray mac;
    bool powered;
    QByteArray socketName, remotePassword;
    QByteArray timeZone;
    uint16_t countdown;
    bool countdownEnabled;
    QDateTime socketDateTime;

private:
    enum Datagram {
        QueryAll,
        Discover,
        Subscribe,
        PowerOff,
        PowerOn,
        ReadTable,
        SocketData,
        TimingData,
        TableModify,
        MaxCommands
    };

    void sendDatagram(Datagram);
    void subscribe();
    void processQueue() {
        start();
    }
    void run();
    void writeSocketData(QByteArray socketName, QByteArray remotePassword, QByteArray timeZone, uint16_t countdown);

    QByteArray commandID[MaxCommands];
    QByteArray datagram[MaxCommands];
    QByteArray rmac; // Reveresed mac
    QByteArray versionID;
    QByteArray icon; // displayed icon
    QByteArray hardwareVersion;
    QByteArray firmwareVersion;
    QByteArray wifiFirmwareVersion; // CC3000 firmware version
    QByteArray staticServerIP;
    QByteArray port = QByteArray::fromHex("10:27");    // 10000 in hex (little endian)
    QByteArray domainServerName;
    QByteArray localIP; // same as QHostAddress ip but in QByteArray, maybe fix later...
    QByteArray localGatewayIP;
    QByteArray localNetMask;
    QByteArray dhcpNode;
    QByteArray discoverable;
    QByteArray timeZoneSet;
    QByteArray socketTableNumber, socketTableVersion, timingTableNumber, timingTableVersion; // FIXME: not used yet

    const QByteArray twenties = QByteArray::fromHex("20 20 20 20 20 20");    // mac address padding, 6 spaces
    const QByteArray zeros = QByteArray::fromHex("00 00 00 00");
    const QByteArray zero = QByteArray::fromHex("00");
    const QByteArray one = QByteArray::fromHex("01");

    QUdpSocket *udpSocket;
    QTimer *subscribeTimer;
    bool subscribed;
    QQueue<Datagram> commands;

};

#endif  /* SOCKET_H */
