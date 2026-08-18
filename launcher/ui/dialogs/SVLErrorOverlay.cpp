#include "SVLErrorOverlay.h"

#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QIcon>
#include <QGuiApplication>

SVLErrorOverlay::SVLErrorOverlay(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setModal(true);
    setMinimumWidth(480);
    setMaximumWidth(540);

    setupUI();
}

void SVLErrorOverlay::setupUI()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);

    m_container = new QFrame(this);
    m_container->setObjectName("errorContainer");
    m_container->setAttribute(Qt::WA_StyledBackground, true);

    auto* shadowEffect = new QGraphicsDropShadowEffect(m_container);
    shadowEffect->setBlurRadius(25);
    shadowEffect->setColor(QColor(0, 0, 0, 180));
    shadowEffect->setOffset(0, 8);
    m_container->setGraphicsEffect(shadowEffect);

    m_container->setStyleSheet(R"(
        QFrame#errorContainer {
            background-color: #111A22;
            border: 1px solid rgba(255, 184, 0, 0.4);
            border-radius: 12px;
        }
        QPushButton#btnClose {
            background-color: #1A2632;
            color: #F1F5F9;
            border: none;
            border-radius: 8px;
            padding: 8px 20px;
            font-weight: 600;
            font-size: 12px;
            min-height: 18px;
        }
        QPushButton#btnClose:hover {
            background-color: #243647;
        }
        QPushButton#btnClose:pressed {
            background-color: #121A22;
        }
        QPushButton#btnRetry {
            background-color: #FFB800;
            color: #05080A;
            border: none;
            border-radius: 8px;
            padding: 8px 24px;
            font-weight: 800;
            font-size: 12px;
            min-height: 18px;
        }
        QPushButton#btnRetry:hover {
            background-color: #FFC72C;
        }
        QPushButton#btnRetry:pressed {
            background-color: #E5A910;
        }
        QPushButton#btnToggleDetails {
            color: #64748B;
            background: transparent;
            text-align: left;
            border: none;
            font-size: 11px;
            font-weight: 600;
            padding: 2px 0px;
        }
        QPushButton#btnToggleDetails:hover {
            color: #94A3B8;
        }
        QTextBrowser#techDetailsBox {
            background-color: #080C0F;
            color: #EF4444;
            border: 1px solid #1F2E3B;
            border-radius: 6px;
            padding: 8px;
            font-family: Consolas, "Courier New", monospace;
            font-size: 11px;
        }
    )");

    auto* containerLayout = new QVBoxLayout(m_container);
    containerLayout->setContentsMargins(24, 24, 24, 24);
    containerLayout->setSpacing(16);

    // 1. Header Row (Icon + Title & Subtitle)
    auto* headerRow = new QHBoxLayout();
    headerRow->setSpacing(14);
    headerRow->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Render clean geometric warning triangle icon
    QPixmap iconPix(36, 36);
    iconPix.fill(Qt::transparent);
    {
        QPainter p(&iconPix);
        p.setRenderHint(QPainter::Antialiasing, true);

        // Draw Warning Triangle in #FFB800
        QPainterPath path;
        path.moveTo(18, 3);
        path.lineTo(33, 31);
        path.lineTo(3, 31);
        path.closeSubpath();

        QPen pen(QColor("#FFB800"), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(QColor(255, 184, 0, 20));
        p.drawPath(path);

        // Exclamation Mark
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#FFB800"));
        p.drawRoundedRect(QRectF(16.75, 11, 2.5, 10), 1.25, 1.25);
        p.drawEllipse(QRectF(16.75, 24, 2.5, 2.5));
    }

    m_iconLabel = new QLabel(m_container);
    m_iconLabel->setPixmap(iconPix);
    m_iconLabel->setFixedSize(36, 36);
    headerRow->addWidget(m_iconLabel, 0, Qt::AlignTop);

    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);

    m_titleLabel = new QLabel(tr("CONNECTION FAILED"), m_container);
    m_titleLabel->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: 800; letter-spacing: 0.3px; border: none;");
    titleLayout->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(tr("Sunveil Master API unreachable"), m_container);
    m_subtitleLabel->setStyleSheet("color: #94A3B8; font-size: 12px; font-weight: 500; border: none;");
    titleLayout->addWidget(m_subtitleLabel);

    headerRow->addLayout(titleLayout, 1);
    containerLayout->addLayout(headerRow);

    // 2. Human-Readable Message
    m_messageLabel = new QLabel(tr("The server manifest could not be synchronized. The network might be temporarily offline or unreachable."), m_container);
    m_messageLabel->setStyleSheet("color: #CBD5E1; font-size: 13px; line-height: 1.4; border: none;");
    m_messageLabel->setWordWrap(true);
    containerLayout->addWidget(m_messageLabel);

    // 3. Technical Details Toggle and Box
    auto* detailsLayout = new QVBoxLayout();
    detailsLayout->setSpacing(6);

    m_toggleDetailsBtn = new QPushButton(tr("▶ Show Technical Details"), m_container);
    m_toggleDetailsBtn->setObjectName("btnToggleDetails");
    m_toggleDetailsBtn->setCursor(Qt::PointingHandCursor);
    connect(m_toggleDetailsBtn, &QPushButton::clicked, this, &SVLErrorOverlay::onToggleDetails);
    detailsLayout->addWidget(m_toggleDetailsBtn);

    m_techDetailsBox = new QTextBrowser(m_container);
    m_techDetailsBox->setObjectName("techDetailsBox");
    m_techDetailsBox->setFixedHeight(80);
    m_techDetailsBox->setVisible(false);
    detailsLayout->addWidget(m_techDetailsBox);

    containerLayout->addLayout(detailsLayout);

    // 4. Action Buttons (Right-aligned)
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnRow->addStretch();

    m_closeBtn = new QPushButton(tr("CLOSE"), m_container);
    m_closeBtn->setObjectName("btnClose");
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(m_closeBtn);

    m_retryBtn = new QPushButton(tr("RETRY"), m_container);
    m_retryBtn->setObjectName("btnRetry");
    m_retryBtn->setCursor(Qt::PointingHandCursor);
    connect(m_retryBtn, &QPushButton::clicked, this, [this]() {
        emit retryRequested();
        accept();
    });
    btnRow->addWidget(m_retryBtn);

    containerLayout->addLayout(btnRow);
    rootLayout->addWidget(m_container);

    adjustSize();
}

void SVLErrorOverlay::setTitle(const QString& title)
{
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void SVLErrorOverlay::setSubtitle(const QString& subtitle)
{
    if (m_subtitleLabel) {
        m_subtitleLabel->setText(subtitle);
    }
}

void SVLErrorOverlay::setMessage(const QString& message)
{
    if (m_messageLabel) {
        m_messageLabel->setText(message);
    }
}

void SVLErrorOverlay::setTechnicalDetails(const QString& details)
{
    if (m_techDetailsBox) {
        m_techDetailsBox->setPlainText(details);
    }
    if (details.isEmpty() && m_toggleDetailsBtn) {
        m_toggleDetailsBtn->setVisible(false);
    } else if (m_toggleDetailsBtn) {
        m_toggleDetailsBtn->setVisible(true);
    }
}

void SVLErrorOverlay::setRetryVisible(bool visible)
{
    if (m_retryBtn) {
        m_retryBtn->setVisible(visible);
    }
}

void SVLErrorOverlay::onToggleDetails()
{
    m_detailsVisible = !m_detailsVisible;
    if (m_techDetailsBox) {
        m_techDetailsBox->setVisible(m_detailsVisible);
    }
    if (m_toggleDetailsBtn) {
        m_toggleDetailsBtn->setText(m_detailsVisible ? tr("▼ Hide Technical Details") : tr("▶ Show Technical Details"));
    }
    adjustSize();
}

bool SVLErrorOverlay::showError(QWidget* parent,
                                const QString& title,
                                const QString& subtitle,
                                const QString& message,
                                const QString& technicalDetails,
                                bool allowRetry)
{
    QWidget* modalParent = parent ? parent->window() : nullptr;
    SVLErrorOverlay dialog(modalParent);
    dialog.setTitle(title);
    dialog.setSubtitle(subtitle);
    dialog.setMessage(message);
    dialog.setTechnicalDetails(technicalDetails);
    dialog.setRetryVisible(allowRetry);

    int result = dialog.exec();
    return (result == QDialog::Accepted);
}
