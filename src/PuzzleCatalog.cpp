#include "PuzzleCatalog.h"

#include <QRegularExpression>
#include <QSettings>

#include <cstring>

namespace {
constexpr PuzzleCatalog::Metadata metadata[] = {
    {"blackbox", "Black Box", "Ball-finding puzzle", "Find the hidden balls by bouncing laser beams off them."},
    {"bridges", "Bridges", "Bridge-placing puzzle", "Connect all the islands with a network of bridges."},
    {"cube", "Cube", "Rolling cube puzzle", "Pick up all the blue squares by rolling the cube over them."},
    {"dominosa", "Dominosa", "Domino tiling puzzle", "Tile the rectangle with a full set of dominoes."},
    {"fifteen", "Fifteen", "Sliding block puzzle", "Slide the tiles around to arrange them into order."},
    {"filling", "Filling", "Polyomino puzzle", "Mark every square with the area of its containing region."},
    {"flip", "Flip", "Tile inversion puzzle", "Flip groups of squares to light them all up at once."},
    {"flood", "Flood", "Flood-filling puzzle", "Turn the grid the same colour in as few flood fills as possible."},
    {"galaxies", "Galaxies", "Symmetric polyomino puzzle", "Divide the grid into rotationally symmetric regions each centred on a dot."},
    {"guess", "Guess", "Combination-guessing puzzle", "Guess the hidden combination of colours."},
    {"inertia", "Inertia", "Gem-collecting puzzle", "Collect all the gems without running into any of the mines."},
    {"keen", "Keen", "Arithmetic Latin square puzzle", "Complete the Latin square in accordance with the arithmetic clues."},
    {"lightup", "Light Up", "Grid illumination puzzle", "Place bulbs to light up all the squares."},
    {"loopy", "Loopy", "Loop-drawing puzzle", "Draw a single closed loop using the clues about adjacent edges."},
    {"magnets", "Magnets", "Magnet-placing puzzle", "Place magnets to satisfy the clues and avoid like poles touching."},
    {"map", "Map", "Map-colouring puzzle", "Colour the map so that adjacent regions are never the same colour."},
    {"mines", "Mines", "Mine-finding puzzle", "Find all the mines without treading on any of them."},
    {"mosaic", "Mosaic", "Grid-filling puzzle", "Fill in the grid from clues about nearby black squares."},
    {"net", "Net", "Network jigsaw puzzle", "Rotate each tile to reassemble the network."},
    {"netslide", "Netslide", "Toroidal sliding network puzzle", "Slide a row at a time to reassemble the network."},
    {"palisade", "Palisade", "Grid-division puzzle", "Divide the grid into equal-sized areas in accordance with the clues."},
    {"pattern", "Pattern", "Pattern puzzle", "Fill in the pattern in the grid, given only the lengths of runs."},
    {"pearl", "Pearl", "Loop-drawing puzzle", "Draw a single closed loop using the corner and straight clues."},
    {"pegs", "Pegs", "Peg solitaire puzzle", "Jump pegs over each other to remove all but one."},
    {"range", "Range", "Visible-distance puzzle", "Place black squares to limit the visible distance from each numbered cell."},
    {"rect", "Rectangles", "Rectangles puzzle", "Divide the grid into rectangles with areas equal to the numbers."},
    {"samegame", "Same Game", "Block-clearing puzzle", "Clear the grid by removing touching groups of the same colour."},
    {"signpost", "Signpost", "Square-connecting puzzle", "Connect the squares into a path following the arrows."},
    {"singles", "Singles", "Number-removing puzzle", "Black out the right set of duplicate numbers."},
    {"sixteen", "Sixteen", "Toroidal sliding block puzzle", "Slide a row at a time to arrange the tiles into order."},
    {"slant", "Slant", "Maze-drawing puzzle", "Draw a maze of slanting lines that matches the clues."},
    {"solo", "Solo", "Number-placement puzzle", "Fill the grid so every row, column and block contains every digit."},
    {"tents", "Tents", "Tent-placing puzzle", "Place a tent next to each tree."},
    {"towers", "Towers", "Tower-placing Latin square puzzle", "Complete the Latin square of towers in accordance with the clues."},
    {"tracks", "Tracks", "Path-finding railway puzzle", "Fill in the railway track according to the clues."},
    {"twiddle", "Twiddle", "Rotational sliding block puzzle", "Rotate the tiles around themselves to arrange them in order."},
    {"undead", "Undead", "Monster-placing puzzle", "Place ghosts, vampires and zombies so the right numbers are visible."},
    {"unequal", "Unequal", "Latin square puzzle", "Complete the Latin square in accordance with the greater-than signs."},
    {"unruly", "Unruly", "Black and white grid puzzle", "Fill in the grid while avoiding runs of three."},
    {"untangle", "Untangle", "Planar graph puzzle", "Reposition the points so that the lines do not cross."},
};
}

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

        const Metadata *info = metadataFor(candidate->htmlhelp_topic);
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

const PuzzleCatalog::Metadata *PuzzleCatalog::metadataFor(const char *name)
{
    if (!name)
        return nullptr;

    for (const Metadata &entry : metadata) {
        if (std::strcmp(entry.name, name) == 0)
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

    const Metadata *info = metadataFor(candidate->htmlhelp_topic);
    const QString haystack = QString::fromLatin1(candidate->name) + u' '
        + (info ? QString::fromLatin1(info->displayName) : QString()) + u' '
        + (info ? QString::fromLatin1(info->description) : QString());
    return haystack.contains(m_filterText.trimmed(), Qt::CaseInsensitive);
}

bool PuzzleCatalog::isFavorite(const game *candidate) const
{
    return m_favorites.contains(QString::fromLatin1(candidate->name));
}
