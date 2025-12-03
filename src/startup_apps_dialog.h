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

#ifndef STARTUPAPPSDIALOG_H
#define STARTUPAPPSDIALOG_H

#include <dabstractdialog.h>
#include <dwindowclosebutton.h>
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

class StartupAppsDialog : public DAbstractDialog
{
    Q_OBJECT

public:
    explicit StartupAppsDialog(QWidget *parent = nullptr);
    ~StartupAppsDialog();

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void refreshApps();
    void filterApps(const QString &text);
    void onAppSelected();
    void toggleApp();
    void removeApp();
    void addApp();
    void onCellChanged(int row, int column);
    void updateTheme(const QString &theme);

private:
    void setupUI();
    void loadStartupApps();
    void updateButtons();
    void setAppEnabled(const QString &filePath, bool enabled);
    QStringList getAutostartDirs();
    void applyThemeStyle();

    QTableWidget *appTable;
    QLineEdit *searchEdit;
    QPushButton *toggleBtn;
    QPushButton *removeBtn;
    QPushButton *addBtn;
    QPushButton *refreshBtn;
    QLabel *statusLabel;
    QLabel *titleLabel;
    DWindowCloseButton *closeButton;

    QList<StartupAppInfo> allApps;
    QString currentFilter;
    bool isDarkTheme;
};

#endif

