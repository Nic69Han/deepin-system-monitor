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

#include "startup_apps_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QPainter>
#include <dthememanager.h>

StartupAppsDialog::StartupAppsDialog(QWidget *parent)
    : DAbstractDialog(parent), isDarkTheme(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(tr("Startup Applications"));
    setFixedSize(650, 450);

    // Connect to theme changes
    connect(Dtk::Widget::DThemeManager::instance(), &Dtk::Widget::DThemeManager::themeChanged,
            this, &StartupAppsDialog::updateTheme);
    isDarkTheme = (Dtk::Widget::DThemeManager::instance()->theme() == "dark");

    setupUI();
    loadStartupApps();
    applyThemeStyle();
}

StartupAppsDialog::~StartupAppsDialog()
{
}

void StartupAppsDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(QRectF(rect()), 8, 8);
    painter.setOpacity(1);
    painter.fillPath(path, isDarkTheme ? QColor("#252525") : QColor("#F8F8F8"));
}

void StartupAppsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    // Title bar
    QWidget *titleBar = new QWidget();
    titleBar->setFixedHeight(40);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 5, 0);

    titleLabel = new QLabel(tr("Startup Applications"));
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");

    closeButton = new DWindowCloseButton();
    closeButton->setFixedSize(27, 23);
    connect(closeButton, &DWindowCloseButton::clicked, this, &DAbstractDialog::close);

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeButton);
    mainLayout->addWidget(titleBar);

    // Content area
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(15, 10, 15, 15);
    contentLayout->setSpacing(10);

    // Top bar: search
    QHBoxLayout *topLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(tr("Search applications..."));
    searchEdit->setFixedWidth(200);
    searchEdit->setFixedHeight(30);
    connect(searchEdit, &QLineEdit::textChanged, this, &StartupAppsDialog::filterApps);

    refreshBtn = new QPushButton(tr("Refresh"));
    refreshBtn->setFixedHeight(30);
    connect(refreshBtn, &QPushButton::clicked, this, &StartupAppsDialog::refreshApps);

    addBtn = new QPushButton(tr("Add..."));
    addBtn->setFixedHeight(30);
    connect(addBtn, &QPushButton::clicked, this, &StartupAppsDialog::addApp);

    topLayout->addWidget(searchEdit);
    topLayout->addStretch();
    topLayout->addWidget(addBtn);
    topLayout->addWidget(refreshBtn);
    contentLayout->addLayout(topLayout);

    // App table
    appTable = new QTableWidget();
    appTable->setColumnCount(4);
    appTable->setHorizontalHeaderLabels({tr("Enabled"), tr("Name"), tr("Command"), tr("Comment")});
    appTable->horizontalHeader()->setStretchLastSection(true);
    appTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    appTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    appTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    appTable->setColumnWidth(0, 60);
    appTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    appTable->setSelectionMode(QAbstractItemView::SingleSelection);
    appTable->verticalHeader()->setVisible(false);
    appTable->setAlternatingRowColors(true);
    connect(appTable, &QTableWidget::itemSelectionChanged, this, &StartupAppsDialog::onAppSelected);
    connect(appTable, &QTableWidget::cellChanged, this, &StartupAppsDialog::onCellChanged);
    contentLayout->addWidget(appTable);

    // Action buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    toggleBtn = new QPushButton(tr("Enable/Disable"));
    removeBtn = new QPushButton(tr("Remove"));

    toggleBtn->setFixedHeight(30);
    removeBtn->setFixedHeight(30);
    toggleBtn->setEnabled(false);
    removeBtn->setEnabled(false);

    connect(toggleBtn, &QPushButton::clicked, this, &StartupAppsDialog::toggleApp);
    connect(removeBtn, &QPushButton::clicked, this, &StartupAppsDialog::removeApp);

    btnLayout->addWidget(toggleBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addStretch();
    contentLayout->addLayout(btnLayout);

    // Status label
    statusLabel = new QLabel();
    contentLayout->addWidget(statusLabel);

    mainLayout->addWidget(contentWidget);
}

QStringList StartupAppsDialog::getAutostartDirs()
{
    QStringList dirs;
    // User autostart directory
    QString userDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    dirs << userDir;
    // System autostart directories
    dirs << "/etc/xdg/autostart";
    return dirs;
}

void StartupAppsDialog::loadStartupApps()
{
    allApps.clear();
    QStringList dirs = getAutostartDirs();
    
    for (const QString &dirPath : dirs) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;
        
        QStringList desktopFiles = dir.entryList(QStringList() << "*.desktop", QDir::Files);
        for (const QString &fileName : desktopFiles) {
            QString filePath = dir.absoluteFilePath(fileName);
            QSettings desktop(filePath, QSettings::IniFormat);
            desktop.beginGroup("Desktop Entry");
            
            StartupAppInfo info;
            info.filePath = filePath;
            info.name = desktop.value("Name").toString();
            info.exec = desktop.value("Exec").toString();
            info.comment = desktop.value("Comment").toString();
            info.hidden = desktop.value("Hidden", false).toBool();
            info.enabled = !desktop.value("X-GNOME-Autostart-enabled", true).toBool() ? false : !info.hidden;
            
            // Skip if already added (user overrides system)
            bool exists = false;
            for (const StartupAppInfo &existing : allApps) {
                if (existing.name == info.name) { exists = true; break; }
            }
            if (!exists && !info.name.isEmpty()) {
                allApps.append(info);
            }
        }
    }
    
    filterApps(searchEdit->text());
    statusLabel->setText(tr("Found %1 startup applications").arg(allApps.size()));
}

void StartupAppsDialog::filterApps(const QString &text)
{
    currentFilter = text.toLower();
    appTable->blockSignals(true);
    appTable->setRowCount(0);

    for (int i = 0; i < allApps.size(); ++i) {
        const StartupAppInfo &app = allApps[i];
        bool matches = currentFilter.isEmpty() ||
                      app.name.toLower().contains(currentFilter) ||
                      app.exec.toLower().contains(currentFilter);

        if (matches) {
            int row = appTable->rowCount();
            appTable->insertRow(row);

            // Checkbox for enabled
            QTableWidgetItem *checkItem = new QTableWidgetItem();
            checkItem->setCheckState(app.enabled ? Qt::Checked : Qt::Unchecked);
            checkItem->setData(Qt::UserRole, i);  // Store index
            appTable->setItem(row, 0, checkItem);

            QTableWidgetItem *nameItem = new QTableWidgetItem(app.name);
            nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
            appTable->setItem(row, 1, nameItem);

            QTableWidgetItem *execItem = new QTableWidgetItem(app.exec);
            execItem->setFlags(execItem->flags() & ~Qt::ItemIsEditable);
            appTable->setItem(row, 2, execItem);

            QTableWidgetItem *commentItem = new QTableWidgetItem(app.comment);
            commentItem->setFlags(commentItem->flags() & ~Qt::ItemIsEditable);
            appTable->setItem(row, 3, commentItem);

            // Color code
            QColor textColor = app.enabled ? QColor(76, 175, 80) : QColor(158, 158, 158);
            nameItem->setForeground(textColor);
        }
    }
    appTable->blockSignals(false);
}

void StartupAppsDialog::refreshApps()
{
    loadStartupApps();
}

void StartupAppsDialog::onAppSelected()
{
    updateButtons();
}

void StartupAppsDialog::updateButtons()
{
    bool hasSelection = appTable->currentRow() >= 0;
    toggleBtn->setEnabled(hasSelection);
    removeBtn->setEnabled(hasSelection);
}

void StartupAppsDialog::onCellChanged(int row, int column)
{
    if (column != 0) return;

    QTableWidgetItem *item = appTable->item(row, 0);
    if (!item) return;

    int appIndex = item->data(Qt::UserRole).toInt();
    if (appIndex < 0 || appIndex >= allApps.size()) return;

    bool enabled = item->checkState() == Qt::Checked;
    setAppEnabled(allApps[appIndex].filePath, enabled);
    allApps[appIndex].enabled = enabled;

    // Update color
    QTableWidgetItem *nameItem = appTable->item(row, 1);
    if (nameItem) {
        QColor textColor = enabled ? QColor(76, 175, 80) : QColor(158, 158, 158);
        nameItem->setForeground(textColor);
    }
}

void StartupAppsDialog::setAppEnabled(const QString &filePath, bool enabled)
{
    QFile file(filePath);
    if (!file.exists()) return;

    // Read file content
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString content = file.readAll();
    file.close();

    // Modify X-GNOME-Autostart-enabled
    QStringList lines = content.split('\n');
    bool found = false;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].startsWith("X-GNOME-Autostart-enabled=")) {
            lines[i] = QString("X-GNOME-Autostart-enabled=%1").arg(enabled ? "true" : "false");
            found = true;
            break;
        }
    }
    if (!found) {
        // Add after [Desktop Entry]
        for (int i = 0; i < lines.size(); ++i) {
            if (lines[i].trimmed() == "[Desktop Entry]") {
                lines.insert(i + 1, QString("X-GNOME-Autostart-enabled=%1").arg(enabled ? "true" : "false"));
                break;
            }
        }
    }

    // Write back
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        statusLabel->setText(tr("Failed to modify: %1").arg(filePath));
        return;
    }
    QTextStream out(&file);
    out << lines.join('\n');
    file.close();

    statusLabel->setText(tr("%1 %2").arg(enabled ? tr("Enabled") : tr("Disabled")).arg(QFileInfo(filePath).baseName()));
}

void StartupAppsDialog::toggleApp()
{
    int row = appTable->currentRow();
    if (row < 0) return;

    QTableWidgetItem *item = appTable->item(row, 0);
    if (!item) return;

    bool newState = item->checkState() != Qt::Checked;
    item->setCheckState(newState ? Qt::Checked : Qt::Unchecked);
}

void StartupAppsDialog::removeApp()
{
    int row = appTable->currentRow();
    if (row < 0) return;

    QTableWidgetItem *item = appTable->item(row, 0);
    if (!item) return;

    int appIndex = item->data(Qt::UserRole).toInt();
    if (appIndex < 0 || appIndex >= allApps.size()) return;

    QString filePath = allApps[appIndex].filePath;

    if (QFile::remove(filePath)) {
        statusLabel->setText(tr("Removed: %1").arg(QFileInfo(filePath).baseName()));
        refreshApps();
    } else {
        statusLabel->setText(tr("Failed to remove: %1").arg(filePath));
    }
}

void StartupAppsDialog::addApp()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select Desktop File"),
                                                     "/usr/share/applications",
                                                     tr("Desktop Files (*.desktop)"));
    if (filePath.isEmpty()) return;

    QString userAutostart = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir().mkpath(userAutostart);

    QString destPath = userAutostart + "/" + QFileInfo(filePath).fileName();
    if (QFile::copy(filePath, destPath)) {
        statusLabel->setText(tr("Added: %1").arg(QFileInfo(filePath).baseName()));
        refreshApps();
    } else {
        statusLabel->setText(tr("Failed to add: %1").arg(filePath));
    }
}

void StartupAppsDialog::updateTheme(const QString &theme)
{
    isDarkTheme = (theme == "dark");
    applyThemeStyle();
    update();
}

void StartupAppsDialog::applyThemeStyle()
{
    QString textColor = isDarkTheme ? "#FFFFFF" : "#303030";
    QString bgColor = isDarkTheme ? "#2D2D2D" : "#FFFFFF";
    QString borderColor = isDarkTheme ? "#444444" : "#CCCCCC";
    QString headerBg = isDarkTheme ? "#3A3A3A" : "#E8E8E8";
    QString altRowColor = isDarkTheme ? "#333333" : "#F5F5F5";
    QString inputBg = isDarkTheme ? "#3A3A3A" : "#FFFFFF";

    // Title label
    titleLabel->setStyleSheet(QString("QLabel { color: %1; background: transparent; font-size: 14px; font-weight: bold; }").arg(textColor));

    // Close button theme
    Dtk::Widget::DThemeManager::instance()->setTheme(closeButton, isDarkTheme ? "dark" : "light");

    QString style = QString(
        "QTableWidget { background-color: %1; color: %2; gridline-color: %3; border: 1px solid %3; alternate-background-color: %5; } "
        "QTableWidget::item { padding: 5px; color: %2; } "
        "QTableWidget::item:selected { background-color: #2ca7f8; color: white; } "
        "QHeaderView::section { background-color: %4; color: %2; border: 1px solid %3; padding: 5px; font-weight: bold; } "
        "QLabel { color: %2; background-color: transparent; } "
        "QLineEdit { background-color: %6; color: %2; border: 1px solid %3; padding: 5px; border-radius: 4px; } "
        "QPushButton { background-color: %4; color: %2; border: 1px solid %3; padding: 5px 15px; border-radius: 4px; } "
        "QPushButton:hover { background-color: #2ca7f8; color: white; border-color: #2ca7f8; } "
        "QPushButton:pressed { background-color: #1a8ddb; } "
        "QPushButton:disabled { background-color: %4; color: #888888; border-color: %3; } "
        "QCheckBox { color: %2; background-color: transparent; } "
        "QCheckBox::indicator { background-color: %6; border: 1px solid %3; border-radius: 2px; }"
    ).arg(bgColor).arg(textColor).arg(borderColor).arg(headerBg).arg(altRowColor).arg(inputBg);

    setStyleSheet(style);
}
