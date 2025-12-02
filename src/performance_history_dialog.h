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

#ifndef PERFORMANCEHISTORYDIALOG_H
#define PERFORMANCEHISTORYDIALOG_H

#include "ddialog.h"
#include <QWidget>
#include <QTimer>
#include <QList>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

DWIDGET_USE_NAMESPACE

struct HistoryPoint {
    qint64 timestamp;
    double value;
};

class HistoryGraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryGraphWidget(QWidget *parent = nullptr);
    void addDataPoint(double value);
    void setMaxPoints(int max);
    void setColor(const QColor &color);
    void setTitle(const QString &title);
    void setUnit(const QString &unit);
    void clear();
    void setDarkTheme(bool dark);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<HistoryPoint> dataPoints;
    int maxPoints;
    QColor graphColor;
    QString title;
    QString unit;
    double maxValue;
    bool darkTheme;
};

class PerformanceHistoryDialog : public DDialog
{
    Q_OBJECT

public:
    explicit PerformanceHistoryDialog(QWidget *parent = nullptr);
    ~PerformanceHistoryDialog();

    void updateCpuHistory(double value);
    void updateMemoryHistory(double value);
    void updateNetworkHistory(double downloadKB, double uploadKB);
    void updateDiskHistory(double readKB, double writeKB);

private slots:
    void onTimeRangeChanged(int index);
    void clearHistory();
    void updateTheme(const QString &theme);

private:
    void setupUI();

    HistoryGraphWidget *cpuGraph;
    HistoryGraphWidget *memoryGraph;
    HistoryGraphWidget *networkDownGraph;
    HistoryGraphWidget *networkUpGraph;
    HistoryGraphWidget *diskReadGraph;
    HistoryGraphWidget *diskWriteGraph;

    QComboBox *timeRangeCombo;
    QPushButton *clearBtn;
    QLabel *statusLabel;

    int currentMaxPoints;
    bool isDarkTheme;
};

#endif

