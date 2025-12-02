/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 */

#include "disk_usage_dialog.h"
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QHeaderView>
#include <QApplication>
#include <dthememanager.h>

DWIDGET_USE_NAMESPACE

// ============ DiskScanWorker Implementation ============

DiskScanWorker::DiskScanWorker(const QString &path, QObject *parent)
    : QThread(parent), scanPath(path), stopRequested(false)
{
}

void DiskScanWorker::stop()
{
    QMutexLocker locker(&mutex);
    stopRequested = true;
}

void DiskScanWorker::run()
{
    QDir dir(scanPath);
    if (!dir.exists()) return;

    qint64 totalSize = 0;
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    QList<DiskItemInfo> items;

    for (const QFileInfo &entry : entries) {
        {
            QMutexLocker locker(&mutex);
            if (stopRequested) return;
        }

        emit scanProgress(entry.filePath());

        DiskItemInfo info;
        info.path = entry.filePath();
        info.name = entry.fileName();
        info.isDir = entry.isDir();

        if (entry.isDir() && !entry.isSymLink()) {
            info.size = calculateDirSize(entry.filePath(), 0);
        } else {
            info.size = entry.size();
        }

        totalSize += info.size;
        items.append(info);
    }

    // Calculate percentages and emit items sorted by size
    std::sort(items.begin(), items.end(), [](const DiskItemInfo &a, const DiskItemInfo &b) {
        return a.size > b.size;
    });

    for (DiskItemInfo &item : items) {
        item.percentage = totalSize > 0 ? (item.size * 100.0 / totalSize) : 0;
        emit itemFound(item);
    }

    emit scanComplete(totalSize);
}

qint64 DiskScanWorker::calculateDirSize(const QString &path, int depth)
{
    if (depth > 20) return 0;  // Prevent infinite recursion

    {
        QMutexLocker locker(&mutex);
        if (stopRequested) return 0;
    }

    qint64 size = 0;
    QDir dir(path);

    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    for (const QFileInfo &entry : entries) {
        if (entry.isDir() && !entry.isSymLink()) {
            size += calculateDirSize(entry.filePath(), depth + 1);
        } else {
            size += entry.size();
        }
    }

    return size;
}

// ============ DiskUsageDialog Implementation ============

DiskUsageDialog::DiskUsageDialog(QWidget *parent, const QString &path)
    : DAbstractDialog(parent), currentPath(path), scanWorker(nullptr), currentTotalSize(0)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setupUI();
    startScan(path);
}

DiskUsageDialog::~DiskUsageDialog()
{
    if (scanWorker) {
        scanWorker->stop();
        scanWorker->wait(1000);
        delete scanWorker;
    }
}

void DiskUsageDialog::setupUI()
{
    setMinimumSize(600, 500);
    setWindowTitle(tr("Disk Usage Analyzer"));

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Close button
    closeButton = new DWindowCloseButton;
    closeButton->setFixedSize(27, 23);
    connect(closeButton, &DWindowCloseButton::clicked, this, &DAbstractDialog::close);
    Dtk::Widget::DThemeManager::instance()->setTheme(closeButton, "light");

    // Navigation
    navLayout = new QHBoxLayout();
    backButton = new QPushButton(tr("← Back"));
    backButton->setEnabled(false);
    connect(backButton, &QPushButton::clicked, this, &DiskUsageDialog::onBackClicked);

    pathLabel = new QLabel(currentPath);
    pathLabel->setStyleSheet("font-weight: bold; font-size: 14px;");

    navLayout->addWidget(backButton);
    navLayout->addWidget(pathLabel, 1);

    // Status label
    statusLabel = new QLabel(tr("Scanning..."));
    statusLabel->setStyleSheet("color: #666;");

    // Progress bar
    scanProgress = new QProgressBar();
    scanProgress->setRange(0, 0);  // Indeterminate
    scanProgress->setFixedHeight(4);
    scanProgress->setTextVisible(false);

    // Tree widget
    treeWidget = new QTreeWidget();
    treeWidget->setHeaderLabels({tr("Name"), tr("Size"), tr("Percentage"), tr("Bar")});
    treeWidget->setColumnWidth(0, 250);
    treeWidget->setColumnWidth(1, 100);
    treeWidget->setColumnWidth(2, 80);
    treeWidget->setColumnWidth(3, 150);
    treeWidget->setAlternatingRowColors(true);
    treeWidget->setSortingEnabled(true);
    treeWidget->header()->setStretchLastSection(true);
    connect(treeWidget, &QTreeWidget::itemDoubleClicked, this, &DiskUsageDialog::onItemDoubleClicked);

    // Layout assembly
    mainLayout->addWidget(closeButton, 0, Qt::AlignRight);
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(scanProgress);
    mainLayout->addWidget(treeWidget);
    mainLayout->addWidget(statusLabel);
}

void DiskUsageDialog::startScan(const QString &path)
{
    if (scanWorker) {
        scanWorker->stop();
        scanWorker->wait(1000);
        delete scanWorker;
    }

    treeWidget->clear();
    currentPath = path;
    pathLabel->setText(path);
    scanProgress->setVisible(true);
    statusLabel->setText(tr("Scanning..."));

    scanWorker = new DiskScanWorker(path, this);
    connect(scanWorker, &DiskScanWorker::itemFound, this, &DiskUsageDialog::onItemFound);
    connect(scanWorker, &DiskScanWorker::scanProgress, this, &DiskUsageDialog::onScanProgress);
    connect(scanWorker, &DiskScanWorker::scanComplete, this, &DiskUsageDialog::onScanComplete);
    scanWorker->start();
}

void DiskUsageDialog::onItemFound(const DiskItemInfo &info)
{
    QTreeWidgetItem *item = new QTreeWidgetItem();

    QString icon = info.isDir ? "📁" : "📄";
    item->setText(0, QString("%1 %2").arg(icon).arg(info.name));
    item->setText(1, formatSize(info.size));
    item->setText(2, QString("%1%").arg(info.percentage, 0, 'f', 1));

    // Store data for sorting and navigation
    item->setData(0, Qt::UserRole, info.path);
    item->setData(1, Qt::UserRole, info.size);
    item->setData(2, Qt::UserRole, info.percentage);
    item->setData(0, Qt::UserRole + 1, info.isDir);

    // Color code by percentage
    QColor color = getSizeColor(info.percentage);
    item->setForeground(2, color);

    treeWidget->addTopLevelItem(item);
}

void DiskUsageDialog::onScanProgress(const QString &currentPath)
{
    statusLabel->setText(tr("Scanning: %1").arg(currentPath));
}

void DiskUsageDialog::onScanComplete(qint64 totalSize)
{
    currentTotalSize = totalSize;
    scanProgress->setVisible(false);
    statusLabel->setText(tr("Total: %1 (%2 items)")
        .arg(formatSize(totalSize))
        .arg(treeWidget->topLevelItemCount()));
}

void DiskUsageDialog::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    bool isDir = item->data(0, Qt::UserRole + 1).toBool();
    if (!isDir) return;

    QString path = item->data(0, Qt::UserRole).toString();
    pathHistory.append(currentPath);
    backButton->setEnabled(true);
    startScan(path);
}

void DiskUsageDialog::onBackClicked()
{
    if (pathHistory.isEmpty()) return;

    QString prevPath = pathHistory.takeLast();
    backButton->setEnabled(!pathHistory.isEmpty());
    startScan(prevPath);
}

QString DiskUsageDialog::formatSize(qint64 bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    return QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unitIndex]);
}

QColor DiskUsageDialog::getSizeColor(double percentage)
{
    if (percentage >= 50) return QColor("#e74c3c");  // Red for large
    if (percentage >= 25) return QColor("#f39c12");  // Orange for medium
    if (percentage >= 10) return QColor("#3498db");  // Blue for small-medium
    return QColor("#27ae60");  // Green for small
}

void DiskUsageDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.setOpacity(1);
    painter.fillPath(path, QColor("#ffffff"));
}
