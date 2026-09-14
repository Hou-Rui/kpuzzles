#include "SessionState.h"

#include <QSettings>

namespace {
const QString restoreLastGameKey = QStringLiteral(
    "session/restoreLastGameEnabled");
const QString lastGameNameKey = QStringLiteral("session/lastGameName");
}

SessionState::SessionState(QObject *parent)
    : QObject(parent)
{
    const QSettings settings;
    m_restoreLastGameEnabled = settings.value(
        restoreLastGameKey, true).toBool();
    m_lastGameName = settings.value(lastGameNameKey).toString();
    m_activeGameName = m_lastGameName;
}

void SessionState::setRestoreLastGameEnabled(bool enabled)
{
    if (m_restoreLastGameEnabled == enabled)
        return;

    m_restoreLastGameEnabled = enabled;
    QSettings settings;
    settings.setValue(restoreLastGameKey, enabled);
    emit restoreLastGameEnabledChanged();
}

void SessionState::setActiveGame(const QString &name)
{
    m_activeGameName = name;
}

void SessionState::clearActiveGame()
{
    m_activeGameName.clear();
}

void SessionState::saveSession() const
{
    QSettings settings;
    if (m_activeGameName.isEmpty())
        settings.remove(lastGameNameKey);
    else
        settings.setValue(lastGameNameKey, m_activeGameName);
}
