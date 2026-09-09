#pragma once

#include <QImage>
#include <QMetaObject>
#include <QQuickPaintedItem>
#include <QString>
#include <QVector>
#include <QVariantList>

extern "C" {
#include "puzzles.h"
}

class QKeyEvent;
class QMouseEvent;
class QQuickWindow;
class QTimer;
class QColor;
class QEvent;

class PuzzleView : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString gameName READ gameName WRITE setGameName NOTIFY gameNameChanged)
    Q_PROPERTY(QString helpText READ helpText NOTIFY helpTextChanged)
    Q_PROPERTY(bool helpAvailable READ helpAvailable NOTIFY helpAvailableChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool canSolve READ canSolve NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool solved READ solved NOTIFY solvedChanged)
    Q_PROPERTY(QVariantList presetEntries READ presetEntries NOTIFY menuChanged)
    Q_PROPERTY(bool canConfigure READ canConfigure NOTIFY menuChanged)
    Q_PROPERTY(QVariantList configuration READ configuration NOTIFY configurationChanged)
    Q_PROPERTY(QString configurationTitle READ configurationTitle NOTIFY configurationTitleChanged)
    Q_PROPERTY(QString configurationError READ configurationError NOTIFY configurationErrorChanged)

public:
    struct FrontendState;

    explicit PuzzleView(QQuickItem *parent = nullptr);
    ~PuzzleView() override;

    QString gameName() const { return m_gameName; }
    void setGameName(const QString &name);
    QString helpText() const { return m_helpText; }
    bool helpAvailable() const { return m_helpAvailable; }
    QString statusText() const { return m_statusText; }
    bool canUndo() const { return m_canUndo; }
    bool canRedo() const { return m_canRedo; }
    bool canSolve() const { return m_canSolve; }
    bool solved() const { return m_solved; }
    QVariantList presetEntries() const { return m_presetEntries; }
    bool canConfigure() const { return m_canConfigure; }
    QVariantList configuration() const { return m_configuration; }
    QString configurationTitle() const { return m_configurationTitle; }
    QString configurationError() const { return m_configurationError; }

    Q_INVOKABLE void newGame();
    Q_INVOKABLE void restartGame();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void solve();
    Q_INVOKABLE void selectPreset(int id);
    Q_INVOKABLE void beginConfiguration();
    Q_INVOKABLE void beginGameIdConfiguration();
    Q_INVOKABLE void beginRandomSeedConfiguration();
    Q_INVOKABLE void setConfigurationValue(int index, const QVariant &value);
    Q_INVOKABLE bool applyConfiguration();
    Q_INVOKABLE void cancelConfiguration();

    // Called by the platform callbacks in the puzzles C API.
    void backendActivateTimer();
    void backendDeactivateTimer();
    static FrontendState *stateFrom(frontend *frontend);

signals:
    void gameNameChanged();
    void helpTextChanged();
    void helpAvailableChanged();
    void statusTextChanged();
    void capabilitiesChanged();
    void solvedChanged();
    void menuChanged();
    void configurationChanged();
    void configurationTitleChanged();
    void configurationErrorChanged();
    void quitRequested();

protected:
    void paint(QPainter *painter) override;
    void geometryChange(const QRectF &newGeometry,
                        const QRectF &oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void clearGame();
    void loadGame(const game *selectedGame);
    void resizePuzzle();
    void rebuildImage();
    qreal devicePixelRatio() const;
    void refreshMenuData();
    void refreshHelpText();
    void appendPresetEntries(const preset_menu *menu, const QString &prefix,
                             int currentPreset);
    void beginConfiguration(int which);
    void clearConfiguration();
    void rebuildConfigurationModel();
    void setConfigurationError(const QString &error);
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
    QString m_helpText;
    bool m_helpAvailable = false;
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
    QVariantList m_presetEntries;
    bool m_canConfigure = false;
    QVariantList m_configuration;
    QString m_configurationTitle;
    QString m_configurationError;
    int m_configurationKind = CFG_SETTINGS;
    config_item *m_pendingConfiguration = nullptr;
    qreal m_devicePixelRatio = 1.0;
    QQuickWindow *m_observedWindow = nullptr;
    QMetaObject::Connection m_screenChangedConnection;
    midend *m_midend = nullptr;
    FrontendState *m_frontendState = nullptr;

    static const drawing_api s_drawingApi;
};
