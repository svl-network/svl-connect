#include "SVLNewsPage.h"

#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QDesktopServices>
#include <QUrl>
#include <QGuiApplication>

#include "Application.h"
#include "BuildConfig.h"

SVLNewsPage::SVLNewsPage(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setModal(true);
    setMinimumWidth(720);
    setMinimumHeight(560);
    resize(780, 600);

    setupUI();
    loadChangelog();
}

void SVLNewsPage::showNews(QWidget* parent)
{
    SVLNewsPage dlg(parent);
    dlg.exec();
}

void SVLNewsPage::setupUI()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);

    m_container = new QFrame(this);
    m_container->setObjectName("newsContainer");
    m_container->setAttribute(Qt::WA_StyledBackground, true);

    auto* shadowEffect = new QGraphicsDropShadowEffect(m_container);
    shadowEffect->setBlurRadius(30);
    shadowEffect->setColor(QColor(0, 0, 0, 200));
    shadowEffect->setOffset(0, 10);
    m_container->setGraphicsEffect(shadowEffect);

    m_container->setStyleSheet(R"(
        QFrame#newsContainer {
            background-color: #111111;
            border: 1px solid #2C2C2E;
            border-radius: 12px;
        }
        QPushButton#btnCloseNews {
            background-color: #00E599;
            color: #000000;
            border: none;
            border-radius: 8px;
            padding: 8px 22px;
            font-weight: 800;
            font-size: 12px;
            min-height: 20px;
        }
        QPushButton#btnCloseNews:hover {
            background-color: #10FFAC;
        }
        QPushButton#btnCloseNews:pressed {
            background-color: #00B377;
        }
        QPushButton#btnGitHub {
            background-color: #1C1C1E;
            color: #FFFFFF;
            border: 1px solid #2C2C2E;
            border-radius: 8px;
            padding: 8px 18px;
            font-weight: 600;
            font-size: 12px;
            min-height: 20px;
        }
        QPushButton#btnGitHub:hover {
            background-color: #2C2C2E;
            border-color: #3F3F46;
        }
        QPushButton#btnHeaderClose {
            background-color: transparent;
            color: #71717A;
            border: none;
            font-size: 16px;
            font-weight: 700;
            padding: 4px 8px;
            border-radius: 6px;
        }
        QPushButton#btnHeaderClose:hover {
            background-color: #2C2C2E;
            color: #FFFFFF;
        }
        QTextBrowser#newsContentBrowser {
            background-color: #1C1C1E;
            border: 1px solid #2C2C2E;
            border-radius: 8px;
            padding: 16px;
            color: #E4E4E7;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            font-size: 13px;
            line-height: 1.5;
        }
        QScrollBar:vertical {
            background: #111111;
            width: 8px;
            margin: 2px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #3F3F46;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background: #52525B;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");

    auto* containerLayout = new QVBoxLayout(m_container);
    containerLayout->setContentsMargins(24, 20, 24, 20);
    containerLayout->setSpacing(14);

    // 1. Header Row
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(3);

    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(8);

    m_titleLabel = new QLabel(tr("Sunveil Network Updates"), m_container);
    m_titleLabel->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: 900; background: transparent; border: none;");
    titleRow->addWidget(m_titleLabel);

    auto* badge = new QLabel(tr("RELEASE %1").arg(BuildConfig.printableVersionString().toUpper()), m_container);
    badge->setStyleSheet("background-color: rgba(0, 229, 153, 0.12); color: #00E599; border: 1px solid rgba(0, 229, 153, 0.35); border-radius: 6px; padding: 2px 8px; font-size: 10px; font-weight: 700;");
    titleRow->addWidget(badge);
    titleRow->addStretch();

    titleCol->addLayout(titleRow);

    m_subtitleLabel = new QLabel(tr("Live compile-time changelog, commit history, and system release notes."), m_container);
    m_subtitleLabel->setStyleSheet("color: #A1A1AA; font-size: 12px; font-weight: 400; background: transparent; border: none;");
    titleCol->addWidget(m_subtitleLabel);

    headerLayout->addLayout(titleCol, 1);

    auto* headerClose = new QPushButton("✕", m_container);
    headerClose->setObjectName("btnHeaderClose");
    headerClose->setCursor(Qt::PointingHandCursor);
    connect(headerClose, &QPushButton::clicked, this, &QDialog::accept);
    headerLayout->addWidget(headerClose, 0, Qt::AlignTop | Qt::AlignRight);

    containerLayout->addLayout(headerLayout);

    // 2. Content Browser
    m_contentBrowser = new QTextBrowser(m_container);
    m_contentBrowser->setObjectName("newsContentBrowser");
    m_contentBrowser->setOpenExternalLinks(true);
    m_contentBrowser->setReadOnly(true);
    containerLayout->addWidget(m_contentBrowser, 1);

    // 3. Footer Row
    auto* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(10);

    m_githubBtn = new QPushButton(tr("🌐 GitHub Repository"), m_container);
    m_githubBtn->setObjectName("btnGitHub");
    m_githubBtn->setCursor(Qt::PointingHandCursor);
    connect(m_githubBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/svl-network/svl-connect"));
    });
    footerLayout->addWidget(m_githubBtn);

    footerLayout->addStretch();

    m_closeBtn = new QPushButton(tr("CLOSE"), m_container);
    m_closeBtn->setObjectName("btnCloseNews");
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    footerLayout->addWidget(m_closeBtn);

    containerLayout->addLayout(footerLayout);
    rootLayout->addWidget(m_container);
}

void SVLNewsPage::loadChangelog()
{
    QString content;
    QFile file(":/documents/changelog.md");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        content = QString::fromUtf8(file.readAll());
        file.close();
    }

    if (content.isEmpty()) {
        content = tr("# Sunveil Network - Changelog\n\n*No local changelog available.*");
    }

    m_contentBrowser->setMarkdown(content);
}
