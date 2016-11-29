#include "MainWindow.h"
#include "ui_mainwindow.h"

#include "SimUtilities.h"
#include "State.h"

namespace mms {

MainWindow::MainWindow(mms::View* view, QWidget *parent) :
    QMainWindow(parent),
    m_view(view),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->m_openGlWidgetContainer->addWidget(view);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent* event) {

    // NOTE: If you're adding or removing anything from this function, make
    // sure to update wiki/Keys.md

    int key = event->key();

    if (key == Qt::Key_P) {
        // Toggle pause (only in discrete mode)
        // TODO: MACK
        /*
        if (
            STRING_TO_INTERFACE_TYPE.value(m_controllerManager->getStaticOptions().interfaceType)
            == InterfaceType::DISCRETE
        ) {
        */
            S()->setPaused(!S()->paused());
        /*
        }
        else {
            qWarning().noquote().nospace()
                << "Pausing the simulator is only allowed in "
                << INTERFACE_TYPE_TO_STRING.value(InterfaceType::DISCRETE)
                << " mode.";
        }
        */
    }
    else if (key == Qt::Key_F) {
        // Faster (only in discrete mode)
        // TODO: MACK
        /*
        if (
            STRING_TO_INTERFACE_TYPE.value(m_controllerManager->getStaticOptions().interfaceType)
            == InterfaceType::DISCRETE
        ) {
        */
            S()->setSimSpeed(S()->simSpeed() * 1.5);
        /*
        }
        else {
            qWarning().noquote().nospace()
                << "Increasing the simulator speed is only allowed in "
                << INTERFACE_TYPE_TO_STRING.value(InterfaceType::DISCRETE)
                << " mode.";
        }
        */
    }
    else if (key == Qt::Key_S) {
        // Slower (only in discrete mode)
        // TODO: MACK
        /*
        if (
            STRING_TO_INTERFACE_TYPE.value(m_controllerManager->getStaticOptions().interfaceType)
            == InterfaceType::DISCRETE
        ) {
        */
            S()->setSimSpeed(S()->simSpeed() / 1.5);
        /*
        }
        else {
            qWarning().noquote().nospace()
                << "Decreasing the simulator speed is only allowed in "
                << INTERFACE_TYPE_TO_STRING.value(InterfaceType::DISCRETE)
                << " mode.";
        }
        */
    }
    else if (key == Qt::Key_L) {
        // Cycle through the available layouts
        S()->setLayoutType(LAYOUT_TYPE_CYCLE.value(S()->layoutType()));
    }
    else if (key == Qt::Key_R) {
        // Toggle rotate zoomed map
        S()->setRotateZoomedMap(!S()->rotateZoomedMap());
    }
    else if (key == Qt::Key_I) {
        // Zoom in
        S()->setZoomedMapScale(S()->zoomedMapScale() * 1.5);
    }
    else if (key == Qt::Key_O) {
        // Zoom out
        S()->setZoomedMapScale(S()->zoomedMapScale() / 1.5);
    }
    else if (key == Qt::Key_T) {
        // Toggle wall truth visibility
        S()->setWallTruthVisible(!S()->wallTruthVisible());
        m_view->getMazeGraphic()->updateWalls();
    }
    else if (key == Qt::Key_C) {
        // Toggle tile colors
        S()->setTileColorsVisible(!S()->tileColorsVisible());
        m_view->getMazeGraphic()->updateColor();
    }
    else if (key == Qt::Key_G) {
        // Toggle tile fog
        S()->setTileFogVisible(!S()->tileFogVisible());
        m_view->getMazeGraphic()->updateFog();
    }
    else if (key == Qt::Key_X) {
        // Toggle tile text
        S()->setTileTextVisible(!S()->tileTextVisible());
        m_view->getMazeGraphic()->updateText();
    }
    else if (key == Qt::Key_D) {
        // Toggle tile distance visibility
        S()->setTileDistanceVisible(!S()->tileDistanceVisible());
        m_view->getMazeGraphic()->updateText();
    }
    else if (key == Qt::Key_H) {
        // Toggle header visibility
        // TODO: MACK
        /*
        S()->setHeaderVisible(!S()->headerVisible());
        m_header->updateLinesAndColumnStartingPositions();
        */
    }
    else if (key == Qt::Key_W) {
        // Toggle wireframe mode
        S()->setWireframeMode(!S()->wireframeMode());
        glPolygonMode(GL_FRONT_AND_BACK, S()->wireframeMode() ? GL_LINE : GL_FILL);
    }
    else if (key == Qt::Key_Q) {
        // Quit
        SimUtilities::quit();
    }
    // TODO: MACK
    /*
    else if (QString("0123456789").indexOf(key) != -1) {
        // Press an input button
        int inputButton = QString("0123456789").indexOf(key);
        if (!S()->inputButtonWasPressed(inputButton)) {
            S()->setInputButtonWasPressed(inputButton, true);
            qInfo().noquote().nospace()
                << "Input button " << inputButton << " was pressed.";
        }
        else {
            qWarning().noquote().nospace()
                << "Input button " << inputButton << " has not yet been"
                << " acknowledged as pressed; pressing it has no effect.";
        }
    }
    */
}

// TODO: MACK
/*
void View::specialKeyPress(int key, int x, int y) {
    if (!INT_TO_KEY.contains(key)) {
        return;
    }
    if (ARROW_KEYS.contains(INT_TO_KEY.value(key))) {
        S()->setArrowKeyIsPressed(INT_TO_KEY.value(key), true);
    }
}

void View::specialKeyRelease(int key, int x, int y) {
    if (!INT_TO_KEY.contains(key)) {
        return;
    }
    if (ARROW_KEYS.contains(INT_TO_KEY.value(key))) {
        S()->setArrowKeyIsPressed(INT_TO_KEY.value(key), false);
    }
}
*/

} // namespace mms
