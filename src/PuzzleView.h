#pragma once

#include <QImage>
#include <QQuickPaintedItem>
#include <QString>
#include <QVector>

extern "C" {
#include "puzzles.h"
}

class QKeyEvent;
class QMouseEvent;
class QTimer;
class QColor;

class PuzzleView : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString gameName READ gameName WRITE setGameName NOTIFY gameNameChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool canSolve READ canSolve NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool solved READ solved NOTIFY solvedChanged)

public:
    struct FrontendState;

    explicit PuzzleView(QQuickItem *parent = nullptr);
    ~PuzzleView() override;

    QString gameName() const { return m_gameName; }
    void setGameName(const QString &name);
    QString statusText() const { return m_statusText; }
    bool canUndo() const { return m_canUndo; }
    bool canRedo() const { return m_canRedo; }
    bool canSolve() const { return m_canSolve; }
    bool solved() const { return m_solved; }

    Q_INVOKABLE void newGame();
    Q_INVOKABLE void restartGame();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void solve();

    // Called by the platform callbacks in the puzzles C API.
    void backendActivateTimer();
    void backendDeactivateTimer();
    static FrontendState *stateFrom(frontend *frontend);

signals:
    void gameNameChanged();
    void statusTextChanged();
    void capabilitiesChanged();
    void solvedChanged();
    void quitRequested();

protected:
    void paint(QPainter *painter) override;
    void geometryChange(const QRectF &newGeometry,
                        const QRectF &oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void clearGame();
    void loadGame(const game *selectedGame);
    void resizePuzzle();
    void refreshColours();
    void refreshCapabilities();
    void processPointer(const QPointF &position, int button);
    int pointerX(const QPointF &position) const;
    int pointerY(const QPointF &position) const;
    void handleResult(int result);

    static QColor colourFor(const PuzzleView *view, int colour);
    static PuzzleView *viewFrom(drawing *drawing);

    static void drawText(drawing *, int, int, int, int, int, int, const char *);
    static void drawRect(drawing *, int, int, int, int, int);
    static void drawLine(drawing *, int, int, int, int, int);
    static void drawPolygon(drawing *, const int *, int, int, int);
    static void drawCircle(drawing *, int, int, int, int, int);
    static void drawUpdate(drawing *, int, int, int, int);
    static void clip(drawing *, int, int, int, int);
    static void unclip(drawing *);
    static void startDraw(drawing *);
    static void endDraw(drawing *);
    static void statusBar(drawing *, const char *);
    static blitter *blitterNew(drawing *, int, int);
    static void blitterFree(drawing *, blitter *);
    static void blitterSave(drawing *, blitter *, int, int);
    static void blitterLoad(drawing *, blitter *, int, int);
    static char *textFallback(drawing *, const char *const *, int);
    static void drawThickLine(drawing *, float, float, float, float, float, int);

    QString m_gameName;
    QString m_statusText;
    bool m_canUndo = false;
    bool m_canRedo = false;
    bool m_canSolve = false;
    bool m_solved = false;
    int m_puzzleWidth = 0;
    int m_puzzleHeight = 0;
    int m_pressedButton = 0;
    QImage m_image;
    QVector<float> m_colours;
    midend *m_midend = nullptr;
    FrontendState *m_frontendState = nullptr;

    static const drawing_api s_drawingApi;
};
