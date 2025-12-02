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
#include <QPainter>
#include <QDateTime>
#include <QPainterPath>

// HistoryGraphWidget implementation
HistoryGraphWidget::HistoryGraphWidget(QWidget *parent)
    : QWidget(parent), maxPoints(60), maxValue(100.0)
{
    setMinimumSize(280, 100);
    graphColor = QColor(0, 150, 136);
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
    
    // Background
    painter.fillRect(rect(), QColor(40, 40, 40));
    
    // Title
    painter.setPen(Qt::white);
    painter.setFont(QFont("Sans", 9, QFont::Bold));
    painter.drawText(margin, 15, title);
    
    // Current value
    if (!dataPoints.isEmpty()) {
        QString valueStr = QString::number(dataPoints.last().value, 'f', 1) + " " + unit;
        painter.drawText(w - 80, 15, valueStr);
    }
    
    // Grid
    painter.setPen(QColor(60, 60, 60));
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
    : DDialog(parent), currentMaxPoints(60)
{
    setWindowTitle(tr("Performance History"));
    setFixedSize(640, 520);
    setupUI();
}

PerformanceHistoryDialog::~PerformanceHistoryDialog()
{
}

void PerformanceHistoryDialog::setupUI()
{
    QWidget *content = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(content);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Controls
    QHBoxLayout *controlLayout = new QHBoxLayout();

    QLabel *rangeLabel = new QLabel(tr("Time Range:"), content);
    timeRangeCombo = new QComboBox(content);
    timeRangeCombo->addItem(tr("1 minute"), 60);
    timeRangeCombo->addItem(tr("5 minutes"), 300);
    timeRangeCombo->addItem(tr("15 minutes"), 900);
    timeRangeCombo->addItem(tr("1 hour"), 3600);
    connect(timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PerformanceHistoryDialog::onTimeRangeChanged);

    clearBtn = new QPushButton(tr("Clear History"), content);
    connect(clearBtn, &QPushButton::clicked, this, &PerformanceHistoryDialog::clearHistory);

    statusLabel = new QLabel(tr("Recording..."), content);

    controlLayout->addWidget(rangeLabel);
    controlLayout->addWidget(timeRangeCombo);
    controlLayout->addStretch();
    controlLayout->addWidget(statusLabel);
    controlLayout->addWidget(clearBtn);

    mainLayout->addLayout(controlLayout);

    // Graphs grid
    QGridLayout *graphGrid = new QGridLayout();
    graphGrid->setSpacing(8);

    cpuGraph = new HistoryGraphWidget(content);
    cpuGraph->setTitle(tr("CPU Usage"));
    cpuGraph->setUnit("%");
    cpuGraph->setColor(QColor(0, 150, 136));

    memoryGraph = new HistoryGraphWidget(content);
    memoryGraph->setTitle(tr("Memory Usage"));
    memoryGraph->setUnit("%");
    memoryGraph->setColor(QColor(156, 39, 176));

    networkDownGraph = new HistoryGraphWidget(content);
    networkDownGraph->setTitle(tr("Network Download"));
    networkDownGraph->setUnit("KB/s");
    networkDownGraph->setColor(QColor(33, 150, 243));

    networkUpGraph = new HistoryGraphWidget(content);
    networkUpGraph->setTitle(tr("Network Upload"));
    networkUpGraph->setUnit("KB/s");
    networkUpGraph->setColor(QColor(255, 152, 0));

    diskReadGraph = new HistoryGraphWidget(content);
    diskReadGraph->setTitle(tr("Disk Read"));
    diskReadGraph->setUnit("KB/s");
    diskReadGraph->setColor(QColor(76, 175, 80));

    diskWriteGraph = new HistoryGraphWidget(content);
    diskWriteGraph->setTitle(tr("Disk Write"));
    diskWriteGraph->setUnit("KB/s");
    diskWriteGraph->setColor(QColor(244, 67, 54));

    graphGrid->addWidget(cpuGraph, 0, 0);
    graphGrid->addWidget(memoryGraph, 0, 1);
    graphGrid->addWidget(networkDownGraph, 1, 0);
    graphGrid->addWidget(networkUpGraph, 1, 1);
    graphGrid->addWidget(diskReadGraph, 2, 0);
    graphGrid->addWidget(diskWriteGraph, 2, 1);

    mainLayout->addLayout(graphGrid);

    addContent(content);
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

