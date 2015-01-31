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

#include "consolereader.h"
#include "discover.h"

void listSockets(std::vector<Socket> const &sockets);

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QUdpSocket *udpSocketSend = new QUdpSocket();
    QUdpSocket *udpSocketGet = new QUdpSocket();

    udpSocketSend->connectToHost(QHostAddress::Broadcast, 10000);
    udpSocketGet->bind(QHostAddress::Any, 10000, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    udpSocketSend->write(discover);
    udpSocketSend->disconnectFromHost();
    delete udpSocketSend;
    std::vector<Socket> *sockets = new std::vector<Socket>;

    readDiscoverDatagrams(udpSocketGet, sockets);
    delete udpSocketGet;

    ConsoleReader reader(sockets);
    reader.start();

    int exitCode = app.exec();
    reader.wait();
    return exitCode;
}
