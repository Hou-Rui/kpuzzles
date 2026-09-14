#pragma once

#include <QObject>
#include <QString>

class SessionState final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool restoreLastGameEnabled READ restoreLastGameEnabled WRITE setRestoreLastGameEnabled NOTIFY restoreLastGameEnabledChanged)
    Q_PROPERTY(QString lastGameName READ lastGameName CONSTANT)

public:
    explicit SessionState(QObject *parent = nullptr);

    bool restoreLastGameEnabled() const { return m_restoreLastGameEnabled; }
    void setRestoreLastGameEnabled(bool enabled);
    QString lastGameName() const { return m_lastGameName; }

    Q_INVOKABLE void setActiveGame(const QString &name);
    Q_INVOKABLE void clearActiveGame();
    void saveSession() const;

signals:
    void restoreLastGameEnabledChanged();

private:
    bool m_restoreLastGameEnabled = true;
    QString m_lastGameName;
    QString m_activeGameName;
};
