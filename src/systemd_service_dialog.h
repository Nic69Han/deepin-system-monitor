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

#ifndef SYSTEMDSERVICEDIALOG_H
#define SYSTEMDSERVICEDIALOG_H

#include "ddialog.h"
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>

DWIDGET_USE_NAMESPACE

struct ServiceInfo {
    QString name;
    QString loadState;
    QString activeState;
    QString subState;
    QString description;
};

class SystemdServiceDialog : public DDialog
{
    Q_OBJECT

public:
    explicit SystemdServiceDialog(QWidget *parent = nullptr);
    ~SystemdServiceDialog();

private slots:
    void refreshServices();
    void filterServices(const QString &text);
    void onServiceSelected();
    void startService();
    void stopService();
    void restartService();
    void enableService();
    void disableService();
    void filterByType(int index);
    void updateTheme(const QString &theme);

private:
    void setupUI();
    void loadServices();
    void updateButtons();
    void executeServiceCommand(const QString &action);

    QTableWidget *serviceTable;
    QLineEdit *searchEdit;
    QComboBox *filterCombo;
    QPushButton *startBtn;
    QPushButton *stopBtn;
    QPushButton *restartBtn;
    QPushButton *enableBtn;
    QPushButton *disableBtn;
    QPushButton *refreshBtn;
    QLabel *statusLabel;
    
    QList<ServiceInfo> allServices;
    QString currentFilter;
};

#endif

