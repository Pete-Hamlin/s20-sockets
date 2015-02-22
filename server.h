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

#ifndef SERVER_H
#define SERVER_H

#include <QThread>

#include "socket.h"

class QUdpSocket;

class Server : public QThread
{
public:
    Server ( std::vector<Socket*> *sockets_vector, uint16_t port );
    Server ( uint16_t port, QByteArray ssid, QByteArray password );
    ~Server();

    void readPendingDatagrams();
    void run();

private:
    QByteArray discover = QByteArray::fromHex ( "68 64 00 06 71 61" );
    QUdpSocket *udpSocketGet;
    std::vector<Socket*> *sockets;

    void discoverSockets(uint16_t port);
    QByteArray listen( QByteArray message = 0 );
};

#endif  /* SERVER_H */
