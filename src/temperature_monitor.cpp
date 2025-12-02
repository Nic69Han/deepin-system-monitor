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

#include "temperature_monitor.h"
#include "dthememanager.h"
#include "utils.h"
#include "constant.h"
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

DWIDGET_USE_NAMESPACE

using namespace Utils;

TemperatureMonitor::TemperatureMonitor(QWidget *parent) : QWidget(parent)
{
    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    setFixedSize(statusBarMaxWidth, 90);

    initTheme();
    readTemperatures();

    connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &TemperatureMonitor::changeTheme);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &TemperatureMonitor::updateStatus);
    updateTimer->start(3000);
}

TemperatureMonitor::~TemperatureMonitor()
{
    delete updateTimer;
}

void TemperatureMonitor::initTheme()
{
    if (DThemeManager::instance()->theme() == "light") {
        textColor = "#303030";
        summaryColor = "#505050";
    } else {
        textColor = "#ffffff";
        summaryColor = "#909090";
    }
}

void TemperatureMonitor::changeTheme(QString )
{
    initTheme();
    repaint();
}

void TemperatureMonitor::updateStatus()
{
    readTemperatures();
    repaint();
}

void TemperatureMonitor::readTemperatures()
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

        // Look for temp*_input files
        QDir sensorDir(hwmonPath);
        for (const QString &file : sensorDir.entryList(QStringList() << "temp*_input")) {
            QString inputPath = hwmonPath + "/" + file;
            QString maxPath = hwmonPath + "/" + QString(file).replace("_input", "_max");
            QString critPath = hwmonPath + "/" + QString(file).replace("_input", "_crit");
            QString labelPath = hwmonPath + "/" + QString(file).replace("_input", "_label");
            
            double temp = 0, maxTemp = 100, critTemp = 100;
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
            
            QFile critFile(critPath);
            if (critFile.open(QIODevice::ReadOnly)) {
                critTemp = QString(critFile.readAll()).trimmed().toDouble() / 1000.0;
                critFile.close();
            }
            
            if (temp > 0 && temp < 150) {
                SensorInfo info;
                info.name = label;
                info.temperature = temp;
                info.maxTemp = maxTemp > 0 ? maxTemp : 100;
                info.criticalTemp = critTemp > 0 ? critTemp : 100;
                sensors.append(info);
            }
        }
    }
    
    // Limit to 4 sensors max
    while (sensors.size() > 4) {
        sensors.removeLast();
    }
    
    // Adjust widget height based on sensor count
    int newHeight = sensors.isEmpty() ? 25 : (sensorStartY + sensors.size() * sensorLineHeight + 5);
    setFixedHeight(newHeight);
}

QColor TemperatureMonitor::getTemperatureColor(double temp, double maxTemp)
{
    double ratio = temp / maxTemp;
    if (ratio > 0.9) return QColor("#FF4444");      // Critical - Red
    if (ratio > 0.75) return QColor("#FF8800");     // Warning - Orange
    if (ratio > 0.5) return QColor("#FFCC00");      // Warm - Yellow
    return QColor("#44BB44");                        // Normal - Green
}

void TemperatureMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (sensors.isEmpty()) {
        QFont font = painter.font();
        font.setPointSize(10);
        painter.setFont(font);
        QColor noSensorColor(textColor);
        noSensorColor.setAlphaF(0.5);
        painter.setPen(QPen(noSensorColor));
        painter.drawText(rect(), Qt::AlignCenter, tr("No temperature sensors"));
        return;
    }

    // Draw title
    QFont titleFont = painter.font();
    titleFont.setPointSize(11);
    titleFont.setWeight(QFont::Light);
    painter.setFont(titleFont);
    QColor titleColor(textColor);
    titleColor.setAlphaF(0.5);
    painter.setPen(QPen(titleColor));
    painter.drawText(QRect(paddingX, titleRenderOffsetY, width() - 2*paddingX, 20),
                     Qt::AlignLeft | Qt::AlignVCenter, tr("🌡️ Temperatures"));

    // Draw sensors
    QFont sensorFont = painter.font();
    sensorFont.setPointSize(9);
    painter.setFont(sensorFont);

    for (int i = 0; i < sensors.size(); i++) {
        const SensorInfo &sensor = sensors[i];
        int y = sensorStartY + i * sensorLineHeight;

        // Sensor name (truncate if too long)
        QString displayName = sensor.name;
        if (displayName.length() > 10) {
            displayName = displayName.left(8) + "..";
        }

        painter.setPen(QPen(QColor(summaryColor)));
        painter.drawText(QRect(paddingX, y, 60, sensorLineHeight),
                         Qt::AlignLeft | Qt::AlignVCenter, displayName);

        // Temperature bar background
        int barX = paddingX + 65;
        int barY = y + (sensorLineHeight - barHeight) / 2;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(128, 128, 128, 50));
        painter.drawRoundedRect(barX, barY, barWidth, barHeight, 3, 3);

        // Temperature bar fill
        double ratio = qMin(1.0, sensor.temperature / sensor.maxTemp);
        int fillWidth = static_cast<int>(barWidth * ratio);
        QColor barColor = getTemperatureColor(sensor.temperature, sensor.maxTemp);
        painter.setBrush(barColor);
        painter.drawRoundedRect(barX, barY, fillWidth, barHeight, 3, 3);

        // Temperature value
        painter.setPen(QPen(QColor(textColor)));
        painter.drawText(QRect(barX + barWidth + 5, y, 45, sensorLineHeight),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QString("%1°C").arg(QString::number(sensor.temperature, 'f', 0)));
    }
}

