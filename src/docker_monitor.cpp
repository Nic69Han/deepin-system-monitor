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

#include "docker_monitor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QTextEdit>

DockerMonitorDialog::DockerMonitorDialog(QWidget *parent)
    : DDialog(parent)
{
    setWindowTitle(tr("Docker Containers"));
    setFixedSize(800, 500);
    setupUI();
    
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &DockerMonitorDialog::refreshContainers);
    refreshTimer->start(5000);
    
    refreshContainers();
}

DockerMonitorDialog::~DockerMonitorDialog()
{
    refreshTimer->stop();
}

void DockerMonitorDialog::setupUI()
{
    QWidget *content = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(content);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Status
    statusLabel = new QLabel(tr("Checking Docker..."), content);
    mainLayout->addWidget(statusLabel);
    
    // Table
    containerTable = new QTableWidget(content);
    containerTable->setColumnCount(6);
    containerTable->setHorizontalHeaderLabels({
        tr("Name"), tr("Image"), tr("Status"), tr("Ports"), tr("CPU"), tr("Memory")
    });
    containerTable->horizontalHeader()->setStretchLastSection(true);
    containerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    containerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    containerTable->setSelectionMode(QAbstractItemView::SingleSelection);
    containerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(containerTable, &QTableWidget::itemSelectionChanged, 
            this, &DockerMonitorDialog::onSelectionChanged);
    mainLayout->addWidget(containerTable);
    
    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    refreshBtn = new QPushButton(tr("Refresh"), content);
    connect(refreshBtn, &QPushButton::clicked, this, &DockerMonitorDialog::refreshContainers);
    
    startBtn = new QPushButton(tr("Start"), content);
    connect(startBtn, &QPushButton::clicked, this, &DockerMonitorDialog::onStartContainer);
    
    stopBtn = new QPushButton(tr("Stop"), content);
    connect(stopBtn, &QPushButton::clicked, this, &DockerMonitorDialog::onStopContainer);
    
    restartBtn = new QPushButton(tr("Restart"), content);
    connect(restartBtn, &QPushButton::clicked, this, &DockerMonitorDialog::onRestartContainer);
    
    removeBtn = new QPushButton(tr("Remove"), content);
    connect(removeBtn, &QPushButton::clicked, this, &DockerMonitorDialog::onRemoveContainer);
    
    logsBtn = new QPushButton(tr("View Logs"), content);
    connect(logsBtn, &QPushButton::clicked, this, &DockerMonitorDialog::onViewLogs);
    
    btnLayout->addWidget(refreshBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(startBtn);
    btnLayout->addWidget(stopBtn);
    btnLayout->addWidget(restartBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addWidget(logsBtn);
    
    mainLayout->addLayout(btnLayout);
    addContent(content);
    
    updateButtons();
}

bool DockerMonitorDialog::isDockerAvailable()
{
    QProcess proc;
    proc.start("docker", {"--version"});
    proc.waitForFinished(3000);
    return proc.exitCode() == 0;
}

void DockerMonitorDialog::refreshContainers()
{
    if (!isDockerAvailable()) {
        statusLabel->setText(tr("Docker is not installed or not running"));
        containerTable->setRowCount(0);
        containers.clear();
        updateButtons();
        return;
    }
    
    containers.clear();
    
    // Get all containers
    QProcess proc;
    proc.start("docker", {"ps", "-a", "--format", 
        "{{.ID}}|{{.Names}}|{{.Image}}|{{.Status}}|{{.Ports}}"});
    proc.waitForFinished(5000);
    
    QString output = proc.readAllStandardOutput();
    QStringList lines = output.split('\n', QString::SkipEmptyParts);
    
    for (const QString &line : lines) {
        QStringList parts = line.split('|');
        if (parts.size() >= 5) {
            ContainerInfo info;
            info.id = parts[0].trimmed();
            info.name = parts[1].trimmed();
            info.image = parts[2].trimmed();
            info.status = parts[3].trimmed();
            info.ports = parts[4].trimmed();
            info.running = info.status.startsWith("Up");
            info.cpu = "-";
            info.memory = "-";
            containers.append(info);
        }
    }
    
    // Get stats for running containers
    if (!containers.isEmpty()) {
        QProcess statsProc;
        statsProc.start("docker", {"stats", "--no-stream", "--format",
            "{{.Name}}|{{.CPUPerc}}|{{.MemUsage}}"});
        statsProc.waitForFinished(5000);
        
        QString statsOutput = statsProc.readAllStandardOutput();
        QStringList statsLines = statsOutput.split('\n', QString::SkipEmptyParts);
        
        for (const QString &statsLine : statsLines) {
            QStringList statsParts = statsLine.split('|');
            if (statsParts.size() >= 3) {
                QString name = statsParts[0].trimmed();
                for (int i = 0; i < containers.size(); i++) {
                    if (containers[i].name == name) {
                        containers[i].cpu = statsParts[1].trimmed();
                        containers[i].memory = statsParts[2].trimmed();
                        break;
                    }
                }
            }
        }
    }

    // Update table
    containerTable->setRowCount(containers.size());
    for (int i = 0; i < containers.size(); i++) {
        const ContainerInfo &c = containers[i];
        containerTable->setItem(i, 0, new QTableWidgetItem(c.name));
        containerTable->setItem(i, 1, new QTableWidgetItem(c.image));
        containerTable->setItem(i, 2, new QTableWidgetItem(c.status));
        containerTable->setItem(i, 3, new QTableWidgetItem(c.ports));
        containerTable->setItem(i, 4, new QTableWidgetItem(c.cpu));
        containerTable->setItem(i, 5, new QTableWidgetItem(c.memory));

        // Color based on status
        QColor rowColor = c.running ? QColor(76, 175, 80, 50) : QColor(158, 158, 158, 50);
        for (int j = 0; j < 6; j++) {
            containerTable->item(i, j)->setBackground(rowColor);
        }
    }

    statusLabel->setText(tr("%1 container(s) found").arg(containers.size()));
    updateButtons();
}

QString DockerMonitorDialog::getSelectedContainerId()
{
    int row = containerTable->currentRow();
    if (row >= 0 && row < containers.size()) {
        return containers[row].id;
    }
    return QString();
}

void DockerMonitorDialog::updateButtons()
{
    int row = containerTable->currentRow();
    bool hasSelection = row >= 0 && row < containers.size();
    bool isRunning = hasSelection && containers[row].running;

    startBtn->setEnabled(hasSelection && !isRunning);
    stopBtn->setEnabled(hasSelection && isRunning);
    restartBtn->setEnabled(hasSelection && isRunning);
    removeBtn->setEnabled(hasSelection && !isRunning);
    logsBtn->setEnabled(hasSelection);
}

void DockerMonitorDialog::onSelectionChanged()
{
    updateButtons();
}

void DockerMonitorDialog::runDockerCommand(const QStringList &args, bool refresh)
{
    QProcess proc;
    proc.start("docker", args);
    proc.waitForFinished(10000);

    if (refresh) {
        refreshContainers();
    }
}

void DockerMonitorDialog::onStartContainer()
{
    QString id = getSelectedContainerId();
    if (!id.isEmpty()) {
        runDockerCommand({"start", id});
    }
}

void DockerMonitorDialog::onStopContainer()
{
    QString id = getSelectedContainerId();
    if (!id.isEmpty()) {
        runDockerCommand({"stop", id});
    }
}

void DockerMonitorDialog::onRestartContainer()
{
    QString id = getSelectedContainerId();
    if (!id.isEmpty()) {
        runDockerCommand({"restart", id});
    }
}

void DockerMonitorDialog::onRemoveContainer()
{
    QString id = getSelectedContainerId();
    if (!id.isEmpty()) {
        int row = containerTable->currentRow();
        QString name = containers[row].name;

        if (QMessageBox::question(this, tr("Remove Container"),
            tr("Are you sure you want to remove container '%1'?").arg(name),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            runDockerCommand({"rm", id});
        }
    }
}

void DockerMonitorDialog::onViewLogs()
{
    QString id = getSelectedContainerId();
    if (!id.isEmpty()) {
        QProcess proc;
        proc.start("docker", {"logs", "--tail", "100", id});
        proc.waitForFinished(5000);

        QString logs = proc.readAllStandardOutput();
        if (logs.isEmpty()) {
            logs = proc.readAllStandardError();
        }
        if (logs.isEmpty()) {
            logs = tr("No logs available");
        }

        DDialog *logDialog = new DDialog(this);
        logDialog->setWindowTitle(tr("Container Logs"));
        logDialog->setFixedSize(600, 400);

        QTextEdit *logText = new QTextEdit(logDialog);
        logText->setReadOnly(true);
        logText->setPlainText(logs);
        logText->setFont(QFont("Monospace", 9));

        logDialog->addContent(logText);
        logDialog->show();
    }
}

