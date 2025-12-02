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
#include <sys/resource.h>
#include <unistd.h>

ResourceLimitDialog::ResourceLimitDialog(int pid, const QString &processName, QWidget *parent)
    : DDialog(parent), m_pid(pid), m_processName(processName)
{
    setWindowTitle(tr("Resource Limits - %1 (PID: %2)").arg(processName).arg(pid));
    setFixedSize(450, 400);
    setupUI();
    loadCurrentLimits();
}

ResourceLimitDialog::~ResourceLimitDialog()
{
}

void ResourceLimitDialog::setupUI()
{
    QWidget *content = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(content);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // CPU Limit Group
    QGroupBox *cpuGroup = new QGroupBox(tr("CPU Limit"), content);
    QVBoxLayout *cpuLayout = new QVBoxLayout(cpuGroup);
    
    cpuLimitCheck = new QCheckBox(tr("Enable CPU limit"), cpuGroup);
    cpuLayout->addWidget(cpuLimitCheck);
    
    QHBoxLayout *cpuSliderLayout = new QHBoxLayout();
    cpuSlider = new QSlider(Qt::Horizontal, cpuGroup);
    cpuSlider->setRange(1, 100);
    cpuSlider->setValue(100);
    cpuSlider->setEnabled(false);
    cpuValueLabel = new QLabel("100%", cpuGroup);
    cpuSliderLayout->addWidget(cpuSlider);
    cpuSliderLayout->addWidget(cpuValueLabel);
    cpuLayout->addLayout(cpuSliderLayout);
    
    connect(cpuLimitCheck, &QCheckBox::toggled, cpuSlider, &QSlider::setEnabled);
    connect(cpuSlider, &QSlider::valueChanged, this, &ResourceLimitDialog::onCpuSliderChanged);
    
    mainLayout->addWidget(cpuGroup);
    
    // Memory Limit Group
    QGroupBox *memGroup = new QGroupBox(tr("Memory Limit"), content);
    QVBoxLayout *memLayout = new QVBoxLayout(memGroup);
    
    memoryLimitCheck = new QCheckBox(tr("Enable memory limit"), memGroup);
    memLayout->addWidget(memoryLimitCheck);
    
    QHBoxLayout *memSliderLayout = new QHBoxLayout();
    memorySlider = new QSlider(Qt::Horizontal, memGroup);
    memorySlider->setRange(64, 8192);
    memorySlider->setValue(1024);
    memorySlider->setEnabled(false);
    memoryValueLabel = new QLabel("1024 MB", memGroup);
    memSliderLayout->addWidget(memorySlider);
    memSliderLayout->addWidget(memoryValueLabel);
    memLayout->addLayout(memSliderLayout);
    
    connect(memoryLimitCheck, &QCheckBox::toggled, memorySlider, &QSlider::setEnabled);
    connect(memorySlider, &QSlider::valueChanged, this, &ResourceLimitDialog::onMemorySliderChanged);
    
    mainLayout->addWidget(memGroup);
    
    // Priority Group
    QGroupBox *prioGroup = new QGroupBox(tr("Process Priority"), content);
    QGridLayout *prioLayout = new QGridLayout(prioGroup);
    
    niceLabel = new QLabel(tr("Nice value (-20 to 19):"), prioGroup);
    niceSpin = new QSpinBox(prioGroup);
    niceSpin->setRange(-20, 19);
    niceSpin->setValue(0);
    connect(niceSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &ResourceLimitDialog::onNiceChanged);
    
    prioLayout->addWidget(niceLabel, 0, 0);
    prioLayout->addWidget(niceSpin, 0, 1);
    
    ioPriorityLabel = new QLabel(tr("I/O Priority:"), prioGroup);
    ioPriorityCombo = new QComboBox(prioGroup);
    ioPriorityCombo->addItem(tr("None (default)"), 0);
    ioPriorityCombo->addItem(tr("Real-time"), 1);
    ioPriorityCombo->addItem(tr("Best-effort"), 2);
    ioPriorityCombo->addItem(tr("Idle"), 3);
    
    prioLayout->addWidget(ioPriorityLabel, 1, 0);
    prioLayout->addWidget(ioPriorityCombo, 1, 1);
    
    mainLayout->addWidget(prioGroup);
    
    // Status
    statusLabel = new QLabel("", content);
    statusLabel->setStyleSheet("color: #888;");
    mainLayout->addWidget(statusLabel);
    
    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    applyBtn = new QPushButton(tr("Apply"), content);
    connect(applyBtn, &QPushButton::clicked, this, &ResourceLimitDialog::applyLimits);
    
    cancelBtn = new QPushButton(tr("Cancel"), content);
    connect(cancelBtn, &QPushButton::clicked, this, &ResourceLimitDialog::close);
    
    btnLayout->addStretch();
    btnLayout->addWidget(applyBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);
    
    addContent(content);
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

