#include "PuzzleCatalog.h"
#include "generated-puzzle-metadata.h"

#include <QRegularExpression>
#include <QSettings>

#include <cstring>

PuzzleCatalog::PuzzleCatalog(QObject *parent)
    : QAbstractListModel(parent)
{
    const QSettings settings;
    const QStringList favoriteNames = settings.value(QStringLiteral("favoritePuzzles")).toStringList();
    m_favorites = QSet<QString>(favoriteNames.cbegin(), favoriteNames.cend());

    m_games.reserve(gamecount);
    rebuildGameOrder();
}

int PuzzleCatalog::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    int count = 0;
    for (const game *candidate : m_games) {
        if (matches(candidate))
            ++count;
    }
    return count;
}

QVariant PuzzleCatalog::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0)
        return {};

    int visibleIndex = 0;
    for (const game *candidate : m_games) {
        if (!matches(candidate))
            continue;
        if (visibleIndex++ != index.row())
            continue;

        const PuzzleMetadata *info = metadataFor(candidate->htmlhelp_topic);
        const QString fallback = QString::fromLatin1(candidate->name)
                                     .replace('_', ' ');
        switch (role) {
        case NameRole:
            return QString::fromLatin1(candidate->name);
        case DisplayNameRole:
            return info ? QString::fromLatin1(info->displayName) : fallback;
        case DescriptionRole:
            return info ? QString::fromLatin1(info->description) : QString();
        case ObjectiveRole:
            return info ? QString::fromLatin1(info->objective) : QString();
        case CanSolveRole:
            return candidate->can_solve;
        case FavoriteRole:
            return isFavorite(candidate);
        case ThumbnailRole:
            return candidate->htmlhelp_topic
                ? QStringLiteral("qrc:/puzzle-thumbnails/%1.png").arg(
                    QString::fromLatin1(candidate->htmlhelp_topic))
                : QString();
        default:
            return {};
        }
    }
    return {};
}

QHash<int, QByteArray> PuzzleCatalog::roleNames() const
{
    return {
        {NameRole, "name"},
        {DisplayNameRole, "displayName"},
        {DescriptionRole, "description"},
        {ObjectiveRole, "objective"},
        {CanSolveRole, "canSolve"},
        {FavoriteRole, "favorite"},
        {ThumbnailRole, "thumbnail"},
    };
}

void PuzzleCatalog::setFilterText(const QString &text)
{
    if (m_filterText == text)
        return;
    beginResetModel();
    m_filterText = text;
    endResetModel();
    emit filterTextChanged();
    emit countChanged();
}

QString PuzzleCatalog::displayNameForGame(const QString &name) const
{
    for (int i = 0; i < gamecount; ++i) {
        const game *candidate = gamelist[i];
        if (name != QString::fromLatin1(candidate->name))
            continue;

        const PuzzleMetadata *info = metadataFor(candidate->htmlhelp_topic);
        return info ? QString::fromLatin1(info->displayName)
                    : QString::fromLatin1(candidate->name).replace('_', ' ');
    }
    return {};
}

void PuzzleCatalog::toggleFavorite(const QString &name)
{
    bool knownGame = false;
    for (int i = 0; i < gamecount; ++i) {
        if (name == QString::fromLatin1(gamelist[i]->name)) {
            knownGame = true;
            break;
        }
    }
    if (!knownGame)
        return;

    beginResetModel();
    if (m_favorites.contains(name))
        m_favorites.remove(name);
    else
        m_favorites.insert(name);
    rebuildGameOrder();
    endResetModel();

    QStringList favoriteNames = m_favorites.values();
    favoriteNames.sort();
    QSettings settings;
    settings.setValue(QStringLiteral("favoritePuzzles"), favoriteNames);
}

const PuzzleMetadata *PuzzleCatalog::metadataFor(const char *id)
{
    if (!id)
        return nullptr;

    for (const PuzzleMetadata &entry : puzzleMetadata) {
        if (std::strcmp(entry.id, id) == 0)
            return &entry;
    }
    return nullptr;
}

void PuzzleCatalog::rebuildGameOrder()
{
    m_games.clear();
    for (int i = 0; i < gamecount; ++i) {
        if (isFavorite(gamelist[i]))
            m_games.append(gamelist[i]);
    }
    for (int i = 0; i < gamecount; ++i) {
        if (!isFavorite(gamelist[i]))
            m_games.append(gamelist[i]);
    }
}

bool PuzzleCatalog::matches(const game *candidate) const
{
    if (m_filterText.trimmed().isEmpty())
        return true;

    const PuzzleMetadata *info = metadataFor(candidate->htmlhelp_topic);
    const QString haystack = QString::fromLatin1(candidate->name) + u' '
        + (info ? QString::fromLatin1(info->displayName) : QString()) + u' '
        + (info ? QString::fromLatin1(info->description) : QString());
    return haystack.contains(m_filterText.trimmed(), Qt::CaseInsensitive);
}

bool PuzzleCatalog::isFavorite(const game *candidate) const
{
    return m_favorites.contains(QString::fromLatin1(candidate->name));
}
