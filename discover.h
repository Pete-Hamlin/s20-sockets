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

QByteArray discover = QByteArray::fromHex("68 64 00 06 71 61");

void readDiscoverDatagrams(QUdpSocket *udpSocketGet, std::vector<Socket> *sockets)
{
    while (udpSocketGet->waitForReadyRead(500)) // 500ms
    {
        while (udpSocketGet->hasPendingDatagrams())
        {
            QByteArray datagramGet;
            datagramGet.resize(udpSocketGet->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;

            udpSocketGet->readDatagram(datagramGet.data(), datagramGet.size(), &sender, &senderPort);

            if (datagramGet != discover && datagramGet.left(2) == QByteArray::fromHex("68 64"))
            {
                bool duplicate = false;
                for(std::vector<Socket>::const_iterator i = sockets->begin() ; i != sockets->end(); ++i)
                {
                    if (i->ip == sender)
                        duplicate = true;
                }
                if(!duplicate)
                {
                    const Socket socket(sender, datagramGet);
                    sockets->push_back(socket);
                }
            }
        }
    }
}
