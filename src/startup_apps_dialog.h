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

#ifndef STARTUPAPPSDIALOG_H
#define STARTUPAPPSDIALOG_H

#include "ddialog.h"
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>

DWIDGET_USE_NAMESPACE

struct StartupAppInfo {
    QString name;
    QString exec;
    QString comment;
    QString filePath;
    bool enabled;
    bool hidden;
};

class StartupAppsDialog : public DDialog
{
    Q_OBJECT

public:
    explicit StartupAppsDialog(QWidget *parent = nullptr);
    ~StartupAppsDialog();

private slots:
    void refreshApps();
    void filterApps(const QString &text);
    void onAppSelected();
    void toggleApp();
    void removeApp();
    void addApp();
    void onCellChanged(int row, int column);

private:
    void setupUI();
    void loadStartupApps();
    void updateButtons();
    void setAppEnabled(const QString &filePath, bool enabled);
    QStringList getAutostartDirs();

    QTableWidget *appTable;
    QLineEdit *searchEdit;
    QPushButton *toggleBtn;
    QPushButton *removeBtn;
    QPushButton *addBtn;
    QPushButton *refreshBtn;
    QLabel *statusLabel;
    
    QList<StartupAppInfo> allApps;
    QString currentFilter;
};

#endif

