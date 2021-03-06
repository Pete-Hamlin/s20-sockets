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

#include <QApplication>
#include <vector>

#include "dialog.h"
#include "server.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    std::vector<Socket*> *sockets = new std::vector<Socket*>;
    Server *server = new Server(sockets);
    Dialog *w = new Dialog(sockets);
    QObject::connect(server, &Server::discovered, w, &Dialog::discovered);
    w->show();

    return app.exec();
}
