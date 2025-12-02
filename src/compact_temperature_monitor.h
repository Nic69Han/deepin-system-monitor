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

#ifndef COMPACTTEMPERATUREMONITOR_H
#define COMPACTTEMPERATUREMONITOR_H

#include <QWidget>
#include <QTimer>
#include <QMap>

struct CompactSensorInfo {
    QString name;
    double temperature;
    double maxTemp;
};

class CompactTemperatureMonitor : public QWidget
{
    Q_OBJECT

public:
    CompactTemperatureMonitor(QWidget *parent = 0);
    ~CompactTemperatureMonitor();

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
    QList<CompactSensorInfo> sensors;
    QString textColor;
    QString summaryColor;
};

#endif

