#include "SVLUpdateOverlay.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QDesktopServices>
#include <QUrl>

SVLUpdateOverlay::SVLUpdateOverlay(const QString& version,
                                   bool isMandatory,
                                   const QString& downloadUrl,
                                   const QString& changelog,
                                   QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      m_version(version),
      m_isMandatory(isMandatory),
      m_downloadUrl(downloadUrl),
      m_changelog(changelog)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setModal(true);
    setMinimumWidth(500);
    setMaximumWidth(560);

    setupUI();
}

void SVLUpdateOverlay::showUpdate(const QString& version,
                                 bool isMandatory,
                                 const QString& downloadUrl,
                                 const QString& changelog,
                                 QWidget* parent)
{
    auto* overlay = new SVLUpdateOverlay(version, isMandatory, downloadUrl, changelog, parent);
    overlay->setAttribute(Qt::WA_DeleteOnClose);
    overlay->exec();
}

void SVLUpdateOverlay::setupUI()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);

    m_container = new QFrame(this);
    m_container->setObjectName("updateContainer");
    m_container->setAttribute(Qt::WA_StyledBackground, true);

    auto* shadowEffect = new QGraphicsDropShadowEffect(m_container);
    shadowEffect->setBlurRadius(30);
    shadowEffect->setColor(QColor(0, 0, 0, 200));
    shadowEffect->setOffset(0, 8);
    m_container->setGraphicsEffect(shadowEffect);

    m_container->setStyleSheet(R"(
        QFrame#updateContainer {
            background-color: #111A22;
            border: 1px solid #FFB800;
            border-radius: 14px;
        }
    )");

    auto* mainLayout = new QVBoxLayout(m_container);
    mainLayout->setContentsMargins(24, 24, 24, 20);
    mainLayout->setSpacing(14);

    // 1. Icon & Header
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(14);

    m_iconLabel = new QLabel(tr("🚀"), m_container);
    m_iconLabel->setStyleSheet("font-size: 32px; background: transparent; border: none;");
    headerLayout->addWidget(m_iconLabel);

    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);

    m_titleLabel = new QLabel(m_isMandatory ? tr("UPDATE REQUIRED") : tr("NEW UPDATE AVAILABLE"), m_container);
    m_titleLabel->setStyleSheet("color: #FFB800; font-size: 17px; font-weight: 900; letter-spacing: 0.5px; border: none;");
    titleLayout->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(tr("Version %1 is ready to install.").arg(m_version), m_container);
    m_subtitleLabel->setStyleSheet("color: #E2E8F0; font-size: 13px; font-weight: 600; border: none;");
    titleLayout->addWidget(m_subtitleLabel);

    headerLayout->addLayout(titleLayout, 1);
    mainLayout->addLayout(headerLayout);

    // 2. Changelog / Release Notes
    auto* changelogTitle = new QLabel(tr("WHAT'S NEW"), m_container);
    changelogTitle->setStyleSheet("color: #94A3B8; font-size: 11px; font-weight: 800; letter-spacing: 0.5px; border: none;");
    mainLayout->addWidget(changelogTitle);

    m_changelogBrowser = new QTextBrowser(m_container);
    m_changelogBrowser->setStyleSheet("background-color: #0B1118; border: 1px solid #1E293B; border-radius: 8px; padding: 10px; color: #CBD5E1; font-size: 12px; line-height: 1.4;");
    m_changelogBrowser->setMinimumHeight(90);
    m_changelogBrowser->setMaximumHeight(140);
    QString desc = m_changelog.isEmpty() ? tr("Includes performance improvements, bug fixes, and protocol updates.") : m_changelog;
    m_changelogBrowser->setMarkdown(desc);
    mainLayout->addWidget(m_changelogBrowser);

    // 3. Actions Row
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    m_laterBtn = new QPushButton(tr("LATER"), m_container);
    m_laterBtn->setCursor(Qt::PointingHandCursor);
    m_laterBtn->setStyleSheet("QPushButton { background-color: #1E293B; color: #94A3B8; border: 1px solid #334155; border-radius: 8px; padding: 10px 20px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: #334155; color: #F1F5F9; }");
    connect(m_laterBtn, &QPushButton::clicked, this, &QDialog::reject);
    m_laterBtn->setVisible(!m_isMandatory);
    btnLayout->addWidget(m_laterBtn);

    m_downloadBtn = new QPushButton(tr("DOWNLOAD & RESTART"), m_container);
    m_downloadBtn->setCursor(Qt::PointingHandCursor);
    m_downloadBtn->setStyleSheet("QPushButton { background-color: #00E599; color: #000000; font-size: 12px; font-weight: 900; letter-spacing: 0.5px; border: none; border-radius: 8px; padding: 10px 24px; min-width: 160px; } QPushButton:hover { background-color: #10FFAC; } QPushButton:pressed { background-color: #00B377; }");
    connect(m_downloadBtn, &QPushButton::clicked, this, &SVLUpdateOverlay::onDownloadClicked);
    btnLayout->addWidget(m_downloadBtn);

    mainLayout->addLayout(btnLayout);
    rootLayout->addWidget(m_container);
}

void SVLUpdateOverlay::onDownloadClicked()
{
    if (!m_downloadUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_downloadUrl));
    }
    accept();
}
