#pragma once

#include <QDialog>

class QLabel;
class QPushButton;
class QTextBrowser;
class QVBoxLayout;
class QHBoxLayout;
class QFrame;

class SVLNewsPage : public QDialog {
    Q_OBJECT

public:
    explicit SVLNewsPage(QWidget* parent = nullptr);
    ~SVLNewsPage() override = default;

    static void showNews(QWidget* parent = nullptr);

private:
    void setupUI();
    void loadChangelog();

    QFrame* m_container = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QTextBrowser* m_contentBrowser = nullptr;
    QPushButton* m_closeBtn = nullptr;
    QPushButton* m_githubBtn = nullptr;
};
