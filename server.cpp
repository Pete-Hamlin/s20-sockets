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

#include <QUdpSocket>

#include "consolereader.h"
#include "server.h"

Server::Server ( std::vector<Socket*> *sockets_vector )
{
    sockets = sockets_vector;
    QUdpSocket *udpSocketSend = new QUdpSocket();
    udpSocketGet = new QUdpSocket();

    udpSocketSend->connectToHost ( QHostAddress::Broadcast, 10000 );
    udpSocketGet->bind ( QHostAddress::Any, 10000);

    udpSocketSend->write ( discover );
    udpSocketSend->disconnectFromHost();
    delete udpSocketSend;

    connect ( udpSocketGet, &QUdpSocket::readyRead, this, &Server::readPendingDatagrams);
    qWarning() << "starting server";
    start();
}

Server::~Server()
{
    delete udpSocketGet;
}

void Server::run()
{
    readPendingDatagrams();
}

void Server::readPendingDatagrams()
{
//     qWarning () << "reading datagam";
    while ( udpSocketGet->hasPendingDatagrams() )
    {
        QByteArray reply;
        reply.resize ( udpSocketGet->pendingDatagramSize() );
        QHostAddress sender;
        quint16 senderPort;

        udpSocketGet->readDatagram ( reply.data(), reply.size(), &sender, &senderPort );

        if ( reply != discover && reply.left ( 2 ) == magicKey ) // check for Magic Key
        {
            if ( reply.mid ( 4, 2 ) == QByteArray::fromHex ( "71 61" ) ) // Reply to discover packet
            {
                bool duplicate = false;
                for ( std::vector<Socket*>::const_iterator i = sockets->begin() ; i != sockets->end(); ++i )
                {
                    if ( (*i)->ip == sender )
                    {
                        duplicate = true;
                        break;
                    }
                }
                if ( !duplicate )
                {
                    qWarning() << "Socket found";
                    Socket *socket = new Socket ( sender, reply );
                    sockets->push_back ( socket );
                }
            }
            else
            {
//                 qWarning() << "preparing to parse datagram";
                QByteArray mac = reply.mid(6,6);
                for ( std::vector<Socket*>::iterator i = sockets->begin() ; i != sockets->end(); ++i )
                {
                    if ( (*i)->mac == mac )
                    {
                        (*i)->parseReply(reply);
                        break;
                    }

                }
            }
        }
    }
}
