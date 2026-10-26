import QtQml
import QtQml.Models
import Qt.labs.platform as Platform

Platform.Menu {
    id: root

    property var entries: []
    property var puzzlePage: null

    Platform.MenuItemGroup {
        id: presetGroup
        exclusive: true
    }

    Component {
        id: presetItemComponent

        Platform.MenuItem {
            property int presetId: -1
            property string presetTitle
            property var presetMenu: null
            property var targetGroup: null

            text: presetTitle
            checkable: true
            checked: presetMenu && presetMenu.puzzlePage
                ? presetMenu.puzzlePage.currentPresetId === presetId : false
            group: targetGroup
            onTriggered: {
                if (presetMenu && presetMenu.puzzlePage)
                    presetMenu.puzzlePage.selectPreset(presetId)
            }
        }
    }

    Instantiator {
        id: presetInstantiator
        model: root.entries

        delegate: QtObject {
            id: factory

            required property int index
            required property var modelData
            readonly property bool isSubmenu:
                modelData.children !== undefined
            property QtObject menuObject: null

            Component.onCompleted: {
                if (factory.isSubmenu) {
                    const submenuComponent = Qt.createComponent(
                        Qt.resolvedUrl("NativePresetMenu.qml"))
                    const childEntries = factory.modelData.children
                    factory.menuObject = submenuComponent.createObject(
                        factory, {
                            title: factory.modelData.title,
                            entries: [],
                            puzzlePage: Qt.binding(function() {
                                return root.puzzlePage
                            })
                        })
                    root.insertMenu(factory.index, factory.menuObject)
                    Qt.callLater(function() {
                        if (factory.menuObject)
                            factory.menuObject.entries = childEntries
                    })
                } else {
                    factory.menuObject = presetItemComponent.createObject(
                        factory, {
                            presetId: factory.modelData.id,
                            presetTitle: factory.modelData.title,
                            presetMenu: root,
                            targetGroup: presetGroup
                        })
                    root.insertItem(factory.index, factory.menuObject)
                }
            }

            Component.onDestruction: {
                if (!factory.menuObject)
                    return
                if (factory.isSubmenu)
                    root.removeMenu(factory.menuObject)
                else
                    root.removeItem(factory.menuObject)
            }
        }
    }
}
