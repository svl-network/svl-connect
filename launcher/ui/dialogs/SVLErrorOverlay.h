#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

class SVLErrorOverlay : public QDialog {
    Q_OBJECT
public:
    explicit SVLErrorOverlay(QWidget* parent = nullptr);
    ~SVLErrorOverlay() override = default;

    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);
    void setMessage(const QString& message);
    void setTechnicalDetails(const QString& details);
    void setRetryVisible(bool visible);

    static bool showError(QWidget* parent,
                          const QString& title,
                          const QString& subtitle,
                          const QString& message,
                          const QString& technicalDetails = QString(),
                          bool allowRetry = true);

signals:
    void retryRequested();

private slots:
    void onToggleDetails();

private:
    void setupUI();

private:
    QFrame* m_container = nullptr;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QLabel* m_messageLabel = nullptr;
    QPushButton* m_toggleDetailsBtn = nullptr;
    QTextBrowser* m_techDetailsBox = nullptr;
    QPushButton* m_closeBtn = nullptr;
    QPushButton* m_retryBtn = nullptr;
    bool m_detailsVisible = false;
};
