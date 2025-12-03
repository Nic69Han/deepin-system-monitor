/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 * Copyright (C) 2024 Nic69Han <65731188+Nic69Han@users.noreply.github.com>
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
#include <QPainter>
#include <dthememanager.h>

QSettings *AlertSettingsDialog::alertSettings = nullptr;
QHash<QString, qint64> AlertSettingsDialog::lastNotificationTime;

AlertSettingsDialog::AlertSettingsDialog(QWidget *parent)
    : DAbstractDialog(parent), isDarkTheme(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(tr("Alert Settings"));
    setFixedSize(450, 480);

    // Connect to theme changes
    connect(Dtk::Widget::DThemeManager::instance(), &Dtk::Widget::DThemeManager::themeChanged,
            this, &AlertSettingsDialog::updateTheme);
    isDarkTheme = (Dtk::Widget::DThemeManager::instance()->theme() == "dark");

    setupUI();
    loadSettings();
    applyThemeStyle();
}

AlertSettingsDialog::~AlertSettingsDialog()
{
}

void AlertSettingsDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(QRectF(rect()), 8, 8);
    painter.setOpacity(1);
    painter.fillPath(path, isDarkTheme ? QColor("#252525") : QColor("#F8F8F8"));
}

void AlertSettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    // Title bar
    QWidget *titleBar = new QWidget();
    titleBar->setFixedHeight(40);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 5, 0);

    titleLabel = new QLabel(tr("Alert Settings"));
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

    // CPU Alert
    QGroupBox *cpuGroup = new QGroupBox(tr("CPU Usage Alert"));
    QHBoxLayout *cpuLayout = new QHBoxLayout(cpuGroup);
    cpuAlertEnabled = new QCheckBox(tr("Enable"));
    cpuThreshold = new QSpinBox();
    cpuThreshold->setRange(50, 100);
    cpuThreshold->setSuffix("%");
    cpuThreshold->setValue(90);
    cpuThreshold->setFixedHeight(28);
    cpuLayout->addWidget(cpuAlertEnabled);
    cpuLayout->addWidget(new QLabel(tr("Threshold:")));
    cpuLayout->addWidget(cpuThreshold);
    cpuLayout->addStretch();
    contentLayout->addWidget(cpuGroup);

    // Memory Alert
    QGroupBox *memGroup = new QGroupBox(tr("Memory Usage Alert"));
    QHBoxLayout *memLayout = new QHBoxLayout(memGroup);
    memoryAlertEnabled = new QCheckBox(tr("Enable"));
    memoryThreshold = new QSpinBox();
    memoryThreshold->setRange(50, 100);
    memoryThreshold->setSuffix("%");
    memoryThreshold->setValue(85);
    memoryThreshold->setFixedHeight(28);
    memLayout->addWidget(memoryAlertEnabled);
    memLayout->addWidget(new QLabel(tr("Threshold:")));
    memLayout->addWidget(memoryThreshold);
    memLayout->addStretch();
    contentLayout->addWidget(memGroup);

    // Disk Alert
    QGroupBox *diskGroup = new QGroupBox(tr("Disk Usage Alert"));
    QHBoxLayout *diskLayout = new QHBoxLayout(diskGroup);
    diskAlertEnabled = new QCheckBox(tr("Enable"));
    diskThreshold = new QSpinBox();
    diskThreshold->setRange(50, 100);
    diskThreshold->setSuffix("%");
    diskThreshold->setValue(90);
    diskThreshold->setFixedHeight(28);
    diskLayout->addWidget(diskAlertEnabled);
    diskLayout->addWidget(new QLabel(tr("Threshold:")));
    diskLayout->addWidget(diskThreshold);
    diskLayout->addStretch();
    contentLayout->addWidget(diskGroup);

    // Temperature Alert
    QGroupBox *tempGroup = new QGroupBox(tr("Temperature Alert"));
    QHBoxLayout *tempLayout = new QHBoxLayout(tempGroup);
    tempAlertEnabled = new QCheckBox(tr("Enable"));
    tempThreshold = new QSpinBox();
    tempThreshold->setRange(50, 100);
    tempThreshold->setSuffix("°C");
    tempThreshold->setValue(80);
    tempThreshold->setFixedHeight(28);
    tempLayout->addWidget(tempAlertEnabled);
    tempLayout->addWidget(new QLabel(tr("Threshold:")));
    tempLayout->addWidget(tempThreshold);
    tempLayout->addStretch();
    contentLayout->addWidget(tempGroup);

    // Network Alert (high bandwidth)
    QGroupBox *netGroup = new QGroupBox(tr("Network Bandwidth Alert"));
    QHBoxLayout *netLayout = new QHBoxLayout(netGroup);
    networkAlertEnabled = new QCheckBox(tr("Enable"));
    networkThreshold = new QSpinBox();
    networkThreshold->setRange(1, 1000);
    networkThreshold->setSuffix(" MB/s");
    networkThreshold->setValue(100);
    networkThreshold->setFixedHeight(28);
    netLayout->addWidget(networkAlertEnabled);
    netLayout->addWidget(new QLabel(tr("Threshold:")));
    netLayout->addWidget(networkThreshold);
    netLayout->addStretch();
    contentLayout->addWidget(netGroup);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    testBtn = new QPushButton(tr("Test Notification"));
    saveBtn = new QPushButton(tr("Save"));
    testBtn->setFixedHeight(30);
    saveBtn->setFixedHeight(30);
    connect(testBtn, &QPushButton::clicked, this, &AlertSettingsDialog::testNotification);
    connect(saveBtn, &QPushButton::clicked, this, &AlertSettingsDialog::saveSettings);
    btnLayout->addWidget(testBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    contentLayout->addLayout(btnLayout);

    // Status
    statusLabel = new QLabel();
    contentLayout->addWidget(statusLabel);

    mainLayout->addWidget(contentWidget);
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

void AlertSettingsDialog::updateTheme(const QString &theme)
{
    isDarkTheme = (theme == "dark");
    applyThemeStyle();
    update();
}

void AlertSettingsDialog::applyThemeStyle()
{
    QString textColor = isDarkTheme ? "#FFFFFF" : "#303030";
    QString bgColor = isDarkTheme ? "#2D2D2D" : "#FFFFFF";
    QString borderColor = isDarkTheme ? "#444444" : "#CCCCCC";
    QString headerBg = isDarkTheme ? "#3A3A3A" : "#E8E8E8";
    QString inputBg = isDarkTheme ? "#3A3A3A" : "#FFFFFF";

    // Title label
    titleLabel->setStyleSheet(QString("QLabel { color: %1; background: transparent; font-size: 14px; font-weight: bold; }").arg(textColor));

    // Close button theme
    Dtk::Widget::DThemeManager::instance()->setTheme(closeButton, isDarkTheme ? "dark" : "light");

    QString style = QString(
        "QGroupBox { background-color: %1; border: 1px solid %3; border-radius: 4px; margin-top: 10px; padding-top: 10px; color: %2; } "
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: %2; background-color: transparent; } "
        "QLabel { color: %2; background-color: transparent; } "
        "QCheckBox { color: %2; background-color: transparent; } "
        "QCheckBox::indicator { background-color: %5; border: 1px solid %3; border-radius: 2px; } "
        "QSpinBox { background-color: %5; color: %2; border: 1px solid %3; padding: 3px; border-radius: 4px; } "
        "QPushButton { background-color: %4; color: %2; border: 1px solid %3; padding: 5px 15px; border-radius: 4px; } "
        "QPushButton:hover { background-color: #2ca7f8; color: white; border-color: #2ca7f8; } "
        "QPushButton:pressed { background-color: #1a8ddb; }"
    ).arg(bgColor).arg(textColor).arg(borderColor).arg(headerBg).arg(inputBg);

    setStyleSheet(style);
}
