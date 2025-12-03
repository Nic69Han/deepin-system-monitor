/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 * Copyright (C) 2024 Nic69Han <65731188+Nic69Han@users.noreply.github.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#ifndef DISKUSAGEDIALOG_H
#define DISKUSAGEDIALOG_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QThread>
#include <QMutex>
#include <ddialog.h>
#include <dwindowclosebutton.h>

DWIDGET_USE_NAMESPACE

struct DiskItemInfo {
    QString path;
    QString name;
    qint64 size;
    bool isDir;
    double percentage;
};

class DiskScanWorker : public QThread
{
    Q_OBJECT
    
public:
    DiskScanWorker(const QString &path, QObject *parent = nullptr);
    void stop();
    
signals:
    void itemFound(const DiskItemInfo &info);
    void scanProgress(const QString &currentPath);
    void scanComplete(qint64 totalSize);
    
protected:
    void run() override;
    
private:
    qint64 calculateDirSize(const QString &path, int depth = 0);
    QString scanPath;
    bool stopRequested;
    QMutex mutex;
};

class DiskUsageDialog : public Dtk::Widget::DAbstractDialog
{
    Q_OBJECT

public:
    DiskUsageDialog(QWidget *parent = nullptr, const QString &path = "/home");
    ~DiskUsageDialog();

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void onItemFound(const DiskItemInfo &info);
    void onScanProgress(const QString &currentPath);
    void onScanComplete(qint64 totalSize);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onBackClicked();
    void startScan(const QString &path);
    void updateTheme(const QString &theme);

private:
    void setupUI();
    void applyThemeStyle();
    QString formatSize(qint64 bytes);
    QColor getSizeColor(double percentage);

    DWindowCloseButton *closeButton;
    QVBoxLayout *mainLayout;
    QHBoxLayout *navLayout;
    QPushButton *backButton;
    QLabel *pathLabel;
    QLabel *titleLabel;
    QLabel *statusLabel;
    QTreeWidget *treeWidget;
    QProgressBar *scanProgress;

    DiskScanWorker *scanWorker;
    QString currentPath;
    QStringList pathHistory;
    qint64 currentTotalSize;
    bool isDarkTheme;
};

#endif // DISKUSAGEDIALOG_H

