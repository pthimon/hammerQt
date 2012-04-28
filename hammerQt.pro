include( custom.pri )
CONFIG += qt \
    thread
supress { 
    QMAKE_CC = @echo \
        \-\-\> \
        $(<) \
        ; \
        icecc
    QMAKE_CXX = @echo \
        \-\-\> \
        $(<) \
        ; \
        icecc
    QMAKE_MOC = @echo \
        \-\-\> \
        Moccing \
        $(<) \
        ... \
        ; \
        $$QMAKE_MOC
    QMAKE_LINK = @echo \
        \-\-\> \
        Linking \
        $@ \
        ... \
        ; \
        $$QMAKE_LINK
}
CONFIG(debug, debug|release) { 
    TARGET = hammerQtd
    QMAKE_CFLAGS = 
    QMAKE_CXXFLAGS = 
    LIBS += -lclusteringd
    DEFINES += DEBUG
}
CONFIG(release, debug|release) { 
    TARGET = hammerQt
    QMAKE_CFLAGS = -O3
    QMAKE_CXXFLAGS = -O3
    LIBS += -lclustering
}
QT += core \
    gui \
    xml \
    opengl
HEADERS += models/games/TargetReplayCommand.h \
    models/games/CaptureBaseGame.h \
    models/InverseModelConvoy.h \
    models/InverseModelRetreat.h \
    hypotheses/CommandHypothesis.h \
    models/games/DefaultGame.h \
    Watchdog.h \
    models/games/Game.h \
    models/InverseModelTrack.h \
    models/games/EvasionGame.h \
    hypotheses/TargetDirectionHypothesis.h \
    hypotheses/TargetConfidences.h \
    models/InverseModelPincer.h \
    gui/GroupMarker.h \
    gui/Marker.h \
    gui/OSGAdapterWidget.h \
    gui/UnitMarker.h \
    gui/camtriangle.h \
    gui/compass2.h \
    gui/guihud.h \
    gui/hammerqt.h \
    gui/mapmotionmodel.h \
    hypotheses/DirectionHypothesis.h \
    hypotheses/Hypothesis.h \
    hypotheses/HypothesisClient.h \
    hypotheses/HypothesisManager.h \
    hypotheses/HypothesisResults.h \
    hypotheses/TargetHypothesis.h \
    logic/Feature.h \
    logic/PlanAdaptor.h \
    logic/PlanCombiner.h \
    models/InverseModelAttack.h \
    models/InverseModelDefend.h \
    models/InverseModelFormation.h \
    models/InverseModelPath.h \
    models/inc/InverseModel.h \
    models/inc/Observations.h \
    models/inc/Targets.h \
    models/inc/TerrainMap.h \
    models/inc/micropather.h \
    terrain/SurfaceObject.h \
    terrain/TerrainFeature.h \
    terrain/TerrainLayer.h \
    terrain/terrain.h \
    terrain/terrain_PGM.h \
    units/PathInterpolation.h \
    units/Projectile.h \
    units/Unit.h \
    units/UnitConfigData.h \
    units/UnitConfigHandler.h \
    units/UnitConfigSchema.h \
    units/UnitFiring.h \
    units/UnitGroup.h \
    units/UnitObserver.h \
    units/UnitRemote.h \
    units/UnitSoldier.h \
    units/UnitTank.h \
    units/spline.h \
    Command.h \
    HTAppBase.h \
    HTAppQt.h \
    HTPhysicsController.h \
    HTapp.h \
    ScenarioData.h \
    ScenarioHandler.h \
    application_HT.h \
    common.h \
    event.h \
    eventlog.h \
    mynetwork.h \
    packets.h \
    models/games/DirectionGame.h \
    models/games/MazeGame.h
SOURCES += models/games/CaptureBaseGame.cpp \
    models/InverseModelConvoy.cpp \
    models/InverseModelRetreat.cpp \
    hypotheses/CommandHypothesis.cpp \
    models/games/Game.cpp \
    models/games/DefaultGame.cpp \
    Watchdog.cpp \
    models/InverseModelTrack.cpp \
    models/games/EvasionGame.cpp \
    hypotheses/TargetDirectionHypothesis.cpp \
    hypotheses/TargetConfidences.cpp \
    models/InverseModelPincer.cpp \
    gui/GroupMarker.cpp \
    gui/Marker.cpp \
    gui/OSGAdapterWidget.cpp \
    gui/UnitMarker.cpp \
    gui/camtriangle.cpp \
    gui/compass2.cpp \
    gui/guihud.cpp \
    gui/hammerqt.cpp \
    gui/mapmotionmodel.cpp \
    hypotheses/DirectionHypothesis.cpp \
    hypotheses/Hypothesis.cpp \
    hypotheses/HypothesisClient.cpp \
    hypotheses/HypothesisManager.cpp \
    hypotheses/HypothesisResults.cpp \
    hypotheses/TargetHypothesis.cpp \
    logic/Feature.cpp \
    logic/PlanAdaptor.cpp \
    logic/PlanCombiner.cpp \
    models/InverseModelAttack.cpp \
    models/InverseModelDefend.cpp \
    models/InverseModelFormation.cpp \
    models/InverseModelPath.cpp \
    models/inc/InverseModel.cpp \
    models/inc/Observations.cpp \
    models/inc/Targets.cpp \
    models/inc/TerrainMap.cpp \
    models/inc/micropather.cpp \
    terrain/SurfaceObject.cpp \
    terrain/TerrainLayer.cpp \
    terrain/terrain.cpp \
    terrain/terrain_PGM.cpp \
    units/PathInterpolation.cpp \
    units/Projectile.cpp \
    units/Unit.cpp \
    units/UnitConfigData.cpp \
    units/UnitConfigHandler.cpp \
    units/UnitConfigSchema.cpp \
    units/UnitFiring.cpp \
    units/UnitGroup.cpp \
    units/UnitObserver.cpp \
    units/UnitRemote.cpp \
    units/UnitSoldier.cpp \
    units/UnitTank.cpp \
    units/spline.cpp \
    HTAppBase.cpp \
    HTAppQt.cpp \
    HTPhysicsController.cpp \
    HTapp.cpp \
    ScenarioData.cpp \
    ScenarioHandler.cpp \
    application_HT.cpp \
    common.cpp \
    eventlog.cpp \
    mynetwork.cpp \
    packets.cpp \
    main.cpp \
    models/games/DirectionGame.cpp \
    models/games/MazeGame.cpp
FORMS += hammerqt.ui
RESOURCES += Icons.qrc
INCLUDEPATH += $${DELTA3D}/inc \
    $${DELTA3D}/ext/inc \
    $${DELTA3D}/ext/inc/CEGUI \
    /usr/include/eigen2 \
    /usr/local/include/clustering \
    /usr/include/CEGUI \
    /usr/include/hawknl 
LIBS += -lgne \
    -lboost_serialization \
    -lncurses \
    -losg \
	-losgGA \
	-losgDB \
	-losgUtil \
	-losgSim \
	-lode \
	-lxerces-c \
	-lgslcblas \
    -lNL \
    -losgViewer \
    -lCEGUIBase \
    -lCEGUIOpenGLRenderer \
    -lgsl \
    -lagg \
    -ldtCore \
    -ldtUtil \
    -ldtNet \
    -ldtABC \
    -ldtGUI \
    -ldtAnim \
    -ldtTerrain \
    -lqwt \
    -lboost_thread-mt \
    -lX11 \
    -L$${DELTA3D}/lib \
    -L$${DELTA3D}/ext/lib
suse_CEGUI:LIBS += -lCEGUILibxmlParser
else:LIBS += -lCEGUIXercesParser
