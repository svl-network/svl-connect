#pragma once

#include <QDialog>
#include <QList>
#include <QString>
#include <memory>

struct SVLModEntry {
    QString projectId;
    QString fileName;
    QString sha256;
    QString downloadUrl;
    QString tier; // "official" or "community"
};

class SVLQuarantineDialog : public QDialog {
    Q_OBJECT

public:
    explicit SVLQuarantineDialog(const QString& serverName,
                                 const QString& serverKey,
                                 const QList<SVLModEntry>& communityMods,
                                 QWidget* parent = nullptr);
    ~SVLQuarantineDialog() override;

private:
    void setupUI(const QString& serverName, const QString& serverKey, const QList<SVLModEntry>& communityMods);

    QList<SVLModEntry> m_communityMods;
};
