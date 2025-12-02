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

#ifndef COMPACTDISKMONITOR_H
#define COMPACTDISKMONITOR_H

#include <QWidget>

class CompactDiskMonitor : public QWidget
{
    Q_OBJECT
    
public:
    CompactDiskMonitor(QWidget *parent = 0);
    ~CompactDiskMonitor();
    
public slots:
    void changeTheme(QString theme);
    void initTheme();
    void updateStatus(unsigned long totalReadKbs, unsigned long totalWriteKbs);
    
protected:
    void paintEvent(QPaintEvent *event);
    
private:
    QList<unsigned long> *readSpeeds;
    QList<unsigned long> *writeSpeeds;
    QPainterPath readPath;
    QPainterPath writePath;
    QString readColor = "#1094D8";
    QString summaryColor;
    QString textColor;
    QString writeColor = "#F7B300";
    unsigned long totalReadKbs = 0;
    unsigned long totalWriteKbs = 0;
    int readRenderMaxHeight = 35;
    int readRenderPaddingX = 13;
    int readRenderPaddingY = 15;
    int readRenderSize = 8;
    int readWaveformsRenderOffsetX = 4;
    int readWaveformsRenderOffsetY = 72;
    int gridPaddingRight = 21;
    int gridPaddingTop = 8;
    int gridRenderOffsetY = 38;
    int gridSize = 15;
    int pointerRadius = 2;
    int pointerRenderPaddingX = 4;
    int pointerRenderPaddingY = 6;
    int pointsNumber = 51;
    int writeRenderMaxHeight = 8;
    int writeRenderPaddingX = 13;
    int writeRenderPaddingY = 30;
    int writeRenderSize = 8;
    int writeWaveformsRenderOffsetY = -3;
    int waveformRenderPadding = 15;
};

#endif    
