import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import kpuzzles 1.0

Kirigami.Page {
    id: root

    property string gameName
    property string displayName
    signal leavePuzzle()
    title: root.displayName

    actions: [
        Kirigami.Action {
            text: qsTr("New game")
            icon.name: "document-new"
            onTriggered: puzzleView.newGame()
        },
        Kirigami.Action {
            text: qsTr("Restart")
            icon.name: "view-refresh"
            onTriggered: puzzleView.restartGame()
        },
        Kirigami.Action {
            text: qsTr("Undo")
            icon.name: "edit-undo"
            enabled: puzzleView.canUndo
            onTriggered: puzzleView.undo()
        },
        Kirigami.Action {
            text: qsTr("Redo")
            icon.name: "edit-redo"
            enabled: puzzleView.canRedo
            onTriggered: puzzleView.redo()
        },
        Kirigami.Action {
            text: qsTr("Solve")
            icon.name: "tools-solve"
            enabled: puzzleView.canSolve && !puzzleView.solved
            onTriggered: puzzleView.solve()
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true

            Controls.Label {
                Layout.fillWidth: true
                text: puzzleView.statusText.length > 0
                      ? puzzleView.statusText
                      : qsTr("Use the mouse, touch, or keyboard to play.")
                color: puzzleView.solved
                       ? Kirigami.Theme.positiveTextColor
                       : Kirigami.Theme.disabledTextColor
                elide: Text.ElideRight
            }

            Controls.Button {
                text: qsTr("Puzzle list")
                icon.name: "go-previous"
                onClicked: root.leavePuzzle()
            }
        }

        PuzzleView {
            id: puzzleView
            Layout.fillWidth: true
            Layout.fillHeight: true
            gameName: root.gameName
            focus: true
            onQuitRequested: root.leavePuzzle()
        }
    }
}
