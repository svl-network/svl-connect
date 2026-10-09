#include "LoginWizardPage.h"
#include "minecraft/auth/AccountList.h"
#include "ui/dialogs/MSALoginDialog.h"
#include "ui_LoginWizardPage.h"

#include "Application.h"

LoginWizardPage::LoginWizardPage(QWidget* parent) : BaseWizardPage(parent), ui(new Ui::LoginWizardPage)
{
    ui->setupUi(this);

    // Sunveil look, same palette as the SVL dialogs
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "QWidget#LoginWizardPage { background-color: #111111; }"
        "QLabel { color: #FFFFFF; border: none; background: transparent; }"
        "QLabel#eyebrowLabel { color: #00E599; font-size: 11px; font-weight: 800; letter-spacing: 1px; }"
        "QLabel#titleLabel { font-size: 20px; font-weight: 800; }"
        "QLabel#descriptionLabel, QLabel#msaLabel { color: #A1A1AA; font-size: 12px; }"
        "QLabel#hintLabel { color: #71717A; font-size: 11px; }"
        "QFrame#msaCard { background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 10px; }"
        "QPushButton#pushButton { background-color: #0078D4; color: #FFFFFF; font-size: 13px; font-weight: 800; border: none;"
        " border-radius: 6px; padding: 10px 18px; }"
        "QPushButton#pushButton:hover { background-color: #1084D9; }");
    ui->pushButton->setCursor(Qt::PointingHandCursor);
}

LoginWizardPage::~LoginWizardPage()
{
    delete ui;
}

void LoginWizardPage::initializePage() {}

bool LoginWizardPage::validatePage()
{
    return true;
}

void LoginWizardPage::retranslate()
{
    ui->retranslateUi(this);
}

void LoginWizardPage::on_pushButton_clicked()
{
    wizard()->hide();
    auto account = MSALoginDialog::newAccount(nullptr);
    wizard()->show();
    if (account) {
        APPLICATION->accounts()->addAccount(account);
        APPLICATION->accounts()->setDefaultAccount(account);
        if (wizard()->currentId() == wizard()->pageIds().last()) {
            wizard()->accept();
        } else {
            wizard()->next();
        }
    }
}
