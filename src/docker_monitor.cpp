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
#include <QPainter>
#include <QDialog>
#include <dthememanager.h>

DockerMonitorDialog::DockerMonitorDialog(QWidget *parent)
    : DAbstractDialog(parent), isDarkTheme(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(tr("Docker Containers"));
    setFixedSize(800, 500);

    // Connect to theme changes
    connect(Dtk::Widget::DThemeManager::instance(), &Dtk::Widget::DThemeManager::themeChanged,
            this, &DockerMonitorDialog::updateTheme);
    isDarkTheme = (Dtk::Widget::DThemeManager::instance()->theme() == "dark");

    setupUI();

    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &DockerMonitorDialog::refreshContainers);
    refreshTimer->start(5000);

    refreshContainers();
    applyThemeStyle();
}

DockerMonitorDialog::~DockerMonitorDialog()
{
    refreshTimer->stop();
}

void DockerMonitorDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(QRectF(rect()), 8, 8);
    painter.setOpacity(1);
    painter.fillPath(path, isDarkTheme ? QColor("#252525") : QColor("#F8F8F8"));
}

void DockerMonitorDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    // Title bar
    QWidget *titleBar = new QWidget();
    titleBar->setFixedHeight(40);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 5, 0);

    titleLabel = new QLabel(tr("Docker Containers"));
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");

    closeButton = new DWindowCloseButton();
    closeButton->setFixedSize(27, 23);
    connect(closeButton, &DWindowCloseButton::clicked, this, &DAbstractDialog::close);

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeButton);
    mainLayout->addWidget(titleBar);

    // Content area
    QWidget *content = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(15, 10, 15, 15);

    // Status
    statusLabel = new QLabel(tr("Checking Docker..."));
    contentLayout->addWidget(statusLabel);

    // Table
    containerTable = new QTableWidget();
    containerTable->setColumnCount(6);
    containerTable->setHorizontalHeaderLabels({
        tr("Name"), tr("Image"), tr("Status"), tr("Ports"), tr("CPU"), tr("Memory")
    });
    containerTable->horizontalHeader()->setStretchLastSection(true);
    containerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    containerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    containerTable->setSelectionMode(QAbstractItemView::SingleSelection);
    containerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    containerTable->setAlternatingRowColors(true);
    containerTable->verticalHeader()->setVisible(false);
    connect(containerTable, &QTableWidget::itemSelectionChanged,
            this, &DockerMonitorDialog::onSelectionChanged);
    contentLayout->addWidget(containerTable);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();

    refreshBtn = new QPushButton(tr("Refresh"));
    refreshBtn->setFixedHeight(30);
    connect(refreshBtn, &QPushButton::clicked, this, &DockerMonitorDialog::refreshContainers);

    startBtn = new QPushButton(tr("Start"));
    startBtn->setFixedHeight(30);
    connect(startBtn, &QPushButton::clicked, this, &DockerMonitorDialog::onStartContainer);

    stopBtn = new QPushButton(tr("Stop"));
    stopBtn->setFixedHeight(30);
    connect(stopBtn, &QPushButton::clicked, this, &DockerMonitorDialog::onStopContainer);

    restartBtn = new QPushButton(tr("Restart"));
    restartBtn->setFixedHeight(30);
    connect(restartBtn, &QPushButton::clicked, this, &DockerMonitorDialog::onRestartContainer);

    removeBtn = new QPushButton(tr("Remove"));
    removeBtn->setFixedHeight(30);
    connect(removeBtn, &QPushButton::clicked, this, &DockerMonitorDialog::onRemoveContainer);

    logsBtn = new QPushButton(tr("View Logs"));
    logsBtn->setFixedHeight(30);
    connect(logsBtn, &QPushButton::clicked, this, &DockerMonitorDialog::onViewLogs);

    btnLayout->addWidget(refreshBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(startBtn);
    btnLayout->addWidget(stopBtn);
    btnLayout->addWidget(restartBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addWidget(logsBtn);

    contentLayout->addLayout(btnLayout);
    mainLayout->addWidget(content);

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

        QDialog *logDialog = new QDialog(this);
        logDialog->setWindowTitle(tr("Container Logs"));
        logDialog->setFixedSize(600, 400);
        logDialog->setAttribute(Qt::WA_DeleteOnClose, true);

        QVBoxLayout *logLayout = new QVBoxLayout(logDialog);
        logLayout->setContentsMargins(10, 10, 10, 10);

        QTextEdit *logText = new QTextEdit(logDialog);
        logText->setReadOnly(true);
        logText->setPlainText(logs);
        logText->setFont(QFont("Monospace", 9));

        // Apply theme to log dialog
        QString textColor = isDarkTheme ? "#FFFFFF" : "#303030";
        QString bgColor = isDarkTheme ? "#252525" : "#F8F8F8";
        logDialog->setStyleSheet(QString("QDialog { background-color: %1; } QTextEdit { background-color: %1; color: %2; border: 1px solid %3; }").arg(bgColor).arg(textColor).arg(isDarkTheme ? "#444444" : "#CCCCCC"));

        logLayout->addWidget(logText);
        logDialog->show();
    }
}

void DockerMonitorDialog::updateTheme(const QString &theme)
{
    isDarkTheme = (theme == "dark");
    applyThemeStyle();
    update();
}

void DockerMonitorDialog::applyThemeStyle()
{
    QString textColor = isDarkTheme ? "#FFFFFF" : "#303030";
    QString bgColor = isDarkTheme ? "#2D2D2D" : "#FFFFFF";
    QString borderColor = isDarkTheme ? "#444444" : "#CCCCCC";
    QString headerBg = isDarkTheme ? "#3A3A3A" : "#E8E8E8";
    QString altRowColor = isDarkTheme ? "#333333" : "#F5F5F5";

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
        "QPushButton { background-color: %4; color: %2; border: 1px solid %3; padding: 5px 15px; border-radius: 4px; } "
        "QPushButton:hover { background-color: #2ca7f8; color: white; border-color: #2ca7f8; } "
        "QPushButton:pressed { background-color: #1a8ddb; } "
        "QPushButton:disabled { background-color: %4; color: #888888; border-color: %3; }"
    ).arg(bgColor).arg(textColor).arg(borderColor).arg(headerBg).arg(altRowColor);

    setStyleSheet(style);
}
