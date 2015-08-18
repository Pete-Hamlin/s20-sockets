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

#include <QComboBox>

#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(std::vector<Socket*> *sockets_vector, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    sockets = sockets_vector;
    ui->setupUi(this);

}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::updateUi()
{
    for (unsigned int i = 0; i < (*sockets).size(); ++i) {
        ui->toggleButton->setText((*sockets)[i]->powered ? QStringLiteral("Turn off") : QStringLiteral("Turn on"));
        ui->comboBox->setItemText(i, (*sockets)[i]->socketName);
    }
}

void Dialog::discovered()
{
    ui->comboBox->clear();
    for (std::vector<Socket*>::const_iterator i = sockets->begin() ; i != sockets->end(); ++i) {
        connect(*i, &Socket::stateChanged, this, &Dialog::updateUi);
        connect(ui->toggleButton, &QPushButton::clicked, this, &Dialog::togglePower);
        ui->comboBox->addItem("Socket");
    }

    updateUi();
}

void Dialog::togglePower()
{
    (*sockets)[ui->comboBox->currentIndex()]->toggle();
}
