#pragma once

#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;
class QTextBrowser;
class QFrame;

class SVLUpdateOverlay : public QDialog {
    Q_OBJECT

public:
    explicit SVLUpdateOverlay(const QString& version,
                              bool isMandatory,
                              const QString& downloadUrl,
                              const QString& changelog,
                              QWidget* parent = nullptr);
    ~SVLUpdateOverlay() override = default;

    static void showUpdate(const QString& version,
                           bool isMandatory,
                           const QString& downloadUrl,
                           const QString& changelog,
                           QWidget* parent = nullptr);

private slots:
    void onDownloadClicked();

private:
    void setupUI();

private:
    QString m_version;
    bool m_isMandatory;
    QString m_downloadUrl;
    QString m_changelog;

    QFrame* m_container = nullptr;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QTextBrowser* m_changelogBrowser = nullptr;
    QPushButton* m_downloadBtn = nullptr;
    QPushButton* m_laterBtn = nullptr;
};
