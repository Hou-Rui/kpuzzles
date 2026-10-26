pragma ComponentBehavior: Bound

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
            checked: presetMenu && presetMenu.puzzlePage ? presetMenu.puzzlePage.currentPresetId === presetId : false
            group: targetGroup
            onTriggered: {
                if (presetMenu && presetMenu.puzzlePage)
                    presetMenu.puzzlePage.selectPreset(presetId);
            }
        }
    }

    Instantiator {
        id: presetInstantiator
        model: root.entries

        // The factory owns the native item it creates; the platform menu only
        // holds a non-owning reference (removeItem/removeMenu never delete it),
        // so the underlying widget QMenu is freed exactly once, at teardown.
        delegate: QtObject {
            id: factory

            required property int index
            required property var modelData
            readonly property bool isSubmenu: modelData.children !== undefined
            property QtObject menuObject: null

            Component.onCompleted: {
                if (factory.isSubmenu) {
                    factory.menuObject = Qt.createComponent(Qt.resolvedUrl("NativePresetMenu.qml")).createObject(factory);
                    factory.menuObject.title = factory.modelData.title;
                    root.insertMenu(factory.index, factory.menuObject);
                    factory.menuObject.puzzlePage = Qt.binding(function () {
                        return root.puzzlePage;
                    });
                    factory.menuObject.entries = factory.modelData.children;
                } else {
                    factory.menuObject = presetItemComponent.createObject(factory, {
                        presetId: factory.modelData.id,
                        presetTitle: factory.modelData.title,
                        presetMenu: root,
                        targetGroup: presetGroup
                    });
                    root.insertItem(factory.index, factory.menuObject);
                }
            }

            Component.onDestruction: {
                const item = factory.menuObject;
                if (!item)
                    return;
                factory.menuObject = null;
                if (factory.isSubmenu)
                    root.removeMenu(item);
                else
                    root.removeItem(item);
            }
        }
    }
}
