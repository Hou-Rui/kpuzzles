pragma ComponentBehavior: Bound

import QtQuick
import Qt.labs.platform as Platform
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    property bool sessionTrackingReady: false
    property Item activePuzzlePage: null
    property string activePuzzleName
    readonly property bool puzzleActive:
        root.pageStack.currentItem === root.activePuzzlePage

    width: 1080
    height: 750
    visible: true
    title: qsTr("Simon Tatham's Portable Puzzle Collection")

    pageStack.initialPage: homePage

    // This is deliberately not assigned to ApplicationWindow.menuBar. On
    // Linux it is exported through the platform's global D-Bus menu without
    // taking up space inside the window.
    Platform.MenuBar {
        window: root

        Platform.Menu {
            title: qsTr("&File")

            Platform.MenuItem {
                text: qsTr("Back to puzzle collection")
                icon.name: "go-previous"
                enabled: root.puzzleActive
                onTriggered: root.pageStack.pop()
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
            title: qsTr("&Game")
            enabled: root.puzzleActive

            NativeActionMenuItem {
                sourceAction: root.puzzleActive
                    ? root.activePuzzlePage.newGameAction : null
            }
            NativeActionMenuItem {
                sourceAction: root.puzzleActive
                    ? root.activePuzzlePage.restartAction : null
            }
            NativeActionMenuItem {
                sourceAction: root.puzzleActive
                    ? root.activePuzzlePage.solveAction : null
            }

            Platform.MenuSeparator {}

            NativePresetMenu {
                title: qsTr("Presets")
                entries: root.puzzleActive
                    ? root.activePuzzlePage.presetMenuEntries : []
                puzzlePage: root.puzzleActive ? root.activePuzzlePage : null
                enabled: entries.length > 0
            }
            NativeActionMenuItem {
                sourceAction: root.puzzleActive
                    ? root.activePuzzlePage.customSettingsAction : null
            }
            NativeActionMenuItem {
                sourceAction: root.puzzleActive
                    ? root.activePuzzlePage.gameIdAction : null
            }
            NativeActionMenuItem {
                sourceAction: root.puzzleActive
                    ? root.activePuzzlePage.randomSeedAction : null
            }
        }

        Platform.Menu {
            title: qsTr("&Edit")
            enabled: root.puzzleActive

            NativeActionMenuItem {
                sourceAction: root.puzzleActive
                    ? root.activePuzzlePage.undoAction : null
            }
            NativeActionMenuItem {
                sourceAction: root.puzzleActive
                    ? root.activePuzzlePage.redoAction : null
            }
        }

        Platform.Menu {
            title: qsTr("&Help")
            enabled: root.puzzleActive

            NativeActionMenuItem {
                sourceAction: root.puzzleActive
                    ? root.activePuzzlePage.helpAction : null
            }
        }
    }

    function openPuzzle(name, displayName) {
        const page = puzzlePageComponent.createObject(root.contentItem, {
            gameName: name,
            displayName: displayName
        })
        if (page) {
            root.activePuzzlePage = page
            root.activePuzzleName = name
            root.pageStack.push(page)
        }
    }

    function updateActiveGame() {
        if (!root.sessionTrackingReady)
            return

        const currentPage = root.pageStack.currentItem
        if (currentPage === root.activePuzzlePage)
            sessionState.setActiveGame(root.activePuzzleName)
        else if (currentPage === homePage)
            sessionState.clearActiveGame()
    }

    Component.onCompleted: {
        if (sessionState.restoreLastGameEnabled
                && sessionState.lastGameName.length > 0) {
            const displayName = puzzleCatalog.displayNameForGame(
                sessionState.lastGameName)
            if (displayName.length > 0)
                root.openPuzzle(sessionState.lastGameName, displayName)
        }
        root.sessionTrackingReady = true
        root.updateActiveGame()
    }

    Connections {
        target: root.pageStack
        function onCurrentItemChanged() { root.updateActiveGame() }
    }

    HomePage {
        id: homePage
        visible: false

        onOpenPuzzle: function(name, displayName) {
            root.openPuzzle(name, displayName)
        }
    }

    Component {
        id: puzzlePageComponent

        PuzzlePage {
            onLeavePuzzle: root.pageStack.pop()
        }
    }
}
