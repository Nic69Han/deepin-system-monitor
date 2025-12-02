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

#include "attributes_dialog.h"
#include "constant.h"
#include "utils.h"
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QHeaderView>
#include <dthememanager.h>
#include <proc/readproc.h>
#include <proc/sysinfo.h>

DWIDGET_USE_NAMESPACE

using namespace Utils;
using namespace Dtk;

AttributesDialog::AttributesDialog(QWidget *parent, int processId) : DAbstractDialog(parent)
{
    pid = processId;

    setAttribute(Qt::WA_DeleteOnClose, true);

    setMinimumWidth(320);

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    nameLayout = new QHBoxLayout();
    nameLayout->setContentsMargins(0, 0, 0, 0);
    cmdlineLayout = new QHBoxLayout();
    cmdlineLayout->setContentsMargins(0, 0, 0, 0);
    startTimeLayout = new QHBoxLayout();
    startTimeLayout->setContentsMargins(0, 0, 0, 0);

    closeButton = new DWindowCloseButton;
    closeButton->setFixedSize(27, 23);
    connect(closeButton, &DWindowCloseButton::clicked, this, &DAbstractDialog::close);
    Dtk::Widget::DThemeManager::instance()->setTheme(closeButton, "light") ;

    iconLabel = new QLabel();

    titleLabel = new QLabel();
    titleLabel->setStyleSheet("QLabel { background-color : transparent; font-size: 14px; font-weight: 500; color : #303030; }");

    nameTitleLabel = new QLabel(QString("%1:").arg(tr("Process name")));
    nameTitleLabel->setStyleSheet("QLabel { background-color : transparent; color : #666666; }");
    nameTitleLabel->setFixedWidth(100);
    nameTitleLabel->setAlignment(Qt::AlignRight);

    nameLabel = new QLabel();
    nameLabel->setStyleSheet("QLabel { background-color : transparent; color : #000000; }");

    nameLayout->addWidget(nameTitleLabel);
    nameLayout->addWidget(nameLabel);
    nameLayout->addSpacing(20);

    cmdlineTitleLabel = new QLabel(QString("%1:").arg(tr("Command line")));
    cmdlineTitleLabel->setStyleSheet("QLabel { background-color : transparent; color : #666666; }");
    cmdlineTitleLabel->setFixedWidth(100);
    cmdlineTitleLabel->setAlignment(Qt::AlignRight);

    cmdlineLabel = new QLabel();
    cmdlineLabel->setStyleSheet("QLabel { background-color : transparent; color : #000000; }");
    cmdlineLabel->setWordWrap(true);
    cmdlineLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    cmdlineLayout->addWidget(cmdlineTitleLabel);
    cmdlineLayout->addWidget(cmdlineLabel);
    cmdlineLayout->addSpacing(20);

    startTimeTitleLabel = new QLabel(QString("%1:").arg(tr("Start time")));
    startTimeTitleLabel->setStyleSheet("QLabel { background-color : transparent; color : #666666; }");
    startTimeTitleLabel->setFixedWidth(100);
    startTimeTitleLabel->setAlignment(Qt::AlignRight);

    startTimeLabel = new QLabel();
    startTimeLabel->setStyleSheet("QLabel { background-color : transparent; color : #000000; }");
    startTimeLabel->setWordWrap(true);

    startTimeLayout->addWidget(startTimeTitleLabel);
    startTimeLayout->addWidget(startTimeLabel);
    startTimeLayout->addSpacing(20);

    // Create tab widget for process details
    tabWidget = new QTabWidget();
    tabWidget->setMinimumHeight(200);
    tabWidget->setStyleSheet("QTabWidget::pane { border: 1px solid #ddd; } "
                             "QTabBar::tab { padding: 8px 16px; } "
                             "QTabBar::tab:selected { background: #fff; border-bottom: 2px solid #2ca7f8; }");

    // Open files tab
    openFilesList = new QListWidget();
    openFilesList->setStyleSheet("QListWidget { border: none; background: transparent; } "
                                  "QListWidget::item { padding: 4px; }");
    tabWidget->addTab(openFilesList, tr("Open Files"));

    // Network ports tab
    networkPortsList = new QListWidget();
    networkPortsList->setStyleSheet("QListWidget { border: none; background: transparent; } "
                                     "QListWidget::item { padding: 4px; }");
    tabWidget->addTab(networkPortsList, tr("Network"));

    // Process tree tab (child processes)
    processTree = new QTreeWidget();
    processTree->setHeaderLabels({tr("PID"), tr("Name"), tr("CPU %"), tr("Memory")});
    processTree->setStyleSheet("QTreeWidget { border: none; background: transparent; }");
    processTree->header()->setStretchLastSection(true);
    tabWidget->addTab(processTree, tr("Child Processes"));

    layout->addWidget(closeButton, 0, Qt::AlignTop | Qt::AlignRight);
    layout->addSpacing(10);
    layout->addWidget(iconLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(10);
    layout->addWidget(titleLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(15);
    layout->addLayout(nameLayout);
    layout->addLayout(cmdlineLayout);
    layout->addLayout(startTimeLayout);
    layout->addSpacing(10);
    layout->addWidget(tabWidget);
    layout->addSpacing(10);

    // Read the list of open processes information.
    PROCTAB* proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLUSR | PROC_FILLCOM);
    static proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_t));

    std::map<int, proc_t> processes;
    while (readproc(proc, &proc_info) != NULL) {
        processes[proc_info.tid]=proc_info;
    }
    closeproc(proc);

    findWindowTitle = new FindWindowTitle();
    findWindowTitle->updateWindowInfos();

    // Read tray icon process.
    QList<int> trayProcessXids = Utils::getTrayWindows();
    QMap<int, int> trayProcessMap;

    for (u_int32_t xid : trayProcessXids) {
        trayProcessMap[findWindowTitle->getWindowPid(xid)] = xid;
    }

    for (auto &i:processes) {
        int processId = (&i.second)->tid;

        if (pid == processId) {
            QString cmdline = Utils::getProcessCmdline(processId);
            QString name = getProcessName(&i.second, cmdline);
            std::string desktopFile = getProcessDesktopFile(pid, name, cmdline, trayProcessMap);
            QPixmap icon = getProcessIcon(pid, desktopFile, findWindowTitle, 96);
            QString displayName = getDisplayNameFromName(name, desktopFile, false);

            iconLabel->setPixmap(icon);
            titleLabel->setText(displayName);
            nameLabel->setText(name);
            cmdlineLabel->setText(cmdline);

            startTimeLabel->setText(QFileInfo(QString("/proc/%1").arg(processId)).created().toString("yyyy-MM-dd hh:mm:ss"));

            break;
        }
    }

    // Load process details tabs
    loadOpenFiles();
    loadNetworkPorts();
    loadProcessTree();

    // Set dialog size to accommodate tabs
    setMinimumSize(450, 500);
}

AttributesDialog::~AttributesDialog()
{
    delete findWindowTitle;
    delete closeButton;
    delete iconLabel;
    delete nameTitleLabel;
    delete nameLabel;
    delete titleLabel;
    delete cmdlineTitleLabel;
    delete startTimeLabel;
    delete startTimeTitleLabel;
    delete cmdlineLabel;
    delete openFilesList;
    delete networkPortsList;
    delete processTree;
    delete tabWidget;
    delete nameLayout;
    delete cmdlineLayout;
    delete startTimeLayout;
    delete layout;
}

int AttributesDialog::getPid()
{
    return pid;
}

void AttributesDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.setOpacity(1);
    painter.fillPath(path, QColor("#ffffff"));
}

void AttributesDialog::loadOpenFiles()
{
    QString fdPath = QString("/proc/%1/fd").arg(pid);
    QDir fdDir(fdPath);

    if (!fdDir.exists()) {
        openFilesList->addItem(tr("Cannot access file descriptors"));
        return;
    }

    QStringList entries = fdDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
    int fileCount = 0;

    for (const QString &entry : entries) {
        QString linkPath = fdPath + "/" + entry;
        QFileInfo info(linkPath);

        if (info.isSymLink()) {
            QString target = info.symLinkTarget();
            // Filter out special files like pipes, sockets shown differently
            if (!target.startsWith("pipe:") && !target.startsWith("socket:") &&
                !target.startsWith("anon_inode:") && !target.isEmpty()) {
                openFilesList->addItem(QString("📄 %1").arg(target));
                fileCount++;
            }
        }

        if (fileCount >= 100) {  // Limit to avoid UI freeze
            openFilesList->addItem(tr("... and more (limited to 100)"));
            break;
        }
    }

    if (fileCount == 0) {
        openFilesList->addItem(tr("No regular files open"));
    }
}

void AttributesDialog::loadNetworkPorts()
{
    // Read TCP connections from /proc/net/tcp
    QStringList protocols = {"tcp", "tcp6", "udp", "udp6"};
    int portCount = 0;

    for (const QString &proto : protocols) {
        QFile netFile(QString("/proc/%1/net/%2").arg(pid).arg(proto));
        if (!netFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // Try global /proc/net
            netFile.setFileName(QString("/proc/net/%1").arg(proto));
            if (!netFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }
        }

        QTextStream in(&netFile);
        QString line = in.readLine(); // Skip header

        while (!in.atEnd()) {
            line = in.readLine();
            QStringList parts = line.simplified().split(' ');

            if (parts.size() >= 10) {
                // Parse local address (format: IP:PORT in hex)
                QString localAddr = parts[1];
                QStringList addrParts = localAddr.split(':');
                if (addrParts.size() == 2) {
                    bool ok;
                    int port = addrParts[1].toInt(&ok, 16);

                    // Check if this connection belongs to our process
                    // by checking inode in /proc/[pid]/fd
                    QString inode = parts[9];
                    QString fdPath = QString("/proc/%1/fd").arg(pid);
                    QDir fdDir(fdPath);

                    for (const QString &fd : fdDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
                        QFileInfo fdInfo(fdPath + "/" + fd);
                        if (fdInfo.isSymLink()) {
                            QString target = fdInfo.symLinkTarget();
                            if (target.contains(QString("socket:[%1]").arg(inode))) {
                                QString state = "";
                                int stateNum = parts[3].toInt(&ok, 16);
                                if (proto.startsWith("tcp")) {
                                    switch(stateNum) {
                                        case 1: state = "ESTABLISHED"; break;
                                        case 2: state = "SYN_SENT"; break;
                                        case 10: state = "LISTEN"; break;
                                        default: state = QString("STATE_%1").arg(stateNum);
                                    }
                                } else {
                                    state = "UDP";
                                }

                                QString icon = (proto.startsWith("tcp")) ? "🔌" : "📡";
                                networkPortsList->addItem(QString("%1 %2 :%3 [%4]")
                                    .arg(icon)
                                    .arg(proto.toUpper())
                                    .arg(port)
                                    .arg(state));
                                portCount++;
                                break;
                            }
                        }
                    }
                }
            }
        }
        netFile.close();
    }

    if (portCount == 0) {
        networkPortsList->addItem(tr("No network connections"));
    }
}

void AttributesDialog::loadProcessTree()
{
    // Find child processes
    QDir procDir("/proc");
    QStringList procEntries = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    int childCount = 0;
    for (const QString &entry : procEntries) {
        bool ok;
        int childPid = entry.toInt(&ok);
        if (!ok) continue;

        // Read parent PID from /proc/[pid]/stat
        QFile statFile(QString("/proc/%1/stat").arg(childPid));
        if (!statFile.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QString statLine = statFile.readLine();
        statFile.close();

        // Parse stat: pid (comm) state ppid ...
        int commEnd = statLine.lastIndexOf(')');
        if (commEnd < 0) continue;

        QStringList afterComm = statLine.mid(commEnd + 2).split(' ');
        if (afterComm.size() < 2) continue;

        int ppid = afterComm[1].toInt();

        if (ppid == pid) {
            // This is a child process
            QString comm = statLine.mid(statLine.indexOf('(') + 1,
                                         commEnd - statLine.indexOf('(') - 1);

            // Get memory info
            QFile statusFile(QString("/proc/%1/status").arg(childPid));
            QString memInfo = "N/A";
            if (statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                while (!statusFile.atEnd()) {
                    QString line = statusFile.readLine();
                    if (line.startsWith("VmRSS:")) {
                        memInfo = line.mid(6).trimmed();
                        break;
                    }
                }
                statusFile.close();
            }

            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, QString::number(childPid));
            item->setText(1, comm);
            item->setText(2, "-");  // CPU usage would need sampling
            item->setText(3, memInfo);
            processTree->addTopLevelItem(item);
            childCount++;
        }
    }

    if (childCount == 0) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, "-");
        item->setText(1, tr("No child processes"));
        item->setText(2, "-");
        item->setText(3, "-");
        processTree->addTopLevelItem(item);
    }

    processTree->resizeColumnToContents(0);
    processTree->resizeColumnToContents(1);
}
