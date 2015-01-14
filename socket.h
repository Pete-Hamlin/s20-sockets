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

#include <QByteArray>
#include <QHostAddress>

class Socket
{
public:
    Socket(QHostAddress, QByteArray);

    QHostAddress ip;
    QByteArray mac;
    bool powered;
    enum {Subscribe, PowerOff, PowerOn};
    
private:
    QByteArray datagram[3];
//         QByteArray::fromHex("68 64 00 1e 63 6c ac cf 23 35 f5 8c 20 20 20 20 20 20 8c f5 35 23 cf ac 20 20 20 20 20 20"), // subscribe
//         QByteArray::fromHex("68 64 00 17 64 63 ac cf 23 35 f5 8c 20 20 20 20 20 20 00 00 00 00 00"), // power off
//         QByteArray::fromHex("68 64 00 17 64 63 ac cf 23 35 f5 8c 20 20 20 20 20 20 00 00 00 00 01") // power on
};
