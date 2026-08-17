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
    darkPalette.setColor(QPalette::Window, QColor(8, 12, 14));           // #080C0E (Deep Void Slate)
    darkPalette.setColor(QPalette::WindowText, QColor(248, 250, 252));    // #F8FAFC
    darkPalette.setColor(QPalette::Base, QColor(14, 20, 24));             // #0E1418
    darkPalette.setColor(QPalette::AlternateBase, QColor(18, 26, 32));    // #121A20
    darkPalette.setColor(QPalette::ToolTipBase, QColor(14, 20, 24));
    darkPalette.setColor(QPalette::ToolTipText, QColor(248, 250, 252));
    darkPalette.setColor(QPalette::Text, QColor(248, 250, 252));
    darkPalette.setColor(QPalette::Button, QColor(14, 20, 24));           // #0E1418
    darkPalette.setColor(QPalette::ButtonText, QColor(248, 250, 252));
    darkPalette.setColor(QPalette::BrightText, QColor(0, 229, 153));      // #00E599 (Emerald)
    darkPalette.setColor(QPalette::Link, QColor(0, 229, 153));
    darkPalette.setColor(QPalette::Highlight, QColor(0, 229, 153));
    darkPalette.setColor(QPalette::HighlightedText, QColor(8, 12, 14));
    darkPalette.setColor(QPalette::PlaceholderText, QColor(100, 116, 139)); // #64748B
    return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double DarkTheme::fadeAmount()
{
    return 0.5;
}

QColor DarkTheme::fadeColor()
{
    return QColor(8, 12, 14);
}

bool DarkTheme::hasStyleSheet()
{
    return true;
}

QString DarkTheme::appStyleSheet()
{
    return R"(
        QMainWindow, QDialog {
            background-color: #080C0E;
            color: #F8FAFC;
        }
        QToolTip {
            color: #F8FAFC;
            background-color: #0E1418;
            border: 1px solid #1E2B33;
            border-radius: 6px;
            padding: 6px 10px;
            font-size: 12px;
        }
        QToolBar {
            background-color: #080C0E;
            border-bottom: 1px solid #1E2B33;
            spacing: 6px;
            padding: 4px 8px;
        }
        QToolButton {
            background: transparent;
            color: #94A3B8;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 6px 12px;
            font-weight: 600;
        }
        QToolButton:hover {
            background-color: #121A20;
            color: #00E599;
            border: 1px solid #1E2B33;
        }
        QToolButton:checked {
            background-color: #151F26;
            color: #00E599;
            border-bottom: 2px solid #00E599;
            border-radius: 4px;
        }
        QMenuBar {
            background-color: #080C0E;
            color: #94A3B8;
            border-bottom: 1px solid #1E2B33;
        }
        QMenuBar::item:selected {
            background-color: #121A20;
            color: #00E599;
        }
        QMenu {
            background-color: #0E1418;
            color: #F8FAFC;
            border: 1px solid #1E2B33;
            border-radius: 8px;
            padding: 4px;
        }
        QMenu::item:selected {
            background-color: #121A20;
            color: #00E599;
            border-radius: 4px;
        }
        QMenu::separator {
            height: 1px;
            background: #1E2B33;
            margin: 4px 8px;
        }
        QScrollBar:vertical {
            background: #080C0E;
            width: 10px;
            margin: 0px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical {
            background: #1E2B33;
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
            background: #080C0E;
            height: 10px;
            margin: 0px;
            border-radius: 5px;
        }
        QScrollBar::handle:horizontal {
            background: #1E2B33;
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
            background-color: #0E1418;
            border: 1px solid #1E2B33;
            border-radius: 6px;
            text-align: center;
            color: #F8FAFC;
            font-weight: bold;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00E599, stop:1 #00C480);
            border-radius: 5px;
        }
        QListView, QTreeView, QTableView {
            background-color: #080C0E;
            border: 1px solid #1E2B33;
            border-radius: 8px;
            color: #F8FAFC;
            outline: none;
        }
        QListView::item:selected, QTreeView::item:selected {
            background-color: #121A20;
            color: #00E599;
            border-radius: 6px;
        }
        QListView::item:hover, QTreeView::item:hover {
            background-color: #0E1418;
            color: #00E599;
        }
    )";
}

QString DarkTheme::tooltip()
{
    return "";
}

