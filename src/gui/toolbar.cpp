/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QAction>
#include <QDebug>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTimer>

#include <DApplication>
#include <DButtonBox>
#include <DSearchEdit>

#include "constant.h"
#include "toolbar.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE
using namespace Utils;

Toolbar::Toolbar(MainWindow *m, QWidget *parent)
    : QWidget(parent)
    , m_mainWindow(m)
{
    installEventFilter(this);  // add event filter
    setMouseTracking(true);    // make MouseMove can response

    setFixedHeight(36);

    // =========layout=========
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // =========tab button=========
    m_switchFuncTabBtnGrp = new DButtonBox();
    m_switchFuncTabBtnGrp->setFixedWidth(240);
    DButtonBoxButton *procBtn = new DButtonBoxButton(
        DApplication::translate("Title.Bar.Switch", "Process"), m_switchFuncTabBtnGrp);
    procBtn->setCheckable(true);
    procBtn->setChecked(true);
    DButtonBoxButton *svcBtn = new DButtonBoxButton(
        DApplication::translate("Title.Bar.Switch", "Service"), m_switchFuncTabBtnGrp);
    svcBtn->setCheckable(true);
    QList<DButtonBoxButton *> list;
    list << procBtn << svcBtn;
    m_switchFuncTabBtnGrp->setButtonList(list, true);

    connect(procBtn, &DButtonBoxButton::clicked, this, [=]() { Q_EMIT procTabButtonClicked(); });
    connect(svcBtn, &DButtonBoxButton::clicked, this, [=]() { Q_EMIT serviceTabButtonClicked(); });

    // =========search=========
    searchEdit = new DSearchEdit();
    searchEdit->setFixedWidth(360);
    searchEdit->setPlaceHolder(DApplication::translate("Title.Bar.Search", "Search"));
    this->installEventFilter(this);

    layout->addWidget(m_switchFuncTabBtnGrp, 0, Qt::AlignLeft);
    layout->addStretch();
    layout->addWidget(searchEdit, 0, Qt::AlignHCenter);
    layout->addStretch();

    searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    connect(searchTimer, &QTimer::timeout, this, &Toolbar::handleSearch);

    connect(searchEdit, &DSearchEdit::textChanged, this, &Toolbar::handleSearchTextChanged,
            Qt::QueuedConnection);
}

Toolbar::~Toolbar() {}

bool Toolbar::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        if (obj == this) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                searchEdit->clear();

                pressEsc();
            }
        } else if (obj == searchEdit) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Tab) {
                pressTab();
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

void Toolbar::handleSearch()
{
    if (searchEdit->text() == searchTextCache) {
        search(searchTextCache);
    }
}

void Toolbar::handleSearchTextChanged()
{
    searchTextCache = searchEdit->text();

    if (searchTimer->isActive()) {
        searchTimer->stop();
    }
    searchTimer->start(300);
}

void Toolbar::focusInput()
{
    if (searchEdit->text() != "") {
        searchEdit->setFocus();
    } else {
        searchEdit->setFocus();
    }
}