#include <QFileDialog>
#include <configobjdialog.h>
#include <QMessageBox>
#include <QtDebug>
#include <QKeyEvent>
#include <QStatusBar>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    mpViz = new FMTstarViz();

    createActions();
    createMenuBar();

    mpMap = NULL;
    mpFMTstar = NULL;

    mpConfigObjDialog = new ConfigObjDialog(this);
    mpConfigObjDialog->hide();

    setCentralWidget(mpViz);

    mpStatusLabel = new QLabel();
    mpStatusProgressBar = new QProgressBar();

    statusBar()->addWidget(mpStatusProgressBar);
    statusBar()->addWidget(mpStatusLabel);

    updateTitle();
}

MainWindow::~MainWindow() {
    if(mpConfigObjDialog) {
        delete mpConfigObjDialog;
        mpConfigObjDialog = NULL;
    }
    if(mpViz) {
        delete mpViz;
        mpViz = NULL;
    }
}

void MainWindow::createMenuBar() {
    mpFileMenu = menuBar()->addMenu("&File");
    mpFileMenu->addAction(mpOpenAction);
    mpFileMenu->addAction(mpSaveAction);
    mpFileMenu->addAction(mpExportAction);

    mpEditMenu = menuBar()->addMenu("&Edit");
    mpEditMenu->addAction(mpLoadMapAction);
    mpEditMenu->addAction(mpLoadObjAction);
    mpEditMenu->addAction(mpRunAction);

    mpContextMenu = new QMenu();
    setContextMenuPolicy(Qt::CustomContextMenu);

    mpContextMenu->addAction(mpAddStartAction);
    mpContextMenu->addAction(mpAddGoalAction);
}

void MainWindow::createActions()
{
    mpOpenAction = new QAction("Open", this);
    mpSaveAction = new QAction("Save", this);
    mpExportAction = new QAction("Export", this);
    mpLoadMapAction = new QAction("Load Map", this);
    mpLoadObjAction = new QAction("Config Objective", this);
    mpRunAction = new QAction("Run", this);

    connect(mpOpenAction, SIGNAL(triggered()), this, SLOT(onOpen()));
    connect(mpSaveAction, SIGNAL(triggered()), this, SLOT(onSave()));
    connect(mpExportAction, SIGNAL(triggered()), this, SLOT(onExport()));
    connect(mpLoadMapAction, SIGNAL(triggered()), this, SLOT(onLoadMap()));
    connect(mpLoadObjAction, SIGNAL(triggered()), this, SLOT(onLoadObj()));
    connect(mpRunAction, SIGNAL(triggered()), this, SLOT(onRun()));

    mpAddStartAction = new QAction("Add Start", this);
    mpAddGoalAction = new QAction("Add Goal", this);

    connect(mpAddStartAction, SIGNAL(triggered()), this, SLOT(onAddStart()));
    connect(mpAddGoalAction, SIGNAL(triggered()), this, SLOT(onAddGoal()));

    connect(this, SIGNAL(customContextMenuRequested(const QPoint)),this, SLOT(contextMenuRequested(QPoint)));
}

void MainWindow::onOpen()
{
    QString tempFilename = QFileDialog::getOpenFileName(this,
             tr("Open File"), "./", tr("XML Files (*.xml)"));

    if(setupPlanning(tempFilename)) {
        repaint();
    }
}

bool MainWindow::setupPlanning(QString filename) {
    if(mpViz) {
        mpViz->m_PPInfo.load_from_file(filename);
        openMap(mpViz->m_PPInfo.m_map_fullpath);
        if(mpConfigObjDialog) {
            mpConfigObjDialog->updateDisplay();
        }
        return true;
    }
    return false;
}

void MainWindow::onSave() {
    QString tempFilename = QFileDialog::getSaveFileName(this, tr("Save File"), "./", tr("XML Files (*.xml)"));

    if(mpViz) {
        mpViz->m_PPInfo.save_to_file(tempFilename);
    }
}

void MainWindow::onExport() {
    QString pathFilename = QFileDialog::getSaveFileName(this, tr("Save File"), "./", tr("Txt Files (*.txt)"));
    if (pathFilename != "") {
        mpViz->m_PPInfo.m_paths_output = pathFilename;
        exportPaths();
    }
}

bool MainWindow::exportPaths() {
    if(mpViz) {
        bool success = false;
        success = mpViz->m_PPInfo.export_path(mpViz->m_PPInfo.m_paths_output);
        success = mpViz->drawPath(mpViz->m_PPInfo.m_paths_output+".png");
        return success;
    }
    return false;
}

void MainWindow::onLoadMap() {
    QString tempFilename = QFileDialog::getOpenFileName(this,
             tr("Open Map File"), "./", tr("Map Files (*.*)"));

    QFileInfo fileInfo(tempFilename);
    QString filename(fileInfo.fileName());
    mpViz->m_PPInfo.m_map_filename = filename;
    mpViz->m_PPInfo.m_map_fullpath = tempFilename;
    qDebug("OPENING ");
    qDebug(mpViz->m_PPInfo.m_map_filename.toStdString().c_str());

    openMap(mpViz->m_PPInfo.m_map_fullpath);
}


bool MainWindow::openMap(QString filename) {
    if(mpMap) {
        delete mpMap;
        mpMap = NULL;
    }
    mpMap = new QPixmap(filename);
    if(mpMap) {
        mpViz->m_PPInfo.m_map_width = mpMap->width();
        mpViz->m_PPInfo.m_map_height = mpMap->height();
        mpViz->setPixmap(*mpMap);
        updateTitle();
        return true;
    }
    return false;
}

void MainWindow::onLoadObj() {
    mpConfigObjDialog->exec();
    updateTitle();
}

void MainWindow::onRun() {
    if (mpViz->m_PPInfo.m_map_width <= 0 || mpViz->m_PPInfo.m_map_height <= 0) {
        QMessageBox msgBox;
        msgBox.setText("Map is not initialized.");
        msgBox.exec();
        return;
    }
    if(mpViz->m_PPInfo.m_start.x()<0 || mpViz->m_PPInfo.m_start.y()<0) {
        QMessageBox msgBox;
        msgBox.setText("Start is not set.");
        msgBox.exec();
        return;
    }
    if(mpViz->m_PPInfo.m_goal.x()<0 || mpViz->m_PPInfo.m_goal.y()<0) {
        QMessageBox msgBox;
        msgBox.setText("Goal is not set.");
        msgBox.exec();
        return;
    }

    planPath();
    repaint();
}

void MainWindow::planPath() {
    if(mpFMTstar) {
        delete mpFMTstar;
        mpFMTstar = NULL;
    }
    if(mpViz->m_PPInfo.mp_found_path) {
        delete mpViz->m_PPInfo.mp_found_path;
        mpViz->m_PPInfo.mp_found_path = NULL;
    }

    mpViz->m_PPInfo.init_func_param();
    QString msg = "RUNNING FMTstar ... \n";
    msg += "SegmentLen( " + QString::number(mpViz->m_PPInfo.m_segment_length) + " ) \n";
    msg += "MaxIterationNum( " + QString::number(mpViz->m_PPInfo.m_max_iteration_num) + " ) \n";
    qDebug(msg.toStdString().c_str());

    mpFMTstar = new FMTstar(mpMap->width(), mpMap->height(), mpViz->m_PPInfo.m_segment_length);

    POS2D start(mpViz->m_PPInfo.m_start.x(), mpViz->m_PPInfo.m_start.y());
    POS2D goal(mpViz->m_PPInfo.m_goal.x(), mpViz->m_PPInfo.m_goal.y());

    mpFMTstar->init(start, goal, mpViz->m_PPInfo.mp_func, mpViz->m_PPInfo.mCostDistribution);
    mpViz->m_PPInfo.get_obstacle_info(mpFMTstar->get_map_info());
    mpViz->setTree(mpFMTstar);

    mpFMTstar->dump_distribution("dist.txt");

    while(mpFMTstar->get_current_iteration() <= mpViz->m_PPInfo.m_max_iteration_num) {
        //QString msg = "CurrentIteration " + QString::number(mpFMTstar->get_current_iteration()) + " ";
        //msg += QString::number(mpFMTstar->get_ball_radius());
        //qDebug(msg.toStdString().c_str());

        mpFMTstar->extend();

        updateStatus();
        repaint();
    }

    Path* path = mpFMTstar->find_path();
    mpViz->m_PPInfo.load_path(path);
}

void MainWindow::onAddStart() {
    mpViz->m_PPInfo.m_start = mCursorPoint;
    repaint();
}

void MainWindow::onAddGoal() {
    mpViz->m_PPInfo.m_goal = mCursorPoint;
    repaint();
}

void MainWindow::contextMenuRequested(QPoint point) {
    mCursorPoint = point;
    mpContextMenu->popup(mapToGlobal(point));
}

void MainWindow::updateTitle() {
    QString title = mpViz->m_PPInfo.m_map_filename;
    setWindowTitle(title);
}

void MainWindow::updateStatus() {
    if(mpViz==NULL || mpFMTstar==NULL) {
        return;
    }
    if(mpStatusProgressBar) {
        mpStatusProgressBar->setMinimum(0);
        mpStatusProgressBar->setMaximum(mpViz->m_PPInfo.m_max_iteration_num);
        mpStatusProgressBar->setValue(mpFMTstar->get_current_iteration());
    }
    if(mpStatusLabel) {
        QString status = "";
        mpStatusLabel->setText(status);
    }
    repaint();
}


