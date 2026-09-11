import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import kpuzzles

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
            displayHint: Kirigami.DisplayHint.KeepVisible
            onTriggered: puzzleView.newGame()
        },
        Kirigami.Action {
            text: qsTr("Restart")
            icon.name: "view-refresh"
            displayHint: Kirigami.DisplayHint.KeepVisible
            onTriggered: puzzleView.restartGame()
        },
        Kirigami.Action {
            text: qsTr("Presets")
            icon.name: "view-list-details"
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: puzzleView.presetEntries.length > 0
            onTriggered: presetsDialog.open()
        },
        Kirigami.Action {
            text: qsTr("Custom settings")
            icon.name: "configure"
            displayHint: Kirigami.DisplayHint.KeepVisible
            enabled: puzzleView.canConfigure
            onTriggered: {
                puzzleView.beginConfiguration()
                configurationDialog.open()
            }
        },
        Kirigami.Action {
            text: qsTr("Undo")
            icon.name: "edit-undo"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: puzzleView.canUndo
            onTriggered: puzzleView.undo()
        },
        Kirigami.Action {
            text: qsTr("Redo")
            icon.name: "edit-redo"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: puzzleView.canRedo
            onTriggered: puzzleView.redo()
        },
        Kirigami.Action {
            text: qsTr("Solve")
            icon.name: "tools-solve"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: puzzleView.canSolve && !puzzleView.solved
            onTriggered: puzzleView.solve()
        },
        Kirigami.Action {
            text: qsTr("Help")
            icon.name: "help-contents"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            enabled: puzzleView.helpAvailable
            onTriggered: helpDialog.open()
        },
        Kirigami.Action {
            text: qsTr("Game ID")
            icon.name: "document-edit"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: {
                puzzleView.beginGameIdConfiguration()
                configurationDialog.open()
            }
        },
        Kirigami.Action {
            text: qsTr("Random seed")
            icon.name: "view-refresh"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: {
                puzzleView.beginRandomSeedConfiguration()
                configurationDialog.open()
            }
        }
    ]

    Kirigami.Dialog {
        id: helpDialog
        modal: true
        title: qsTr("How to play %1").arg(root.displayName)
        padding: Kirigami.Units.largeSpacing
        preferredWidth: 700
        maximumWidth: 700
        standardButtons: Kirigami.Dialog.Close

        Text {
            id: helpContent
            text: puzzleView.helpText
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            color: Kirigami.Theme.textColor
            renderType: Text.NativeRendering
        }
    }

    Kirigami.Dialog {
        id: presetsDialog
        modal: true
        title: qsTr("Presets")
        padding: 0
        preferredWidth: 620
        maximumWidth: 620
        standardButtons: Kirigami.Dialog.Cancel

        ListView {
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

    Kirigami.Dialog {
        id: configurationDialog
        modal: true
        title: puzzleView.configurationTitle
        padding: Kirigami.Units.largeSpacing
        preferredWidth: 620
        maximumWidth: 620
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

        onAccepted: {
            if (!puzzleView.applyConfiguration())
                configurationDialog.open()
        }
        onRejected: puzzleView.cancelConfiguration()

        ColumnLayout {
            id: configurationForm
            implicitWidth: 560
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
                    id: configurationEntry

                    required property int index
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Label {
                        Layout.fillWidth: true
                        text: configurationEntry.modelData.name
                        wrapMode: Text.WordWrap
                    }

                    Controls.TextField {
                        Layout.fillWidth: true
                        visible: configurationEntry.modelData.type === 0
                        text: configurationEntry.modelData.value
                        onEditingFinished: puzzleView.setConfigurationValue(
                            configurationEntry.index, text)
                    }

                    Controls.ComboBox {
                        Layout.fillWidth: true
                        visible: configurationEntry.modelData.type === 1
                        model: configurationEntry.modelData.choices
                        currentIndex: configurationEntry.modelData.type === 1
                            ? configurationEntry.modelData.selected : -1
                        onActivated: puzzleView.setConfigurationValue(
                            configurationEntry.index, currentIndex)
                    }

                    Controls.CheckBox {
                        Layout.fillWidth: true
                        visible: configurationEntry.modelData.type === 2
                        checked: configurationEntry.modelData.value
                        onToggled: puzzleView.setConfigurationValue(
                            configurationEntry.index, checked)
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        PuzzleView {
            id: puzzleView
            Layout.fillWidth: true
            Layout.fillHeight: true
            gameName: root.gameName
            focus: true
            onQuitRequested: root.leavePuzzle()
        }

        Controls.Label {
            Layout.fillWidth: true
            text: (puzzleView.solved && puzzleView.statusText.length === 0)
                  ? qsTr("COMPLETED!")
                  : puzzleView.statusText
            color: puzzleView.solved
                   ? Kirigami.Theme.positiveTextColor
                   : Kirigami.Theme.textColor
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }
}
