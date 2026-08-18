#pragma once

#include <QDialog>
#include <QString>

class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
class QListWidget;

class SVLAccountManager : public QDialog {
    Q_OBJECT

public:
    explicit SVLAccountManager(QWidget* parent = nullptr);
    ~SVLAccountManager() override = default;

    static void showAccountManager(QWidget* parent = nullptr);

private slots:
    void onMicrosoftLoginClicked();
    void onAddOfflineAccountClicked();
    void onAccountSelectionChanged();
    void onSetDefaultClicked();
    void onRemoveAccountClicked();
    void refreshAccountList();

private:
    void setupUI();

private:
    QListWidget* m_accountListWidget = nullptr;
    QLineEdit* m_offlineUsernameEdit = nullptr;
    QPushButton* m_addOfflineBtn = nullptr;
    QPushButton* m_msaLoginBtn = nullptr;
    QPushButton* m_setDefaultBtn = nullptr;
    QPushButton* m_removeBtn = nullptr;
    QLabel* m_activeAccountLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
};
