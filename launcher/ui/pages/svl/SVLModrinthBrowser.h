#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QList>
#include <QTimer>

class QLineEdit;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QComboBox;
class QNetworkReply;

struct ModrinthProjectItem {
    QString id;
    QString slug;
    QString title;
    QString description;
    QString author;
    QString iconUrl;
    QStringList categories;
    int downloads = 0;
    int follows = 0;
    bool verified = true;
};

class SVLModrinthBrowser : public QDialog {
    Q_OBJECT

public:
    explicit SVLModrinthBrowser(const QString& targetModsDir = QString(), QWidget* parent = nullptr);
    ~SVLModrinthBrowser() override = default;

    static void showBrowser(const QString& targetModsDir = QString(), QWidget* parent = nullptr);

private slots:
    void performSearch();
    void onSearchCompleted();
    void onCategoryChanged(int index);
    void installMod(const ModrinthProjectItem& item, QPushButton* installBtn);

private:
    void setupUI();
    void renderResults(const QList<ModrinthProjectItem>& items);
    QWidget* createModCard(const ModrinthProjectItem& item);
    void downloadAndVerifyJar(const QString& downloadUrl, const QString& fileName, const QString& expectedSha512, QPushButton* installBtn);

private:
    QString m_targetModsDir;
    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_categoryCombo = nullptr;
    QPushButton* m_searchBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QWidget* m_resultsContainer = nullptr;
    QVBoxLayout* m_resultsLayout = nullptr;
    QTimer* m_debounceTimer = nullptr;
    QNetworkReply* m_searchReply = nullptr;
};
