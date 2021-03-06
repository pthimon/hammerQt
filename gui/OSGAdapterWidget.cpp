/* -*-c++-*-
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2007, Alion Science and Technology
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * David Guthrie
 */

#include "OSGAdapterWidget.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wextra"

#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgViewer/GraphicsWindow>

#include <dtCore/system.h>

#include <QPainter>
#include <QBrush>
#include <QColor>

#pragma GCC diagnostic warning "-Wextra"

namespace dtQt {
class QtKeyboardMap {

public:
	QtKeyboardMap() {
		mKeyMap[Qt::Key_Escape] = osgGA::GUIEventAdapter::KEY_Escape;
		mKeyMap[Qt::Key_Home] = osgGA::GUIEventAdapter::KEY_Home;
		mKeyMap[Qt::Key_Enter] = osgGA::GUIEventAdapter::KEY_KP_Enter;
		mKeyMap[Qt::Key_End] = osgGA::GUIEventAdapter::KEY_End;
		mKeyMap[Qt::Key_Return] = osgGA::GUIEventAdapter::KEY_Return;
		mKeyMap[Qt::Key_PageUp] = osgGA::GUIEventAdapter::KEY_Page_Up;
		mKeyMap[Qt::Key_PageDown] = osgGA::GUIEventAdapter::KEY_Page_Down;
		mKeyMap[Qt::Key_Left] = osgGA::GUIEventAdapter::KEY_Left;
		mKeyMap[Qt::Key_Right] = osgGA::GUIEventAdapter::KEY_Right;
		mKeyMap[Qt::Key_Up] = osgGA::GUIEventAdapter::KEY_Up;
		mKeyMap[Qt::Key_Down] = osgGA::GUIEventAdapter::KEY_Down;
		mKeyMap[Qt::Key_Backspace] = osgGA::GUIEventAdapter::KEY_BackSpace;
		mKeyMap[Qt::Key_Tab] = osgGA::GUIEventAdapter::KEY_Tab;
		mKeyMap[Qt::Key_Space] = osgGA::GUIEventAdapter::KEY_Space;
		mKeyMap[Qt::Key_Delete] = osgGA::GUIEventAdapter::KEY_Delete;

		mKeyMap[Qt::Key_F1] = osgGA::GUIEventAdapter::KEY_F1;
		mKeyMap[Qt::Key_F2] = osgGA::GUIEventAdapter::KEY_F2;
		mKeyMap[Qt::Key_F3] = osgGA::GUIEventAdapter::KEY_F3;
		mKeyMap[Qt::Key_F4] = osgGA::GUIEventAdapter::KEY_F4;
		mKeyMap[Qt::Key_F5] = osgGA::GUIEventAdapter::KEY_F5;
		mKeyMap[Qt::Key_F6] = osgGA::GUIEventAdapter::KEY_F6;
		mKeyMap[Qt::Key_F7] = osgGA::GUIEventAdapter::KEY_F7;
		mKeyMap[Qt::Key_F8] = osgGA::GUIEventAdapter::KEY_F8;
		mKeyMap[Qt::Key_F9] = osgGA::GUIEventAdapter::KEY_F9;
		mKeyMap[Qt::Key_F10] = osgGA::GUIEventAdapter::KEY_F10;
		mKeyMap[Qt::Key_F11] = osgGA::GUIEventAdapter::KEY_F11;
		mKeyMap[Qt::Key_F12] = osgGA::GUIEventAdapter::KEY_F12;
		mKeyMap[Qt::Key_F13] = osgGA::GUIEventAdapter::KEY_F13;
		mKeyMap[Qt::Key_F14] = osgGA::GUIEventAdapter::KEY_F14;
		mKeyMap[Qt::Key_F15] = osgGA::GUIEventAdapter::KEY_F15;
		mKeyMap[Qt::Key_F16] = osgGA::GUIEventAdapter::KEY_F16;
		mKeyMap[Qt::Key_F17] = osgGA::GUIEventAdapter::KEY_F17;
		mKeyMap[Qt::Key_F18] = osgGA::GUIEventAdapter::KEY_F18;
		mKeyMap[Qt::Key_F19] = osgGA::GUIEventAdapter::KEY_F19;
		mKeyMap[Qt::Key_F20] = osgGA::GUIEventAdapter::KEY_F20;

		mKeyMap[Qt::Key_hyphen] = '-';
		mKeyMap[Qt::Key_Equal] = '=';

		mKeyMap[Qt::Key_division] = osgGA::GUIEventAdapter::KEY_KP_Divide;
		mKeyMap[Qt::Key_multiply] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
		mKeyMap[Qt::Key_Minus] = '-';
		mKeyMap[Qt::Key_Plus] = '+';
		//mKeyMap[Qt::Key_H              ] = osgGA::GUIEventAdapter::KEY_KP_Home;
		//mKeyMap[Qt::Key_                    ] = osgGA::GUIEventAdapter::KEY_KP_Up;
		//mKeyMap[92                    ] = osgGA::GUIEventAdapter::KEY_KP_Page_Up;
		//mKeyMap[86                    ] = osgGA::GUIEventAdapter::KEY_KP_Left;
		//mKeyMap[87                    ] = osgGA::GUIEventAdapter::KEY_KP_Begin;
		//mKeyMap[88                    ] = osgGA::GUIEventAdapter::KEY_KP_Right;
		//mKeyMap[83                    ] = osgGA::GUIEventAdapter::KEY_KP_End;
		//mKeyMap[84                    ] = osgGA::GUIEventAdapter::KEY_KP_Down;
		//mKeyMap[85                    ] = osgGA::GUIEventAdapter::KEY_KP_Page_Down;
		mKeyMap[Qt::Key_Insert] = osgGA::GUIEventAdapter::KEY_KP_Insert;
		//mKeyMap[Qt::Key_Delete        ] = osgGA::GUIEventAdapter::KEY_KP_Delete;

	}

	~QtKeyboardMap() {
	}

	int remapKey(QKeyEvent* event) {
		KeyMap::iterator itr = mKeyMap.find(event->key());
		if (itr == mKeyMap.end()) {
			return int(*(event->text().toAscii().data()));
		} else {
			return itr->second;
		}
	}

private:
	typedef std::map<unsigned int, int> KeyMap;
	KeyMap mKeyMap;
};

static QtKeyboardMap STATIC_KEY_MAP;

//////////////////////////////////////////////////////////////////////////////////
OSGAdapterWidget::OSGAdapterWidget(QWidget* parent,
		const QGLWidget* shareWidget, Qt::WindowFlags f) :
	QGLWidget(parent, shareWidget, f) {
	setAutoBufferSwap(true);

	setMinimumHeight(300);
	setMinimumWidth(400);

	// This enables us to track mouse movement even when
	// no button is pressed.  The motion models depend
	// on tracking the mouse location to work properly.
	setMouseTracking(true);

	//enables keyboard events
	setFocusPolicy(Qt::StrongFocus);

	setAutoFillBackground(false);
}

//////////////////////////////////////////////////////////////////////////////////
OSGAdapterWidget::~OSGAdapterWidget() {
}

//////////////////////////////////////////////////////////////////////////////////
osgViewer::GraphicsWindow& OSGAdapterWidget::GetGraphicsWindow() {
	return *mGraphicsWindow;
}

//////////////////////////////////////////////////////////////////////////////////
const osgViewer::GraphicsWindow& OSGAdapterWidget::GetGraphicsWindow() const {
	return *mGraphicsWindow;
}

//////////////////////////////////////////////////////////////////////////////////
void OSGAdapterWidget::SetGraphicsWindow(osgViewer::GraphicsWindow& newWindow) {
	mGraphicsWindow = &newWindow;
	mGraphicsWindow->resized(0, 0, width(), height());
}

//////////////////////////////////////////////////////////////////////////////////
void OSGAdapterWidget::paintGL() {
	dtCore::System & system = dtCore::System::GetInstance();
	if (system.IsRunning()) {
		system.StepWindow();
	}
	//repaint the widget on the next iteration of the event loop
	update();
}

//////////////////////////////////////////////////////////////////////////////////
void OSGAdapterWidget::resizeGL(int width, int height) {
	if (mGraphicsWindow.valid()) {
		mGraphicsWindow->getEventQueue()->windowResize(0, 0, width, height);
		mGraphicsWindow->resized(0, 0, width, height);
	}
}

//////////////////////////////////////////////////////////////////////////////////
void OSGAdapterWidget::hideEvent( QHideEvent * /*event*/ ) {
	//still allow updates to the widget even if it has been hidden
	setAttribute(Qt::WA_WState_Visible, true);
}

//////////////////////////////////////////////////////////////////////////////////
void OSGAdapterWidget::keyPressEvent(QKeyEvent* event) {
	if (mGraphicsWindow.valid()) {
		int value = STATIC_KEY_MAP.remapKey(event);
		mGraphicsWindow->getEventQueue()->keyPress(value);
	}
}

//////////////////////////////////////////////////////////////////////////////////
void OSGAdapterWidget::keyReleaseEvent(QKeyEvent* event) {
	if (mGraphicsWindow.valid()) {
		int value = STATIC_KEY_MAP.remapKey(event);
		mGraphicsWindow->getEventQueue()->keyRelease(value);
	}
}

//////////////////////////////////////////////////////////////////////////////////
void OSGAdapterWidget::mousePressEvent(QMouseEvent* event) {
	int button = 0;
	switch (event->button()) {
	case (Qt::LeftButton):
		button = 1;
		break;
	case (Qt::MidButton):
		button = 2;
		break;
	case (Qt::RightButton):
		button = 3;
		break;
	case (Qt::NoButton):
		button = 0;
		break;
	default:
		button = 0;
		break;
	}
	if (mGraphicsWindow.valid()) {
		mGraphicsWindow->getEventQueue()->mouseButtonPress(event->x(),
				event->y(), button);
	}
}

//////////////////////////////////////////////////////////////////////////////////
void OSGAdapterWidget::mouseReleaseEvent(QMouseEvent* event) {
	int button = 0;
	switch (event->button()) {
	case Qt::LeftButton:
		button = 1;
		break;
	case Qt::MidButton:
		button = 2;
		break;
	case Qt::RightButton:
		button = 3;
		break;
	case Qt::NoButton:
		button = 0;
		break;
	default:
		button = 0;
		break;
	}

	if (mGraphicsWindow.valid()) {
		mGraphicsWindow->getEventQueue()->mouseButtonRelease(event->x(),
				event->y(), button);
	}
}

//////////////////////////////////////////////////////////////////////////////////
void OSGAdapterWidget::mouseMoveEvent(QMouseEvent* event) {
	if (mGraphicsWindow.valid()) {
		mGraphicsWindow->getEventQueue()->mouseMotion(event->x(), event->y());
	}
}

void OSGAdapterWidget::wheelEvent(QWheelEvent * event) {
	if (mGraphicsWindow.valid()) {
		if (event->delta() > 0) {
			mGraphicsWindow->getEventQueue()->mouseScroll(
					osgGA::GUIEventAdapter::SCROLL_UP);
			mGraphicsWindow->getEventQueue()->mouseScroll(
					osgGA::GUIEventAdapter::SCROLL_UP);
			mGraphicsWindow->getEventQueue()->mouseScroll(
					osgGA::GUIEventAdapter::SCROLL_UP);
		} else if (event->delta() < 0) {
			mGraphicsWindow->getEventQueue()->mouseScroll(
					osgGA::GUIEventAdapter::SCROLL_DOWN);
			mGraphicsWindow->getEventQueue()->mouseScroll(
					osgGA::GUIEventAdapter::SCROLL_DOWN);
			mGraphicsWindow->getEventQueue()->mouseScroll(
					osgGA::GUIEventAdapter::SCROLL_DOWN);
		}
	}
}
}
