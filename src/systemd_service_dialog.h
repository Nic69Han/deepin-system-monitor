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

#ifndef SYSTEMDSERVICEDIALOG_H
#define SYSTEMDSERVICEDIALOG_H

#include <dabstractdialog.h>
#include <dwindowclosebutton.h>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>

DWIDGET_USE_NAMESPACE

struct ServiceInfo {
    QString name;
    QString loadState;
    QString activeState;
    QString subState;
    QString description;
};

class SystemdServiceDialog : public DAbstractDialog
{
    Q_OBJECT

public:
    explicit SystemdServiceDialog(QWidget *parent = nullptr);
    ~SystemdServiceDialog();

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void refreshServices();
    void filterServices(const QString &text);
    void onServiceSelected();
    void startService();
    void stopService();
    void restartService();
    void enableService();
    void disableService();
    void filterByType(int index);
    void updateTheme(const QString &theme);

private:
    void setupUI();
    void loadServices();
    void updateButtons();
    void executeServiceCommand(const QString &action);
    void applyThemeStyle();

    QTableWidget *serviceTable;
    QLineEdit *searchEdit;
    QComboBox *filterCombo;
    QPushButton *startBtn;
    QPushButton *stopBtn;
    QPushButton *restartBtn;
    QPushButton *enableBtn;
    QPushButton *disableBtn;
    QPushButton *refreshBtn;
    QLabel *statusLabel;
    QLabel *titleLabel;
    DWindowCloseButton *closeButton;

    QList<ServiceInfo> allServices;
    QString currentFilter;
    bool isDarkTheme;
};

#endif

