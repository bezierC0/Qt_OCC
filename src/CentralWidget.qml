import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Controls 1.4

SplitView {
    anchors.fill: parent

    // 左侧树
    TreeView {
        id: modelTree
        width: 200
        model: ListModel {
            ListElement { name: "node1" }
            ListElement { name: "node2" }
            ListElement { name: "node3" }
        }
        TableViewColumn { role: "name"; title: "name"; width: 180 }
    }

    // 右侧视图区
    Rectangle {
        color: "#222"
        anchors.fill: parent
        // 后续替换为OpenGL视图
        Text {
            anchors.centerIn: parent
            color: "white"
            text: "ViewerWidget (QML)"
        }
    }
} 