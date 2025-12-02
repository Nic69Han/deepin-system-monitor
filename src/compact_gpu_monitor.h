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

#ifndef COMPACTGPUMONITOR_H
#define COMPACTGPUMONITOR_H

#include <QWidget>
#include <QTimer>
#include "gpu_monitor.h"

class CompactGpuMonitor : public QWidget
{
    Q_OBJECT

public:
    CompactGpuMonitor(QWidget *parent = 0);
    ~CompactGpuMonitor();

public slots:
    void changeTheme(QString theme);
    void initTheme();
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event);

private:
    QList<double> gpuPercents;
    QPainterPath gpuPath;
    QString textColor;
    QString summaryColor;
    QString gpuColor = "#00CC00";  // Green for GPU

    int gpuRenderMaxHeight = 50;
    int gpuWaveformsRenderOffsetY = 72;
    int gridPaddingRight = 21;
    int gridPaddingTop = 8;
    int gridRenderOffsetY = 12;
    int gridSize = 15;
    int pointsNumber = 51;
    int waveformRenderPadding = 15;

    int gpuTextRenderSize = 8;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 6;
    int gpuRenderPaddingX = 13;
    int gpuRenderPaddingY = 2;
    int pointerRadius = 2;

    double totalGpuPercent = 0;
    bool gpuAvailable = false;
    GpuInfo currentGpuInfo;
    QTimer *updateTimer;
};

#endif

