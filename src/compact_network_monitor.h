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

#ifndef COMPACTNETWORKMONITOR_H
#define COMPACTNETWORKMONITOR_H

#include <QWidget>

class CompactNetworkMonitor : public QWidget
{
    Q_OBJECT
    
public:
    CompactNetworkMonitor(QWidget *parent = 0);
    ~CompactNetworkMonitor();
    
public slots:
    void changeTheme(QString theme);
    void initTheme();
    void updateStatus(long totalRecvBytes, long totalSentBytes, float totalRecvKbs, float totalSentKbs);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QList<double> *downloadSpeeds;
    QList<double> *uploadSpeeds;
    QPainterPath downloadPath;
    QPainterPath uploadPath;
    QString downloadColor = "#55D500";
    QString summaryColor;
    QString textColor;
    QString uploadColor = "#C362FF";
    float totalRecvKbs = 0;
    float totalSentKbs = 0;
    int downloadRenderMaxHeight = 35;
    int downloadRenderPaddingX = 13;
    int downloadRenderPaddingY = 12;
    int downloadRenderSize = 8;
    int downloadWaveformsRenderOffsetX = 4;
    int downloadWaveformsRenderOffsetY = 72;
    int gridPaddingRight = 21;
    int gridPaddingTop = 8;
    int gridRenderOffsetY = 35;
    int gridSize = 15;
    int pointerRadius = 2;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 6;
    int pointsNumber = 51;
    int textPadding = 10;
    int uploadRenderMaxHeight = 8;
    int uploadRenderPaddingX = 13;
    int uploadRenderPaddingY = 28;
    int uploadRenderSize = 8;
    int uploadWaveformsRenderOffsetY = -3;
    int waveformRenderPadding = 15;
    long totalRecvBytes = 0;
    long totalSentBytes = 0;
};

#endif    
