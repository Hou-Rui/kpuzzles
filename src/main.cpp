#include "PuzzleCatalog.h"
#include "PuzzleView.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char **argv)
{
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE"))
        qputenv("QT_QUICK_CONTROLS_STYLE", "org.kde.desktop");

    QGuiApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("kpuzzles"));
    application.setApplicationDisplayName(QStringLiteral("K Puzzles"));
    application.setOrganizationDomain(QStringLiteral("kde.org"));

    qmlRegisterType<PuzzleView>("kpuzzles", 1, 0, "PuzzleView");

    PuzzleCatalog catalog;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("puzzleCatalog"),
                                             &catalog);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &application, [] { QCoreApplication::exit(EXIT_FAILURE); },
                     Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("kpuzzles"), QStringLiteral("Main"));

    return application.exec();
}
