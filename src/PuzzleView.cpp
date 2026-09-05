#include "PuzzleView.h"

// puzzles.h intentionally provides simple min/max macros for its C sources;
// remove them before including C++ standard-library headers.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <QElapsedTimer>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QRandomGenerator>
#include <QTimer>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

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
    QColor background(242, 242, 242);
    if (QGuiApplication::instance())
        background = QGuiApplication::palette().color(QPalette::Base);

    output[0] = static_cast<float>(background.redF());
    output[1] = static_cast<float>(background.greenF());
    output[2] = static_cast<float>(background.blueF());
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
}

PuzzleView::~PuzzleView()
{
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

void PuzzleView::clearGame()
{
    if (!m_frontendState)
        return;
    m_frontendState->timer->stop();
    m_frontendState->clock.invalidate();

    if (m_midend) {
        midend_free(m_midend);
        m_midend = nullptr;
    }
    m_image = {};
    m_colours.clear();
    m_puzzleWidth = 0;
    m_puzzleHeight = 0;
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
}

void PuzzleView::refreshColours()
{
    if (!m_midend)
        return;

    int count = 0;
    float *colours = midend_colours(m_midend, &count);
    m_colours = QVector<float>(colours, colours + count * 3);
    sfree(colours);
}

void PuzzleView::resizePuzzle()
{
    if (!m_midend)
        return;

    int availableWidth = std::max(1, qRound(width()));
    int availableHeight = std::max(1, qRound(height()));
    int puzzleWidth = availableWidth;
    int puzzleHeight = availableHeight;
    midend_size(m_midend, &puzzleWidth, &puzzleHeight, false, 1.0);

    m_puzzleWidth = puzzleWidth;
    m_puzzleHeight = puzzleHeight;
    m_image = QImage(m_puzzleWidth, m_puzzleHeight,
                     QImage::Format_ARGB32_Premultiplied);
    m_image.fill(Qt::transparent);
    midend_redraw(m_midend);
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

void PuzzleView::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    if (m_image.isNull())
        return;

    const qreal x = (width() - m_image.width()) / 2.0;
    const qreal y = (height() - m_image.height()) / 2.0;
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
    if (fillColour >= 0)
        state->painter->setBrush(colourFor(view, fillColour));
    else
        state->painter->setBrush(Qt::NoBrush);
    if (outlineColour >= 0)
        state->painter->setPen(QPen(colourFor(view, outlineColour)));
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
    if (fillColour >= 0)
        state->painter->setBrush(colourFor(view, fillColour));
    else
        state->painter->setBrush(Qt::NoBrush);
    if (outlineColour >= 0)
        state->painter->setPen(QPen(colourFor(view, outlineColour)));
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
        blitter->image = view->m_image.copy(x, y, blitter->width,
                                            blitter->height);
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
