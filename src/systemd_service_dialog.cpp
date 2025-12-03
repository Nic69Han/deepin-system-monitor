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
#include <QPainter>
#include <dthememanager.h>

SystemdServiceDialog::SystemdServiceDialog(QWidget *parent)
    : DAbstractDialog(parent), isDarkTheme(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(tr("System Services"));
    setFixedSize(750, 550);

    // Connect to theme changes
    connect(Dtk::Widget::DThemeManager::instance(), &Dtk::Widget::DThemeManager::themeChanged,
            this, &SystemdServiceDialog::updateTheme);
    isDarkTheme = (Dtk::Widget::DThemeManager::instance()->theme() == "dark");

    setupUI();
    loadServices();
    applyThemeStyle();
}

SystemdServiceDialog::~SystemdServiceDialog()
{
}

void SystemdServiceDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(QRectF(rect()), 8, 8);
    painter.setOpacity(1);
    painter.fillPath(path, isDarkTheme ? QColor("#252525") : QColor("#F8F8F8"));
}

void SystemdServiceDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    // Title bar
    QWidget *titleBar = new QWidget();
    titleBar->setFixedHeight(40);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 5, 0);

    titleLabel = new QLabel(tr("System Services"));
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");

    closeButton = new DWindowCloseButton();
    closeButton->setFixedSize(27, 23);
    connect(closeButton, &DWindowCloseButton::clicked, this, &DAbstractDialog::close);

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeButton);
    mainLayout->addWidget(titleBar);

    // Content area
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(15, 10, 15, 15);
    contentLayout->setSpacing(10);

    // Top bar: search and filter
    QHBoxLayout *topLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(tr("Search services..."));
    searchEdit->setFixedWidth(200);
    searchEdit->setFixedHeight(30);
    connect(searchEdit, &QLineEdit::textChanged, this, &SystemdServiceDialog::filterServices);

    filterCombo = new QComboBox();
    filterCombo->addItem(tr("All Services"), "all");
    filterCombo->addItem(tr("Running"), "running");
    filterCombo->addItem(tr("Stopped"), "dead");
    filterCombo->addItem(tr("Failed"), "failed");
    filterCombo->setFixedHeight(30);
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SystemdServiceDialog::filterByType);

    refreshBtn = new QPushButton(tr("Refresh"));
    refreshBtn->setFixedHeight(30);
    connect(refreshBtn, &QPushButton::clicked, this, &SystemdServiceDialog::refreshServices);

    topLayout->addWidget(searchEdit);
    topLayout->addWidget(filterCombo);
    topLayout->addStretch();
    topLayout->addWidget(refreshBtn);
    contentLayout->addLayout(topLayout);

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
    serviceTable->setAlternatingRowColors(true);
    connect(serviceTable, &QTableWidget::itemSelectionChanged, this, &SystemdServiceDialog::onServiceSelected);
    contentLayout->addWidget(serviceTable);

    // Action buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    startBtn = new QPushButton(tr("Start"));
    stopBtn = new QPushButton(tr("Stop"));
    restartBtn = new QPushButton(tr("Restart"));
    enableBtn = new QPushButton(tr("Enable"));
    disableBtn = new QPushButton(tr("Disable"));

    startBtn->setFixedHeight(30);
    stopBtn->setFixedHeight(30);
    restartBtn->setFixedHeight(30);
    enableBtn->setFixedHeight(30);
    disableBtn->setFixedHeight(30);

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
    contentLayout->addLayout(btnLayout);

    // Status label
    statusLabel = new QLabel();
    contentLayout->addWidget(statusLabel);

    mainLayout->addWidget(contentWidget);
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

void SystemdServiceDialog::updateTheme(const QString &theme)
{
    isDarkTheme = (theme == "dark");
    applyThemeStyle();
    update();  // Force repaint
}

void SystemdServiceDialog::applyThemeStyle()
{
    QString textColor = isDarkTheme ? "#FFFFFF" : "#303030";
    QString bgColor = isDarkTheme ? "#2D2D2D" : "#FFFFFF";
    QString borderColor = isDarkTheme ? "#444444" : "#CCCCCC";
    QString headerBg = isDarkTheme ? "#3A3A3A" : "#E8E8E8";
    QString altRowColor = isDarkTheme ? "#333333" : "#F5F5F5";
    QString inputBg = isDarkTheme ? "#3A3A3A" : "#FFFFFF";

    // Title label
    titleLabel->setStyleSheet(QString("QLabel { color: %1; background: transparent; font-size: 14px; font-weight: bold; }").arg(textColor));

    // Close button theme
    Dtk::Widget::DThemeManager::instance()->setTheme(closeButton, isDarkTheme ? "dark" : "light");

    QString style = QString(
        "QTableWidget { background-color: %1; color: %2; gridline-color: %3; border: 1px solid %3; alternate-background-color: %5; } "
        "QTableWidget::item { padding: 5px; color: %2; } "
        "QTableWidget::item:selected { background-color: #2ca7f8; color: white; } "
        "QHeaderView::section { background-color: %4; color: %2; border: 1px solid %3; padding: 5px; font-weight: bold; } "
        "QLabel { color: %2; background-color: transparent; } "
        "QLineEdit { background-color: %6; color: %2; border: 1px solid %3; padding: 5px; border-radius: 4px; } "
        "QComboBox { background-color: %6; color: %2; border: 1px solid %3; padding: 5px; border-radius: 4px; } "
        "QComboBox QAbstractItemView { background-color: %1; color: %2; selection-background-color: #2ca7f8; } "
        "QComboBox::drop-down { border: none; width: 20px; } "
        "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; border-right: 5px solid transparent; border-top: 5px solid %2; } "
        "QPushButton { background-color: %4; color: %2; border: 1px solid %3; padding: 5px 15px; border-radius: 4px; } "
        "QPushButton:hover { background-color: #2ca7f8; color: white; border-color: #2ca7f8; } "
        "QPushButton:pressed { background-color: #1a8ddb; } "
        "QPushButton:disabled { background-color: %4; color: #888888; border-color: %3; }"
    ).arg(bgColor).arg(textColor).arg(borderColor).arg(headerBg).arg(altRowColor).arg(inputBg);

    setStyleSheet(style);
}
