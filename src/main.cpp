#include "PuzzleCatalog.h"
#include "PuzzleView.h"

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char **argv)
{
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE"))
        qputenv("QT_QUICK_CONTROLS_STYLE", "org.kde.desktop");

    QGuiApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("kpuzzles"));
    application.setApplicationDisplayName(QStringLiteral("KPuzzles"));
    application.setOrganizationDomain(QStringLiteral("kde.org"));
    application.setDesktopFileName(QStringLiteral("org.kde.kpuzzles"));
    application.setWindowIcon(QIcon::fromTheme(
        QStringLiteral("org.kde.kpuzzles"),
        QIcon(QStringLiteral(":/icons/org.kde.kpuzzles.svg"))));

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
