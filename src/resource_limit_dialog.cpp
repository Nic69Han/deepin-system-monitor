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

#include "resource_limit_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QProcess>
#include <QFile>
#include <QDebug>
#include <QPainter>
#include <sys/resource.h>
#include <unistd.h>
#include <dthememanager.h>

ResourceLimitDialog::ResourceLimitDialog(int pid, const QString &processName, QWidget *parent)
    : DAbstractDialog(parent), m_pid(pid), m_processName(processName), isDarkTheme(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(tr("Resource Limits - %1 (PID: %2)").arg(processName).arg(pid));
    setFixedSize(450, 460);

    // Connect to theme changes
    connect(Dtk::Widget::DThemeManager::instance(), &Dtk::Widget::DThemeManager::themeChanged,
            this, &ResourceLimitDialog::updateTheme);
    isDarkTheme = (Dtk::Widget::DThemeManager::instance()->theme() == "dark");

    setupUI();
    loadCurrentLimits();
    applyThemeStyle();
}

ResourceLimitDialog::~ResourceLimitDialog()
{
}

void ResourceLimitDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(QRectF(rect()), 8, 8);
    painter.setOpacity(1);
    painter.fillPath(path, isDarkTheme ? QColor("#252525") : QColor("#F8F8F8"));
}

void ResourceLimitDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    // Title bar
    QWidget *titleBar = new QWidget();
    titleBar->setFixedHeight(40);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 5, 0);

    titleLabel = new QLabel(tr("Resource Limits - %1").arg(m_processName));
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
    contentLayout->setSpacing(12);
    contentLayout->setContentsMargins(15, 10, 15, 15);

    // CPU Limit Group
    QGroupBox *cpuGroup = new QGroupBox(tr("CPU Limit"));
    QVBoxLayout *cpuLayout = new QVBoxLayout(cpuGroup);

    cpuLimitCheck = new QCheckBox(tr("Enable CPU limit"));
    cpuLayout->addWidget(cpuLimitCheck);

    QHBoxLayout *cpuSliderLayout = new QHBoxLayout();
    cpuSlider = new QSlider(Qt::Horizontal);
    cpuSlider->setRange(1, 100);
    cpuSlider->setValue(100);
    cpuSlider->setEnabled(false);
    cpuValueLabel = new QLabel("100%");
    cpuSliderLayout->addWidget(cpuSlider);
    cpuSliderLayout->addWidget(cpuValueLabel);
    cpuLayout->addLayout(cpuSliderLayout);

    connect(cpuLimitCheck, &QCheckBox::toggled, cpuSlider, &QSlider::setEnabled);
    connect(cpuSlider, &QSlider::valueChanged, this, &ResourceLimitDialog::onCpuSliderChanged);

    contentLayout->addWidget(cpuGroup);

    // Memory Limit Group
    QGroupBox *memGroup = new QGroupBox(tr("Memory Limit"));
    QVBoxLayout *memLayout = new QVBoxLayout(memGroup);

    memoryLimitCheck = new QCheckBox(tr("Enable memory limit"));
    memLayout->addWidget(memoryLimitCheck);

    QHBoxLayout *memSliderLayout = new QHBoxLayout();
    memorySlider = new QSlider(Qt::Horizontal);
    memorySlider->setRange(64, 8192);
    memorySlider->setValue(1024);
    memorySlider->setEnabled(false);
    memoryValueLabel = new QLabel("1024 MB");
    memSliderLayout->addWidget(memorySlider);
    memSliderLayout->addWidget(memoryValueLabel);
    memLayout->addLayout(memSliderLayout);

    connect(memoryLimitCheck, &QCheckBox::toggled, memorySlider, &QSlider::setEnabled);
    connect(memorySlider, &QSlider::valueChanged, this, &ResourceLimitDialog::onMemorySliderChanged);

    contentLayout->addWidget(memGroup);

    // Priority Group
    QGroupBox *prioGroup = new QGroupBox(tr("Process Priority"));
    QGridLayout *prioLayout = new QGridLayout(prioGroup);

    niceLabel = new QLabel(tr("Nice value (-20 to 19):"));
    niceSpin = new QSpinBox();
    niceSpin->setRange(-20, 19);
    niceSpin->setValue(0);
    niceSpin->setFixedHeight(28);
    connect(niceSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ResourceLimitDialog::onNiceChanged);

    prioLayout->addWidget(niceLabel, 0, 0);
    prioLayout->addWidget(niceSpin, 0, 1);

    ioPriorityLabel = new QLabel(tr("I/O Priority:"));
    ioPriorityCombo = new QComboBox();
    ioPriorityCombo->addItem(tr("None (default)"), 0);
    ioPriorityCombo->addItem(tr("Real-time"), 1);
    ioPriorityCombo->addItem(tr("Best-effort"), 2);
    ioPriorityCombo->addItem(tr("Idle"), 3);
    ioPriorityCombo->setFixedHeight(28);

    prioLayout->addWidget(ioPriorityLabel, 1, 0);
    prioLayout->addWidget(ioPriorityCombo, 1, 1);

    contentLayout->addWidget(prioGroup);

    // Status
    statusLabel = new QLabel("");
    contentLayout->addWidget(statusLabel);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    applyBtn = new QPushButton(tr("Apply"));
    applyBtn->setFixedHeight(30);
    connect(applyBtn, &QPushButton::clicked, this, &ResourceLimitDialog::applyLimits);

    cancelBtn = new QPushButton(tr("Cancel"));
    cancelBtn->setFixedHeight(30);
    connect(cancelBtn, &QPushButton::clicked, this, &ResourceLimitDialog::close);

    btnLayout->addStretch();
    btnLayout->addWidget(applyBtn);
    btnLayout->addWidget(cancelBtn);
    contentLayout->addLayout(btnLayout);

    mainLayout->addWidget(content);
}

void ResourceLimitDialog::loadCurrentLimits()
{
    // Get current nice value
    int currentNice = getpriority(PRIO_PROCESS, m_pid);
    niceSpin->setValue(currentNice);
    
    statusLabel->setText(tr("Current nice value: %1").arg(currentNice));
}

void ResourceLimitDialog::onCpuSliderChanged(int value)
{
    cpuValueLabel->setText(QString("%1%").arg(value));
}

void ResourceLimitDialog::onMemorySliderChanged(int value)
{
    memoryValueLabel->setText(QString("%1 MB").arg(value));
}

void ResourceLimitDialog::onNiceChanged(int value)
{
    Q_UNUSED(value);
}

void ResourceLimitDialog::applyLimits()
{
    bool success = true;
    QString results;

    // Apply nice value
    if (setNiceValue(niceSpin->value())) {
        results += tr("Nice value set to %1\n").arg(niceSpin->value());
    } else {
        results += tr("Failed to set nice value (requires root)\n");
        success = false;
    }

    // Apply I/O priority
    int ioPrio = ioPriorityCombo->currentData().toInt();
    if (ioPrio > 0) {
        if (setIoPriority(ioPrio)) {
            results += tr("I/O priority set\n");
        } else {
            results += tr("Failed to set I/O priority\n");
        }
    }

    // Apply CPU limit using cpulimit if enabled
    if (cpuLimitCheck->isChecked()) {
        if (setCpuLimit(cpuSlider->value())) {
            results += tr("CPU limit set to %1%\n").arg(cpuSlider->value());
        } else {
            results += tr("Failed to set CPU limit (cpulimit not installed?)\n");
        }
    }

    // Apply memory limit using cgroups if enabled
    if (memoryLimitCheck->isChecked()) {
        if (setMemoryLimit(memorySlider->value())) {
            results += tr("Memory limit set to %1 MB\n").arg(memorySlider->value());
        } else {
            results += tr("Failed to set memory limit (requires cgroups)\n");
        }
    }

    statusLabel->setText(results);

    if (success) {
        statusLabel->setStyleSheet("color: green;");
    } else {
        statusLabel->setStyleSheet("color: orange;");
    }
}

bool ResourceLimitDialog::setNiceValue(int nice)
{
    // Use renice command
    QProcess proc;
    proc.start("renice", {QString::number(nice), "-p", QString::number(m_pid)});
    proc.waitForFinished(3000);

    if (proc.exitCode() != 0) {
        // Try with pkexec for root privileges
        proc.start("pkexec", {"renice", QString::number(nice), "-p", QString::number(m_pid)});
        proc.waitForFinished(10000);
    }

    return proc.exitCode() == 0;
}

bool ResourceLimitDialog::setIoPriority(int priority)
{
    // Use ionice command
    // priority: 1=realtime, 2=best-effort, 3=idle
    QProcess proc;
    proc.start("ionice", {"-c", QString::number(priority), "-p", QString::number(m_pid)});
    proc.waitForFinished(3000);
    return proc.exitCode() == 0;
}

bool ResourceLimitDialog::setCpuLimit(int percent)
{
    // Check if cpulimit is installed
    QProcess checkProc;
    checkProc.start("which", {"cpulimit"});
    checkProc.waitForFinished(2000);

    if (checkProc.exitCode() != 0) {
        return false;
    }

    // Start cpulimit in background
    QProcess::startDetached("cpulimit", {"-p", QString::number(m_pid),
                                          "-l", QString::number(percent),
                                          "-b"});
    return true;
}

bool ResourceLimitDialog::setMemoryLimit(int megabytes)
{
    // Try using cgroups v2
    QString cgroupPath = QString("/sys/fs/cgroup/user.slice/user-%1.slice/memory.max")
                         .arg(getuid());

    QFile cgroupFile(cgroupPath);
    if (cgroupFile.open(QIODevice::WriteOnly)) {
        QString limit = QString::number((qint64)megabytes * 1024 * 1024);
        cgroupFile.write(limit.toUtf8());
        cgroupFile.close();
        return true;
    }

    // Fallback: try ulimit via shell (only affects new processes)
    return false;
}

void ResourceLimitDialog::updateTheme(const QString &theme)
{
    bool isDark = (theme == "dark");
    isDarkTheme = isDark;
    applyThemeStyle();
    update();
}

void ResourceLimitDialog::applyThemeStyle()
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
        "QSlider { background-color: transparent; } "
        "QSlider::groove:horizontal { background-color: %3; height: 6px; border-radius: 3px; } "
        "QSlider::handle:horizontal { background-color: #2ca7f8; width: 14px; margin: -4px 0; border-radius: 7px; } "
        "QSpinBox { background-color: %5; color: %2; border: 1px solid %3; padding: 3px; border-radius: 4px; } "
        "QComboBox { background-color: %5; color: %2; border: 1px solid %3; padding: 5px; border-radius: 4px; } "
        "QComboBox QAbstractItemView { background-color: %1; color: %2; selection-background-color: #2ca7f8; } "
        "QComboBox::drop-down { border: none; width: 20px; } "
        "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; border-right: 5px solid transparent; border-top: 5px solid %2; } "
        "QPushButton { background-color: %4; color: %2; border: 1px solid %3; padding: 5px 15px; border-radius: 4px; } "
        "QPushButton:hover { background-color: #2ca7f8; color: white; border-color: #2ca7f8; } "
        "QPushButton:pressed { background-color: #1a8ddb; }"
    ).arg(bgColor).arg(textColor).arg(borderColor).arg(headerBg).arg(inputBg);

    setStyleSheet(style);
}
