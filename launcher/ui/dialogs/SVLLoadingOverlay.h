#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>

class SVLLoadingOverlay : public QDialog {
    Q_OBJECT
public:
    explicit SVLLoadingOverlay(QWidget* parent = nullptr);
    ~SVLLoadingOverlay() override = default;

    void setPrimaryStatus(const QString& text);
    void setDetailStatus(const QString& text);
    void setProgress(int current, int total);
    void setIndeterminate(bool indeterminate);

signals:
    void cancelRequested();

private:
    void setupUI();

private:
    QLabel* m_emblemLabel = nullptr;
    QLabel* m_primaryStatus = nullptr;
    QLabel* m_detailStatus = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_cancelBtn = nullptr;
    QTimer* m_pulseTimer = nullptr;
    int m_pulseStep = 0;
};
