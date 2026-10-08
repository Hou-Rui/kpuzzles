#pragma once

#include <QAbstractListModel>
#include <QSet>
#include <QVector>

extern "C" {
#include "puzzles.h"
}

struct PuzzleMetadata;

class PuzzleCatalog final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        DisplayNameRole,
        DescriptionRole,
        ObjectiveRole,
        CanSolveRole,
        FavoriteRole,
        ThumbnailRole,
    };

    explicit PuzzleCatalog(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filterText() const { return m_filterText; }
    void setFilterText(const QString &text);

    Q_INVOKABLE QString displayNameForGame(const QString &name) const;
    Q_INVOKABLE void toggleFavorite(const QString &name);

signals:
    void countChanged();
    void filterTextChanged();

private:
    static const PuzzleMetadata *metadataFor(const char *id);
    void rebuildGameOrder();
    bool matches(const game *candidate) const;
    bool isFavorite(const game *candidate) const;

    QVector<const game *> m_games;
    QSet<QString> m_favorites;
    QString m_filterText;
};
