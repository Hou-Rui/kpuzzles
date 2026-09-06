import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    width: 960
    height: 720
    visible: true
    title: qsTr("K Puzzles")

    pageStack.initialPage: homePage

    Component {
        id: homePage

        HomePage {
            onOpenPuzzle: function(name, displayName) {
                root.pageStack.push(puzzlePage, {
                    gameName: name,
                    displayName: displayName
                })
            }
        }
    }

    Component {
        id: puzzlePage

        PuzzlePage {
            onLeavePuzzle: root.pageStack.pop()
        }
    }
}
