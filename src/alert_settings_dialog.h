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

#ifndef ALERTSETTINGSDIALOG_H
#define ALERTSETTINGSDIALOG_H

#include "ddialog.h"
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTimer>

DWIDGET_USE_NAMESPACE

class AlertSettingsDialog : public DDialog
{
    Q_OBJECT

public:
    explicit AlertSettingsDialog(QWidget *parent = nullptr);
    ~AlertSettingsDialog();

    static bool isAlertEnabled(const QString &type);
    static int getThreshold(const QString &type);
    static void checkAndNotify(const QString &type, double currentValue, const QString &label);

private slots:
    void saveSettings();
    void testNotification();

private:
    void setupUI();
    void loadSettings();

    QCheckBox *cpuAlertEnabled;
    QSpinBox *cpuThreshold;
    QCheckBox *memoryAlertEnabled;
    QSpinBox *memoryThreshold;
    QCheckBox *diskAlertEnabled;
    QSpinBox *diskThreshold;
    QCheckBox *tempAlertEnabled;
    QSpinBox *tempThreshold;
    QCheckBox *networkAlertEnabled;
    QSpinBox *networkThreshold;
    
    QPushButton *saveBtn;
    QPushButton *testBtn;
    QLabel *statusLabel;
    
    static QSettings *alertSettings;
    static QHash<QString, qint64> lastNotificationTime;
};

#endif

