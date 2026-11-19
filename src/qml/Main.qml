pragma ComponentBehavior: Bound

import QtQuick
import Qt.labs.platform as Platform
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    property PuzzlePage activePuzzlePage: null
    readonly property bool puzzleActive: root.pageStack.currentItem === root.activePuzzlePage

    width: 1080
    height: 750
    visible: true
    title: qsTr("Simon Tatham's Portable Puzzle Collection")

    pageStack.initialPage: homePage

    component NativeActionMenuItem: Platform.MenuItem {
        id: menuItem

        property Kirigami.Action sourceAction: null

        text: sourceAction ? sourceAction.text : ""
        enabled: sourceAction ? sourceAction.enabled : false
        checkable: sourceAction ? sourceAction.checkable : false
        checked: sourceAction ? sourceAction.checked : false
        icon.name: sourceAction ? sourceAction.icon.name : ""
        shortcut: sourceAction ? sourceAction.shortcut : ""

        onTriggered: {
            if (sourceAction)
                sourceAction.trigger(menuItem);
        }
    }

    readonly property list<Kirigami.Action> puzzleActions: [
        Kirigami.Action {
            id: newGameAction
            text: qsTr("New game")
            icon.name: "document-new"
            shortcut: StandardKey.New
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: root.puzzleActive
            onTriggered: root.activePuzzlePage.puzzleView.newGame()
        },
        Kirigami.Action {
            id: restartAction
            text: qsTr("Restart")
            icon.name: "view-refresh"
            shortcut: StandardKey.Refresh
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: root.puzzleActive
            onTriggered: root.activePuzzlePage.puzzleView.restartGame()
        },
        Kirigami.Action {
            id: presetsAction
            text: qsTr("Presets")
            icon.name: "view-list-details"
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: root.puzzleActive && root.activePuzzlePage.puzzleView.presetEntries.length > 0
            onTriggered: root.activePuzzlePage.presetsDialog.open()
        },
        Kirigami.Action {
            id: customSettingsAction
            text: qsTr("Custom settings")
            icon.name: "configure"
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: root.puzzleActive && root.activePuzzlePage.puzzleView.canConfigure
            onTriggered: {
                root.activePuzzlePage.puzzleView.beginConfiguration();
                root.activePuzzlePage.configurationDialog.open();
            }
        },
        Kirigami.Action {
            id: undoAction
            text: qsTr("Undo")
            icon.name: "edit-undo"
            shortcut: StandardKey.Undo
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: root.puzzleActive && root.activePuzzlePage.puzzleView.canUndo
            onTriggered: root.activePuzzlePage.puzzleView.undo()
        },
        Kirigami.Action {
            id: redoAction
            text: qsTr("Redo")
            icon.name: "edit-redo"
            shortcut: StandardKey.Redo
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: root.puzzleActive && root.activePuzzlePage.puzzleView.canRedo
            onTriggered: root.activePuzzlePage.puzzleView.redo()
        },
        Kirigami.Action {
            id: solveAction
            text: qsTr("Solve")
            icon.name: "tools-solve"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: root.puzzleActive && root.activePuzzlePage.puzzleView.canSolve && !root.activePuzzlePage.puzzleView.solved
            onTriggered: root.activePuzzlePage.puzzleView.solve()
        },
        Kirigami.Action {
            id: helpAction
            text: qsTr("Help")
            icon.name: "help-contents"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: root.puzzleActive && root.activePuzzlePage.puzzleView.helpAvailable
            onTriggered: root.activePuzzlePage.helpDialog.open()
        },
        Kirigami.Action {
            id: gameIdAction
            text: qsTr("Game ID")
            icon.name: "document-edit"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: root.puzzleActive
            onTriggered: {
                root.activePuzzlePage.puzzleView.beginGameIdConfiguration();
                root.activePuzzlePage.configurationDialog.open();
            }
        },
        Kirigami.Action {
            id: randomSeedAction
            text: qsTr("Random seed")
            icon.name: "view-refresh"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: root.puzzleActive
            onTriggered: {
                root.activePuzzlePage.puzzleView.beginRandomSeedConfiguration();
                root.activePuzzlePage.configurationDialog.open();
            }
        }
    ]

    // This is deliberately not assigned to ApplicationWindow.menuBar. On
    // Linux it is exported through the platform's global D-Bus menu without
    // taking up space inside the window.
    Platform.MenuBar {
        window: root

        Platform.Menu {
            title: qsTr("&Game")

            NativeActionMenuItem {
                sourceAction: newGameAction
            }
            NativeActionMenuItem {
                sourceAction: restartAction
            }
            NativeActionMenuItem {
                sourceAction: solveAction
            }

            Platform.MenuSeparator {}

            NativeActionMenuItem {
                sourceAction: presetsAction
            }
            NativeActionMenuItem {
                sourceAction: customSettingsAction
            }
            NativeActionMenuItem {
                sourceAction: gameIdAction
            }
            NativeActionMenuItem {
                sourceAction: randomSeedAction
            }

            Platform.MenuSeparator {}

            Platform.MenuItem {
                text: qsTr("&Quit")
                icon.name: "application-exit"
                shortcut: StandardKey.Quit
                role: Platform.MenuItem.QuitRole
                onTriggered: Qt.quit()
            }
        }

        Platform.Menu {
            title: qsTr("&Edit")

            NativeActionMenuItem {
                sourceAction: undoAction
            }
            NativeActionMenuItem {
                sourceAction: redoAction
            }
        }

        Platform.Menu {
            title: qsTr("&Help")

            NativeActionMenuItem {
                sourceAction: helpAction
            }
        }
    }

    function openPuzzle(name, displayName) {
        const page = root.pageStack.push(puzzlePageComponent, {
            gameName: name,
            displayName: displayName,
            actions: root.puzzleActions
        });
        if (page) {
            root.activePuzzlePage = page;
            // Remember the game for the next launch's session restore.  The
            // session game is never cleared, so quitting on the home page
            // still restores the last opened game.
            sessionState.setActiveGame(name);
        }
    }

    Component.onCompleted: {
        if (sessionState.restoreLastGameEnabled && sessionState.lastGameName.length > 0) {
            const displayName = puzzleCatalog.displayNameForGame(sessionState.lastGameName);
            if (displayName.length > 0)
                root.openPuzzle(sessionState.lastGameName, displayName);
        }
    }

    HomePage {
        id: homePage
        visible: false

        onOpenPuzzle: function (name, displayName) {
            root.openPuzzle(name, displayName);
        }
    }

    Component {
        id: puzzlePageComponent

        PuzzlePage {
            onLeavePuzzle: root.pageStack.pop()
        }
    }
}
