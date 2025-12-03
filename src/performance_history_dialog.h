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

#include <dabstractdialog.h>
#include <dwindowclosebutton.h>
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

class PerformanceHistoryDialog : public DAbstractDialog
{
    Q_OBJECT

public:
    explicit PerformanceHistoryDialog(QWidget *parent = nullptr);
    ~PerformanceHistoryDialog();

    void updateCpuHistory(double value);
    void updateMemoryHistory(double value);
    void updateNetworkHistory(double downloadKB, double uploadKB);
    void updateDiskHistory(double readKB, double writeKB);

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void onTimeRangeChanged(int index);
    void clearHistory();
    void updateTheme(const QString &theme);
    void collectData();

private:
    void setupUI();
    void applyThemeStyle();
    double getCpuUsage();
    double getMemoryUsage();
    void getNetworkStats(double &rxKB, double &txKB);
    void getDiskStats(double &readKB, double &writeKB);
    void updateSystemInfo();
    QString formatUptime(long seconds);

    HistoryGraphWidget *cpuGraph;
    HistoryGraphWidget *memoryGraph;
    HistoryGraphWidget *networkDownGraph;
    HistoryGraphWidget *networkUpGraph;
    HistoryGraphWidget *diskReadGraph;
    HistoryGraphWidget *diskWriteGraph;

    QComboBox *timeRangeCombo;
    QPushButton *clearBtn;
    QLabel *statusLabel;
    QLabel *titleLabel;
    DWindowCloseButton *closeButton;

    // System info labels
    QLabel *processCountLabel;
    QLabel *threadCountLabel;
    QLabel *handleCountLabel;
    QLabel *uptimeLabel;
    QLabel *cpuSpeedLabel;
    QLabel *cacheSizeLabel;

    QTimer *dataTimer;
    int currentMaxPoints;
    bool isDarkTheme;

    // For CPU calculation
    unsigned long long prevCpuTotal;
    unsigned long long prevCpuIdle;

    // For network calculation
    unsigned long long prevRxBytes;
    unsigned long long prevTxBytes;

    // For disk calculation
    unsigned long long prevReadSectors;
    unsigned long long prevWriteSectors;
};

#endif

