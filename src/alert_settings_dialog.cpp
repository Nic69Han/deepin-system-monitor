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

#include "alert_settings_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QProcess>
#include <QDateTime>
#include <QStandardPaths>

QSettings *AlertSettingsDialog::alertSettings = nullptr;
QHash<QString, qint64> AlertSettingsDialog::lastNotificationTime;

AlertSettingsDialog::AlertSettingsDialog(QWidget *parent)
    : DDialog(parent)
{
    setWindowTitle(tr("Alert Settings"));
    setFixedSize(450, 420);
    setupUI();
    loadSettings();
}

AlertSettingsDialog::~AlertSettingsDialog()
{
}

void AlertSettingsDialog::setupUI()
{
    QWidget *contentWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // CPU Alert
    QGroupBox *cpuGroup = new QGroupBox(tr("CPU Usage Alert"));
    QHBoxLayout *cpuLayout = new QHBoxLayout(cpuGroup);
    cpuAlertEnabled = new QCheckBox(tr("Enable"));
    cpuThreshold = new QSpinBox();
    cpuThreshold->setRange(50, 100);
    cpuThreshold->setSuffix("%");
    cpuThreshold->setValue(90);
    cpuLayout->addWidget(cpuAlertEnabled);
    cpuLayout->addWidget(new QLabel(tr("Threshold:")));
    cpuLayout->addWidget(cpuThreshold);
    cpuLayout->addStretch();
    mainLayout->addWidget(cpuGroup);

    // Memory Alert
    QGroupBox *memGroup = new QGroupBox(tr("Memory Usage Alert"));
    QHBoxLayout *memLayout = new QHBoxLayout(memGroup);
    memoryAlertEnabled = new QCheckBox(tr("Enable"));
    memoryThreshold = new QSpinBox();
    memoryThreshold->setRange(50, 100);
    memoryThreshold->setSuffix("%");
    memoryThreshold->setValue(85);
    memLayout->addWidget(memoryAlertEnabled);
    memLayout->addWidget(new QLabel(tr("Threshold:")));
    memLayout->addWidget(memoryThreshold);
    memLayout->addStretch();
    mainLayout->addWidget(memGroup);

    // Disk Alert
    QGroupBox *diskGroup = new QGroupBox(tr("Disk Usage Alert"));
    QHBoxLayout *diskLayout = new QHBoxLayout(diskGroup);
    diskAlertEnabled = new QCheckBox(tr("Enable"));
    diskThreshold = new QSpinBox();
    diskThreshold->setRange(50, 100);
    diskThreshold->setSuffix("%");
    diskThreshold->setValue(90);
    diskLayout->addWidget(diskAlertEnabled);
    diskLayout->addWidget(new QLabel(tr("Threshold:")));
    diskLayout->addWidget(diskThreshold);
    diskLayout->addStretch();
    mainLayout->addWidget(diskGroup);

    // Temperature Alert
    QGroupBox *tempGroup = new QGroupBox(tr("Temperature Alert"));
    QHBoxLayout *tempLayout = new QHBoxLayout(tempGroup);
    tempAlertEnabled = new QCheckBox(tr("Enable"));
    tempThreshold = new QSpinBox();
    tempThreshold->setRange(50, 100);
    tempThreshold->setSuffix("°C");
    tempThreshold->setValue(80);
    tempLayout->addWidget(tempAlertEnabled);
    tempLayout->addWidget(new QLabel(tr("Threshold:")));
    tempLayout->addWidget(tempThreshold);
    tempLayout->addStretch();
    mainLayout->addWidget(tempGroup);

    // Network Alert (high bandwidth)
    QGroupBox *netGroup = new QGroupBox(tr("Network Bandwidth Alert"));
    QHBoxLayout *netLayout = new QHBoxLayout(netGroup);
    networkAlertEnabled = new QCheckBox(tr("Enable"));
    networkThreshold = new QSpinBox();
    networkThreshold->setRange(1, 1000);
    networkThreshold->setSuffix(" MB/s");
    networkThreshold->setValue(100);
    netLayout->addWidget(networkAlertEnabled);
    netLayout->addWidget(new QLabel(tr("Threshold:")));
    netLayout->addWidget(networkThreshold);
    netLayout->addStretch();
    mainLayout->addWidget(netGroup);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    testBtn = new QPushButton(tr("Test Notification"));
    saveBtn = new QPushButton(tr("Save"));
    connect(testBtn, &QPushButton::clicked, this, &AlertSettingsDialog::testNotification);
    connect(saveBtn, &QPushButton::clicked, this, &AlertSettingsDialog::saveSettings);
    btnLayout->addWidget(testBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    mainLayout->addLayout(btnLayout);

    // Status
    statusLabel = new QLabel();
    mainLayout->addWidget(statusLabel);

    addContent(contentWidget);
}

void AlertSettingsDialog::loadSettings()
{
    if (!alertSettings) {
        QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        alertSettings = new QSettings(configPath + "/deepin-system-monitor-alerts.conf", QSettings::IniFormat);
    }
    
    cpuAlertEnabled->setChecked(alertSettings->value("cpu/enabled", false).toBool());
    cpuThreshold->setValue(alertSettings->value("cpu/threshold", 90).toInt());
    memoryAlertEnabled->setChecked(alertSettings->value("memory/enabled", false).toBool());
    memoryThreshold->setValue(alertSettings->value("memory/threshold", 85).toInt());
    diskAlertEnabled->setChecked(alertSettings->value("disk/enabled", false).toBool());
    diskThreshold->setValue(alertSettings->value("disk/threshold", 90).toInt());
    tempAlertEnabled->setChecked(alertSettings->value("temp/enabled", false).toBool());
    tempThreshold->setValue(alertSettings->value("temp/threshold", 80).toInt());
    networkAlertEnabled->setChecked(alertSettings->value("network/enabled", false).toBool());
    networkThreshold->setValue(alertSettings->value("network/threshold", 100).toInt());
}

void AlertSettingsDialog::saveSettings()
{
    if (!alertSettings) {
        QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        alertSettings = new QSettings(configPath + "/deepin-system-monitor-alerts.conf", QSettings::IniFormat);
    }

    alertSettings->setValue("cpu/enabled", cpuAlertEnabled->isChecked());
    alertSettings->setValue("cpu/threshold", cpuThreshold->value());
    alertSettings->setValue("memory/enabled", memoryAlertEnabled->isChecked());
    alertSettings->setValue("memory/threshold", memoryThreshold->value());
    alertSettings->setValue("disk/enabled", diskAlertEnabled->isChecked());
    alertSettings->setValue("disk/threshold", diskThreshold->value());
    alertSettings->setValue("temp/enabled", tempAlertEnabled->isChecked());
    alertSettings->setValue("temp/threshold", tempThreshold->value());
    alertSettings->setValue("network/enabled", networkAlertEnabled->isChecked());
    alertSettings->setValue("network/threshold", networkThreshold->value());
    alertSettings->sync();

    statusLabel->setText(tr("Settings saved successfully!"));
}

void AlertSettingsDialog::testNotification()
{
    QProcess::startDetached("notify-send", {
        "-i", "dialog-warning",
        "-u", "normal",
        "System Monitor Alert",
        tr("This is a test notification from System Monitor.")
    });
    statusLabel->setText(tr("Test notification sent!"));
}

bool AlertSettingsDialog::isAlertEnabled(const QString &type)
{
    if (!alertSettings) {
        QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        alertSettings = new QSettings(configPath + "/deepin-system-monitor-alerts.conf", QSettings::IniFormat);
    }
    return alertSettings->value(type + "/enabled", false).toBool();
}

int AlertSettingsDialog::getThreshold(const QString &type)
{
    if (!alertSettings) {
        QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        alertSettings = new QSettings(configPath + "/deepin-system-monitor-alerts.conf", QSettings::IniFormat);
    }
    return alertSettings->value(type + "/threshold", 90).toInt();
}

void AlertSettingsDialog::checkAndNotify(const QString &type, double currentValue, const QString &label)
{
    if (!isAlertEnabled(type)) return;

    int threshold = getThreshold(type);
    if (currentValue < threshold) return;

    // Rate limit: max 1 notification per type per 60 seconds
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastNotificationTime.contains(type)) {
        if (now - lastNotificationTime[type] < 60000) return;
    }
    lastNotificationTime[type] = now;

    QString title = "System Monitor Alert";
    QString message;
    QString urgency = "normal";

    if (type == "cpu") {
        message = QString("CPU usage is at %1% (threshold: %2%)").arg(currentValue, 0, 'f', 1).arg(threshold);
    } else if (type == "memory") {
        message = QString("Memory usage is at %1% (threshold: %2%)").arg(currentValue, 0, 'f', 1).arg(threshold);
    } else if (type == "disk") {
        message = QString("Disk usage is at %1% (threshold: %2%)").arg(currentValue, 0, 'f', 1).arg(threshold);
    } else if (type == "temp") {
        message = QString("%1 temperature is at %2°C (threshold: %3°C)").arg(label).arg(currentValue, 0, 'f', 0).arg(threshold);
        if (currentValue > threshold + 10) urgency = "critical";
    } else if (type == "network") {
        message = QString("Network bandwidth is at %1 MB/s (threshold: %2 MB/s)").arg(currentValue, 0, 'f', 1).arg(threshold);
    }

    QProcess::startDetached("notify-send", {
        "-i", "dialog-warning",
        "-u", urgency,
        title,
        message
    });
}

