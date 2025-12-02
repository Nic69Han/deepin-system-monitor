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

#ifndef DOCKERMONITOR_H
#define DOCKERMONITOR_H

#include "ddialog.h"
#include <QWidget>
#include <QTimer>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QProcess>

DWIDGET_USE_NAMESPACE

struct ContainerInfo {
    QString id;
    QString name;
    QString image;
    QString status;
    QString ports;
    QString cpu;
    QString memory;
    bool running;
};

class DockerMonitorDialog : public DDialog
{
    Q_OBJECT

public:
    explicit DockerMonitorDialog(QWidget *parent = nullptr);
    ~DockerMonitorDialog();

private slots:
    void refreshContainers();
    void onStartContainer();
    void onStopContainer();
    void onRestartContainer();
    void onRemoveContainer();
    void onViewLogs();
    void onSelectionChanged();

private:
    void setupUI();
    void updateButtons();
    QString getSelectedContainerId();
    bool isDockerAvailable();
    void runDockerCommand(const QStringList &args, bool refresh = true);

    QTableWidget *containerTable;
    QPushButton *refreshBtn;
    QPushButton *startBtn;
    QPushButton *stopBtn;
    QPushButton *restartBtn;
    QPushButton *removeBtn;
    QPushButton *logsBtn;
    QLabel *statusLabel;
    QTimer *refreshTimer;
    
    QList<ContainerInfo> containers;
};

#endif

