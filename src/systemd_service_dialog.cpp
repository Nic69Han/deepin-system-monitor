/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#include "systemd_service_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QProcess>
#include <QMessageBox>

SystemdServiceDialog::SystemdServiceDialog(QWidget *parent)
    : DDialog(parent)
{
    setWindowTitle(tr("System Services"));
    setFixedSize(750, 550);
    setupUI();
    loadServices();
}

SystemdServiceDialog::~SystemdServiceDialog()
{
}

void SystemdServiceDialog::setupUI()
{
    QWidget *contentWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Top bar: search and filter
    QHBoxLayout *topLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(tr("Search services..."));
    searchEdit->setFixedWidth(200);
    connect(searchEdit, &QLineEdit::textChanged, this, &SystemdServiceDialog::filterServices);

    filterCombo = new QComboBox();
    filterCombo->addItem(tr("All Services"), "all");
    filterCombo->addItem(tr("Running"), "running");
    filterCombo->addItem(tr("Stopped"), "dead");
    filterCombo->addItem(tr("Failed"), "failed");
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &SystemdServiceDialog::filterByType);

    refreshBtn = new QPushButton(tr("Refresh"));
    connect(refreshBtn, &QPushButton::clicked, this, &SystemdServiceDialog::refreshServices);

    topLayout->addWidget(searchEdit);
    topLayout->addWidget(filterCombo);
    topLayout->addStretch();
    topLayout->addWidget(refreshBtn);
    mainLayout->addLayout(topLayout);

    // Service table
    serviceTable = new QTableWidget();
    serviceTable->setColumnCount(4);
    serviceTable->setHorizontalHeaderLabels({tr("Service"), tr("Status"), tr("State"), tr("Description")});
    serviceTable->horizontalHeader()->setStretchLastSection(true);
    serviceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    serviceTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    serviceTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    serviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    serviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    serviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    serviceTable->verticalHeader()->setVisible(false);
    connect(serviceTable, &QTableWidget::itemSelectionChanged, this, &SystemdServiceDialog::onServiceSelected);
    mainLayout->addWidget(serviceTable);

    // Action buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    startBtn = new QPushButton(tr("Start"));
    stopBtn = new QPushButton(tr("Stop"));
    restartBtn = new QPushButton(tr("Restart"));
    enableBtn = new QPushButton(tr("Enable"));
    disableBtn = new QPushButton(tr("Disable"));

    startBtn->setEnabled(false);
    stopBtn->setEnabled(false);
    restartBtn->setEnabled(false);
    enableBtn->setEnabled(false);
    disableBtn->setEnabled(false);

    connect(startBtn, &QPushButton::clicked, this, &SystemdServiceDialog::startService);
    connect(stopBtn, &QPushButton::clicked, this, &SystemdServiceDialog::stopService);
    connect(restartBtn, &QPushButton::clicked, this, &SystemdServiceDialog::restartService);
    connect(enableBtn, &QPushButton::clicked, this, &SystemdServiceDialog::enableService);
    connect(disableBtn, &QPushButton::clicked, this, &SystemdServiceDialog::disableService);

    btnLayout->addWidget(startBtn);
    btnLayout->addWidget(stopBtn);
    btnLayout->addWidget(restartBtn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(enableBtn);
    btnLayout->addWidget(disableBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // Status label
    statusLabel = new QLabel();
    mainLayout->addWidget(statusLabel);

    addContent(contentWidget);
}

void SystemdServiceDialog::loadServices()
{
    allServices.clear();
    QProcess process;
    process.start("systemctl", {"list-units", "--type=service", "--all", "--no-pager", "--no-legend"});
    process.waitForFinished(5000);
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', QString::SkipEmptyParts);
    
    for (const QString &line : lines) {
        QStringList parts = line.simplified().split(' ');
        if (parts.size() >= 4) {
            ServiceInfo info;
            info.name = parts[0].remove(".service");
            info.loadState = parts[1];
            info.activeState = parts[2];
            info.subState = parts[3];
            info.description = parts.mid(4).join(' ');
            allServices.append(info);
        }
    }
    
    filterServices(searchEdit->text());
    statusLabel->setText(tr("Loaded %1 services").arg(allServices.size()));
}

void SystemdServiceDialog::filterServices(const QString &text)
{
    currentFilter = text.toLower();
    filterByType(filterCombo->currentIndex());
}

void SystemdServiceDialog::filterByType(int index)
{
    QString typeFilter = filterCombo->itemData(index).toString();
    serviceTable->setRowCount(0);
    
    for (const ServiceInfo &service : allServices) {
        bool matchesText = currentFilter.isEmpty() || 
                          service.name.toLower().contains(currentFilter) ||
                          service.description.toLower().contains(currentFilter);
        bool matchesType = (typeFilter == "all") || (service.subState == typeFilter);
        
        if (matchesText && matchesType) {
            int row = serviceTable->rowCount();
            serviceTable->insertRow(row);
            serviceTable->setItem(row, 0, new QTableWidgetItem(service.name));
            serviceTable->setItem(row, 1, new QTableWidgetItem(service.activeState));
            serviceTable->setItem(row, 2, new QTableWidgetItem(service.subState));
            serviceTable->setItem(row, 3, new QTableWidgetItem(service.description));
            
            // Color code status
            QColor statusColor;
            if (service.subState == "running") statusColor = QColor(76, 175, 80);
            else if (service.subState == "failed") statusColor = QColor(244, 67, 54);
            else statusColor = QColor(158, 158, 158);
            serviceTable->item(row, 2)->setForeground(statusColor);
        }
    }
}

void SystemdServiceDialog::refreshServices()
{
    loadServices();
}

void SystemdServiceDialog::onServiceSelected()
{
    updateButtons();
}

void SystemdServiceDialog::updateButtons()
{
    bool hasSelection = serviceTable->currentRow() >= 0;
    startBtn->setEnabled(hasSelection);
    stopBtn->setEnabled(hasSelection);
    restartBtn->setEnabled(hasSelection);
    enableBtn->setEnabled(hasSelection);
    disableBtn->setEnabled(hasSelection);
}

void SystemdServiceDialog::executeServiceCommand(const QString &action)
{
    int row = serviceTable->currentRow();
    if (row < 0) return;

    QString serviceName = serviceTable->item(row, 0)->text() + ".service";

    QProcess process;
    process.start("pkexec", {"systemctl", action, serviceName});
    process.waitForFinished(30000);

    if (process.exitCode() == 0) {
        statusLabel->setText(tr("Action '%1' on %2: Success").arg(action).arg(serviceName));
        refreshServices();
    } else {
        QString error = process.readAllStandardError();
        statusLabel->setText(tr("Action '%1' on %2: Failed - %3").arg(action).arg(serviceName).arg(error.simplified()));
    }
}

void SystemdServiceDialog::startService()
{
    executeServiceCommand("start");
}

void SystemdServiceDialog::stopService()
{
    executeServiceCommand("stop");
}

void SystemdServiceDialog::restartService()
{
    executeServiceCommand("restart");
}

void SystemdServiceDialog::enableService()
{
    executeServiceCommand("enable");
}

void SystemdServiceDialog::disableService()
{
    executeServiceCommand("disable");
}

