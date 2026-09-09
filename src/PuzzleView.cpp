#include "PuzzleView.h"
#include "generated-night-colours.h"

// puzzles.h intentionally provides simple min/max macros for its C sources;
// remove them before including C++ standard-library headers.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QQuickWindow>
#include <QRandomGenerator>
#include <QScreen>
#include <QTimer>
#include <QVariantMap>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace {
bool darkPalette()
{
    if (!QGuiApplication::instance())
        return false;
    const QPalette palette = QGuiApplication::palette();
    return palette.color(QPalette::Base).lightnessF()
         < palette.color(QPalette::Text).lightnessF();
}

void setPaletteColour(QVector<float> &colours, int index,
                      const QColor &colour)
{
    const int offset = index * 3;
    colours[offset] = static_cast<float>(colour.redF());
    colours[offset + 1] = static_cast<float>(colour.greenF());
    colours[offset + 2] = static_cast<float>(colour.blueF());
}
}

struct PuzzleView::FrontendState {
    PuzzleView *view = nullptr;
    QTimer *timer = nullptr;
    QElapsedTimer clock;
    QPainter *painter = nullptr;
};

// blitter is intentionally opaque in puzzles.h.  The QImage copy is the
// equivalent of the Cairo backing surface used by the GTK frontend.
struct blitter {
    QImage image;
    int width = 0;
    int height = 0;
};

extern "C" void fatal(const char *format, ...)
{
    std::fprintf(stderr, "kpuzzles: fatal error: ");
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
    std::exit(EXIT_FAILURE);
}

extern "C" void get_random_seed(void **randomSeed, int *randomSeedSize)
{
    constexpr int wordCount = 8;
    auto *seed = snewn(wordCount, quint32);
    QRandomGenerator::system()->generate(seed, seed + wordCount);
    *randomSeed = seed;
    *randomSeedSize = static_cast<int>(wordCount * sizeof(quint32));
}

extern "C" void frontend_default_colour(frontend *, float *output)
{
    // Keep the engine's derived colours stable in both themes. Some games
    // calculate most of their palette as fractions of this background; using
    // the real dark background would collapse those colours towards black.
    constexpr float derivedBackground = 0.8F;
    output[0] = derivedBackground;
    output[1] = derivedBackground;
    output[2] = derivedBackground;
}

extern "C" void activate_timer(frontend *frontend)
{
    if (auto *state = PuzzleView::stateFrom(frontend))
        state->view->backendActivateTimer();
}

extern "C" void deactivate_timer(frontend *frontend)
{
    if (auto *state = PuzzleView::stateFrom(frontend))
        state->view->backendDeactivateTimer();
}

const drawing_api PuzzleView::s_drawingApi = {
    1,
    &PuzzleView::drawText,
    &PuzzleView::drawRect,
    &PuzzleView::drawLine,
    &PuzzleView::drawPolygon,
    &PuzzleView::drawCircle,
    &PuzzleView::drawUpdate,
    &PuzzleView::clip,
    &PuzzleView::unclip,
    &PuzzleView::startDraw,
    &PuzzleView::endDraw,
    &PuzzleView::statusBar,
    &PuzzleView::blitterNew,
    &PuzzleView::blitterFree,
    &PuzzleView::blitterSave,
    &PuzzleView::blitterLoad,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    &PuzzleView::textFallback,
    &PuzzleView::drawThickLine,
};

PuzzleView::PuzzleView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(QQuickItem::ItemHasContents, true);
    setFocus(true);

    m_frontendState = new FrontendState;
    m_frontendState->view = this;
    m_frontendState->timer = new QTimer(this);
    m_frontendState->timer->setInterval(16);
    connect(m_frontendState->timer, &QTimer::timeout, this, [this] {
        if (!m_midend || !m_frontendState->clock.isValid())
            return;
        const qint64 elapsed = m_frontendState->clock.restart();
        midend_timer(m_midend, static_cast<float>(elapsed) / 1000.0F);
    });

    connect(this, &QQuickItem::windowChanged, this,
            [this](QQuickWindow *window) {
                if (m_observedWindow) {
                    m_observedWindow->removeEventFilter(this);
                    QObject::disconnect(m_screenChangedConnection);
                }

                m_observedWindow = window;
                if (m_observedWindow) {
                    m_observedWindow->installEventFilter(this);
                    m_screenChangedConnection = connect(
                        m_observedWindow, &QWindow::screenChanged, this,
                        [this](QScreen *) { rebuildImage(); });
                }
                rebuildImage();
            });

    // QML/Kirigami can react to a palette change while this page remains
    // alive. Re-query the puzzle colours and redraw so a live light/dark
    // theme switch does not leave the QImage in the old colour scheme.
    if (auto *application = qobject_cast<QGuiApplication *>(
            QCoreApplication::instance()))
        application->installEventFilter(this);
}

PuzzleView::~PuzzleView()
{
    if (m_observedWindow) {
        m_observedWindow->removeEventFilter(this);
        QObject::disconnect(m_screenChangedConnection);
    }
    if (auto *application = qobject_cast<QGuiApplication *>(
            QCoreApplication::instance()))
        application->removeEventFilter(this);
    clearGame();
    delete m_frontendState;
    m_frontendState = nullptr;
}

PuzzleView::FrontendState *PuzzleView::stateFrom(frontend *frontend)
{
    return reinterpret_cast<FrontendState *>(frontend);
}

PuzzleView *PuzzleView::viewFrom(drawing *drawing)
{
    return stateFrom(reinterpret_cast<frontend *>(drawing->handle))->view;
}

void PuzzleView::setGameName(const QString &name)
{
    if (m_gameName == name && m_midend)
        return;

    if (m_gameName != name) {
        m_gameName = name;
        emit gameNameChanged();
    }

    const QByteArray encodedName = name.toLatin1();
    const game *selectedGame = nullptr;
    for (int i = 0; i < gamecount; ++i) {
        if (std::strcmp(gamelist[i]->name, encodedName.constData()) == 0) {
            selectedGame = gamelist[i];
            break;
        }
    }
    loadGame(selectedGame);
}

void PuzzleView::refreshHelpText()
{
    QString help;
    if (m_midend) {
        const game *currentGame = midend_which_game(m_midend);
        if (currentGame && currentGame->htmlhelp_topic) {
            QFile file(QStringLiteral(":/puzzles-help/%1.html").arg(
                QString::fromLatin1(currentGame->htmlhelp_topic)));
            if (file.open(QIODevice::ReadOnly)) {
                help = QString::fromUtf8(file.readAll());
                const qsizetype firstLine = help.indexOf(QLatin1Char('\n'));
                if (firstLine >= 0)
                    help = help.mid(firstLine + 1).trimmed();
            }
        }
    }

    const bool oldHelpAvailable = m_helpAvailable;
    const QString oldHelpText = m_helpText;
    m_helpAvailable = !help.isEmpty();
    m_helpText = help;
    if (oldHelpAvailable != m_helpAvailable)
        emit helpAvailableChanged();
    if (oldHelpText != m_helpText)
        emit helpTextChanged();
}

void PuzzleView::clearGame()
{
    if (!m_frontendState)
        return;
    m_frontendState->timer->stop();
    m_frontendState->clock.invalidate();
    clearConfiguration();

    if (m_midend) {
        midend_free(m_midend);
        m_midend = nullptr;
    }
    const bool oldHelpAvailable = m_helpAvailable;
    const bool hadHelpText = !m_helpText.isEmpty();
    m_helpAvailable = false;
    m_helpText.clear();
    if (oldHelpAvailable)
        emit helpAvailableChanged();
    if (hadHelpText)
        emit helpTextChanged();
    m_image = {};
    m_colours.clear();
    m_puzzleWidth = 0;
    m_puzzleHeight = 0;
    m_devicePixelRatio = 1.0;
    m_pressedButton = 0;

    const bool oldCanUndo = m_canUndo;
    const bool oldCanRedo = m_canRedo;
    const bool oldCanSolve = m_canSolve;
    const bool oldSolved = m_solved;
    m_canUndo = false;
    m_canRedo = false;
    m_canSolve = false;
    m_solved = false;
    if (oldCanUndo || oldCanRedo || oldCanSolve)
        emit capabilitiesChanged();
    if (oldSolved)
        emit solvedChanged();
    if (!m_statusText.isEmpty()) {
        m_statusText.clear();
        emit statusTextChanged();
    }
    update();
    refreshMenuData();
}

void PuzzleView::loadGame(const game *selectedGame)
{
    clearGame();
    if (!selectedGame)
        return;

    auto *frontend = reinterpret_cast<::frontend *>(m_frontendState);
    m_midend = midend_new(frontend, selectedGame, &s_drawingApi,
                          m_frontendState);
    midend_new_game(m_midend);
    refreshColours();
    resizePuzzle();
    refreshCapabilities();
    refreshMenuData();
    refreshHelpText();
}

void PuzzleView::refreshColours()
{
    if (!m_midend)
        return;

    int count = 0;
    float *colours = midend_colours(m_midend, &count);
    m_colours = QVector<float>(colours, colours + count * 3);
    sfree(colours);

    if (!darkPalette() || count <= 0)
        return;

    // The first colour is the outer canvas background for every game,
    // including Untangle (whose playable area uses its second colour).
    setPaletteColour(m_colours, 0,
                     QGuiApplication::palette().color(QPalette::Base));

    const game *currentGame = midend_which_game(m_midend);
    const char *sourceName = currentGame ? currentGame->htmlhelp_topic : nullptr;
    if (!sourceName)
        return;

    for (const NightColourOverride &override : nightColourOverrides) {
        if (override.index < count &&
            std::strcmp(sourceName, override.game) == 0) {
            setPaletteColour(m_colours, override.index,
                             QColor::fromRgb(override.rgb));
        }
    }
}

void PuzzleView::refreshMenuData()
{
    const bool oldCanConfigure = m_canConfigure;
    const QVariantList oldPresets = m_presetEntries;

    m_presetEntries.clear();
    m_canConfigure = false;
    if (m_midend) {
        preset_menu *menu = midend_get_presets(m_midend, nullptr);
        const int currentPreset = midend_which_preset(m_midend);
        appendPresetEntries(menu, QString(), currentPreset);
        m_canConfigure = midend_which_game(m_midend)->can_configure;
    }

    if (oldCanConfigure != m_canConfigure || oldPresets != m_presetEntries)
        emit menuChanged();
}

void PuzzleView::appendPresetEntries(const preset_menu *menu,
                                     const QString &prefix,
                                     int currentPreset)
{
    if (!menu)
        return;

    for (int i = 0; i < menu->n_entries; ++i) {
        const preset_menu_entry &entry = menu->entries[i];
        const QString title = QString::fromUtf8(entry.title);
        if (entry.params) {
            const QString label = prefix.isEmpty()
                ? title : prefix + QStringLiteral(" / ") + title;
            QVariantMap item;
            item.insert(QStringLiteral("id"), entry.id);
            item.insert(QStringLiteral("title"), title);
            item.insert(QStringLiteral("category"), prefix);
            item.insert(QStringLiteral("label"), label);
            item.insert(QStringLiteral("selected"), entry.id == currentPreset);
            m_presetEntries.append(item);
        } else {
            const QString childPrefix = prefix.isEmpty()
                ? title : prefix + QStringLiteral(" / ") + title;
            appendPresetEntries(entry.submenu, childPrefix, currentPreset);
        }
    }
}

void PuzzleView::clearConfiguration()
{
    const bool hadConfiguration = m_pendingConfiguration ||
        !m_configuration.isEmpty() || !m_configurationTitle.isEmpty() ||
        !m_configurationError.isEmpty();
    if (m_pendingConfiguration) {
        free_cfg(m_pendingConfiguration);
        m_pendingConfiguration = nullptr;
    }
    m_configuration.clear();
    m_configurationTitle.clear();
    m_configurationError.clear();
    if (hadConfiguration) {
        emit configurationChanged();
        emit configurationTitleChanged();
        emit configurationErrorChanged();
    }
}

void PuzzleView::setConfigurationError(const QString &error)
{
    if (m_configurationError == error)
        return;
    m_configurationError = error;
    emit configurationErrorChanged();
}

void PuzzleView::rebuildConfigurationModel()
{
    const QVariantList oldConfiguration = m_configuration;
    m_configuration.clear();
    if (!m_pendingConfiguration)
        return;

    for (config_item *item = m_pendingConfiguration;
         item->type != C_END; ++item) {
        QVariantMap row;
        row.insert(QStringLiteral("name"),
                   QString::fromUtf8(item->name ? item->name : ""));
        row.insert(QStringLiteral("type"), item->type);

        if (item->type == C_STRING) {
            row.insert(QStringLiteral("value"),
                       QString::fromUtf8(item->u.string.sval));
        } else if (item->type == C_BOOLEAN) {
            row.insert(QStringLiteral("value"), item->u.boolean.bval);
        } else if (item->type == C_CHOICES) {
            QVariantList choices;
            const char separator = item->u.choices.choicenames[0];
            const char *start = item->u.choices.choicenames + 1;
            while (*start) {
                const char *end = std::strchr(start, separator);
                if (!end)
                    end = start + std::strlen(start);
                choices.append(QString::fromUtf8(start,
                                                  end - start));
                if (!*end)
                    break;
                start = end + 1;
            }
            row.insert(QStringLiteral("value"), item->u.choices.selected);
            row.insert(QStringLiteral("selected"), item->u.choices.selected);
            row.insert(QStringLiteral("choices"), choices);
        }
        m_configuration.append(row);
    }

    if (oldConfiguration != m_configuration)
        emit configurationChanged();
}

void PuzzleView::beginConfiguration(int which)
{
    if (!m_midend || (which == CFG_SETTINGS && !m_canConfigure))
        return;

    clearConfiguration();
    char *title = nullptr;
    m_pendingConfiguration = midend_get_config(m_midend, which, &title);
    m_configurationKind = which;
    if (title) {
        m_configurationTitle = QString::fromUtf8(title);
        sfree(title);
    }
    setConfigurationError(QString());
    rebuildConfigurationModel();
    emit configurationTitleChanged();
}

void PuzzleView::setConfigurationValue(int index, const QVariant &value)
{
    if (!m_pendingConfiguration || index < 0)
        return;

    config_item *item = m_pendingConfiguration;
    for (int current = 0; current < index && item->type != C_END;
         ++current, ++item) {
    }
    if (item->type == C_END)
        return;

    switch (item->type) {
    case C_STRING: {
        const QByteArray encoded = value.toString().toUtf8();
        sfree(item->u.string.sval);
        item->u.string.sval = dupstr(encoded.constData());
        break;
    }
    case C_BOOLEAN:
        item->u.boolean.bval = value.toBool();
        break;
    case C_CHOICES: {
        int optionCount = 0;
        const char separator = item->u.choices.choicenames[0];
        const char *start = item->u.choices.choicenames + 1;
        while (*start) {
            ++optionCount;
            const char *end = std::strchr(start, separator);
            if (!end)
                break;
            start = end + 1;
        }
        if (optionCount > 0)
            item->u.choices.selected = std::clamp(
                value.toInt(), 0, optionCount - 1);
        break;
    }
    default:
        return;
    }

    rebuildConfigurationModel();
}

void PuzzleView::resizePuzzle()
{
    if (!m_midend)
        return;

    int availableWidth = std::max(1, qRound(width()));
    int availableHeight = std::max(1, qRound(height()));
    int puzzleWidth = availableWidth;
    int puzzleHeight = availableHeight;
    // The item occupies the page space left after the status row and toolbar.
    // Treat that size as a user-requested drawing area so the midend chooses
    // the largest tile size that keeps the whole puzzle inside it.
    midend_size(m_midend, &puzzleWidth, &puzzleHeight, true, 1.0);

    m_puzzleWidth = puzzleWidth;
    m_puzzleHeight = puzzleHeight;
    rebuildImage();
}

qreal PuzzleView::devicePixelRatio() const
{
    if (m_observedWindow) {
        const qreal dpr = m_observedWindow->devicePixelRatio();
        return dpr > 0.0 ? dpr : 1.0;
    }
    if (auto *screen = QGuiApplication::primaryScreen()) {
        const qreal dpr = screen->devicePixelRatio();
        return dpr > 0.0 ? dpr : 1.0;
    }
    return 1.0;
}

void PuzzleView::rebuildImage()
{
    if (!m_midend || m_puzzleWidth <= 0 || m_puzzleHeight <= 0)
        return;

    m_devicePixelRatio = devicePixelRatio();
    const int pixelWidth = std::max(1, static_cast<int>(
        std::ceil(m_puzzleWidth * m_devicePixelRatio)));
    const int pixelHeight = std::max(1, static_cast<int>(
        std::ceil(m_puzzleHeight * m_devicePixelRatio)));

    m_image = QImage(pixelWidth, pixelHeight,
                     QImage::Format_ARGB32_Premultiplied);
    m_image.setDevicePixelRatio(m_devicePixelRatio);
    m_image.fill(Qt::transparent);
    // m_image has just been replaced, so the old drawstate no longer
    // describes what is present on the backing surface. A plain
    // midend_redraw() may skip unchanged tiles and leave this new image
    // transparent. Recreate the drawstate and force a complete redraw.
    midend_force_redraw(m_midend);
    update();
}

void PuzzleView::refreshCapabilities()
{
    if (!m_midend)
        return;
    const bool canUndoNow = midend_can_undo(m_midend);
    const bool canRedoNow = midend_can_redo(m_midend);
    const bool canSolveNow = midend_which_game(m_midend)->can_solve;
    const bool solvedNow = midend_status(m_midend) > 0;
    const bool capabilitiesChangedNow = canUndoNow != m_canUndo ||
        canRedoNow != m_canRedo || canSolveNow != m_canSolve;
    const bool solvedChangedNow = solvedNow != m_solved;
    m_canUndo = canUndoNow;
    m_canRedo = canRedoNow;
    m_canSolve = canSolveNow;
    m_solved = solvedNow;
    if (capabilitiesChangedNow)
        emit capabilitiesChanged();
    if (solvedChangedNow)
        emit solvedChanged();
}

void PuzzleView::handleResult(int result)
{
    refreshCapabilities();
    if (result == PKR_QUIT)
        emit quitRequested();
    update();
}

void PuzzleView::newGame()
{
    if (m_midend)
        handleResult(midend_process_key(m_midend, 0, 0, UI_NEWGAME));
}

void PuzzleView::restartGame()
{
    if (!m_midend)
        return;
    midend_restart_game(m_midend);
    refreshCapabilities();
    update();
}

void PuzzleView::undo()
{
    if (m_midend)
        handleResult(midend_process_key(m_midend, 0, 0, UI_UNDO));
}

void PuzzleView::redo()
{
    if (m_midend)
        handleResult(midend_process_key(m_midend, 0, 0, UI_REDO));
}

void PuzzleView::solve()
{
    if (m_midend)
        handleResult(midend_process_key(m_midend, 0, 0, UI_SOLVE));
}

void PuzzleView::selectPreset(int id)
{
    if (!m_midend)
        return;

    preset_menu *menu = midend_get_presets(m_midend, nullptr);
    game_params *params = preset_menu_lookup_by_id(menu, id);
    if (!params)
        return;

    cancelConfiguration();
    midend_set_params(m_midend, params);
    midend_new_game(m_midend);
    refreshColours();
    resizePuzzle();
    refreshCapabilities();
    refreshMenuData();
}

void PuzzleView::beginConfiguration()
{
    beginConfiguration(CFG_SETTINGS);
}

void PuzzleView::beginGameIdConfiguration()
{
    beginConfiguration(CFG_DESC);
}

void PuzzleView::beginRandomSeedConfiguration()
{
    beginConfiguration(CFG_SEED);
}

bool PuzzleView::applyConfiguration()
{
    if (!m_midend || !m_pendingConfiguration)
        return false;

    const char *error = midend_set_config(m_midend, m_configurationKind,
                                          m_pendingConfiguration);
    if (error) {
        setConfigurationError(QString::fromUtf8(error));
        return false;
    }

    free_cfg(m_pendingConfiguration);
    m_pendingConfiguration = nullptr;
    m_configuration.clear();
    m_configurationTitle.clear();
    setConfigurationError(QString());
    emit configurationChanged();
    emit configurationTitleChanged();

    if (m_configurationKind != CFG_PREFS) {
        midend_new_game(m_midend);
        refreshColours();
        resizePuzzle();
        refreshCapabilities();
        refreshMenuData();
    }
    return true;
}

void PuzzleView::cancelConfiguration()
{
    clearConfiguration();
}

void PuzzleView::backendActivateTimer()
{
    if (m_frontendState && !m_frontendState->timer->isActive()) {
        m_frontendState->clock.start();
        m_frontendState->timer->start();
    }
}

void PuzzleView::backendDeactivateTimer()
{
    if (m_frontendState) {
        m_frontendState->timer->stop();
        m_frontendState->clock.invalidate();
    }
}

int PuzzleView::pointerX(const QPointF &position) const
{
    return qRound(position.x() - (width() - m_puzzleWidth) / 2.0);
}

int PuzzleView::pointerY(const QPointF &position) const
{
    return qRound(position.y() - (height() - m_puzzleHeight) / 2.0);
}

void PuzzleView::processPointer(const QPointF &position, int button)
{
    if (m_midend)
        handleResult(midend_process_key(m_midend, pointerX(position),
                                        pointerY(position), button));
}

void PuzzleView::geometryChange(const QRectF &newGeometry,
                                const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        resizePuzzle();
}

void PuzzleView::mousePressEvent(QMouseEvent *event)
{
    setFocus(true);
    if (event->button() == Qt::LeftButton)
        m_pressedButton = LEFT_BUTTON;
    else if (event->button() == Qt::MiddleButton)
        m_pressedButton = MIDDLE_BUTTON;
    else if (event->button() == Qt::RightButton)
        m_pressedButton = RIGHT_BUTTON;
    else
        m_pressedButton = 0;

    if (m_pressedButton) {
        processPointer(event->position(), m_pressedButton);
        event->accept();
    } else {
        QQuickPaintedItem::mousePressEvent(event);
    }
}

void PuzzleView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_pressedButton) {
        QQuickPaintedItem::mouseMoveEvent(event);
        return;
    }

    const int dragButton = m_pressedButton - LEFT_BUTTON + LEFT_DRAG;
    processPointer(event->position(), dragButton);
    event->accept();
}

void PuzzleView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_pressedButton) {
        QQuickPaintedItem::mouseReleaseEvent(event);
        return;
    }

    const int releaseButton = m_pressedButton - LEFT_BUTTON + LEFT_RELEASE;
    processPointer(event->position(), releaseButton);
    m_pressedButton = 0;
    event->accept();
}

void PuzzleView::keyPressEvent(QKeyEvent *event)
{
    if (!m_midend) {
        event->ignore();
        return;
    }

    int button = 0;
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_Z)
            button = (event->modifiers() & Qt::ShiftModifier) ? UI_REDO : UI_UNDO;
        else if (event->key() == Qt::Key_Y)
            button = UI_REDO;
    }
    if (!button) {
        switch (event->key()) {
        case Qt::Key_Up: button = CURSOR_UP; break;
        case Qt::Key_Down: button = CURSOR_DOWN; break;
        case Qt::Key_Left: button = CURSOR_LEFT; break;
        case Qt::Key_Right: button = CURSOR_RIGHT; break;
        case Qt::Key_Return:
        case Qt::Key_Enter: button = CURSOR_SELECT; break;
        case Qt::Key_Escape: button = UI_QUIT; break;
        default:
            if (!event->text().isEmpty())
                button = event->text().at(0).unicode();
            break;
        }
    }

    if (button) {
        handleResult(midend_process_key(m_midend, 0, 0, button));
        event->accept();
    } else {
        event->ignore();
    }
}

bool PuzzleView::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == QCoreApplication::instance() &&
        event->type() == QEvent::ApplicationPaletteChange && m_midend) {
        refreshColours();
        if (!m_image.isNull())
            m_image.fill(colourFor(this, 0));
        midend_force_redraw(m_midend);
        refreshCapabilities();
    } else if (watched == m_observedWindow &&
               event->type() == QEvent::DevicePixelRatioChange) {
        rebuildImage();
    }
    return QQuickPaintedItem::eventFilter(watched, event);
}

void PuzzleView::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    if (m_image.isNull())
        return;

    const QSizeF logicalImageSize = m_image.deviceIndependentSize();
    const qreal x = (width() - logicalImageSize.width()) / 2.0;
    const qreal y = (height() - logicalImageSize.height()) / 2.0;
    painter->drawImage(QPointF(x, y), m_image);
}

QColor PuzzleView::colourFor(const PuzzleView *view, int colour)
{
    const int offset = colour * 3;
    if (colour < 0 || offset + 2 >= view->m_colours.size())
        return Qt::black;

    return QColor::fromRgbF(view->m_colours[offset],
                            view->m_colours[offset + 1],
                            view->m_colours[offset + 2]);
}

void PuzzleView::startDraw(drawing *drawing)
{
    auto *view = viewFrom(drawing);
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    delete state->painter;
    state->painter = new QPainter(&view->m_image);
    state->painter->setRenderHint(QPainter::Antialiasing, true);
}

void PuzzleView::endDraw(drawing *drawing)
{
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    delete state->painter;
    state->painter = nullptr;
    viewFrom(drawing)->update();
}

void PuzzleView::drawText(drawing *drawing, int x, int y, int fontType,
                          int fontSize, int align, int colour,
                          const char *text)
{
    auto *view = viewFrom(drawing);
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    if (!state->painter)
        return;

    QFont font(fontType == FONT_FIXED ? QStringLiteral("monospace")
                                      : QStringLiteral("sans-serif"));
    font.setStyleHint(fontType == FONT_FIXED ? QFont::TypeWriter
                                             : QFont::SansSerif);
    // Match the GTK frontend's bold Pango font and prefer pixel-aligned
    // glyph hinting for the small clue and number fonts used by the games.
    font.setBold(true);
    font.setHintingPreference(QFont::PreferFullHinting);
    font.setPixelSize(std::max(1, fontSize));
    const QString string = QString::fromUtf8(text);
    const QFontMetrics metrics(font);
    const QRect bounds = metrics.boundingRect(string);
    int left = x;
    if (align & ALIGN_HCENTRE)
        left -= bounds.width() / 2;
    else if (align & ALIGN_HRIGHT)
        left -= bounds.width();

    int top = y - metrics.ascent();
    if (align & ALIGN_VCENTRE)
        top = y - (metrics.ascent() + metrics.descent()) / 2;

    state->painter->setFont(font);
    state->painter->setPen(colourFor(view, colour));
    state->painter->drawText(QPoint(left, top + metrics.ascent()), string);
}

void PuzzleView::drawRect(drawing *drawing, int x, int y, int width,
                          int height, int colour)
{
    auto *view = viewFrom(drawing);
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    if (!state->painter)
        return;
    state->painter->setPen(Qt::NoPen);
    state->painter->setBrush(colourFor(view, colour));
    state->painter->drawRect(x, y, width, height);
}

void PuzzleView::drawLine(drawing *drawing, int x1, int y1, int x2, int y2,
                          int colour)
{
    auto *view = viewFrom(drawing);
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    if (!state->painter)
        return;
    QPen pen(colourFor(view, colour));
    pen.setWidthF(1.0);
    state->painter->setPen(pen);
    state->painter->setBrush(Qt::NoBrush);
    state->painter->drawLine(QLineF(x1 + 0.5, y1 + 0.5,
                                    x2 + 0.5, y2 + 0.5));
}

void PuzzleView::drawPolygon(drawing *drawing, const int *coordinates,
                             int pointCount, int fillColour,
                             int outlineColour)
{
    auto *view = viewFrom(drawing);
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    if (!state->painter)
        return;
    QPolygon polygon;
    polygon.reserve(pointCount);
    for (int i = 0; i < pointCount; ++i)
        polygon.append(QPoint(coordinates[i * 2], coordinates[i * 2 + 1]));
    QColor fill;
    if (fillColour >= 0) {
        fill = colourFor(view, fillColour);
        state->painter->setBrush(fill);
    }
    else
        state->painter->setBrush(Qt::NoBrush);
    if (outlineColour >= 0) {
        const QColor outline = outlineColour == fillColour && fill.isValid()
            ? fill : colourFor(view, outlineColour);
        state->painter->setPen(QPen(outline));
    }
    else
        state->painter->setPen(Qt::NoPen);
    state->painter->drawPolygon(polygon);
}

void PuzzleView::drawCircle(drawing *drawing, int centerX, int centerY,
                            int radius, int fillColour, int outlineColour)
{
    auto *view = viewFrom(drawing);
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    if (!state->painter)
        return;
    QColor fill;
    if (fillColour >= 0) {
        fill = colourFor(view, fillColour);
        state->painter->setBrush(fill);
    }
    else
        state->painter->setBrush(Qt::NoBrush);
    if (outlineColour >= 0) {
        const QColor outline = outlineColour == fillColour && fill.isValid()
            ? fill : colourFor(view, outlineColour);
        state->painter->setPen(QPen(outline));
    }
    else
        state->painter->setPen(Qt::NoPen);
    state->painter->drawEllipse(QPointF(centerX + 0.5, centerY + 0.5),
                                radius, radius);
}

void PuzzleView::drawUpdate(drawing *, int, int, int, int)
{
    // QQuickPaintedItem repaints as a whole; the midend still calls this so
    // the backend's incremental redraw contract remains intact.
}

void PuzzleView::clip(drawing *drawing, int x, int y, int width, int height)
{
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    if (state->painter) {
        state->painter->save();
        state->painter->setClipRect(QRect(x, y, width, height));
    }
}

void PuzzleView::unclip(drawing *drawing)
{
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    if (state->painter)
        state->painter->restore();
}

void PuzzleView::statusBar(drawing *drawing, const char *text)
{
    auto *view = viewFrom(drawing);
    const QString status = QString::fromUtf8(text);
    if (view->m_statusText == status)
        return;
    view->m_statusText = status;
    emit view->statusTextChanged();
}

blitter *PuzzleView::blitterNew(drawing *, int width, int height)
{
    auto *result = new blitter;
    result->width = width;
    result->height = height;
    return result;
}

void PuzzleView::blitterFree(drawing *, blitter *blitter)
{
    delete blitter;
}

void PuzzleView::blitterSave(drawing *drawing, blitter *blitter, int x, int y)
{
    auto *view = viewFrom(drawing);
    if (!view->m_image.isNull())
        blitter->image = view->m_image.copy(
            qRound(x * view->m_devicePixelRatio),
            qRound(y * view->m_devicePixelRatio),
            std::max(1, qRound(blitter->width * view->m_devicePixelRatio)),
            std::max(1, qRound(blitter->height * view->m_devicePixelRatio)));
    if (!blitter->image.isNull())
        blitter->image.setDevicePixelRatio(view->m_devicePixelRatio);
}

void PuzzleView::blitterLoad(drawing *drawing, blitter *blitter, int x, int y)
{
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    if (state->painter && !blitter->image.isNull())
        state->painter->drawImage(x, y, blitter->image);
}

char *PuzzleView::textFallback(drawing *, const char *const *strings,
                               int count)
{
    return dupstr(count > 0 ? strings[0] : "");
}

void PuzzleView::drawThickLine(drawing *drawing, float thickness, float x1,
                               float y1, float x2, float y2, int colour)
{
    auto *view = viewFrom(drawing);
    auto *state = stateFrom(reinterpret_cast<frontend *>(drawing->handle));
    if (!state->painter)
        return;
    QPen pen(colourFor(view, colour));
    pen.setWidthF(std::max(1.0F, thickness));
    pen.setCapStyle(Qt::SquareCap);
    state->painter->setPen(pen);
    state->painter->drawLine(QLineF(x1, y1, x2, y2));
}
