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

#ifndef GPUMONITOR_H
#define GPUMONITOR_H

#include <QList>
#include <QPointF>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QString>

struct GpuInfo {
    QString name;           // GPU name (e.g., "NVIDIA GeForce GTX 1080")
    QString vendor;         // "NVIDIA", "AMD", "Intel"
    int usagePercent;       // GPU usage percentage (0-100)
    int memoryUsedMB;       // VRAM used in MB
    int memoryTotalMB;      // Total VRAM in MB
    int temperatureC;       // Temperature in Celsius (-1 if not available)
};

class GpuMonitor : public QWidget
{
    Q_OBJECT

public:
    GpuMonitor(QWidget *parent = 0);
    ~GpuMonitor();

    static bool hasGpu();
    static GpuInfo getGpuInfo();

public slots:
    void changeTheme(QString theme);
    void initTheme();
    void render();
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event);

private:
    // GPU detection methods
    static bool hasNvidiaGpu();
    static bool hasAmdGpu();
    static bool hasIntelGpu();
    static GpuInfo getNvidiaInfo();
    static GpuInfo getAmdInfo();
    static GpuInfo getIntelInfo();

    QPixmap iconImage;
    QPixmap iconLightImage;
    QPixmap iconDarkImage;
    QList<double> *gpuPercents;
    QPainterPath gpuPath;
    QString numberColor;
    QString ringBackgroundColor;
    QString ringForegroundColor;
    QString textColor;
    QTimer *timer;
    QTimer *updateTimer;
    
    GpuInfo currentGpuInfo;
    bool gpuAvailable;

    double animationFrames = 20;
    double ringBackgroundOpacity;
    double ringForegroundOpacity;
    int animationIndex = 0;
    int gpuRenderMaxHeight = 30;
    int iconPadding = 0;
    int iconRenderOffsetY = 110;
    int paddingRight = 10;
    int percentRenderOffsetY = 85;
    int pointsNumber = 24;
    int ringRadius = 55;
    int ringRenderOffsetY = 50;
    int ringWidth = 6;
    int titleAreaPaddingX = 5;
    int titleRenderOffsetY = 105;
    int waveformsRenderOffsetX;
    int waveformsRenderOffsetY = 55;
};

#endif

