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

#ifndef RESOURCELIMITDIALOG_H
#define RESOURCELIMITDIALOG_H

#include "ddialog.h"
#include <QWidget>
#include <QSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>

DWIDGET_USE_NAMESPACE

class ResourceLimitDialog : public DDialog
{
    Q_OBJECT

public:
    explicit ResourceLimitDialog(int pid, const QString &processName, QWidget *parent = nullptr);
    ~ResourceLimitDialog();

private slots:
    void applyLimits();
    void onCpuSliderChanged(int value);
    void onMemorySliderChanged(int value);
    void onNiceChanged(int value);

private:
    void setupUI();
    void loadCurrentLimits();
    bool setCpuLimit(int percent);
    bool setMemoryLimit(int megabytes);
    bool setNiceValue(int nice);
    bool setIoPriority(int priority);

    int m_pid;
    QString m_processName;
    
    QCheckBox *cpuLimitCheck;
    QSlider *cpuSlider;
    QLabel *cpuValueLabel;
    
    QCheckBox *memoryLimitCheck;
    QSlider *memorySlider;
    QLabel *memoryValueLabel;
    
    QLabel *niceLabel;
    QSpinBox *niceSpin;
    
    QLabel *ioPriorityLabel;
    QComboBox *ioPriorityCombo;
    
    QPushButton *applyBtn;
    QPushButton *cancelBtn;
    QLabel *statusLabel;
};

#endif

