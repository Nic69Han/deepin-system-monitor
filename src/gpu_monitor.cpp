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
#include "gpu_monitor.h"
#include "dthememanager.h"
#include "smooth_curve_generator.h"
#include "utils.h"
#include <QDebug>
#include <QPainter>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <DHiDPIHelper>

DWIDGET_USE_NAMESPACE

using namespace Utils;

GpuMonitor::GpuMonitor(QWidget *parent) : QWidget(parent)
{
    iconDarkImage = DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("icon_cpu_dark.svg"));
    iconLightImage = DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("icon_cpu_light.svg"));

    initTheme();
    connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &GpuMonitor::changeTheme);

    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();

    // Check if GPU is available - hide widget if not
    gpuAvailable = hasGpu();
    if (gpuAvailable) {
        setFixedSize(statusBarMaxWidth, 250);
    } else {
        // Minimal height when no GPU - just show a small indicator
        setFixedSize(statusBarMaxWidth, 40);
    }
    waveformsRenderOffsetX = (statusBarMaxWidth - 140) / 2;

    gpuPercents = new QList<double>();
    for (int i = 0; i < pointsNumber; i++) {
        gpuPercents->append(0);
    }

    // gpuAvailable already set above
    if (gpuAvailable) {
        currentGpuInfo = getGpuInfo();
    }

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(render()));
    timer->start(30);

    // Update GPU info every 2 seconds
    updateTimer = new QTimer();
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    updateTimer->start(2000);
}

GpuMonitor::~GpuMonitor()
{
    delete gpuPercents;
    delete timer;
    delete updateTimer;
}

void GpuMonitor::initTheme()
{
    if (DThemeManager::instance()->theme() == "light") {
        textColor = "#303030";
        numberColor = "#000000";
        ringForegroundColor = "#00AA00";  // Green for GPU
        ringForegroundOpacity = 1;
        ringBackgroundColor = "#000000";
        ringBackgroundOpacity = 0.05;
        iconImage = iconLightImage;
    } else {
        textColor = "#ffffff";
        numberColor = "#D4D4D4";
        ringForegroundColor = "#00CC00";  // Green for GPU
        ringForegroundOpacity = 1;
        ringBackgroundColor = "#00CC00";
        ringBackgroundOpacity = 0.1;
        iconImage = iconDarkImage;
    }
}

void GpuMonitor::changeTheme(QString)
{
    initTheme();
}

void GpuMonitor::render()
{
    if (animationIndex < animationFrames) {
        animationIndex++;
        repaint();
    } else {
        timer->stop();
    }
}

void GpuMonitor::updateStatus()
{
    if (!gpuAvailable) {
        gpuAvailable = hasGpu();
        if (!gpuAvailable) return;
    }

    currentGpuInfo = getGpuInfo();
    gpuPercents->append(currentGpuInfo.usagePercent);

    if (gpuPercents->size() > pointsNumber) {
        gpuPercents->pop_front();
    }

    QList<QPointF> points;
    double gpuMaxHeight = 0;
    for (int i = 0; i < gpuPercents->size(); i++) {
        if (gpuPercents->at(i) > gpuMaxHeight) {
            gpuMaxHeight = gpuPercents->at(i);
        }
    }

    for (int i = 0; i < gpuPercents->size(); i++) {
        if (gpuMaxHeight < gpuRenderMaxHeight) {
            points.append(QPointF(i * 5, gpuPercents->at(i)));
        } else {
            points.append(QPointF(i * 5, gpuPercents->at(i) * gpuRenderMaxHeight / gpuMaxHeight));
        }
    }

    gpuPath = SmoothCurveGenerator::generateSmoothCurve(points);

    if (gpuPercents->size() >= 2 && gpuPercents->last() != gpuPercents->at(gpuPercents->size() - 2)) {
        animationIndex = 0;
        timer->start(30);
    }
}

bool GpuMonitor::hasGpu()
{
    return hasNvidiaGpu() || hasAmdGpu() || hasIntelGpu();
}

bool GpuMonitor::hasNvidiaGpu()
{
    QProcess process;
    process.start("nvidia-smi", QStringList() << "-L");
    process.waitForFinished(1000);
    return process.exitCode() == 0 && !process.readAllStandardOutput().isEmpty();
}

bool GpuMonitor::hasAmdGpu()
{
    // Check for AMD GPU via /sys/class/drm
    QDir drmDir("/sys/class/drm");
    QStringList cards = drmDir.entryList(QStringList() << "card*", QDir::Dirs);
    for (const QString &card : cards) {
        QFile vendorFile(QString("/sys/class/drm/%1/device/vendor").arg(card));
        if (vendorFile.open(QIODevice::ReadOnly)) {
            QString vendor = vendorFile.readAll().trimmed();
            if (vendor == "0x1002") {  // AMD vendor ID
                return true;
            }
        }
    }
    return false;
}

bool GpuMonitor::hasIntelGpu()
{
    QDir drmDir("/sys/class/drm");
    QStringList cards = drmDir.entryList(QStringList() << "card*", QDir::Dirs);
    for (const QString &card : cards) {
        QFile vendorFile(QString("/sys/class/drm/%1/device/vendor").arg(card));
        if (vendorFile.open(QIODevice::ReadOnly)) {
            QString vendor = vendorFile.readAll().trimmed();
            if (vendor == "0x8086") {  // Intel vendor ID
                return true;
            }
        }
    }
    return false;
}


GpuInfo GpuMonitor::getGpuInfo()
{
    if (hasNvidiaGpu()) {
        return getNvidiaInfo();
    } else if (hasAmdGpu()) {
        return getAmdInfo();
    } else if (hasIntelGpu()) {
        return getIntelInfo();
    }

    GpuInfo info;
    info.name = "Unknown GPU";
    info.vendor = "Unknown";
    info.usagePercent = 0;
    info.memoryUsedMB = 0;
    info.memoryTotalMB = 0;
    info.temperatureC = -1;
    return info;
}

GpuInfo GpuMonitor::getNvidiaInfo()
{
    GpuInfo info;
    info.vendor = "NVIDIA";
    info.usagePercent = 0;
    info.memoryUsedMB = 0;
    info.memoryTotalMB = 0;
    info.temperatureC = -1;

    QProcess process;
    process.start("nvidia-smi", QStringList()
        << "--query-gpu=name,utilization.gpu,memory.used,memory.total,temperature.gpu"
        << "--format=csv,noheader,nounits");
    process.waitForFinished(2000);

    if (process.exitCode() == 0) {
        QString output = process.readAllStandardOutput().trimmed();
        QStringList parts = output.split(",");
        if (parts.size() >= 5) {
            info.name = parts[0].trimmed();
            info.usagePercent = parts[1].trimmed().toInt();
            info.memoryUsedMB = parts[2].trimmed().toInt();
            info.memoryTotalMB = parts[3].trimmed().toInt();
            info.temperatureC = parts[4].trimmed().toInt();
        }
    }
    return info;
}

GpuInfo GpuMonitor::getAmdInfo()
{
    GpuInfo info;
    info.vendor = "AMD";
    info.name = "AMD GPU";
    info.usagePercent = 0;
    info.memoryUsedMB = 0;
    info.memoryTotalMB = 0;
    info.temperatureC = -1;

    // Try to read from /sys/class/drm/card0/device/
    QFile gpuBusyFile("/sys/class/drm/card0/device/gpu_busy_percent");
    if (gpuBusyFile.open(QIODevice::ReadOnly)) {
        info.usagePercent = QString(gpuBusyFile.readAll().trimmed()).toInt();
    }

    // Read VRAM usage
    QFile vramUsedFile("/sys/class/drm/card0/device/mem_info_vram_used");
    if (vramUsedFile.open(QIODevice::ReadOnly)) {
        info.memoryUsedMB = QString(vramUsedFile.readAll().trimmed()).toLongLong() / (1024 * 1024);
    }

    QFile vramTotalFile("/sys/class/drm/card0/device/mem_info_vram_total");
    if (vramTotalFile.open(QIODevice::ReadOnly)) {
        info.memoryTotalMB = QString(vramTotalFile.readAll().trimmed()).toLongLong() / (1024 * 1024);
    }

    // Read temperature from hwmon
    QDir hwmonDir("/sys/class/drm/card0/device/hwmon");
    QStringList hwmons = hwmonDir.entryList(QStringList() << "hwmon*", QDir::Dirs);
    if (!hwmons.isEmpty()) {
        QFile tempFile(QString("/sys/class/drm/card0/device/hwmon/%1/temp1_input").arg(hwmons.first()));
        if (tempFile.open(QIODevice::ReadOnly)) {
            info.temperatureC = QString(tempFile.readAll().trimmed()).toInt() / 1000;
        }
    }

    return info;
}

GpuInfo GpuMonitor::getIntelInfo()
{
    GpuInfo info;
    info.vendor = "Intel";
    info.name = "Intel GPU";
    info.usagePercent = 0;
    info.memoryUsedMB = 0;
    info.memoryTotalMB = 0;
    info.temperatureC = -1;

    // Intel integrated GPUs share system memory, usage detection is limited
    // Try intel_gpu_top if available (requires root)
    QFile rcFile("/sys/class/drm/card0/device/rc6_residency_ms");
    if (rcFile.open(QIODevice::ReadOnly)) {
        // RC6 is power saving state, lower = more GPU usage
        // This is a rough approximation
    }

    return info;
}

void GpuMonitor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (!gpuAvailable) {
        // Draw "No GPU" message
        QFont font = painter.font();
        font.setPointSize(14);
        painter.setFont(font);
        painter.setPen(QPen(QColor(textColor)));
        painter.drawText(rect(), Qt::AlignCenter, tr("No GPU detected"));
        return;
    }

    QFont font = painter.font();
    font.setPointSize(20);
    font.setWeight(QFont::Light);

    QFontMetrics fm(font);
    int titleWidth = fm.width(tr("GPU"));

    int iconTitleWidth = iconImage.width() + iconPadding + titleWidth;

    painter.drawPixmap(QPoint((rect().x() + (rect().width() - iconTitleWidth) / 2) - titleAreaPaddingX - paddingRight, iconRenderOffsetY), iconImage);

    painter.setFont(font);
    painter.setPen(QPen(QColor(textColor)));
    painter.drawText(QRect((rect().x() + (rect().width() - iconTitleWidth) / 2) + iconImage.width() + iconPadding - titleAreaPaddingX - paddingRight,
                           rect().y() + titleRenderOffsetY,
                           titleWidth,
                           30
                         ), Qt::AlignCenter, tr("GPU"));

    double percent = 0;
    if (gpuPercents->size() >= 2) {
        percent = (gpuPercents->at(gpuPercents->size() - 2) + easeInOut(animationIndex / animationFrames) * (gpuPercents->last() - gpuPercents->at(gpuPercents->size() - 2)));
    }

    setFontSize(painter, 15);
    painter.setPen(QPen(QColor(numberColor)));
    painter.drawText(QRect(rect().x() - paddingRight,
                           rect().y() + percentRenderOffsetY,
                           rect().width(),
                           30
                         ), Qt::AlignCenter, QString("%1%").arg(QString::number(percent, 'f', 1)));

    drawLoadingRing(
        painter,
        rect().x() + rect().width() / 2 - paddingRight,
        rect().y() + ringRenderOffsetY,
        ringRadius,
        ringWidth,
        300,
        150,
        ringForegroundColor, ringForegroundOpacity,
        ringBackgroundColor, ringBackgroundOpacity,
        percent / 100
        );

    painter.translate(waveformsRenderOffsetX, waveformsRenderOffsetY);
    painter.scale(1, -1);

    painter.setPen(QPen(QColor("#00CC00"), 2));  // Green for GPU
    painter.drawPath(gpuPath);
}

