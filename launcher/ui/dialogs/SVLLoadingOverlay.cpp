#include "SVLLoadingOverlay.h"

#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>

SVLLoadingOverlay::SVLLoadingOverlay(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFixedSize(540, 360);
    setModal(true);

    setupUI();

    m_pulseTimer = new QTimer(this);
    connect(m_pulseTimer, &QTimer::timeout, this, [this]() {
        m_pulseStep = (m_pulseStep + 1) % 4;
        QString dots;
        for (int i = 0; i <= m_pulseStep; ++i) {
            dots += " .";
        }
        if (m_primaryStatus && !m_primaryStatus->text().isEmpty()) {
            QString baseText = m_primaryStatus->property("baseText").toString();
            if (!baseText.isEmpty()) {
                m_primaryStatus->setText(baseText + dots);
            }
        }
    });
    m_pulseTimer->start(400);
}

void SVLLoadingOverlay::setupUI()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(10, 10, 10, 10);

    auto* cardFrame = new QFrame(this);
    cardFrame->setObjectName("loadingCard");
    cardFrame->setAttribute(Qt::WA_StyledBackground, true);
    cardFrame->setStyleSheet(R"(
        QFrame#loadingCard {
            background-color: #080C0F;
            border: 1px solid #1F2E3B;
            border-radius: 16px;
        }
        QLabel#loadingEmblem {
            font-size: 52px;
            color: #00E599;
            font-weight: 900;
        }
        QLabel#loadingPrimaryStatus {
            color: #FFFFFF;
            font-size: 13px;
            font-weight: 800;
            letter-spacing: 1.5px;
            text-transform: uppercase;
        }
        QLabel#loadingDetailStatus {
            color: #64748B;
            font-size: 11px;
            font-weight: 500;
        }
        QProgressBar#loadingBar {
            background-color: #0E161C;
            border: 1px solid #1F2E3B;
            border-radius: 4px;
            text-align: center;
            height: 6px;
            max-height: 6px;
            color: transparent;
        }
        QProgressBar#loadingBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00E599, stop:1 #00C985);
            border-radius: 3px;
        }
        QPushButton#cancelBtn {
            background-color: transparent;
            color: #64748B;
            border: 1px solid #1F2E3B;
            border-radius: 8px;
            padding: 6px 20px;
            font-size: 12px;
            font-weight: 700;
            letter-spacing: 0.5px;
        }
        QPushButton#cancelBtn:hover {
            background-color: #0E161C;
            color: #EF4444;
            border-color: #EF4444;
        }
    )");

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(32);
    shadow->setColor(QColor(0, 229, 153, 40));
    shadow->setOffset(0, 4);
    cardFrame->setGraphicsEffect(shadow);

    auto* layout = new QVBoxLayout(cardFrame);
    layout->setContentsMargins(36, 36, 36, 28);
    layout->setSpacing(14);
    layout->setAlignment(Qt::AlignCenter);

    // Emblem
    m_emblemLabel = new QLabel(tr("⚡"), cardFrame);
    m_emblemLabel->setObjectName("loadingEmblem");
    m_emblemLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_emblemLabel);

    // Primary status
    m_primaryStatus = new QLabel(tr("CONNECTING TO SUNVEIL REALM SERVICES"), cardFrame);
    m_primaryStatus->setObjectName("loadingPrimaryStatus");
    m_primaryStatus->setProperty("baseText", tr("CONNECTING TO SUNVEIL REALM SERVICES"));
    m_primaryStatus->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_primaryStatus);

    // Progress bar
    m_progressBar = new QProgressBar(cardFrame);
    m_progressBar->setObjectName("loadingBar");
    m_progressBar->setRange(0, 0); // indeterminate
    m_progressBar->setFixedHeight(6);
    layout->addWidget(m_progressBar);

    // Detail status
    m_detailStatus = new QLabel(tr("Synchronizing client manifest assets and dependencies..."), cardFrame);
    m_detailStatus->setObjectName("loadingDetailStatus");
    m_detailStatus->setAlignment(Qt::AlignCenter);
    m_detailStatus->setWordWrap(true);
    layout->addWidget(m_detailStatus);

    layout->addSpacing(10);

    // Cancel action
    m_cancelBtn = new QPushButton(tr("CANCEL"), cardFrame);
    m_cancelBtn->setObjectName("cancelBtn");
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
        emit cancelRequested();
        reject();
    });
    layout->addWidget(m_cancelBtn, 0, Qt::AlignCenter);

    rootLayout->addWidget(cardFrame);
}

void SVLLoadingOverlay::setPrimaryStatus(const QString& text)
{
    if (m_primaryStatus) {
        m_primaryStatus->setProperty("baseText", text.toUpper());
        m_primaryStatus->setText(text.toUpper());
    }
}

void SVLLoadingOverlay::setDetailStatus(const QString& text)
{
    if (m_detailStatus) {
        m_detailStatus->setText(text);
    }
}

void SVLLoadingOverlay::setProgress(int current, int total)
{
    if (m_progressBar) {
        if (total <= 0) {
            m_progressBar->setRange(0, 0);
        } else {
            m_progressBar->setRange(0, total);
            m_progressBar->setValue(current);
        }
    }
}

void SVLLoadingOverlay::setIndeterminate(bool indeterminate)
{
    if (m_progressBar) {
        if (indeterminate) {
            m_progressBar->setRange(0, 0);
        } else {
            m_progressBar->setRange(0, 100);
        }
    }
}
