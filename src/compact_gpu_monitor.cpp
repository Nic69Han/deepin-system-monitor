/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 * Copyright (C) 2024 Nic69Han - GPU Monitor Addition
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "constant.h"
#include "dthememanager.h"
#include "compact_gpu_monitor.h"
#include "smooth_curve_generator.h"
#include "utils.h"
#include <QDebug>
#include <QPainter>
#include <QApplication>
#include <DHiDPIHelper>

DWIDGET_USE_NAMESPACE

using namespace Utils;

CompactGpuMonitor::CompactGpuMonitor(QWidget *parent) : QWidget(parent)
{
    initTheme();

    connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &CompactGpuMonitor::changeTheme);

    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedWidth(statusBarMaxWidth);

    // Check GPU availability first to set appropriate size
    gpuAvailable = GpuMonitor::hasGpu();
    if (gpuAvailable) {
        setFixedHeight(160);
        currentGpuInfo = GpuMonitor::getGpuInfo();
    } else {
        // Minimal height when no GPU
        setFixedHeight(30);
    }

    pointsNumber = int(statusBarMaxWidth / 5.4);

    for (int j = 0; j < pointsNumber; j++) {
        gpuPercents.append(0);
    }

    // Update GPU info every 2 seconds
    updateTimer = new QTimer();
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    updateTimer->start(2000);
}

CompactGpuMonitor::~CompactGpuMonitor()
{
    delete updateTimer;
}

void CompactGpuMonitor::initTheme()
{
    if (DThemeManager::instance()->theme() == "light") {
        textColor = "#303030";
        summaryColor = "#505050";
    } else {
        textColor = "#ffffff";
        summaryColor = "#909090";
    }
}

void CompactGpuMonitor::changeTheme(QString)
{
    initTheme();
}

void CompactGpuMonitor::updateStatus()
{
    if (!gpuAvailable) {
        gpuAvailable = GpuMonitor::hasGpu();
        if (!gpuAvailable) return;
    }

    currentGpuInfo = GpuMonitor::getGpuInfo();
    totalGpuPercent = currentGpuInfo.usagePercent;

    gpuPercents.append(totalGpuPercent);

    if (gpuPercents.size() > pointsNumber) {
        gpuPercents.pop_front();
    }

    QList<QPointF> points;
    double maxHeight = 0;
    for (int i = 0; i < gpuPercents.size(); i++) {
        if (gpuPercents.at(i) > maxHeight) {
            maxHeight = gpuPercents.at(i);
        }
    }

    for (int i = 0; i < gpuPercents.size(); i++) {
        if (maxHeight < gpuRenderMaxHeight) {
            points.append(QPointF(i * 5, gpuPercents.at(i)));
        } else {
            points.append(QPointF(i * 5, gpuPercents.at(i) * gpuRenderMaxHeight / maxHeight));
        }
    }

    gpuPath = SmoothCurveGenerator::generateSmoothCurve(points);

    repaint();
}

void CompactGpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (!gpuAvailable) {
        setFontSize(painter, gpuTextRenderSize);
        painter.setPen(QPen(QColor(summaryColor)));
        painter.drawText(rect(), Qt::AlignCenter, tr("No GPU detected"));
        return;
    }

    // Draw GPU summary
    setFontSize(painter, gpuTextRenderSize);
    QFontMetrics fm = painter.fontMetrics();

    QString gpuTitle = QString("%1 %2%").arg(tr("GPU")).arg(QString::number(totalGpuPercent, 'f', 1));

    // Draw temperature if available
    QString tempStr;
    if (currentGpuInfo.temperatureC > 0) {
        tempStr = QString(" | %1°C").arg(currentGpuInfo.temperatureC);
        gpuTitle += tempStr;
    }

    // Draw VRAM if available
    if (currentGpuInfo.memoryTotalMB > 0) {
        gpuTitle += QString(" | %1/%2 MB").arg(currentGpuInfo.memoryUsedMB).arg(currentGpuInfo.memoryTotalMB);
    }

    painter.setOpacity(1);
    painter.setPen(QPen(QColor(gpuColor)));
    painter.setBrush(QBrush(QColor(gpuColor)));
    painter.drawEllipse(QPointF(rect().x() + pointerRenderPaddingX, rect().y() + gpuRenderPaddingY + pointerRenderPaddingY), pointerRadius, pointerRadius);

    setFontSize(painter, gpuTextRenderSize);
    painter.setPen(QPen(QColor(summaryColor)));
    painter.drawText(QRect(rect().x() + gpuRenderPaddingX,
                           rect().y() + gpuRenderPaddingY,
                           fm.width(gpuTitle),
                           rect().height()),
                     Qt::AlignLeft | Qt::AlignTop,
                     gpuTitle);

    // Draw background grid
    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen framePen;
    painter.setOpacity(0.1);
    framePen.setColor(QColor(textColor));
    framePen.setWidth(0.5);
    painter.setPen(framePen);

    int penSize = 1;
    int gridX = rect().x() + penSize;
    int gridY = rect().y() + gridRenderOffsetY + gridPaddingTop;
    int gridWidth = rect().width() - gridPaddingRight - penSize * 2;
    int gridHeight = gpuRenderMaxHeight + waveformRenderPadding;

    QPainterPath framePath;
    painter.setBrush(QBrush());
    framePath.addRect(QRect(gridX, gridY, gridWidth, gridHeight));
    painter.drawPath(framePath);

    // Draw grid
    QPen gridPen;
    QVector<qreal> dashes;
    qreal space = 3;
    dashes << 5 << space;
    painter.setOpacity(0.05);
    gridPen.setColor(QColor(textColor));
    gridPen.setWidth(0.5);
    gridPen.setDashPattern(dashes);
    painter.setPen(gridPen);

    int gridLineX = gridX;
    while (gridLineX < gridX + gridWidth - gridSize) {
        gridLineX += gridSize;
        painter.drawLine(gridLineX, gridY + 1, gridLineX, gridY + gridHeight - 1);
    }
    int gridLineY = gridY;
    while (gridLineY < gridY + gridHeight - gridSize) {
        gridLineY += gridSize;
        painter.drawLine(gridX + 1, gridLineY, gridX + gridWidth - 1, gridLineY);
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setOpacity(1);
    painter.translate((rect().width() - pointsNumber * 5) / 2 - 7, gpuWaveformsRenderOffsetY + gridPaddingTop);
    painter.scale(1, -1);

    qreal devicePixelRatio = qApp->devicePixelRatio();
    qreal curveWidth = 1.2;
    if (devicePixelRatio > 1) {
        curveWidth = 2;
    }

    painter.setPen(QPen(QColor(gpuColor), curveWidth));
    painter.setBrush(QBrush());
    painter.drawPath(gpuPath);
}

