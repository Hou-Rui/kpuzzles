pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    property bool sessionTrackingReady: false
    property Item activePuzzlePage: null
    property string activePuzzleName

    width: 1080
    height: 750
    visible: true
    title: qsTr("Simon Tatham's Portable Puzzle Collection")

    pageStack.initialPage: homePage

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
