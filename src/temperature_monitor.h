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

#ifndef TEMPERATUREMONITOR_H
#define TEMPERATUREMONITOR_H

#include <QWidget>
#include <QTimer>
#include <QMap>

struct SensorInfo {
    QString name;
    double temperature;
    double maxTemp;
    double criticalTemp;
};

class TemperatureMonitor : public QWidget
{
    Q_OBJECT

public:
    TemperatureMonitor(QWidget *parent = 0);
    ~TemperatureMonitor();

public slots:
    void changeTheme(QString theme);
    void initTheme();
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event);

private:
    void readTemperatures();
    QColor getTemperatureColor(double temp, double maxTemp);

    QTimer *updateTimer;
    QList<SensorInfo> sensors;
    QString textColor;
    QString summaryColor;
    
    int titleRenderOffsetY = 5;
    int sensorStartY = 28;
    int sensorLineHeight = 18;
    int barHeight = 8;
    int barWidth = 80;
    int paddingX = 10;
};

#endif

