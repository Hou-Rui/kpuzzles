import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: root

    signal openPuzzle(string name, string displayName)

    titleDelegate: Controls.TextField {
        id: searchField

        Layout.fillWidth: true
        placeholderText: qsTr("Search puzzles...")
        text: puzzleCatalog.filterText
        onTextChanged: puzzleCatalog.filterText = text
        Keys.onEscapePressed: {
            clear();
            focus = false;
        }
    }

    Kirigami.CardsListView {
        id: puzzleGrid
        model: puzzleCatalog

        delegate: Kirigami.AbstractCard {
            id: card
            required property string name
            required property string displayName
            required property string description
            required property string objective
            required property bool canSolve

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

                    Item {
                        Layout.fillHeight: true
                    }

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
