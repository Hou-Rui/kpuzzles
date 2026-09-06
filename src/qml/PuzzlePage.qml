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
        },
        Kirigami.Action {
            text: qsTr("Help")
            icon.name: "help-contents"
            enabled: puzzleView.helpAvailable
            onTriggered: helpDialog.open()
        },
        Kirigami.Action {
            text: qsTr("Zoom in")
            icon.name: "zoom-in"
            onTriggered: puzzleView.zoomIn()
        },
        Kirigami.Action {
            text: qsTr("Zoom out")
            icon.name: "zoom-out"
            onTriggered: puzzleView.zoomOut()
        },
        Kirigami.Action {
            text: qsTr("Reset zoom")
            icon.name: "zoom-original"
            onTriggered: puzzleView.resetZoom()
        }
    ]

    Controls.Dialog {
        id: helpDialog
        modal: true
        title: qsTr("How to play %1").arg(root.displayName)
        width: Math.min(root.width - Kirigami.Units.largeSpacing * 2, 700)
        standardButtons: Controls.Dialog.Close

        contentItem: Controls.ScrollView {
            id: helpScroll
            implicitWidth: 640
            implicitHeight: Math.min(helpContent.implicitHeight, 520)
            clip: true
            contentWidth: availableWidth
            contentHeight: helpContent.implicitHeight

            Text {
                id: helpContent
                width: helpScroll.availableWidth
                text: puzzleView.helpText
                textFormat: Text.RichText
                wrapMode: Text.WordWrap
                color: Kirigami.Theme.textColor
                renderType: Text.NativeRendering
            }
        }
    }

    Controls.Dialog {
        id: presetsDialog
        modal: true
        title: qsTr("Presets")
        width: Math.min(root.width - Kirigami.Units.largeSpacing * 2, 620)
        standardButtons: Controls.Dialog.Cancel

        contentItem: ListView {
            id: presetList
            implicitWidth: 560
            implicitHeight: Math.min(contentHeight, 520)
            clip: true
            model: puzzleView.presetEntries

            delegate: Controls.RadioDelegate {
                required property var modelData
                property int presetId: modelData.id

                width: presetList.width
                text: modelData.label
                checked: modelData.selected
                onClicked: {
                    puzzleView.selectPreset(presetId)
                    presetsDialog.close()
                }
            }
        }
    }

    Controls.Dialog {
        id: configurationDialog
        modal: true
        title: puzzleView.configurationTitle
        width: Math.min(root.width - Kirigami.Units.largeSpacing * 2, 620)
        standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel

        onAccepted: {
            if (!puzzleView.applyConfiguration())
                configurationDialog.open()
        }
        onRejected: puzzleView.cancelConfiguration()

        contentItem: Flickable {
            implicitHeight: Math.min(configurationForm.implicitHeight, 520)
            contentWidth: availableWidth
            contentHeight: configurationForm.implicitHeight
            clip: true

            ColumnLayout {
                id: configurationForm
                width: configurationDialog.width - Kirigami.Units.largeSpacing * 2
                spacing: Kirigami.Units.smallSpacing

                Controls.Label {
                    Layout.fillWidth: true
                    visible: puzzleView.configurationError.length > 0
                    text: puzzleView.configurationError
                    color: Kirigami.Theme.negativeTextColor
                    wrapMode: Text.WordWrap
                }

                Repeater {
                    model: puzzleView.configuration

                    delegate: ColumnLayout {
                        required property var modelData
                        property int configurationIndex: index
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Controls.Label {
                            Layout.fillWidth: true
                            text: modelData.name
                            wrapMode: Text.WordWrap
                        }

                        Controls.TextField {
                            Layout.fillWidth: true
                            visible: modelData.type === 0
                            text: modelData.value
                            onEditingFinished: puzzleView.setConfigurationValue(
                                configurationIndex, text)
                        }

                        Controls.ComboBox {
                            Layout.fillWidth: true
                            visible: modelData.type === 1
                            model: modelData.choices
                            currentIndex: modelData.selected
                            onActivated: puzzleView.setConfigurationValue(
                                configurationIndex, currentIndex)
                        }

                        Controls.CheckBox {
                            Layout.fillWidth: true
                            visible: modelData.type === 2
                            checked: modelData.value
                            onToggled: puzzleView.setConfigurationValue(
                                configurationIndex, checked)
                        }
                    }
                }
            }
        }
    }

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

        Controls.ToolBar {
            Layout.fillWidth: true

            contentItem: Kirigami.ActionToolBar {
                actions: [
                    Kirigami.Action {
                        text: qsTr("Presets")
                        icon.name: "view-list-details"
                        enabled: puzzleView.presetEntries.length > 0
                        onTriggered: presetsDialog.open()
                    },
                    Kirigami.Action {
                        text: qsTr("Custom settings")
                        icon.name: "configure"
                        enabled: puzzleView.canConfigure
                        onTriggered: {
                            puzzleView.beginConfiguration()
                            configurationDialog.open()
                        }
                    },
                    Kirigami.Action {
                        text: qsTr("Game ID")
                        icon.name: "document-edit"
                        onTriggered: {
                            puzzleView.beginGameIdConfiguration()
                            configurationDialog.open()
                        }
                    },
                    Kirigami.Action {
                        text: qsTr("Random seed")
                        icon.name: "view-refresh"
                        onTriggered: {
                            puzzleView.beginRandomSeedConfiguration()
                            configurationDialog.open()
                        }
                    }
                ]
                alignment: Qt.AlignLeft
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
