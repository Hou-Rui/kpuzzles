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
            required property bool favorite
            required property string thumbnail

            contentItem: Item {
                implicitWidth: contentLayout.implicitWidth
                implicitHeight: contentLayout.implicitHeight
                clip: true

                Item {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    width: Math.min(parent.width, height / 0.3)
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: card.thumbnail
                        sourceSize: Qt.size(320, 320)
                        sourceClipRect: Qt.rect(0, 0, 320, 96)
                        fillMode: Image.PreserveAspectCrop
                        horizontalAlignment: Image.AlignRight
                        verticalAlignment: Image.AlignTop
                        opacity: 0.1
                        asynchronous: true
                        smooth: true
                    }
                }

                ColumnLayout {
                    id: contentLayout
                    anchors.fill: parent
                    spacing: Kirigami.Units.smallSpacing
                    
                    RowLayout {
                        Kirigami.Heading {
                            Layout.fillWidth: true
                            level: 2
                            text: card.displayName
                        }
                        Controls.Label {
                            text: card.description
                            color: Kirigami.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }
                    
                    Kirigami.Separator {
                        Layout.fillWidth: true
                    }

                    Controls.Label {
                        Layout.fillWidth: true
                        text: card.objective
                        wrapMode: Text.WordWrap
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignRight

                        Controls.ToolButton {
                            text: card.favorite
                                ? qsTr("Remove from Favorites")
                                : qsTr("Add to Favorites")
                            display: Controls.AbstractButton.IconOnly
                            icon.name: card.favorite
                                ? "starred-symbolic"
                                : "non-starred-symbolic"
                            onClicked: puzzleCatalog.toggleFavorite(card.name)

                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.text: text
                        }

                        Controls.Button {
                            text: qsTr("Play")
                            icon.name: "media-playback-start"
                            onClicked: root.openPuzzle(card.name, card.displayName)
                        }
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
