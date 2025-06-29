import QtQuick 2.14
import QtQuick.Controls 2.14

ToolBar {
    Row {
        ToolButton {
            text: "File"
            onClicked: fileMenu.open()
            Menu {
                id: fileMenu
                x: parent.x // 确保菜单在按钮下方对齐
                y: parent.height // 确保菜单在按钮下方弹出
                MenuItem { text: "Open"; onClicked: toolbarBackend.openFile() }
            }
        }
        ToolButton {
            text: "View"
            onClicked: viewMenu.open()
            Menu {
                id: viewMenu
                x: parent.x
                y: parent.height
                MenuItem { text: "Fit"; onClicked: toolbarBackend.viewFit() }
            }
        }
        ToolButton {
            text: "Analysis"
            onClicked: analysisMenu.open()
            Menu {
                id: analysisMenu
                x: parent.x
                y: parent.height
                MenuItem { text: "Interference"; onClicked: toolbarBackend.checkInterference() }
            }
        }
        ToolButton { text: "transform"; onClicked: toolbarBackend.transform() }
        ToolButton { text: "clipping"; onClicked: toolbarBackend.clipping() }
        ToolButton { text: "explosion"; onClicked: toolbarBackend.explosion() }
    }
}
