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

#include "compact_temperature_monitor.h"
#include "dthememanager.h"
#include "utils.h"
#include "constant.h"
#include <QPainter>
#include <QDir>
#include <QFile>

DWIDGET_USE_NAMESPACE

using namespace Utils;

CompactTemperatureMonitor::CompactTemperatureMonitor(QWidget *parent) : QWidget(parent)
{
    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedSize(statusBarMaxWidth, 55);

    initTheme();
    readTemperatures();

    connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &CompactTemperatureMonitor::changeTheme);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &CompactTemperatureMonitor::updateStatus);
    updateTimer->start(3000);
}

CompactTemperatureMonitor::~CompactTemperatureMonitor()
{
    delete updateTimer;
}

void CompactTemperatureMonitor::initTheme()
{
    if (DThemeManager::instance()->theme() == "light") {
        textColor = "#303030";
        summaryColor = "#505050";
    } else {
        textColor = "#ffffff";
        summaryColor = "#909090";
    }
}

void CompactTemperatureMonitor::changeTheme(QString )
{
    initTheme();
    repaint();
}

void CompactTemperatureMonitor::updateStatus()
{
    readTemperatures();
    repaint();
}

void CompactTemperatureMonitor::readTemperatures()
{
    sensors.clear();
    QDir hwmonDir("/sys/class/hwmon");
    
    for (const QString &hwmon : hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString hwmonPath = "/sys/class/hwmon/" + hwmon;
        QString nameFile = hwmonPath + "/name";
        QString sensorName = "Unknown";
        
        QFile nf(nameFile);
        if (nf.open(QIODevice::ReadOnly)) {
            sensorName = QString(nf.readAll()).trimmed();
            nf.close();
        }

        QDir sensorDir(hwmonPath);
        for (const QString &file : sensorDir.entryList(QStringList() << "temp*_input")) {
            QString inputPath = hwmonPath + "/" + file;
            QString maxPath = hwmonPath + "/" + QString(file).replace("_input", "_max");
            QString labelPath = hwmonPath + "/" + QString(file).replace("_input", "_label");
            
            double temp = 0, maxTemp = 100;
            QString label = sensorName;
            
            QFile tempFile(inputPath);
            if (tempFile.open(QIODevice::ReadOnly)) {
                temp = QString(tempFile.readAll()).trimmed().toDouble() / 1000.0;
                tempFile.close();
            }
            
            QFile labelFile(labelPath);
            if (labelFile.open(QIODevice::ReadOnly)) {
                label = QString(labelFile.readAll()).trimmed();
                labelFile.close();
            }
            
            QFile maxFile(maxPath);
            if (maxFile.open(QIODevice::ReadOnly)) {
                maxTemp = QString(maxFile.readAll()).trimmed().toDouble() / 1000.0;
                maxFile.close();
            }
            
            if (temp > 0 && temp < 150) {
                CompactSensorInfo info;
                info.name = label;
                info.temperature = temp;
                info.maxTemp = maxTemp > 0 ? maxTemp : 100;
                sensors.append(info);
            }
        }
    }
    
    while (sensors.size() > 3) {
        sensors.removeLast();
    }
    
    int newHeight = sensors.isEmpty() ? 20 : (20 + sensors.size() * 12);
    setFixedHeight(newHeight);
}

QColor CompactTemperatureMonitor::getTemperatureColor(double temp, double maxTemp)
{
    double ratio = temp / maxTemp;
    if (ratio > 0.9) return QColor("#FF4444");
    if (ratio > 0.75) return QColor("#FF8800");
    if (ratio > 0.5) return QColor("#FFCC00");
    return QColor("#44BB44");
}

void CompactTemperatureMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    if (sensors.isEmpty()) {
        QColor noSensorColor(textColor);
        noSensorColor.setAlphaF(0.5);
        painter.setPen(QPen(noSensorColor));
        painter.drawText(rect(), Qt::AlignCenter, tr("No temp sensors"));
        return;
    }

    // Compact: show sensors horizontally or in small rows
    int x = 8;
    int y = 5;
    
    for (int i = 0; i < sensors.size(); i++) {
        const CompactSensorInfo &sensor = sensors[i];
        
        QString shortName = sensor.name.left(6);
        QColor tempColor = getTemperatureColor(sensor.temperature, sensor.maxTemp);
        
        painter.setPen(QPen(QColor(summaryColor)));
        painter.drawText(x, y + 10, shortName + ":");
        
        painter.setPen(QPen(tempColor));
        painter.drawText(x + 45, y + 10, QString("%1°").arg(QString::number(sensor.temperature, 'f', 0)));
        
        y += 12;
    }
}

