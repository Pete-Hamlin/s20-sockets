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

#include "discover.h"

void readDiscoverDatagrams(QUdpSocket *udpSocketGet, std::vector<Socket> &sockets);
void listSockets(std::vector<Socket> const &sockets);

int main(int argc, char *argv[])
{
    QUdpSocket *udpSocketSend = new QUdpSocket();
    QUdpSocket *udpSocketGet = new QUdpSocket();

    udpSocketSend->connectToHost(QHostAddress::Broadcast, 10000);
    udpSocketGet->bind(QHostAddress::Any, 10000);

    udpSocketSend->write(discover);
    udpSocketSend->disconnectFromHost();
    delete udpSocketSend;
    std::vector<Socket> sockets;

    readDiscoverDatagrams(udpSocketGet, sockets);
    delete udpSocketGet;

    char command;
    unsigned int number = 0;
    bool cont=true;
    while (cont)
    {
        listSockets(sockets);
        std::cout << "d - show table data\ns - pick another socket (default is 1)\np - toggle power state\nq - quit" << std::endl;
        std::cin >> command;
        switch (command)
        {
            case 'd':
                sockets[number].tableData();
                break;
            case 'p':
                sockets[number].toggle();
                break;
            case 'q':
                cont = false;
                break;
            case 's':
                std::cin >> number;
                --number; // count from 0
                break;
            default:
                std::cout << "Invalid command" << std::endl;
        }
    }
    return 0;
}

void listSockets(std::vector<Socket> const &sockets)
{
    for (std::vector<Socket>::const_iterator i = sockets.begin() ; i != sockets.end(); ++i)
    {
        std::cout << "IP Address: " << i->ip.toString().toStdString() << "\t MAC Address: " << i->mac.toHex().toStdString()  << "\t Power: " << (i->powered ? "On" : "Off") << std::endl;
    }
}
