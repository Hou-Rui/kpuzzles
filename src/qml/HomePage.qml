import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root

    signal openPuzzle(string name, string displayName)
    title: qsTr("Puzzles")

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: qsTr("Simon Tatham's Portable Puzzle Collection")
            wrapMode: Text.WordWrap
        }

        Controls.Label {
            Layout.fillWidth: true
            text: qsTr("Choose a puzzle to begin. Every game runs in this window.")
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.WordWrap
        }

        Controls.TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Search puzzles…")
            text: puzzleCatalog.filterText
            onTextChanged: puzzleCatalog.filterText = text
            Keys.onEscapePressed: {
                clear()
                focus = false
            }
        }

        Controls.Label {
            Layout.fillWidth: true
            text: qsTr("%1 puzzles").arg(puzzleCatalog.count)
            color: Kirigami.Theme.disabledTextColor
        }

        GridView {
            id: puzzleGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: puzzleCatalog
            cellWidth: Math.max(260, width / Math.max(1, Math.floor(width / 300)))
            cellHeight: 156
            leftMargin: Kirigami.Units.smallSpacing
            rightMargin: Kirigami.Units.smallSpacing
            topMargin: Kirigami.Units.smallSpacing

            delegate: Kirigami.AbstractCard {
                id: card
                required property string name
                required property string displayName
                required property string description
                required property string objective
                required property bool canSolve

                width: puzzleGrid.cellWidth - Kirigami.Units.smallSpacing
                height: puzzleGrid.cellHeight - Kirigami.Units.smallSpacing

                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing

                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.fillHeight: true
                        radius: width / 2
                        color: Kirigami.Theme.highlightColor
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Heading {
                            Layout.fillWidth: true
                            level: 3
                            text: card.displayName
                            elide: Text.ElideRight
                        }

                        Controls.Label {
                            Layout.fillWidth: true
                            text: card.description
                            color: Kirigami.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }

                        Item { Layout.fillHeight: true }

                        Controls.Button {
                            Layout.alignment: Qt.AlignRight
                            text: qsTr("Play")
                            icon.name: "media-playback-start"
                            onClicked: root.openPuzzle(card.name, card.displayName)
                        }
                    }
                }
            }

            Controls.Label {
                anchors.centerIn: parent
                visible: puzzleCatalog.count === 0
                text: qsTr("No puzzles match your search.")
                color: Kirigami.Theme.disabledTextColor
            }
        }
    }
}
