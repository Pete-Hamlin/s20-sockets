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

#include <QNetworkConfiguration>
#include <QNetworkConfigurationManager>
#include <QNetworkSession>

#include <QTimer>
#include <QUdpSocket>
#include <iostream>

#include "consolereader.h"
#include "server.h"

Server::Server ( std::vector<Socket*> *sockets_vector )
{
    sockets = sockets_vector;
    udpSocketGet = new QUdpSocket();
    udpSocketGet->bind ( QHostAddress::Any, 10000 );
    connect ( udpSocketGet, &QUdpSocket::readyRead, this, &Server::readPendingDatagrams);
    discoverSockets();
    QTimer *discoverTimer = new QTimer(this);
    discoverTimer->setInterval(1*60*1000); // 1 min
    discoverTimer->setSingleShot(false);
    connect(discoverTimer, &QTimer::timeout, this, &Server::discoverSockets);
    discoverTimer->start();

    start();
}

Server::Server(uint16_t port, QByteArray password)
{
    QNetworkConfiguration *cfg = new QNetworkConfiguration;
    QNetworkConfigurationManager *ncm = new QNetworkConfigurationManager;
    QEventLoop *loop = new QEventLoop;
    loop->connect(ncm, &QNetworkConfigurationManager::updateCompleted, loop, &QEventLoop::quit);
    ncm->updateConfigurations();
    loop->exec();
    delete loop;
    *cfg = ncm->defaultConfiguration();
    QByteArray ssid = cfg->name().toLocal8Bit();

    auto nc = ncm->allConfigurations();

    for (auto &x : nc)
    {
        if (x.bearerType() == QNetworkConfiguration::BearerWLAN)
        {
            if (x.name() == "WiWo-S20")
            {
                std::cout << "Connecting to WiWo-S20 wireless" << std::endl;
                cfg = &x;
            }
        }
    }

    auto session = new QNetworkSession(*cfg, this);
    session->open();
    std::cout << "Wait for connected!" << std::endl;
    if (session->waitForOpened())
        std::cout << "Connected!" << std::endl;

    QUdpSocket *udpSocketSend = new QUdpSocket();
    udpSocketGet = new QUdpSocket();
    udpSocketGet->bind ( QHostAddress::Any, port);

    QByteArray reply;

    udpSocketGet->writeDatagram ( QByteArray::fromStdString("HF-A11ASSISTHREAD"), QHostAddress::Broadcast, port );
    reply = listen(QByteArray::fromStdString("HF-A11ASSISTHREAD"));
    QList<QByteArray> list = reply.split(',');
    QHostAddress ip(QString::fromLatin1(list[0]));
    std::cout << "IP: " << ip.toString().toStdString() << std::endl;
    udpSocketGet->writeDatagram ( QByteArray::fromStdString("+ok"), ip, port );
    udpSocketGet->writeDatagram ( QByteArray::fromStdString("AT+WSSSID=") + ssid + QByteArray::fromStdString("\r"), ip, port );
    listen();
    udpSocketGet->writeDatagram ( QByteArray::fromStdString("AT+WSKEY=WPA2PSK,AES,") + password + QByteArray::fromStdString("\r"), ip, port ); // FIXME: support different security settings
    // OPEN, SHARED, WPAPSK......NONE, WEP, TKIP, AES
    listen();
    udpSocketGet->writeDatagram ( QByteArray::fromStdString("AT+WMODE=STA\r"), ip, port );
    listen();
    udpSocketGet->writeDatagram ( QByteArray::fromStdString("AT+Z\r"), ip, port ); // reboot
    session->close();
    // FIXME: discover the new socket
    std::cout << "Finished" << std::endl;
}

Server::~Server()
{
    delete udpSocketGet;
}

QByteArray Server::listen(QByteArray message)
{
    QByteArray reply;
    QHostAddress sender;
    quint16 senderPort;
    bool stop = false;
    while ( !stop )
    {
        QThread::msleep(50);
        while ( udpSocketGet->hasPendingDatagrams() )
        {
            reply.resize ( udpSocketGet->pendingDatagramSize() );
            udpSocketGet->readDatagram ( reply.data(), reply.size(), &sender, &senderPort );
            if (reply != message)
            {
                stop = true;
                std::cout << reply.toStdString() << std::endl;
            }
        }
    }
    return reply;
}

void Server::run()
{
    readPendingDatagrams();
}

void Server::readPendingDatagrams()
{
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
                    Socket *socket = new Socket ( sender, reply );
                    sockets->push_back ( socket );
		    Q_EMIT discovered();
                }
            }
            else
            {
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

void Server::discoverSockets()
{
    QUdpSocket *udpSocketSend = new QUdpSocket();
    udpSocketSend->connectToHost ( QHostAddress::Broadcast, 10000 );
    udpSocketSend->write ( discover );
    udpSocketSend->write ( discover );
    udpSocketSend->disconnectFromHost();
    delete udpSocketSend;
}
