#include "SVLQuarantineDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QDialogButtonBox>

SVLQuarantineDialog::SVLQuarantineDialog(const QString& serverName,
                                         const QString& serverKey,
                                         const QList<SVLModEntry>& communityMods,
                                         QWidget* parent)
    : QDialog(parent), m_communityMods(communityMods)
{
    setupUI(serverName, serverKey, communityMods);
}

SVLQuarantineDialog::~SVLQuarantineDialog() = default;

void SVLQuarantineDialog::setupUI(const QString& serverName,
                                  const QString& serverKey,
                                  const QList<SVLModEntry>& communityMods)
{
    setWindowTitle(tr("⚠️ Security Warning: Unverified Community Content"));
    setMinimumSize(680, 440);
    resize(720, 480);

    setStyleSheet(R"(
        QDialog {
            background-color: #181825;
            color: #CDD6F4;
            font-family: 'Segoe UI', Roboto, sans-serif;
        }
        QLabel#titleLabel {
            font-size: 17px;
            font-weight: bold;
            color: #F38BA8;
        }
        QLabel#warningBox {
            background-color: #31202B;
            border: 1px solid #F38BA8;
            border-radius: 6px;
            padding: 10px;
            color: #F5E0DC;
            font-size: 13px;
        }
        QTableWidget {
            background-color: #1E1E2E;
            border: 1px solid #313244;
            border-radius: 6px;
            gridline-color: #313244;
            color: #CDD6F4;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 12px;
        }
        QHeaderView::section {
            background-color: #181825;
            color: #BAC2DE;
            padding: 6px;
            border: 1px solid #313244;
            font-weight: bold;
            font-family: 'Segoe UI', Roboto, sans-serif;
            font-size: 12px;
        }
        QPushButton#trustButton {
            background-color: #F59E0B;
            color: #11111B;
            font-weight: bold;
            border-radius: 6px;
            padding: 8px 18px;
            font-size: 13px;
        }
        QPushButton#trustButton:hover {
            background-color: #D97706;
        }
        QPushButton#cancelButton {
            background-color: #313244;
            color: #CDD6F4;
            border-radius: 6px;
            padding: 8px 18px;
            font-size: 13px;
        }
        QPushButton#cancelButton:hover {
            background-color: #45475A;
        }
    )");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    auto* titleLabel = new QLabel(tr("⚠️ Untrusted / Community Mod Quarantine"), this);
    titleLabel->setObjectName("titleLabel");
    mainLayout->addWidget(titleLabel);

    auto* warningBox = new QLabel(
        tr("The server <b>%1</b> (<code>%2</code>) requires %3 unverified or self-hosted mod/plugin package(s).<br>"
           "Because these files are not signed or verified by Modrinth CDN, ensure you trust the server administrator before installing.")
            .arg(serverName.toHtmlEscaped(), serverKey.toHtmlEscaped(), QString::number(communityMods.size())),
        this
    );
    warningBox->setObjectName("warningBox");
    warningBox->setWordWrap(true);
    mainLayout->addWidget(warningBox);

    auto* table = new QTableWidget(communityMods.size(), 3, this);
    table->setHorizontalHeaderLabels({tr("File Name"), tr("SHA-256 Hash"), tr("Tier")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for (int row = 0; row < communityMods.size(); ++row) {
        const auto& mod = communityMods.at(row);
        auto* nameItem = new QTableWidgetItem(mod.fileName.isEmpty() ? mod.projectId : mod.fileName);
        auto* hashItem = new QTableWidgetItem(mod.sha256);
        auto* tierItem = new QTableWidgetItem(mod.tier.toUpper());

        tierItem->setForeground(QColor("#F59E0B"));

        table->setItem(row, 0, nameItem);
        table->setItem(row, 1, hashItem);
        table->setItem(row, 2, tierItem);
    }
    mainLayout->addWidget(table);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* cancelButton = new QPushButton(tr("Cancel"), this);
    cancelButton->setObjectName("cancelButton");
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton);

    auto* trustButton = new QPushButton(tr("Connect & Trust Community Content"), this);
    trustButton->setObjectName("trustButton");
    connect(trustButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(trustButton);

    mainLayout->addLayout(buttonLayout);
}
