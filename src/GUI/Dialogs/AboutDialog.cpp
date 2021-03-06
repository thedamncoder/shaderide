/**
 * AboutDialog Class
 *
 * -------------------------------------------------------------------------------
 * This file is part of "Shader IDE" -> https://github.com/thedamncoder/shaderide.
 * -------------------------------------------------------------------------------
 *
 * Copyright (c) 2019 - 2021 Florian Roth (The Damn Coder)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <QApplication>
#include <QScreen>
#include <QPixmap>
#include "AboutDialog.hpp"
#include "src/Core/ApplicationDefaults.hpp"
#include "src/Core/Memory.hpp"
#include "src/GUI/Style/AboutDialogStyle.hpp"
#include "src/version.hpp"

using namespace ShaderIDE::GUI;

AboutDialog::AboutDialog(QWidget* parent)
        : QDialog(parent)
{
    InitLayout();
    InitBottomLayout();
}

AboutDialog::~AboutDialog()
{
    // Bottom Layout
    Memory::Release(closeButton);
    Memory::Release(linkLabel);
    Memory::Release(bottomLayout);

    // Main Layout
    Memory::Release(bottomLayoutSpacer);
    Memory::Release(licenseLinkLabel);
    Memory::Release(copyrightLabel);
    Memory::Release(versionLabel);
    Memory::Release(logoLabel);
    Memory::Release(mainLayout);
}

void AboutDialog::InitLayout()
{
    // Window
    setWindowTitle("About");
    setWindowFlags(Qt::FramelessWindowHint);
    setFixedWidth(512 + 20); // Logo width + content margin
    setFixedHeight(350);
    setStyleSheet(STYLE_ABOUTDIALOG);

    // Layout
    mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);
    mainLayout->setAlignment(Qt::AlignTop);
    setLayout(mainLayout);

    // Logo
    auto logo = QPixmap::fromImage(QImage(SHADERIDE_LOGO_PATH));

    auto* screen = qApp->primaryScreen();
    logo.setDevicePixelRatio(screen->devicePixelRatio());

    logoLabel = new QLabel();
    logoLabel->setPixmap(logo);
    logoLabel->setStyleSheet("background-color: rgb(50, 50, 50);");
    logoLabel->setContentsMargins(10, 10, 10, 10);
    mainLayout->addWidget(logoLabel, 0, Qt::AlignTop);

    // Version
    auto versionText = QString("Version ") + SHADERIDE_VERSION;
    versionLabel = new QLabel(versionText);
    versionLabel->setContentsMargins(10, 10, 10, 20);

    auto versionLabelFont = versionLabel->font();
    versionLabelFont.setPointSizeF(8.0f);
    versionLabelFont.setWeight(QFont::Weight::Bold);
    versionLabel->setFont(versionLabelFont);

    mainLayout->addWidget(versionLabel, 0, Qt::AlignTop);

    // Copyright
    copyrightLabel = new QLabel(SHADERIDE_COPYRIGHT);
    copyrightLabel->setContentsMargins(10, 0, 10, 10);
    mainLayout->addWidget(copyrightLabel, 0, Qt::AlignTop);

    // License
    QString licenseLink =
            QString("<a href=\"") + SHADERIDE_LICENSE_URL + "\" "
                    "style=\"color: #3187BE;\">"
                    "Show License</a>";

    licenseLinkLabel = new QLabel(licenseLink);
    licenseLinkLabel->setTextFormat(Qt::RichText);
    licenseLinkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    licenseLinkLabel->setOpenExternalLinks(true);
    licenseLinkLabel->setContentsMargins(10, 0, 10, 0);
    mainLayout->addWidget(licenseLinkLabel);

    // Bottom Layout Spacer
    bottomLayoutSpacer = new QSpacerItem(
            width(),
            50,
            QSizePolicy::Maximum,
            QSizePolicy::Maximum
    );

    mainLayout->addSpacerItem(bottomLayoutSpacer);
}

void AboutDialog::InitBottomLayout()
{
    // Layout
    bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(10, 0, 10, 10);
    bottomLayout->setAlignment(Qt::AlignBottom);
    mainLayout->addLayout(bottomLayout);

    // Link Label
    QString links = QString("") +
                    "&nbsp;&nbsp;"
                    "<a href=\"" + SHADERIDE_GITHUB_URL + "\">"
                        "<img src=\":/icons/icon-github.png\">"
                    "</a>";

    linkLabel = new QLabel(links);
    linkLabel->setTextFormat(Qt::RichText);
    linkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    linkLabel->setOpenExternalLinks(true);
    bottomLayout->addWidget(linkLabel, 0, Qt::AlignLeft | Qt::AlignBottom);

    // Close Button
    closeButton = new QPushButton("Close");
    bottomLayout->addWidget(closeButton, 0, Qt::AlignRight | Qt::AlignBottom);

    connect(closeButton, SIGNAL(clicked(bool)),
            this, SLOT(close()));
}
