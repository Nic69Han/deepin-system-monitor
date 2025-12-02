/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#ifndef COMPACTMEMORYMONITOR_H
#define COMPACTMEMORYMONITOR_H

#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class CompactMemoryMonitor : public QWidget
{
    Q_OBJECT
    
public:
    CompactMemoryMonitor(QWidget *parent = 0);
    ~CompactMemoryMonitor();
    
public slots:
    void changeTheme(QString theme);
    void initTheme();
    void render();
    void updateStatus(long uMemory, long tMemory, long uSwap, long tSwap);
    
protected:
    QPointF getEndPointerCoordinate(double percent, int r);
    void paintEvent(QPaintEvent *event);
    
    QString memoryBackgroundColor;
    QString memoryColor = "#FF2997";
    QString memoryForegroundColor;
    QString numberColor;
    QString summaryColor;
    QString swapBackgroundColor;
    QString swapColor = "#00B4C7";
    QString swapForegroundColor;
    QString textColor;
    QTimer *timer;
    QVBoxLayout *layout;
    double animationFrames = 20;
    double memoryBackgroundOpacity;
    double memoryForegroundOpacity;
    double swapBackgroundOpacity;
    double swapForegroundOpacity;
    int animationIndex = 0;
    int insideRingRadius = 22;
    int lineHeight = 14;
    int memoryPercentRenderSize = 9;
    int memoryRenderPaddingX = 13;
    int memoryRenderPaddingY = 8;
    int memoryRenderSize = 8;
    int outsideRingRadius = 27;
    int pointerRadius = 2;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 6;
    int ringCenterPointerX;
    int ringCenterPointerY = 32;
    int ringWidth = 4;
    int swapRenderPaddingX = 13;
    int swapRenderPaddingY = 38;
    int swapRenderSize = 8;
    long prevUsedMemory;
    long prevUsedSwap;
    long totalMemory;
    long totalSwap;
    long usedMemory;
    long usedSwap;
};

#endif    
