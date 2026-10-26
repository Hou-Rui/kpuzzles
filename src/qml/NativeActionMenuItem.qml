import Qt.labs.platform as Platform

Platform.MenuItem {
    id: root

    property var sourceAction: null

    text: sourceAction ? sourceAction.text : ""
    enabled: sourceAction ? sourceAction.enabled : false
    checkable: sourceAction ? sourceAction.checkable : false
    checked: sourceAction ? sourceAction.checked : false
    icon.name: sourceAction ? sourceAction.icon.name : ""

    onTriggered: {
        if (sourceAction)
            sourceAction.trigger(root)
    }
}
