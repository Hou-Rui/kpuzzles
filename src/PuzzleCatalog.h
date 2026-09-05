#pragma once

#include <QAbstractListModel>
#include <QVector>

extern "C" {
#include "puzzles.h"
}

class PuzzleCatalog final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)

public:
    struct Metadata {
        const char *name;
        const char *displayName;
        const char *description;
        const char *objective;
    };

    enum Role {
        NameRole = Qt::UserRole + 1,
        DisplayNameRole,
        DescriptionRole,
        ObjectiveRole,
        CanSolveRole,
    };

    explicit PuzzleCatalog(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filterText() const { return m_filterText; }
    void setFilterText(const QString &text);

signals:
    void countChanged();
    void filterTextChanged();

private:
    static const Metadata *metadataFor(const char *name);
    bool matches(const game *candidate) const;

    QVector<const game *> m_games;
    QString m_filterText;
};
