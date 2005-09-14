
#include "gui/scrollbar.h"
//Added by qt3to4:
#include <QMouseEvent>

void cVerticalScrollBar::scrollUp(cControl *sender) {
	setPos(qMax<int>(min(), pos() - 1));
}

void cVerticalScrollBar::scrollDown(cControl *sender) {
	setPos(qMin<int>(max(), pos() + 1));
}

cVerticalScrollBar::cVerticalScrollBar(int x, int y, unsigned int height) : btnUp(0), btnDown(0), handle(0) {
	scrollCallback_ = 0;

	btnUp = new cImageButton(0, 0, 0xFA, 0xFB);
	setBounds(x, y, btnUp->width(), height); // Set Width before adding another button
	connect(btnUp, SIGNAL(onButtonPress(cControl*)), this, SLOT(scrollUp(cControl*)));
	btnUp->setPressRepeatRate(60);
	addControl(btnUp);

	btnDown = new cImageButton(0, 0, 0xFC, 0xFD);
	btnDown->setPosition(0, height - btnDown->height());
	connect(btnDown, SIGNAL(onButtonPress(cControl*)), this, SLOT(scrollDown(cControl*)));
	btnDown->setPressRepeatRate(60);
	addControl(btnDown);

	background = new cTiledGumpImage(0x100);
	background->setBounds(0, btnUp->height(), btnUp->width(), height - btnUp->height() - btnDown->height());
	addControl(background, true);

	handle = new cGumpImage(0xfe);
	handle->setPosition(0, btnUp->height());
	addControl(handle);

	draggingHandle_ = false;

	pos_ = 0;
	setRange(0, 0);
}

cVerticalScrollBar::~cVerticalScrollBar() {
}

cControl *cVerticalScrollBar::getControl(int x, int y) {
	cControl *result = cContainer::getControl(x, y);

	if (result == background || result == handle) {
		return this; // All events should go to us instead
	} else {
		return result;
	}
}

void cVerticalScrollBar::onMouseDown(QMouseEvent *e) {
	QPoint pos = mapFromGlobal(e->pos());

	if (pos.y() >= handle->y() && pos.y() < handle->y() + handle->height()) {
		mouseDownY = pos.y() - handle->y();
		draggingHandle_ = true;
	}
}

void cVerticalScrollBar::onMouseUp(QMouseEvent *e) {
	draggingHandle_ = false;
}

void cVerticalScrollBar::onMouseMotion(int xrel, int yrel, QMouseEvent *e) {
	if (draggingHandle_) {
		int y = mapFromGlobal(e->pos()).y(); // Get the y position relative to our control

		int newpos = getPosFromTrackerY(y);
		setPos(newpos);
		//handle->setPosition(handle->x(), newy);
	}
}

unsigned int cVerticalScrollBar::getTrackerYFromPos(int pos) {
	if (pos == (int)min_) {
		return btnUp->height();
	} else if (pos == (int)max_) {
		return height_ - btnDown->height() - handle->height();
	} else {
		return btnUp->height() + (int)(pos * pixelPerStep);
	}
}

unsigned int cVerticalScrollBar::getPosFromTrackerY(int y) {
	if (y < btnUp->height() + handle->height()) {
		return min_;
	} else if (y > height_ - btnDown->height() - handle->height()) {
		return max_;
	} else {
		return qMin<int>(max_, qMax<int>(min_, ((y - (int)btnUp->height()) / pixelPerStep)));
	}
}

void cVerticalScrollBar::onScroll(int oldpos) {
	if (scrollCallback_) {
		scrollCallback_(this, oldpos);
	}
	emit scrolled(pos_);
}

void cVerticalScrollBar::setHandleId(ushort id) {
	if (handle) {
		handle->setId(id);
		handle->setY(getTrackerYFromPos(pos_));
	}
}

void cVerticalScrollBar::setBackgroundId(ushort id) {
	if (background) {
		background->setId(id);
	}
}

void cVerticalScrollBar::setUpButtonIds(ushort unpressed, ushort pressed, ushort hover) {
	if (btnUp) {
		btnUp->setStateGump(BS_UNPRESSED, unpressed);
		btnUp->setStateGump(BS_PRESSED, pressed);
		btnUp->setStateGump(BS_HOVER, hover);
		setBounds(x_, y_, btnUp->width(), height_); // Set Width before adding another button

		// Reposition background
		background->setBounds(0, btnUp->height(), btnUp->width(), height_ - btnUp->height() - btnDown->height());
		handle->setY(getTrackerYFromPos(pos_));
	}
}

void cVerticalScrollBar::setDownButtonIds(ushort unpressed, ushort pressed, ushort hover) {
	if (btnDown) {
		btnDown->setStateGump(BS_UNPRESSED, unpressed);
		btnDown->setStateGump(BS_PRESSED, pressed);
		btnDown->setStateGump(BS_HOVER, hover);
		btnDown->setPosition(0, height_ - btnDown->height());
		setBounds(x_, y_, btnDown->width(), height_); // Set Width before adding another button

		// Reposition background
		background->setBounds(0, btnUp->height(), btnUp->width(), height_ - btnUp->height() - btnDown->height());
		handle->setY(getTrackerYFromPos(pos_));
	}
}

void cVerticalScrollBar::onChangeBounds(int oldx, int oldy, int oldwidth, int oldheight) {
	pixelPerStep = qMax<float>(0, (float)getInnerHeight() / (float)getValues());
	cContainer::onChangeBounds(oldx, oldy, oldwidth, oldheight);
}
