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

#include "consolereader.h"

ConsoleReader::ConsoleReader ( std::vector<Socket*> *sockets_vector )
{
    sockets = sockets_vector;
    start();
}

ConsoleReader::~ConsoleReader()
{
    delete discoveryTimer;
}

void ConsoleReader::run()
{
    QThread::sleep(1); // wait until sockets are discovered
    for ( unsigned i = 0; i < sockets->size(); ++i )
    {
        connect((*sockets)[i], &Socket::stateChanged, this, &ConsoleReader::listSockets);
    }
    listSockets();

    std::string command;
    unsigned int number = 0;
    bool cont = true;

    while ( cont )
    {
        std::cin >> command;
        switch ( command[0] )
        {
        case 'd':
            (*sockets) [number]->tableData();
            break;
        case 'n':
        {
            command.erase(0,1);
            (*sockets) [number]->changeSocketName(QString::fromStdString(command));
            break;
        }
        case 'p':
            (*sockets) [number]->toggle();
            break;
        case 'q':
            cont = false;
            emit ( QCoreApplication::quit() );
            break;
        case 's':
            std::cin >> number;
            --number; // count from 0
            listSockets();
            break;
        default:
            std::cout << "Invalid command: try again" << std::endl;
        }
    }
}

void ConsoleReader::listSockets()
{
    for ( std::vector<Socket*>::const_iterator i = sockets->begin() ; i != sockets->end(); ++i )
    {
        std::cout << "___________________________________________________________________________\n" << std::endl;
        std::cout << "IP Address: " << (*i)->ip.toString().toStdString() << "\t MAC Address: " << (*i)->mac.toHex().toStdString()  << "\t Power: " << ( (*i)->powered ? "On" : "Off" ) << std::endl;
        std::cout << "Socket Name: " << (*i)->name.toStdString() << "\t Remote Password: " << (*i)->remotePassword.toStdString() << std::endl;
    }
    std::cout << "___________________________________________________________________________\n" << std::endl;
    std::cout << "d - update table data\nnName - change socket name to Name (max 16 characters)\ns - pick another socket (default is 1)\np - toggle power state\nq - quit" << std::endl;
}
