pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    width: 1080
    height: 720
    visible: true
    title: qsTr("Simon Tatham's Portable Puzzle Collection")

    pageStack.initialPage: homePage

    HomePage {
        id: homePage
        visible: false

        onOpenPuzzle: function(name, displayName) {
            const page = puzzlePageComponent.createObject(root.contentItem, {
                gameName: name,
                displayName: displayName
            })
            if (page)
                root.pageStack.push(page)
        }
    }

    Component {
        id: puzzlePageComponent

        PuzzlePage {
            onLeavePuzzle: root.pageStack.pop()
        }
    }
}
