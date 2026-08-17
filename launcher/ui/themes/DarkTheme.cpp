// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Sunveil Connect - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
 *  Copyright (C) 2024 TheKodeToad <TheKodeToad@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */
#include "DarkTheme.h"

#include <QObject>

QString DarkTheme::id()
{
    return "dark";
}

QString DarkTheme::name()
{
    return QObject::tr("Sunveil Dark");
}

QPalette DarkTheme::colorScheme()
{
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(8, 12, 15));           // #080C0F (Deep Slate Tone)
    darkPalette.setColor(QPalette::WindowText, QColor(241, 245, 249));   // #F1F5F9
    darkPalette.setColor(QPalette::Base, QColor(14, 22, 28));            // #0E161C
    darkPalette.setColor(QPalette::AlternateBase, QColor(17, 26, 34));   // #111A22
    darkPalette.setColor(QPalette::ToolTipBase, QColor(14, 22, 28));
    darkPalette.setColor(QPalette::ToolTipText, QColor(248, 250, 252));
    darkPalette.setColor(QPalette::Text, QColor(241, 245, 249));
    darkPalette.setColor(QPalette::Button, QColor(14, 22, 28));          // #0E161C
    darkPalette.setColor(QPalette::ButtonText, QColor(241, 245, 249));
    darkPalette.setColor(QPalette::BrightText, QColor(0, 229, 153));     // #00E599 (Emerald)
    darkPalette.setColor(QPalette::Link, QColor(0, 229, 153));
    darkPalette.setColor(QPalette::Highlight, QColor(0, 229, 153));
    darkPalette.setColor(QPalette::HighlightedText, QColor(4, 8, 10));   // #04080A
    darkPalette.setColor(QPalette::PlaceholderText, QColor(100, 116, 139)); // #64748B
    return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double DarkTheme::fadeAmount()
{
    return 0.5;
}

QColor DarkTheme::fadeColor()
{
    return QColor(8, 12, 15);
}

bool DarkTheme::hasStyleSheet()
{
    return true;
}

QString DarkTheme::appStyleSheet()
{
    return R"(
        QMainWindow, QWidget#centralWidget, QDialog {
            background-color: #080C0F;
            color: #F1F5F9;
            font-family: "Segoe UI", -apple-system, BlinkMacSystemFont, Roboto, sans-serif;
        }
        QToolTip {
            color: #F8FAFC;
            background-color: #0E161C;
            border: 1px solid #1F2E3B;
            border-radius: 8px;
            padding: 6px 10px;
            font-size: 12px;
        }
        QToolBar {
            background-color: #0C1217;
            border-bottom: 1px solid rgba(255, 255, 255, 0.08);
            padding: 8px 16px;
            spacing: 12px;
        }
        QToolButton {
            background: transparent;
            color: #8E9BAE;
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 8px 18px;
            font-weight: 600;
            font-size: 13px;
        }
        QToolButton:hover {
            background-color: #141E26;
            color: #00E599;
            border: 1px solid rgba(0, 229, 153, 0.25);
        }
        QToolButton:checked {
            background-color: rgba(0, 229, 153, 0.12);
            color: #00E599;
            border: 1px solid #00E599;
        }
        QMenuBar {
            background-color: #080C0F;
            color: #8E9BAE;
            border-bottom: 1px solid rgba(255, 255, 255, 0.08);
        }
        QMenuBar::item:selected {
            background-color: #111A22;
            color: #00E599;
        }
        QMenu {
            background-color: #0E161C;
            color: #F1F5F9;
            border: 1px solid #1F2E3B;
            border-radius: 10px;
            padding: 6px;
        }
        QMenu::item:selected {
            background-color: #141E26;
            color: #00E599;
            border-radius: 6px;
        }
        QMenu::separator {
            height: 1px;
            background: #1F2E3B;
            margin: 4px 8px;
        }
        QScrollBar:vertical {
            background: #080C0F;
            width: 10px;
            margin: 0px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical {
            background: #1F2E3B;
            min-height: 24px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover {
            background: #00E599;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background: #080C0F;
            height: 10px;
            margin: 0px;
            border-radius: 5px;
        }
        QScrollBar::handle:horizontal {
            background: #1F2E3B;
            min-width: 24px;
            border-radius: 5px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #00E599;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        QProgressBar {
            background-color: #0E161C;
            border: 1px solid #1F2E3B;
            border-radius: 8px;
            text-align: center;
            color: #F1F5F9;
            font-weight: bold;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00E599, stop:1 #00C985);
            border-radius: 7px;
        }
        QListView, QTreeView, QTableView {
            background-color: #080C0F;
            border: 1px solid #1F2E3B;
            border-radius: 12px;
            color: #F1F5F9;
            outline: none;
        }
        QListView::item:selected, QTreeView::item:selected {
            background-color: #111A22;
            color: #00E599;
            border-radius: 8px;
        }
        QListView::item:hover, QTreeView::item:hover {
            background-color: #0E161C;
            color: #00E599;
        }
        QStatusBar {
            background-color: #080C0F;
            border-top: 1px solid rgba(255, 255, 255, 0.05);
            color: #64748B;
            font-size: 12px;
        }

        /* SVLConnect Realms Page & Server Cards */
        QWidget#SVLConnectPage {
            background-color: #080C0F;
            color: #F1F5F9;
        }
        QFrame#filterBarFrame {
            background-color: #0C1217;
            border: 1px solid #1F2E3B;
            border-radius: 12px;
        }
        QLineEdit#realmSearchInput {
            background-color: #0E161C;
            border: 1px solid #223545;
            border-radius: 8px;
            padding: 8px 14px;
            color: #FFFFFF;
            font-size: 13px;
        }
        QLineEdit#realmSearchInput:focus {
            border: 1px solid #00E599;
            background-color: #121C24;
        }
        QPushButton#refreshButton {
            background-color: #0E161C;
            color: #CBD5E1;
            border: 1px solid #223545;
            border-radius: 8px;
            padding: 8px 18px;
            font-weight: 700;
            font-size: 13px;
        }
        QPushButton#refreshButton:hover {
            background-color: #141E26;
            border-color: #00E599;
            color: #00E599;
        }
        QPushButton#refreshButton:pressed {
            background-color: #0E161C;
        }
        QLabel#statusPillBadge {
            background-color: rgba(0, 229, 153, 0.12);
            color: #00E599;
            border: 1px solid rgba(0, 229, 153, 0.35);
            border-radius: 8px;
            padding: 4px 12px;
            font-size: 12px;
            font-weight: 600;
        }
        QFrame#serverCard {
            background-color: #111A22;
            border: 1px solid #1F2E3B;
            border-radius: 10px;
        }
        QFrame#serverCard:hover {
            border: 1px solid #00E599;
        }
        QLabel#serverNameLabel {
            font-size: 16px;
            font-weight: 700;
            color: #FFFFFF;
            background: transparent;
            border: none;
        }
        QLabel#serverMotdLabel {
            font-size: 13px;
            color: #94A3B8;
            background: transparent;
            border: none;
        }
        QLabel#playerCountBadge {
            font-size: 12px;
            font-weight: 600;
            color: #00E599;
            background: rgba(0, 229, 153, 0.12);
            border: 1px solid rgba(0, 229, 153, 0.35);
            border-radius: 6px;
            padding: 4px 10px;
        }
        QLabel#badgeVerified {
            background-color: rgba(0, 229, 153, 0.12);
            color: #00E599;
            border: 1px solid rgba(0, 229, 153, 0.40);
            border-radius: 6px;
            padding: 4px 10px;
            font-weight: 600;
            font-size: 12px;
        }
        QLabel#badgeCommunity {
            background-color: rgba(245, 158, 11, 0.12);
            color: #F59E0B;
            border: 1px solid rgba(245, 158, 11, 0.40);
            border-radius: 6px;
            padding: 4px 10px;
            font-weight: 600;
            font-size: 12px;
        }
        QLabel.metaPill {
            background-color: #182430;
            color: #CBD5E1;
            border: 1px solid #273A4D;
            border-radius: 6px;
            padding: 4px 10px;
            font-size: 12px;
        }
        QPushButton#cardDetailsBtn {
            background-color: #0E161C;
            color: #CBD5E1;
            border: 1px solid #223545;
            border-radius: 6px;
            padding: 6px 14px;
            font-weight: 600;
            font-size: 13px;
        }
        QPushButton#cardDetailsBtn:hover {
            background-color: #141E26;
            border-color: #00E599;
            color: #00E599;
        }
        QPushButton#joinServerButton {
            background-color: #00E599;
            color: #04080A;
            font-size: 13px;
            font-weight: 700;
            border: none;
            border-radius: 6px;
            padding: 6px 18px;
        }
        QPushButton#joinServerButton:hover {
            background-color: #10FFAC;
        }
        QPushButton#joinServerButton:pressed {
            background-color: #00B377;
        }

        /* Realm Detail Page */
        QWidget#SVLRealmDetailPage {
            background-color: #080C0F;
            color: #F1F5F9;
        }
        QPushButton#detailBackButton {
            background-color: #0E161C;
            color: #94A3B8;
            border: 1px solid #223545;
            border-radius: 8px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 700;
        }
        QPushButton#detailBackButton:hover {
            background-color: #141E26;
            border-color: #00E599;
            color: #00E599;
        }
        QFrame#realmBannerFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(0, 229, 153, 0.25), stop:0.6 #0C1217, stop:1 #080C0F);
            border: 1px solid #1F2E3B;
            border-radius: 14px;
        }
        QFrame#leftPanelFrame {
            background-color: #0C1217;
            border: 1px solid #1F2E3B;
            border-radius: 14px;
        }
        QFrame#rightCardFrame {
            background-color: #111A22;
            border: 1px solid #1F2E3B;
            border-radius: 14px;
        }
        QPushButton#detailConnectBtn {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00E599, stop:1 #00C985);
            color: #04080A;
            font-size: 15px;
            font-weight: 800;
            letter-spacing: 0.5px;
            border: none;
            border-radius: 10px;
            padding: 12px 28px;
        }
        QPushButton#detailConnectBtn:hover {
            background: #10FFAC;
            color: #04080A;
        }
        QPushButton#detailConnectBtn:pressed {
            background: #00B377;
            color: #020405;
        }
    )";
}

QString DarkTheme::tooltip()
{
    return "";
}

