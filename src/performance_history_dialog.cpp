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

#include "performance_history_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QPainter>
#include <QDateTime>
#include <QPainterPath>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <dthememanager.h>

// HistoryGraphWidget implementation
HistoryGraphWidget::HistoryGraphWidget(QWidget *parent)
    : QWidget(parent), maxPoints(60), maxValue(100.0), darkTheme(true)
{
    setMinimumSize(280, 100);
    graphColor = QColor(0, 150, 136);
}

void HistoryGraphWidget::setDarkTheme(bool dark)
{
    darkTheme = dark;
    update();
}

void HistoryGraphWidget::addDataPoint(double value)
{
    HistoryPoint point;
    point.timestamp = QDateTime::currentMSecsSinceEpoch();
    point.value = value;
    dataPoints.append(point);
    
    while (dataPoints.size() > maxPoints) {
        dataPoints.removeFirst();
    }
    
    if (value > maxValue && maxValue < 1000000) {
        maxValue = value * 1.2;
    }
    
    update();
}

void HistoryGraphWidget::setMaxPoints(int max)
{
    maxPoints = max;
    while (dataPoints.size() > maxPoints) {
        dataPoints.removeFirst();
    }
    update();
}

void HistoryGraphWidget::setColor(const QColor &color)
{
    graphColor = color;
    update();
}

void HistoryGraphWidget::setTitle(const QString &t)
{
    title = t;
    update();
}

void HistoryGraphWidget::setUnit(const QString &u)
{
    unit = u;
    update();
}

void HistoryGraphWidget::clear()
{
    dataPoints.clear();
    maxValue = 100.0;
    update();
}

void HistoryGraphWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    int margin = 5;
    int graphTop = 20;
    int graphHeight = h - graphTop - margin;
    
    // Background - theme aware
    QColor bgColor = darkTheme ? QColor(40, 40, 40) : QColor(245, 245, 245);
    QColor textColor = darkTheme ? Qt::white : Qt::black;
    QColor gridColor = darkTheme ? QColor(60, 60, 60) : QColor(200, 200, 200);

    painter.fillRect(rect(), bgColor);

    // Title
    painter.setPen(textColor);
    painter.setFont(QFont("Sans", 9, QFont::Bold));
    painter.drawText(margin, 15, title);

    // Current value
    if (!dataPoints.isEmpty()) {
        QString valueStr = QString::number(dataPoints.last().value, 'f', 1) + " " + unit;
        painter.drawText(w - 80, 15, valueStr);
    }

    // Grid
    painter.setPen(gridColor);
    for (int i = 0; i <= 4; i++) {
        int y = graphTop + (graphHeight * i / 4);
        painter.drawLine(margin, y, w - margin, y);
    }
    
    if (dataPoints.size() < 2) return;
    
    // Draw graph
    QPainterPath path;
    double xStep = (double)(w - 2 * margin) / (maxPoints - 1);
    
    for (int i = 0; i < dataPoints.size(); i++) {
        double x = margin + i * xStep;
        double y = graphTop + graphHeight - (dataPoints[i].value / maxValue) * graphHeight;
        y = qBound((double)graphTop, y, (double)(graphTop + graphHeight));
        
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }
    
    // Fill under curve
    QPainterPath fillPath = path;
    fillPath.lineTo(margin + (dataPoints.size() - 1) * xStep, graphTop + graphHeight);
    fillPath.lineTo(margin, graphTop + graphHeight);
    fillPath.closeSubpath();
    
    QColor fillColor = graphColor;
    fillColor.setAlpha(80);
    painter.fillPath(fillPath, fillColor);
    
    // Draw line
    painter.setPen(QPen(graphColor, 2));
    painter.drawPath(path);
}

// PerformanceHistoryDialog implementation
PerformanceHistoryDialog::PerformanceHistoryDialog(QWidget *parent)
    : DAbstractDialog(parent), currentMaxPoints(60), isDarkTheme(false),
      prevCpuTotal(0), prevCpuIdle(0), prevRxBytes(0), prevTxBytes(0),
      prevReadSectors(0), prevWriteSectors(0)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(tr("Performance History"));
    setFixedSize(750, 620);

    // Connect to theme changes
    connect(Dtk::Widget::DThemeManager::instance(), &Dtk::Widget::DThemeManager::themeChanged,
            this, &PerformanceHistoryDialog::updateTheme);
    isDarkTheme = (Dtk::Widget::DThemeManager::instance()->theme() == "dark");

    setupUI();
    applyThemeStyle();

    // Start data collection timer
    dataTimer = new QTimer(this);
    connect(dataTimer, &QTimer::timeout, this, &PerformanceHistoryDialog::collectData);
    dataTimer->start(1000);  // Collect data every second

    // Collect initial data point
    collectData();
}

PerformanceHistoryDialog::~PerformanceHistoryDialog()
{
    if (dataTimer) {
        dataTimer->stop();
    }
}

void PerformanceHistoryDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(QRectF(rect()), 8, 8);
    painter.setOpacity(1);
    painter.fillPath(path, isDarkTheme ? QColor("#252525") : QColor("#F8F8F8"));
}

void PerformanceHistoryDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    // Title bar
    QWidget *titleBar = new QWidget();
    titleBar->setFixedHeight(40);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 5, 0);

    titleLabel = new QLabel(tr("Performance History"));
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
    contentLayout->setSpacing(8);
    contentLayout->setContentsMargins(15, 10, 15, 15);

    // Controls
    QHBoxLayout *controlLayout = new QHBoxLayout();

    QLabel *rangeLabel = new QLabel(tr("Time Range:"));
    timeRangeCombo = new QComboBox();
    timeRangeCombo->addItem(tr("1 minute"), 60);
    timeRangeCombo->addItem(tr("5 minutes"), 300);
    timeRangeCombo->addItem(tr("15 minutes"), 900);
    timeRangeCombo->addItem(tr("1 hour"), 3600);
    timeRangeCombo->setFixedHeight(28);
    connect(timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PerformanceHistoryDialog::onTimeRangeChanged);

    clearBtn = new QPushButton(tr("Clear History"));
    clearBtn->setFixedHeight(28);
    connect(clearBtn, &QPushButton::clicked, this, &PerformanceHistoryDialog::clearHistory);

    statusLabel = new QLabel(tr("Recording..."));

    controlLayout->addWidget(rangeLabel);
    controlLayout->addWidget(timeRangeCombo);
    controlLayout->addStretch();
    controlLayout->addWidget(statusLabel);
    controlLayout->addWidget(clearBtn);

    contentLayout->addLayout(controlLayout);

    // Graphs grid
    QGridLayout *graphGrid = new QGridLayout();
    graphGrid->setSpacing(8);

    cpuGraph = new HistoryGraphWidget();
    cpuGraph->setTitle(tr("CPU Usage"));
    cpuGraph->setUnit("%");
    cpuGraph->setColor(QColor(0, 150, 136));

    memoryGraph = new HistoryGraphWidget();
    memoryGraph->setTitle(tr("Memory Usage"));
    memoryGraph->setUnit("%");
    memoryGraph->setColor(QColor(156, 39, 176));

    networkDownGraph = new HistoryGraphWidget();
    networkDownGraph->setTitle(tr("Network Download"));
    networkDownGraph->setUnit("KB/s");
    networkDownGraph->setColor(QColor(33, 150, 243));

    networkUpGraph = new HistoryGraphWidget();
    networkUpGraph->setTitle(tr("Network Upload"));
    networkUpGraph->setUnit("KB/s");
    networkUpGraph->setColor(QColor(255, 152, 0));

    diskReadGraph = new HistoryGraphWidget();
    diskReadGraph->setTitle(tr("Disk Read"));
    diskReadGraph->setUnit("KB/s");
    diskReadGraph->setColor(QColor(76, 175, 80));

    diskWriteGraph = new HistoryGraphWidget();
    diskWriteGraph->setTitle(tr("Disk Write"));
    diskWriteGraph->setUnit("KB/s");
    diskWriteGraph->setColor(QColor(244, 67, 54));

    graphGrid->addWidget(cpuGraph, 0, 0);
    graphGrid->addWidget(memoryGraph, 0, 1);
    graphGrid->addWidget(networkDownGraph, 1, 0);
    graphGrid->addWidget(networkUpGraph, 1, 1);
    graphGrid->addWidget(diskReadGraph, 2, 0);
    graphGrid->addWidget(diskWriteGraph, 2, 1);

    contentLayout->addLayout(graphGrid);

    // System Information Panel
    QFrame *sysInfoFrame = new QFrame();
    sysInfoFrame->setFrameShape(QFrame::StyledPanel);
    sysInfoFrame->setFixedHeight(70);

    QGridLayout *sysInfoLayout = new QGridLayout(sysInfoFrame);
    sysInfoLayout->setContentsMargins(10, 5, 10, 5);
    sysInfoLayout->setSpacing(8);

    // Row 1: Processes, Threads, Handles
    QLabel *processTitle = new QLabel(tr("Processes:"));
    processTitle->setObjectName("infoTitle");
    processCountLabel = new QLabel("0");
    processCountLabel->setObjectName("infoValue");

    QLabel *threadTitle = new QLabel(tr("Threads:"));
    threadTitle->setObjectName("infoTitle");
    threadCountLabel = new QLabel("0");
    threadCountLabel->setObjectName("infoValue");

    QLabel *handleTitle = new QLabel(tr("Handles:"));
    handleTitle->setObjectName("infoTitle");
    handleCountLabel = new QLabel("0");
    handleCountLabel->setObjectName("infoValue");

    // Row 2: Uptime, CPU Speed, Cache
    QLabel *uptimeTitle = new QLabel(tr("Uptime:"));
    uptimeTitle->setObjectName("infoTitle");
    uptimeLabel = new QLabel("0:00:00");
    uptimeLabel->setObjectName("infoValue");

    QLabel *cpuSpeedTitle = new QLabel(tr("CPU Speed:"));
    cpuSpeedTitle->setObjectName("infoTitle");
    cpuSpeedLabel = new QLabel("0 MHz");
    cpuSpeedLabel->setObjectName("infoValue");

    QLabel *cacheTitle = new QLabel(tr("CPU Cache:"));
    cacheTitle->setObjectName("infoTitle");
    cacheSizeLabel = new QLabel("0 KB");
    cacheSizeLabel->setObjectName("infoValue");

    sysInfoLayout->addWidget(processTitle, 0, 0);
    sysInfoLayout->addWidget(processCountLabel, 0, 1);
    sysInfoLayout->addWidget(threadTitle, 0, 2);
    sysInfoLayout->addWidget(threadCountLabel, 0, 3);
    sysInfoLayout->addWidget(handleTitle, 0, 4);
    sysInfoLayout->addWidget(handleCountLabel, 0, 5);

    sysInfoLayout->addWidget(uptimeTitle, 1, 0);
    sysInfoLayout->addWidget(uptimeLabel, 1, 1);
    sysInfoLayout->addWidget(cpuSpeedTitle, 1, 2);
    sysInfoLayout->addWidget(cpuSpeedLabel, 1, 3);
    sysInfoLayout->addWidget(cacheTitle, 1, 4);
    sysInfoLayout->addWidget(cacheSizeLabel, 1, 5);

    sysInfoLayout->setColumnStretch(1, 1);
    sysInfoLayout->setColumnStretch(3, 1);
    sysInfoLayout->setColumnStretch(5, 1);

    contentLayout->addWidget(sysInfoFrame);

    mainLayout->addWidget(content);
}

void PerformanceHistoryDialog::onTimeRangeChanged(int index)
{
    currentMaxPoints = timeRangeCombo->itemData(index).toInt();
    cpuGraph->setMaxPoints(currentMaxPoints);
    memoryGraph->setMaxPoints(currentMaxPoints);
    networkDownGraph->setMaxPoints(currentMaxPoints);
    networkUpGraph->setMaxPoints(currentMaxPoints);
    diskReadGraph->setMaxPoints(currentMaxPoints);
    diskWriteGraph->setMaxPoints(currentMaxPoints);
}

void PerformanceHistoryDialog::clearHistory()
{
    cpuGraph->clear();
    memoryGraph->clear();
    networkDownGraph->clear();
    networkUpGraph->clear();
    diskReadGraph->clear();
    diskWriteGraph->clear();
}

void PerformanceHistoryDialog::updateCpuHistory(double value)
{
    cpuGraph->addDataPoint(value);
}

void PerformanceHistoryDialog::updateMemoryHistory(double value)
{
    memoryGraph->addDataPoint(value);
}

void PerformanceHistoryDialog::updateNetworkHistory(double downloadKB, double uploadKB)
{
    networkDownGraph->addDataPoint(downloadKB);
    networkUpGraph->addDataPoint(uploadKB);
}

void PerformanceHistoryDialog::updateDiskHistory(double readKB, double writeKB)
{
    diskReadGraph->addDataPoint(readKB);
    diskWriteGraph->addDataPoint(writeKB);
}

void PerformanceHistoryDialog::updateTheme(const QString &theme)
{
    isDarkTheme = (theme == "dark");
    applyThemeStyle();
    update();
}

void PerformanceHistoryDialog::applyThemeStyle()
{
    QString textColor = isDarkTheme ? "#FFFFFF" : "#303030";
    QString bgColor = isDarkTheme ? "#2D2D2D" : "#FFFFFF";
    QString borderColor = isDarkTheme ? "#444444" : "#CCCCCC";
    QString headerBg = isDarkTheme ? "#3A3A3A" : "#E8E8E8";
    QString inputBg = isDarkTheme ? "#3A3A3A" : "#FFFFFF";
    QString titleColor = isDarkTheme ? "#888888" : "#666666";
    QString valueColor = isDarkTheme ? "#FFFFFF" : "#303030";
    QString frameBg = isDarkTheme ? "#2D2D2D" : "#F5F5F5";

    // Title label
    titleLabel->setStyleSheet(QString("QLabel { color: %1; background: transparent; font-size: 14px; font-weight: bold; }").arg(textColor));

    // Close button theme
    Dtk::Widget::DThemeManager::instance()->setTheme(closeButton, isDarkTheme ? "dark" : "light");

    QString style = QString(
        "QLabel { color: %2; background-color: transparent; } "
        "QLabel#infoTitle { color: %6; font-size: 11px; } "
        "QLabel#infoValue { color: %7; font-size: 12px; font-weight: bold; } "
        "QFrame { background-color: %8; border: 1px solid %3; border-radius: 6px; } "
        "QComboBox { background-color: %5; color: %2; border: 1px solid %3; padding: 5px; border-radius: 4px; } "
        "QComboBox QAbstractItemView { background-color: %1; color: %2; selection-background-color: #2ca7f8; } "
        "QComboBox::drop-down { border: none; width: 20px; } "
        "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; border-right: 5px solid transparent; border-top: 5px solid %2; } "
        "QPushButton { background-color: %4; color: %2; border: 1px solid %3; padding: 5px 15px; border-radius: 4px; } "
        "QPushButton:hover { background-color: #2ca7f8; color: white; border-color: #2ca7f8; } "
        "QPushButton:pressed { background-color: #1a8ddb; }"
    ).arg(bgColor).arg(textColor).arg(borderColor).arg(headerBg).arg(inputBg).arg(titleColor).arg(valueColor).arg(frameBg);

    setStyleSheet(style);

    // Update graph backgrounds
    cpuGraph->setDarkTheme(isDarkTheme);
    memoryGraph->setDarkTheme(isDarkTheme);
    networkDownGraph->setDarkTheme(isDarkTheme);
    networkUpGraph->setDarkTheme(isDarkTheme);
    diskReadGraph->setDarkTheme(isDarkTheme);
    diskWriteGraph->setDarkTheme(isDarkTheme);
}

void PerformanceHistoryDialog::collectData()
{
    // Collect CPU usage
    double cpuUsage = getCpuUsage();
    cpuGraph->addDataPoint(cpuUsage);

    // Collect Memory usage
    double memUsage = getMemoryUsage();
    memoryGraph->addDataPoint(memUsage);

    // Collect Network stats
    double rxKB = 0, txKB = 0;
    getNetworkStats(rxKB, txKB);
    networkDownGraph->addDataPoint(rxKB);
    networkUpGraph->addDataPoint(txKB);

    // Collect Disk stats
    double readKB = 0, writeKB = 0;
    getDiskStats(readKB, writeKB);
    diskReadGraph->addDataPoint(readKB);
    diskWriteGraph->addDataPoint(writeKB);

    // Update system info
    updateSystemInfo();

    statusLabel->setText(tr("Recording... CPU: %1% | Memory: %2%")
        .arg(cpuUsage, 0, 'f', 1).arg(memUsage, 0, 'f', 1));
}

double PerformanceHistoryDialog::getCpuUsage()
{
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;

    QTextStream in(&file);
    QString line = in.readLine();
    file.close();

    QStringList parts = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (parts.size() < 5) return 0;

    unsigned long long user = parts[1].toULongLong();
    unsigned long long nice = parts[2].toULongLong();
    unsigned long long system = parts[3].toULongLong();
    unsigned long long idle = parts[4].toULongLong();
    unsigned long long iowait = parts.size() > 5 ? parts[5].toULongLong() : 0;

    unsigned long long total = user + nice + system + idle + iowait;
    unsigned long long totalIdle = idle + iowait;

    double cpuPercent = 0;
    if (prevCpuTotal > 0) {
        unsigned long long totalDiff = total - prevCpuTotal;
        unsigned long long idleDiff = totalIdle - prevCpuIdle;
        if (totalDiff > 0) {
            cpuPercent = 100.0 * (totalDiff - idleDiff) / totalDiff;
        }
    }

    prevCpuTotal = total;
    prevCpuIdle = totalIdle;

    return cpuPercent;
}

double PerformanceHistoryDialog::getMemoryUsage()
{
    QFile file("/proc/meminfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;

    QTextStream in(&file);
    unsigned long long memTotal = 0, memAvailable = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("MemTotal:")) {
            memTotal = line.split(QRegExp("\\s+"))[1].toULongLong();
        } else if (line.startsWith("MemAvailable:")) {
            memAvailable = line.split(QRegExp("\\s+"))[1].toULongLong();
            break;
        }
    }
    file.close();

    if (memTotal > 0) {
        return 100.0 * (memTotal - memAvailable) / memTotal;
    }
    return 0;
}

void PerformanceHistoryDialog::getNetworkStats(double &rxKB, double &txKB)
{
    QFile file("/proc/net/dev");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        rxKB = txKB = 0;
        return;
    }

    QTextStream in(&file);
    unsigned long long totalRx = 0, totalTx = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("lo:") || !line.contains(":")) continue;

        QStringList parts = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        if (parts.size() >= 10) {
            // Remove interface name (first part includes the colon)
            QString firstPart = parts[0];
            int colonPos = firstPart.indexOf(':');
            if (colonPos >= 0) {
                QString rxStr = firstPart.mid(colonPos + 1);
                if (!rxStr.isEmpty()) {
                    totalRx += rxStr.toULongLong();
                } else if (parts.size() > 1) {
                    totalRx += parts[1].toULongLong();
                }
            }
            totalTx += parts[parts.size() >= 10 ? 9 : 8].toULongLong();
        }
    }
    file.close();

    if (prevRxBytes > 0) {
        rxKB = (totalRx - prevRxBytes) / 1024.0;
        txKB = (totalTx - prevTxBytes) / 1024.0;
    } else {
        rxKB = txKB = 0;
    }

    prevRxBytes = totalRx;
    prevTxBytes = totalTx;
}

void PerformanceHistoryDialog::getDiskStats(double &readKB, double &writeKB)
{
    QFile file("/proc/diskstats");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        readKB = writeKB = 0;
        return;
    }

    QTextStream in(&file);
    unsigned long long totalRead = 0, totalWrite = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QStringList parts = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);

        if (parts.size() >= 14) {
            QString devName = parts[2];
            // Only count main disks (sda, nvme0n1, etc.) not partitions
            if ((devName.startsWith("sd") && devName.length() == 3) ||
                (devName.startsWith("nvme") && devName.contains("n") && !devName.contains("p"))) {
                totalRead += parts[5].toULongLong();   // sectors read
                totalWrite += parts[9].toULongLong();  // sectors written
            }
        }
    }
    file.close();

    // Sectors are typically 512 bytes
    if (prevReadSectors > 0) {
        readKB = (totalRead - prevReadSectors) * 512.0 / 1024.0;
        writeKB = (totalWrite - prevWriteSectors) * 512.0 / 1024.0;
    } else {
        readKB = writeKB = 0;
    }

    prevReadSectors = totalRead;
    prevWriteSectors = totalWrite;
}

QString PerformanceHistoryDialog::formatUptime(long seconds)
{
    long days = seconds / 86400;
    long hours = (seconds % 86400) / 3600;
    long mins = (seconds % 3600) / 60;
    long secs = seconds % 60;

    if (days > 0) {
        return QString("%1d %2:%3:%4")
            .arg(days)
            .arg(hours, 2, 10, QChar('0'))
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(mins, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

void PerformanceHistoryDialog::updateSystemInfo()
{
    // Process and thread count
    int processCount = 0;
    int threadCount = 0;
    int handleCount = 0;

    QDir procDir("/proc");
    QStringList procList = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &entry : procList) {
        bool ok;
        int pid = entry.toInt(&ok);
        if (ok && pid > 0) {
            processCount++;

            // Count threads for this process
            QDir taskDir(QString("/proc/%1/task").arg(pid));
            if (taskDir.exists()) {
                threadCount += taskDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).count();
            }

            // Count file handles for this process
            QDir fdDir(QString("/proc/%1/fd").arg(pid));
            if (fdDir.exists()) {
                handleCount += fdDir.entryList(QDir::Files | QDir::NoDotAndDotDot).count();
            }
        }
    }

    processCountLabel->setText(QString::number(processCount));
    threadCountLabel->setText(QString::number(threadCount));
    handleCountLabel->setText(QString::number(handleCount));

    // Uptime
    QFile uptimeFile("/proc/uptime");
    if (uptimeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&uptimeFile);
        QString line = in.readLine();
        double upSecs = line.split(" ")[0].toDouble();
        uptimeLabel->setText(formatUptime((long)upSecs));
        uptimeFile.close();
    }

    // CPU Speed - read current and base frequencies
    QString cpuSpeedText = "";
    QFile cpuFreqFile("/proc/cpuinfo");
    if (cpuFreqFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&cpuFreqFile);
        double currentMHz = 0;
        int cpuCount = 0;

        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("cpu MHz")) {
                QStringList parts = line.split(":");
                if (parts.size() >= 2) {
                    currentMHz += parts[1].trimmed().toDouble();
                    cpuCount++;
                }
            }
        }
        cpuFreqFile.close();

        if (cpuCount > 0) {
            currentMHz /= cpuCount;  // Average across cores

            // Try to get base speed from scaling_max_freq
            QFile maxFreqFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
            double baseMHz = 0;
            if (maxFreqFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream maxIn(&maxFreqFile);
                baseMHz = maxIn.readLine().toDouble() / 1000.0;  // kHz to MHz
                maxFreqFile.close();
            }

            if (baseMHz > 0) {
                cpuSpeedText = QString("%1 MHz (base: %2 MHz)")
                    .arg(currentMHz, 0, 'f', 0)
                    .arg(baseMHz, 0, 'f', 0);
            } else {
                cpuSpeedText = QString("%1 MHz").arg(currentMHz, 0, 'f', 0);
            }
        }
    }
    cpuSpeedLabel->setText(cpuSpeedText.isEmpty() ? "N/A" : cpuSpeedText);

    // CPU Cache sizes
    QString cacheText = "";
    QFile cacheFile("/proc/cpuinfo");
    if (cacheFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&cacheFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("cache size")) {
                QStringList parts = line.split(":");
                if (parts.size() >= 2) {
                    cacheText = parts[1].trimmed();
                    break;
                }
            }
        }
        cacheFile.close();
    }

    // Try to get more detailed cache info
    if (cacheText.isEmpty()) {
        long l1d = 0, l1i = 0, l2 = 0, l3 = 0;

        for (int i = 0; i < 4; i++) {
            QString levelPath = QString("/sys/devices/system/cpu/cpu0/cache/index%1/level").arg(i);
            QString sizePath = QString("/sys/devices/system/cpu/cpu0/cache/index%1/size").arg(i);
            QString typePath = QString("/sys/devices/system/cpu/cpu0/cache/index%1/type").arg(i);

            QFile levelFile(levelPath);
            QFile sizeFile(sizePath);
            QFile typeFile(typePath);

            if (levelFile.open(QIODevice::ReadOnly) && sizeFile.open(QIODevice::ReadOnly)) {
                int level = QTextStream(&levelFile).readLine().toInt();
                QString sizeStr = QTextStream(&sizeFile).readLine().trimmed();
                long sizeKB = sizeStr.replace("K", "").toLong();

                QString type = "";
                if (typeFile.open(QIODevice::ReadOnly)) {
                    type = QTextStream(&typeFile).readLine().trimmed();
                    typeFile.close();
                }

                if (level == 1) {
                    if (type == "Data") l1d = sizeKB;
                    else if (type == "Instruction") l1i = sizeKB;
                } else if (level == 2) {
                    l2 = sizeKB;
                } else if (level == 3) {
                    l3 = sizeKB;
                }

                levelFile.close();
                sizeFile.close();
            }
        }

        QStringList cacheParts;
        if (l1d > 0 || l1i > 0) cacheParts << QString("L1: %1K").arg(l1d + l1i);
        if (l2 > 0) cacheParts << QString("L2: %1K").arg(l2);
        if (l3 > 0) cacheParts << QString("L3: %1M").arg(l3 / 1024);

        cacheText = cacheParts.join(" / ");
    }

    cacheSizeLabel->setText(cacheText.isEmpty() ? "N/A" : cacheText);
}
