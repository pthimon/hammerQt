/********************************************************************************
** Form generated from reading UI file 'hammerqt.ui'
**
** Created
**      by: Qt User Interface Compiler version 4.6.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_HAMMERQT_H
#define UI_HAMMERQT_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDockWidget>
#include <QtGui/QFormLayout>
#include <QtGui/QGraphicsView>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QSlider>
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QTableWidget>
#include <QtGui/QToolBar>
#include <QtGui/QTreeWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_hammerQtClass
{
public:
    QAction *actionExit;
    QAction *actionNew;
    QAction *action_Open;
    QAction *action_Path_planning;
    QAction *action_Fullscreen;
    QAction *action_Save;
    QAction *actionSave_As;
    QAction *action_Line;
    QAction *action_Column;
    QAction *actionCi_rcle;
    QAction *action_Wedge;
    QAction *action_None;
    QAction *actionAttack;
    QAction *actionDefend;
    QAction *action_None_2;
    QAction *action_Attack;
    QAction *action_Defend;
    QAction *action_Launch;
    QAction *action_Save_2;
    QAction *action_Replay;
    QAction *actionDemonstration;
    QAction *action_Pincer;
    QAction *action_Retreat;
    QAction *action_Convoy;
    QWidget *centralwidget;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout_2;
    QLabel *bluePausedLabel;
    QLabel *redPausedLabel;
    QTabWidget *tabWidget;
    QWidget *tab3D;
    QGridLayout *gridLayout_3;
    QVBoxLayout *layout3d;
    QWidget *tabMap;
    QVBoxLayout *verticalLayout;
    QGraphicsView *mapView;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QSlider *zoomSlider;
    QWidget *tab;
    QGridLayout *gridLayout_2;
    QVBoxLayout *resultsTab;
    QMenuBar *menubar;
    QMenu *menu_File;
    QMenu *menu_View;
    QMenu *menuFo_rmations;
    QMenu *menu_Manoeuvre;
    QMenu *menu_Hypotheses;
    QMenu *menuLogic;
    QStatusBar *statusbar;
    QDockWidget *dockWidget;
    QWidget *dockWidgetContents;
    QGridLayout *gridLayout;
    QTableWidget *unitsTable;
    QGroupBox *groupBox;
    QFormLayout *formLayout_2;
    QLabel *remainingLabelText;
    QLabel *timeRemainingLabel;
    QLabel *label_3;
    QLabel *timeGoalLabel;
    QLabel *scaleLabelText;
    QLabel *timeScaleLabel;
    QToolBar *toolBar;
    QDockWidget *hypothesesDockWidget;
    QWidget *dockWidgetContents_5;
    QGridLayout *gridLayout_4;
    QCheckBox *plotCheckBox;
    QTreeWidget *hypothesesTable;

    void setupUi(QMainWindow *hammerQtClass)
    {
        if (hammerQtClass->objectName().isEmpty())
            hammerQtClass->setObjectName(QString::fromUtf8("hammerQtClass"));
        hammerQtClass->resize(800, 600);
        actionExit = new QAction(hammerQtClass);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/images/16x16/application-exit.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionExit->setIcon(icon);
        actionNew = new QAction(hammerQtClass);
        actionNew->setObjectName(QString::fromUtf8("actionNew"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/images/16x16/document-new.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionNew->setIcon(icon1);
        action_Open = new QAction(hammerQtClass);
        action_Open->setObjectName(QString::fromUtf8("action_Open"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/images/16x16/document-open.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_Open->setIcon(icon2);
        action_Path_planning = new QAction(hammerQtClass);
        action_Path_planning->setObjectName(QString::fromUtf8("action_Path_planning"));
        action_Path_planning->setCheckable(true);
        action_Fullscreen = new QAction(hammerQtClass);
        action_Fullscreen->setObjectName(QString::fromUtf8("action_Fullscreen"));
        action_Fullscreen->setCheckable(true);
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/images/16x16/view-fullscreen.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_Fullscreen->setIcon(icon3);
        action_Save = new QAction(hammerQtClass);
        action_Save->setObjectName(QString::fromUtf8("action_Save"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/images/16x16/document-save.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_Save->setIcon(icon4);
        actionSave_As = new QAction(hammerQtClass);
        actionSave_As->setObjectName(QString::fromUtf8("actionSave_As"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/images/16x16/document-save-as.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSave_As->setIcon(icon5);
        action_Line = new QAction(hammerQtClass);
        action_Line->setObjectName(QString::fromUtf8("action_Line"));
        action_Line->setCheckable(true);
        action_Line->setChecked(false);
        action_Column = new QAction(hammerQtClass);
        action_Column->setObjectName(QString::fromUtf8("action_Column"));
        action_Column->setCheckable(true);
        actionCi_rcle = new QAction(hammerQtClass);
        actionCi_rcle->setObjectName(QString::fromUtf8("actionCi_rcle"));
        actionCi_rcle->setCheckable(true);
        action_Wedge = new QAction(hammerQtClass);
        action_Wedge->setObjectName(QString::fromUtf8("action_Wedge"));
        action_Wedge->setCheckable(true);
        action_None = new QAction(hammerQtClass);
        action_None->setObjectName(QString::fromUtf8("action_None"));
        action_None->setCheckable(true);
        action_None->setChecked(true);
        actionAttack = new QAction(hammerQtClass);
        actionAttack->setObjectName(QString::fromUtf8("actionAttack"));
        actionDefend = new QAction(hammerQtClass);
        actionDefend->setObjectName(QString::fromUtf8("actionDefend"));
        action_None_2 = new QAction(hammerQtClass);
        action_None_2->setObjectName(QString::fromUtf8("action_None_2"));
        action_None_2->setCheckable(true);
        action_None_2->setChecked(true);
        action_Attack = new QAction(hammerQtClass);
        action_Attack->setObjectName(QString::fromUtf8("action_Attack"));
        action_Attack->setCheckable(true);
        action_Defend = new QAction(hammerQtClass);
        action_Defend->setObjectName(QString::fromUtf8("action_Defend"));
        action_Defend->setCheckable(true);
        action_Launch = new QAction(hammerQtClass);
        action_Launch->setObjectName(QString::fromUtf8("action_Launch"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/images/16x16/document-open-remote.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_Launch->setIcon(icon6);
        action_Save_2 = new QAction(hammerQtClass);
        action_Save_2->setObjectName(QString::fromUtf8("action_Save_2"));
        action_Save_2->setIcon(icon4);
        action_Replay = new QAction(hammerQtClass);
        action_Replay->setObjectName(QString::fromUtf8("action_Replay"));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/images/16x16/document-open-recent.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_Replay->setIcon(icon7);
        actionDemonstration = new QAction(hammerQtClass);
        actionDemonstration->setObjectName(QString::fromUtf8("actionDemonstration"));
        action_Pincer = new QAction(hammerQtClass);
        action_Pincer->setObjectName(QString::fromUtf8("action_Pincer"));
        action_Pincer->setCheckable(true);
        action_Retreat = new QAction(hammerQtClass);
        action_Retreat->setObjectName(QString::fromUtf8("action_Retreat"));
        action_Retreat->setCheckable(true);
        action_Convoy = new QAction(hammerQtClass);
        action_Convoy->setObjectName(QString::fromUtf8("action_Convoy"));
        action_Convoy->setCheckable(true);
        centralwidget = new QWidget(hammerQtClass);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        centralWidget = new QWidget(centralwidget);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        centralWidget->setGeometry(QRect(50, 60, 296, 321));
        verticalLayout_2 = new QVBoxLayout(centralWidget);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        bluePausedLabel = new QLabel(centralWidget);
        bluePausedLabel->setObjectName(QString::fromUtf8("bluePausedLabel"));
        bluePausedLabel->setMaximumSize(QSize(16777215, 30));
        QPalette palette;
        QBrush brush(QColor(255, 255, 255, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
        QBrush brush1(QColor(0, 0, 255, 255));
        brush1.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Button, brush1);
        QBrush brush2(QColor(127, 127, 255, 255));
        brush2.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Light, brush2);
        QBrush brush3(QColor(63, 63, 255, 255));
        brush3.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Midlight, brush3);
        QBrush brush4(QColor(0, 0, 127, 255));
        brush4.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Dark, brush4);
        QBrush brush5(QColor(0, 0, 170, 255));
        brush5.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Mid, brush5);
        palette.setBrush(QPalette::Active, QPalette::Text, brush);
        palette.setBrush(QPalette::Active, QPalette::BrightText, brush);
        QBrush brush6(QColor(0, 0, 0, 255));
        brush6.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::ButtonText, brush6);
        palette.setBrush(QPalette::Active, QPalette::Base, brush);
        palette.setBrush(QPalette::Active, QPalette::Window, brush1);
        palette.setBrush(QPalette::Active, QPalette::Shadow, brush6);
        palette.setBrush(QPalette::Active, QPalette::AlternateBase, brush2);
        QBrush brush7(QColor(255, 255, 220, 255));
        brush7.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::ToolTipBase, brush7);
        palette.setBrush(QPalette::Active, QPalette::ToolTipText, brush6);
        palette.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Button, brush1);
        palette.setBrush(QPalette::Inactive, QPalette::Light, brush2);
        palette.setBrush(QPalette::Inactive, QPalette::Midlight, brush3);
        palette.setBrush(QPalette::Inactive, QPalette::Dark, brush4);
        palette.setBrush(QPalette::Inactive, QPalette::Mid, brush5);
        palette.setBrush(QPalette::Inactive, QPalette::Text, brush);
        palette.setBrush(QPalette::Inactive, QPalette::BrightText, brush);
        palette.setBrush(QPalette::Inactive, QPalette::ButtonText, brush6);
        palette.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Window, brush1);
        palette.setBrush(QPalette::Inactive, QPalette::Shadow, brush6);
        palette.setBrush(QPalette::Inactive, QPalette::AlternateBase, brush2);
        palette.setBrush(QPalette::Inactive, QPalette::ToolTipBase, brush7);
        palette.setBrush(QPalette::Inactive, QPalette::ToolTipText, brush6);
        palette.setBrush(QPalette::Disabled, QPalette::WindowText, brush4);
        palette.setBrush(QPalette::Disabled, QPalette::Button, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::Light, brush2);
        palette.setBrush(QPalette::Disabled, QPalette::Midlight, brush3);
        palette.setBrush(QPalette::Disabled, QPalette::Dark, brush4);
        palette.setBrush(QPalette::Disabled, QPalette::Mid, brush5);
        palette.setBrush(QPalette::Disabled, QPalette::Text, brush4);
        palette.setBrush(QPalette::Disabled, QPalette::BrightText, brush);
        palette.setBrush(QPalette::Disabled, QPalette::ButtonText, brush4);
        palette.setBrush(QPalette::Disabled, QPalette::Base, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::Window, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::Shadow, brush6);
        palette.setBrush(QPalette::Disabled, QPalette::AlternateBase, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::ToolTipBase, brush7);
        palette.setBrush(QPalette::Disabled, QPalette::ToolTipText, brush6);
        bluePausedLabel->setPalette(palette);
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        bluePausedLabel->setFont(font);
        bluePausedLabel->setAutoFillBackground(true);
        bluePausedLabel->setAlignment(Qt::AlignCenter);

        verticalLayout_2->addWidget(bluePausedLabel);

        redPausedLabel = new QLabel(centralWidget);
        redPausedLabel->setObjectName(QString::fromUtf8("redPausedLabel"));
        redPausedLabel->setMaximumSize(QSize(16777215, 30));
        QPalette palette1;
        palette1.setBrush(QPalette::Active, QPalette::WindowText, brush);
        QBrush brush8(QColor(251, 0, 4, 255));
        brush8.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Button, brush8);
        QBrush brush9(QColor(255, 121, 124, 255));
        brush9.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Light, brush9);
        QBrush brush10(QColor(253, 60, 64, 255));
        brush10.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Midlight, brush10);
        QBrush brush11(QColor(125, 0, 2, 255));
        brush11.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Dark, brush11);
        QBrush brush12(QColor(167, 0, 2, 255));
        brush12.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Mid, brush12);
        palette1.setBrush(QPalette::Active, QPalette::Text, brush6);
        palette1.setBrush(QPalette::Active, QPalette::BrightText, brush);
        palette1.setBrush(QPalette::Active, QPalette::ButtonText, brush6);
        palette1.setBrush(QPalette::Active, QPalette::Base, brush);
        palette1.setBrush(QPalette::Active, QPalette::Window, brush8);
        palette1.setBrush(QPalette::Active, QPalette::Shadow, brush6);
        QBrush brush13(QColor(253, 127, 129, 255));
        brush13.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::AlternateBase, brush13);
        palette1.setBrush(QPalette::Active, QPalette::ToolTipBase, brush7);
        palette1.setBrush(QPalette::Active, QPalette::ToolTipText, brush6);
        palette1.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
        palette1.setBrush(QPalette::Inactive, QPalette::Button, brush8);
        palette1.setBrush(QPalette::Inactive, QPalette::Light, brush9);
        palette1.setBrush(QPalette::Inactive, QPalette::Midlight, brush10);
        palette1.setBrush(QPalette::Inactive, QPalette::Dark, brush11);
        palette1.setBrush(QPalette::Inactive, QPalette::Mid, brush12);
        palette1.setBrush(QPalette::Inactive, QPalette::Text, brush6);
        palette1.setBrush(QPalette::Inactive, QPalette::BrightText, brush);
        palette1.setBrush(QPalette::Inactive, QPalette::ButtonText, brush6);
        palette1.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette1.setBrush(QPalette::Inactive, QPalette::Window, brush8);
        palette1.setBrush(QPalette::Inactive, QPalette::Shadow, brush6);
        palette1.setBrush(QPalette::Inactive, QPalette::AlternateBase, brush13);
        palette1.setBrush(QPalette::Inactive, QPalette::ToolTipBase, brush7);
        palette1.setBrush(QPalette::Inactive, QPalette::ToolTipText, brush6);
        palette1.setBrush(QPalette::Disabled, QPalette::WindowText, brush11);
        palette1.setBrush(QPalette::Disabled, QPalette::Button, brush8);
        palette1.setBrush(QPalette::Disabled, QPalette::Light, brush9);
        palette1.setBrush(QPalette::Disabled, QPalette::Midlight, brush10);
        palette1.setBrush(QPalette::Disabled, QPalette::Dark, brush11);
        palette1.setBrush(QPalette::Disabled, QPalette::Mid, brush12);
        palette1.setBrush(QPalette::Disabled, QPalette::Text, brush11);
        palette1.setBrush(QPalette::Disabled, QPalette::BrightText, brush);
        palette1.setBrush(QPalette::Disabled, QPalette::ButtonText, brush11);
        palette1.setBrush(QPalette::Disabled, QPalette::Base, brush8);
        palette1.setBrush(QPalette::Disabled, QPalette::Window, brush8);
        palette1.setBrush(QPalette::Disabled, QPalette::Shadow, brush6);
        palette1.setBrush(QPalette::Disabled, QPalette::AlternateBase, brush8);
        palette1.setBrush(QPalette::Disabled, QPalette::ToolTipBase, brush7);
        palette1.setBrush(QPalette::Disabled, QPalette::ToolTipText, brush6);
        redPausedLabel->setPalette(palette1);
        redPausedLabel->setFont(font);
        redPausedLabel->setAutoFillBackground(true);
        redPausedLabel->setAlignment(Qt::AlignCenter);

        verticalLayout_2->addWidget(redPausedLabel);

        tabWidget = new QTabWidget(centralWidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setEnabled(true);
        tab3D = new QWidget();
        tab3D->setObjectName(QString::fromUtf8("tab3D"));
        gridLayout_3 = new QGridLayout(tab3D);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        layout3d = new QVBoxLayout();
        layout3d->setObjectName(QString::fromUtf8("layout3d"));

        gridLayout_3->addLayout(layout3d, 0, 0, 1, 1);

        tabWidget->addTab(tab3D, QString());
        tabMap = new QWidget();
        tabMap->setObjectName(QString::fromUtf8("tabMap"));
        verticalLayout = new QVBoxLayout(tabMap);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        mapView = new QGraphicsView(tabMap);
        mapView->setObjectName(QString::fromUtf8("mapView"));
        mapView->setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);
        mapView->setDragMode(QGraphicsView::ScrollHandDrag);
        mapView->setCacheMode(QGraphicsView::CacheBackground);
        mapView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
        mapView->setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing|QGraphicsView::DontSavePainterState);

        verticalLayout->addWidget(mapView);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(tabMap);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        zoomSlider = new QSlider(tabMap);
        zoomSlider->setObjectName(QString::fromUtf8("zoomSlider"));
        zoomSlider->setOrientation(Qt::Horizontal);

        horizontalLayout->addWidget(zoomSlider);


        verticalLayout->addLayout(horizontalLayout);

        tabWidget->addTab(tabMap, QString());
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        gridLayout_2 = new QGridLayout(tab);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        resultsTab = new QVBoxLayout();
        resultsTab->setObjectName(QString::fromUtf8("resultsTab"));

        gridLayout_2->addLayout(resultsTab, 0, 0, 1, 1);

        tabWidget->addTab(tab, QString());

        verticalLayout_2->addWidget(tabWidget);

        hammerQtClass->setCentralWidget(centralwidget);
        menubar = new QMenuBar(hammerQtClass);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 20));
        menu_File = new QMenu(menubar);
        menu_File->setObjectName(QString::fromUtf8("menu_File"));
        menu_View = new QMenu(menubar);
        menu_View->setObjectName(QString::fromUtf8("menu_View"));
        menuFo_rmations = new QMenu(menubar);
        menuFo_rmations->setObjectName(QString::fromUtf8("menuFo_rmations"));
        menu_Manoeuvre = new QMenu(menubar);
        menu_Manoeuvre->setObjectName(QString::fromUtf8("menu_Manoeuvre"));
        menu_Hypotheses = new QMenu(menubar);
        menu_Hypotheses->setObjectName(QString::fromUtf8("menu_Hypotheses"));
        menuLogic = new QMenu(menubar);
        menuLogic->setObjectName(QString::fromUtf8("menuLogic"));
        hammerQtClass->setMenuBar(menubar);
        statusbar = new QStatusBar(hammerQtClass);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        hammerQtClass->setStatusBar(statusbar);
        dockWidget = new QDockWidget(hammerQtClass);
        dockWidget->setObjectName(QString::fromUtf8("dockWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(200);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(dockWidget->sizePolicy().hasHeightForWidth());
        dockWidget->setSizePolicy(sizePolicy);
        dockWidget->setMaximumSize(QSize(524287, 524287));
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
        gridLayout = new QGridLayout(dockWidgetContents);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        unitsTable = new QTableWidget(dockWidgetContents);
        unitsTable->setObjectName(QString::fromUtf8("unitsTable"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(200);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(unitsTable->sizePolicy().hasHeightForWidth());
        unitsTable->setSizePolicy(sizePolicy1);
        unitsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        unitsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        unitsTable->setShowGrid(false);

        gridLayout->addWidget(unitsTable, 0, 0, 1, 1);

        groupBox = new QGroupBox(dockWidgetContents);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        sizePolicy.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy);
        QFont font1;
        font1.setBold(false);
        font1.setWeight(50);
        groupBox->setFont(font1);
        groupBox->setAlignment(Qt::AlignCenter);
        groupBox->setFlat(false);
        groupBox->setCheckable(false);
        formLayout_2 = new QFormLayout(groupBox);
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        formLayout_2->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        formLayout_2->setRowWrapPolicy(QFormLayout::WrapLongRows);
        formLayout_2->setLabelAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        formLayout_2->setFormAlignment(Qt::AlignHCenter|Qt::AlignTop);
        remainingLabelText = new QLabel(groupBox);
        remainingLabelText->setObjectName(QString::fromUtf8("remainingLabelText"));
        remainingLabelText->setFont(font);

        formLayout_2->setWidget(0, QFormLayout::LabelRole, remainingLabelText);

        timeRemainingLabel = new QLabel(groupBox);
        timeRemainingLabel->setObjectName(QString::fromUtf8("timeRemainingLabel"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, timeRemainingLabel);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setFont(font);

        formLayout_2->setWidget(1, QFormLayout::LabelRole, label_3);

        timeGoalLabel = new QLabel(groupBox);
        timeGoalLabel->setObjectName(QString::fromUtf8("timeGoalLabel"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, timeGoalLabel);

        scaleLabelText = new QLabel(groupBox);
        scaleLabelText->setObjectName(QString::fromUtf8("scaleLabelText"));
        scaleLabelText->setFont(font);

        formLayout_2->setWidget(2, QFormLayout::LabelRole, scaleLabelText);

        timeScaleLabel = new QLabel(groupBox);
        timeScaleLabel->setObjectName(QString::fromUtf8("timeScaleLabel"));

        formLayout_2->setWidget(2, QFormLayout::FieldRole, timeScaleLabel);


        gridLayout->addWidget(groupBox, 1, 0, 1, 1);

        dockWidget->setWidget(dockWidgetContents);
        hammerQtClass->addDockWidget(static_cast<Qt::DockWidgetArea>(2), dockWidget);
        toolBar = new QToolBar(hammerQtClass);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        hammerQtClass->addToolBar(Qt::TopToolBarArea, toolBar);
        hypothesesDockWidget = new QDockWidget(hammerQtClass);
        hypothesesDockWidget->setObjectName(QString::fromUtf8("hypothesesDockWidget"));
        dockWidgetContents_5 = new QWidget();
        dockWidgetContents_5->setObjectName(QString::fromUtf8("dockWidgetContents_5"));
        gridLayout_4 = new QGridLayout(dockWidgetContents_5);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        plotCheckBox = new QCheckBox(dockWidgetContents_5);
        plotCheckBox->setObjectName(QString::fromUtf8("plotCheckBox"));
        plotCheckBox->setChecked(true);

        gridLayout_4->addWidget(plotCheckBox, 1, 0, 1, 1);

        hypothesesTable = new QTreeWidget(dockWidgetContents_5);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        hypothesesTable->setHeaderItem(__qtreewidgetitem);
        hypothesesTable->setObjectName(QString::fromUtf8("hypothesesTable"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(200);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(hypothesesTable->sizePolicy().hasHeightForWidth());
        hypothesesTable->setSizePolicy(sizePolicy2);
        hypothesesTable->setSelectionMode(QAbstractItemView::MultiSelection);
        hypothesesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        hypothesesTable->header()->setVisible(false);

        gridLayout_4->addWidget(hypothesesTable, 0, 0, 1, 1);

        hypothesesDockWidget->setWidget(dockWidgetContents_5);
        hammerQtClass->addDockWidget(static_cast<Qt::DockWidgetArea>(2), hypothesesDockWidget);

        menubar->addAction(menu_File->menuAction());
        menubar->addAction(menu_View->menuAction());
        menubar->addAction(menu_Hypotheses->menuAction());
        menubar->addAction(menuFo_rmations->menuAction());
        menubar->addAction(menu_Manoeuvre->menuAction());
        menubar->addAction(menuLogic->menuAction());
        menu_File->addAction(actionNew);
        menu_File->addAction(action_Open);
        menu_File->addSeparator();
        menu_File->addAction(action_Replay);
        menu_File->addSeparator();
        menu_File->addAction(action_Save);
        menu_File->addAction(actionSave_As);
        menu_File->addSeparator();
        menu_File->addAction(actionExit);
        menu_View->addAction(action_Fullscreen);
        menuFo_rmations->addAction(action_None);
        menuFo_rmations->addSeparator();
        menuFo_rmations->addAction(action_Line);
        menuFo_rmations->addAction(action_Column);
        menuFo_rmations->addAction(actionCi_rcle);
        menuFo_rmations->addAction(action_Wedge);
        menu_Manoeuvre->addAction(action_None_2);
        menu_Manoeuvre->addSeparator();
        menu_Manoeuvre->addAction(action_Attack);
        menu_Manoeuvre->addAction(action_Retreat);
        menu_Manoeuvre->addAction(action_Defend);
        menu_Manoeuvre->addAction(action_Pincer);
        menu_Manoeuvre->addAction(action_Convoy);
        menu_Manoeuvre->addSeparator();
        menu_Manoeuvre->addAction(action_Path_planning);
        menu_Hypotheses->addAction(action_Launch);
        menu_Hypotheses->addSeparator();
        menu_Hypotheses->addAction(action_Save_2);
        menuLogic->addAction(actionDemonstration);
        toolBar->addAction(action_Launch);
        toolBar->addAction(action_Save_2);
        toolBar->addSeparator();
        toolBar->addAction(action_Fullscreen);

        retranslateUi(hammerQtClass);

        tabWidget->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(hammerQtClass);
    } // setupUi

    void retranslateUi(QMainWindow *hammerQtClass)
    {
        hammerQtClass->setWindowTitle(QApplication::translate("hammerQtClass", "HammerQt", 0, QApplication::UnicodeUTF8));
        actionExit->setText(QApplication::translate("hammerQtClass", "E&xit", 0, QApplication::UnicodeUTF8));
        actionNew->setText(QApplication::translate("hammerQtClass", "&New", 0, QApplication::UnicodeUTF8));
        action_Open->setText(QApplication::translate("hammerQtClass", "&Open", 0, QApplication::UnicodeUTF8));
        action_Path_planning->setText(QApplication::translate("hammerQtClass", "&Path planning", 0, QApplication::UnicodeUTF8));
        action_Fullscreen->setText(QApplication::translate("hammerQtClass", "&Fullscreen", 0, QApplication::UnicodeUTF8));
        action_Save->setText(QApplication::translate("hammerQtClass", "&Save", 0, QApplication::UnicodeUTF8));
        actionSave_As->setText(QApplication::translate("hammerQtClass", "Save &As...", 0, QApplication::UnicodeUTF8));
        action_Line->setText(QApplication::translate("hammerQtClass", "&Line", 0, QApplication::UnicodeUTF8));
        action_Column->setText(QApplication::translate("hammerQtClass", "&Column", 0, QApplication::UnicodeUTF8));
        actionCi_rcle->setText(QApplication::translate("hammerQtClass", "Ci&rcle", 0, QApplication::UnicodeUTF8));
        action_Wedge->setText(QApplication::translate("hammerQtClass", "&Wedge", 0, QApplication::UnicodeUTF8));
        action_None->setText(QApplication::translate("hammerQtClass", "&None", 0, QApplication::UnicodeUTF8));
        actionAttack->setText(QApplication::translate("hammerQtClass", "Attack", 0, QApplication::UnicodeUTF8));
        actionDefend->setText(QApplication::translate("hammerQtClass", "Defend", 0, QApplication::UnicodeUTF8));
        action_None_2->setText(QApplication::translate("hammerQtClass", "&None", 0, QApplication::UnicodeUTF8));
        action_Attack->setText(QApplication::translate("hammerQtClass", "&Attack", 0, QApplication::UnicodeUTF8));
        action_Defend->setText(QApplication::translate("hammerQtClass", "&Defend", 0, QApplication::UnicodeUTF8));
        action_Launch->setText(QApplication::translate("hammerQtClass", "&Launch", 0, QApplication::UnicodeUTF8));
        action_Save_2->setText(QApplication::translate("hammerQtClass", "&Save", 0, QApplication::UnicodeUTF8));
        action_Replay->setText(QApplication::translate("hammerQtClass", "Open &Replay...", 0, QApplication::UnicodeUTF8));
        actionDemonstration->setText(QApplication::translate("hammerQtClass", "Next step of demonstration...", 0, QApplication::UnicodeUTF8));
        action_Pincer->setText(QApplication::translate("hammerQtClass", "&Pincer", 0, QApplication::UnicodeUTF8));
        action_Retreat->setText(QApplication::translate("hammerQtClass", "&Retreat", 0, QApplication::UnicodeUTF8));
        action_Convoy->setText(QApplication::translate("hammerQtClass", "&Convoy", 0, QApplication::UnicodeUTF8));
        bluePausedLabel->setText(QApplication::translate("hammerQtClass", "Paused by blue player", 0, QApplication::UnicodeUTF8));
        redPausedLabel->setText(QApplication::translate("hammerQtClass", "Paused by red player", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab3D), QApplication::translate("hammerQtClass", "3D View", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("hammerQtClass", "Zoom", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tabMap), QApplication::translate("hammerQtClass", "Map View", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("hammerQtClass", "Results", 0, QApplication::UnicodeUTF8));
        menu_File->setTitle(QApplication::translate("hammerQtClass", "&File", 0, QApplication::UnicodeUTF8));
        menu_View->setTitle(QApplication::translate("hammerQtClass", "&View", 0, QApplication::UnicodeUTF8));
        menuFo_rmations->setTitle(QApplication::translate("hammerQtClass", "Fo&rmations", 0, QApplication::UnicodeUTF8));
        menu_Manoeuvre->setTitle(QApplication::translate("hammerQtClass", "&Manoeuvre", 0, QApplication::UnicodeUTF8));
        menu_Hypotheses->setTitle(QApplication::translate("hammerQtClass", "&Hypotheses", 0, QApplication::UnicodeUTF8));
        menuLogic->setTitle(QApplication::translate("hammerQtClass", "Logic", 0, QApplication::UnicodeUTF8));
        dockWidget->setWindowTitle(QApplication::translate("hammerQtClass", "Units", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("hammerQtClass", "Time", 0, QApplication::UnicodeUTF8));
        remainingLabelText->setText(QApplication::translate("hammerQtClass", "Remaining: ", 0, QApplication::UnicodeUTF8));
        timeRemainingLabel->setText(QApplication::translate("hammerQtClass", "0s", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("hammerQtClass", "In goal area:", 0, QApplication::UnicodeUTF8));
        timeGoalLabel->setText(QApplication::translate("hammerQtClass", "0s", 0, QApplication::UnicodeUTF8));
        scaleLabelText->setText(QApplication::translate("hammerQtClass", "Scale:", 0, QApplication::UnicodeUTF8));
        timeScaleLabel->setText(QApplication::translate("hammerQtClass", "1x", 0, QApplication::UnicodeUTF8));
        toolBar->setWindowTitle(QApplication::translate("hammerQtClass", "toolBar", 0, QApplication::UnicodeUTF8));
        hypothesesDockWidget->setWindowTitle(QApplication::translate("hammerQtClass", "Hypotheses", 0, QApplication::UnicodeUTF8));
        plotCheckBox->setText(QApplication::translate("hammerQtClass", "Plot Results", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class hammerQtClass: public Ui_hammerQtClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HAMMERQT_H
