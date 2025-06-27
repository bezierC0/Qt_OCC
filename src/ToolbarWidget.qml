import QtQuick 2.14
import QtQuick.Controls 2.14

ToolBar {
    Row {
        ToolButton {
            text: "File"
            onClicked: fileMenu.open()
            Menu {
                id: fileMenu
                MenuItem { text: "Open"; onTriggered: toolbarBackend.openFile() }
            }
        }
        ToolButton {
            text: "View"
            onClicked: viewMenu.open()
            Menu {
                id: viewMenu
                MenuItem { text: "Fit"; onTriggered: toolbarBackend.viewFit() }
            }
        }
        ToolButton {
            text: "Analysis"
            onClicked: analysisMenu.open()
            Menu {
                id: analysisMenu
                MenuItem { text: "Interference"; onTriggered: toolbarBackend.checkInterference() }
            }
        }
        ToolButton { text: "transform"; onClicked: toolbarBackend.transform() }
        ToolButton { text: "clipping"; onClicked: toolbarBackend.clipping() }
        ToolButton { text: "explosion"; onClicked: toolbarBackend.explosion() }
    }
} 
