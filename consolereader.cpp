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
#include "server.h"

ConsoleReader::ConsoleReader(std::vector<Socket*> *sockets_vector)
{
    sockets = sockets_vector;
    start();
}

void ConsoleReader::run()
{
    std::cout << "Copyright (C) 2015 Andrius Štikonas.\nLicense GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law." << std::endl;
    connectSignals();
    listSockets();

    std::string command;
    activeSocket = 0;
    bool cont = true;

    while (cont) {
        std::cin >> command;
        switch (command[0]) {
        case 'a': {
            std::cout << "Please set your Orvibo socket to pair mode (rapidly blinking blue light) and wait until new wifi network (WiWo-S20) appears" << std::endl;
            std::string ssid, password;
            std::cout << "Wireless name (\"c\" for current wifi): ";
            std::cin >> ssid;
            std::cout << "Password: ";
            std::cin >> password;
            Server *server = new Server(48899, QByteArray::fromStdString(ssid), QByteArray::fromStdString(password)); // HF-LPB100 chip can be controlled over port 48899
            break;
        }
        case 'A': {
            std::cout << "Please set your Orvibo socket to factory reset mode (rapidly blinking red light)" << std::endl;
            std::string password;
            std::cout << "Password: ";
            std::cin >> password;
            broadcastPassword(QString::fromStdString(password)); // HF-LPB100 chip can be controlled over port 49999
            break;
        }
        case 'd':
            (*sockets) [activeSocket]->tableData();
            break;
        case 'D':
            (*sockets) [activeSocket]->discover();
            break;
        case 'n': {
            std::string name;
            std::cout << "Please enter a new name: ";
            std::cin >> name;
            (*sockets) [activeSocket]->changeSocketName(QString::fromStdString(name));
            break;
        }
        case 'p':
            (*sockets) [activeSocket]->toggle();
            break;
        case 'o':
            if (command == "off")
                (*sockets) [activeSocket]->powerOff();
            else if (command == "on")
                (*sockets) [activeSocket]->powerOn();
            else {
                uint16_t offTime;
                std::cout << "Off time in seconds: ";
                std::cin >> offTime;
                (*sockets) [activeSocket]->setOffTimer(offTime);
            }
            break;
        case 'O':
            (*sockets) [activeSocket]->toggleOffTimer();
            break;
        case 'P': {
            std::string password;
            std::cout << "Please enter a new password: ";
            std::cin >> password;
            (*sockets) [activeSocket]->changeSocketPassword(QString::fromStdString(password));
            break;
        }
        case 'q':
            cont = false;
            emit(QCoreApplication::quit());
            break;
        case 's':
            std::cin >> activeSocket;
            --activeSocket; // count from 0
            listSockets();
            break;
        case 't': {
            int timezone;
            std::cout << "Please enter a new timezone (integer from -11 to 12): ";
            std::cin >> timezone;
            if (timezone >= -11 && timezone <= 12)
                (*sockets) [activeSocket]->changeTimezone(timezone);
            else
                std::cout << "Invalid timezone";
            break;
        }
        default:
            std::cout << "Invalid command: try again" << std::endl;
        }
    }
}

void ConsoleReader::listSockets()
{
    for (std::vector<Socket*>::const_iterator i = sockets->begin() ; i != sockets->end(); ++i) {
        std::cout << "_____________________________________________________________________________\n" << std::endl;
        std::cout << "IP Address: " << (*i)->ip.toString().remove("::ffff:").toStdString() << "\t MAC Address: " << (*i)->mac.toHex().toStdString()  << "\t Power: " << ((*i)->powered ? "On" : "Off") << std::endl;
        std::cout << "Socket Name: " << (*i)->socketName.toStdString() << "\t Remote Password: " << (*i)->remotePassword.toStdString() << "\t Timezone: " << +(*i)->timezone << std::endl;
        std::cout << "Off timer: " << (*i)->offTime << " " << ((*i)->offTimerEnabled ? "(enabled)" : "(disabled)")  << "\t\t Time: " << (*i)->socketDateTime.toString().toStdString() << std::endl;
    }
    std::cout << "_____________________________________________________________________________\n" << std::endl;
    if (sockets->size() > 0) {
        std::cout << "Active socket: " << (*sockets) [activeSocket]->socketName.toStdString() << std::endl;
    }
    std::cout << "a - add unpaired socket (WiFi needed)\n";
    std::cout << "A - add unpaired socket (no WiFi needed)\n";
    std::cout << "d - update table data\n";
    std::cout << "D - resend discovery packet to the current socket\n";
    std::cout << "n - change socket name (max 16 characters)\n";
    std::cout << "o - set switch off timer\n";
    std::cout << "O - enable/disable switch off timer\n";
    std::cout << "p - toggle power state (there are also \"on\" and \"off\" commands)\n";
    std::cout << "P - change remote password (max 12 characters)\nq - quit\ns - select another socket (default is 1)\n";
    std::cout << "t - change timezone" << std::endl;
    std::cout << "Enter command: " << std::endl;
}

void ConsoleReader::connectSignals()
{
    for (unsigned i = 0; i < sockets->size(); ++i) {
        connect((*sockets)[i], &Socket::stateChanged, this, &ConsoleReader::listSockets);
    }
}
