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
    darkPalette.setColor(QPalette::Window, QColor(17, 17, 17));          // #111111
    darkPalette.setColor(QPalette::WindowText, QColor(255, 255, 255));  // #FFFFFF
    darkPalette.setColor(QPalette::Base, QColor(28, 28, 30));           // #1C1C1E
    darkPalette.setColor(QPalette::AlternateBase, QColor(44, 44, 46));  // #2C2C2E
    darkPalette.setColor(QPalette::ToolTipBase, QColor(28, 28, 30));
    darkPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Text, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Button, QColor(44, 44, 46));         // #2C2C2E
    darkPalette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::BrightText, QColor(0, 229, 153));    // #00E599
    darkPalette.setColor(QPalette::Link, QColor(0, 229, 153));
    darkPalette.setColor(QPalette::Highlight, QColor(0, 229, 153));
    darkPalette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));    // #000000
    darkPalette.setColor(QPalette::PlaceholderText, QColor(161, 161, 170)); // #A1A1AA
    return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double DarkTheme::fadeAmount()
{
    return 0.5;
}

QColor DarkTheme::fadeColor()
{
    return QColor(17, 17, 17);
}

bool DarkTheme::hasStyleSheet()
{
    return true;
}

QString DarkTheme::appStyleSheet()
{
    return R"(
        /* ===================================================================
           SUNVEIL CONNECT WITH NEUTRAL CHARCOAL / MODRINTH DESIGN SYSTEM
           =================================================================== */

        /* Root Canvas */
        QMainWindow, QWidget#centralWidget, QDialog {
            background-color: #111111;
            color: #FFFFFF;
            font-family: -apple-system, BlinkMacSystemFont, "Inter", "Segoe UI", Roboto, sans-serif;
            font-size: 13px;
        }

        /* Tooltip */
        QToolTip {
            color: #FFFFFF;
            background-color: #1C1C1E;
            border: 1px solid #2C2C2E;
            border-radius: 6px;
            padding: 6px 10px;
            font-size: 12px;
        }

        /* Top Navigation Bar */
        QToolBar {
            background-color: #111111;
            border-bottom: 1px solid #2C2C2E;
            padding: 6px 20px;
            spacing: 6px;
        }

        QToolButton {
            background-color: transparent;
            color: #A1A1AA;
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 600;
            font-size: 13px;
        }

        QToolButton:hover {
            background-color: #2C2C2E;
            color: #FFFFFF;
        }

        QToolButton:checked, QToolButton:pressed {
            background-color: rgba(0, 229, 153, 0.12);
            color: #00E599;
            border: 1px solid rgba(0, 229, 153, 0.35);
            font-weight: 700;
        }

        QToolButton:disabled {
            color: #52525B;
            background-color: transparent;
        }

        /* Menus & Menu Bars */
        QMenuBar {
            background-color: #111111;
            color: #A1A1AA;
            border-bottom: 1px solid #2C2C2E;
        }

        QMenuBar::item:selected {
            background-color: #2C2C2E;
            color: #00E599;
        }

        QMenu {
            background-color: #1C1C1E;
            color: #FFFFFF;
            border: 1px solid #2C2C2E;
            border-radius: 8px;
            padding: 4px;
        }

        QMenu::item:selected {
            background-color: #2C2C2E;
            color: #00E599;
            border-radius: 6px;
        }

        QMenu::separator {
            height: 1px;
            background: #2C2C2E;
            margin: 4px 6px;
        }

        /* Scrollbars */
        QScrollBar:vertical {
            background: #111111;
            width: 6px;
            margin: 0px;
            border-radius: 3px;
        }

        QScrollBar::handle:vertical {
            background: #2C2C2E;
            min-height: 24px;
            border-radius: 3px;
        }

        QScrollBar::handle:vertical:hover {
            background: #3F3F46;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QScrollBar:horizontal {
            background: #111111;
            height: 6px;
            margin: 0px;
            border-radius: 3px;
        }

        QScrollBar::handle:horizontal {
            background: #2C2C2E;
            min-width: 24px;
            border-radius: 3px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #3F3F46;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }

        /* Progress Bars */
        QProgressBar {
            background-color: #2C2C2E;
            border: none;
            border-radius: 4px;
            text-align: center;
            color: #FFFFFF;
            font-weight: 600;
        }

        QProgressBar::chunk {
            background-color: #00E599;
            border-radius: 4px;
        }

        /* Lists & Trees */
        QListView, QTreeView, QTableView, QTextBrowser {
            background-color: #1C1C1E;
            border: 1px solid #2C2C2E;
            border-radius: 8px;
            color: #FFFFFF;
            outline: none;
        }

        QListView::item:selected, QTreeView::item:selected {
            background-color: #2C2C2E;
            color: #00E599;
            border-radius: 6px;
        }

        QListView::item:hover, QTreeView::item:hover {
            background-color: #2C2C2E;
            color: #FFFFFF;
        }

        /* Status Bar */
        QStatusBar {
            background-color: #111111;
            border-top: 1px solid #2C2C2E;
            color: #A1A1AA;
            font-size: 12px;
            padding: 4px 14px;
        }

        /* Search & Inputs */
        QLineEdit {
            background-color: #1C1C1E;
            border: 1px solid #2C2C2E;
            border-radius: 8px;
            padding: 8px 12px;
            color: #FFFFFF;
            font-size: 13px;
        }

        QLineEdit:hover {
            border-color: #3F3F46;
        }

        QLineEdit:focus {
            border: 1px solid #00E599;
            background-color: #1C1C1E;
        }

        QLineEdit:disabled {
            background-color: #111111;
            color: #52525B;
            border-color: #2C2C2E;
        }

        /* Server Cards */
        QFrame#serverCard {
            background-color: #1C1C1E;
            border: 1px solid #2C2C2E;
            border-radius: 12px;
        }

        QFrame#serverCard:hover {
            background-color: #242426;
            border: 1px solid #3F3F46;
        }

        QLabel#serverTitle {
            color: #FFFFFF;
            font-size: 18px;
            font-weight: 800;
        }

        QLabel#serverMotd {
            color: #A1A1AA;
            font-size: 13px;
        }

        QLabel#badgeVerified {
            background-color: rgba(0, 229, 153, 0.12);
            color: #00E599;
            border: 1px solid rgba(0, 229, 153, 0.35);
            border-radius: 6px;
            padding: 3px 8px;
            font-size: 11px;
            font-weight: 700;
        }

        QLabel#badgeMeta {
            background-color: #111111;
            color: #A1A1AA;
            border: 1px solid #2C2C2E;
            border-radius: 6px;
            padding: 3px 8px;
            font-size: 11px;
            font-weight: 600;
        }

        QLabel#playerCountBadge {
            background-color: #111111;
            color: #00E599;
            border: 1px solid #2C2C2E;
            border-radius: 6px;
            padding: 4px 10px;
            font-size: 12px;
            font-weight: 700;
        }

        /* Primary Call-to-Action (Sunveil Emerald Button) */
        QPushButton#btnConnect, QPushButton#heroConnectBtn, QPushButton#playButton {
            background-color: #00E599;
            color: #000000;
            font-size: 13px;
            font-weight: 800;
            letter-spacing: 0.3px;
            border: none;
            border-radius: 8px;
            padding: 9px 24px;
            min-height: 20px;
        }

        QPushButton#btnConnect:hover, QPushButton#heroConnectBtn:hover, QPushButton#playButton:hover {
            background-color: #10FFAC;
            color: #000000;
        }

        QPushButton#btnConnect:pressed, QPushButton#heroConnectBtn:pressed, QPushButton#playButton:pressed {
            background-color: #00B377;
            padding: 10px 23px 8px 25px;
        }

        QPushButton#btnConnect:disabled, QPushButton#heroConnectBtn:disabled, QPushButton#playButton:disabled {
            background-color: #2C2C2E;
            color: #52525B;
        }

        /* Secondary Buttons */
        QPushButton#btnSecondary, QPushButton#cardDetailsBtn, QPushButton#detailBackButton, QPushButton#refreshButton {
            background-color: #2C2C2E;
            color: #FFFFFF;
            border: 1px solid #2C2C2E;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 600;
            font-size: 12px;
            min-height: 18px;
        }

        QPushButton#btnSecondary:hover, QPushButton#cardDetailsBtn:hover, QPushButton#detailBackButton:hover, QPushButton#refreshButton:hover {
            background-color: #3F3F46;
            border-color: #52525B;
            color: #FFFFFF;
        }

        QPushButton#btnSecondary:pressed, QPushButton#cardDetailsBtn:pressed, QPushButton#detailBackButton:pressed, QPushButton#refreshButton:pressed {
            background-color: #1C1C1E;
        }

        QPushButton#btnSecondary:focus-visible, QPushButton#cardDetailsBtn:focus-visible, QPushButton#detailBackButton:focus-visible, QPushButton#refreshButton:focus-visible {
            border: 1px solid #00E599;
            outline: none;
        }

        QPushButton#btnSecondary:disabled, QPushButton#cardDetailsBtn:disabled, QPushButton#detailBackButton:disabled, QPushButton#refreshButton:disabled {
            background-color: #1C1C1E;
            color: #52525B;
            border-color: #2C2C2E;
        }

        /* Detail Page Containers */
        QFrame#leftPanelFrame, QFrame#rightCardFrame {
            background-color: #1C1C1E;
            border: 1px solid #2C2C2E;
            border-radius: 12px;
        }
    )";
}

QString DarkTheme::tooltip()
{
    return "";
}

