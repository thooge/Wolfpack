
#include "game/mobile.h"
#include "engine.h"

cMobile::cMobile(unsigned short x, unsigned short y, signed char z, enFacet facet, unsigned int serial) : cDynamicEntity(x, y, z, facet, serial) {
	body_ = 1;
	hue_ = 0;
	direction_ = 0;
	partialHue_ = false;
	type_ = MOBILE;
	sequence_ = 0;
	currentAction_ = getIdleAction();
	currentActionEnd_ = 0;
	nextFrame = 0;
	frame = 0;
	smoothMoveEnd = 0;
}

void cMobile::smoothMove(int xoffset, int yoffset, unsigned int duration) {
	drawxoffset = xoffset;
	drawyoffset = yoffset;
	smoothMoveTime = duration;
	smoothMoveEnd = SDL_GetTicks() + duration;
}

cMobile::~cMobile() {
	freeSequence();

	if (this == Player) {
		Player = 0; // Reset player to null -> important
	}
}

void cMobile::playAction(unsigned char action, unsigned int duration) {
	if (currentAction_ != action) {
		freeSequence(); // Free old sequence if the actions dont match
	}

	// Set the action and the duration
	currentAction_ = action;
	currentActionEnd_ = SDL_GetTicks() + duration;
}

void cMobile::freeSequence() {
	if (sequence_) {
		sequence_->decref();
		sequence_ = 0;
	}
}

void cMobile::refreshSequence() {
	if (sequence_) {
		sequence_->decref();
	}

	sequence_ = Animations->readSequence(body_, currentAction_, direction_, hue_, partialHue_);	
	
	// Try to maintain the flow of the animation
	if (sequence_ && frame >= sequence_->frameCount()) {
		frame = 0;
		nextFrame = SDL_GetTicks() + getFrameDelay();
	}
}

unsigned int cMobile::getFrameDelay() {
	return 370;
}

unsigned char cMobile::getIdleAction() {
	return 0;
}

void cMobile::draw(int cellx, int celly, int leftClip, int topClip, int rightClip, int bottomClip) {
	//return; // Don't draw yet

	// See if the current action expired
	if (currentActionEnd_ != 0 && currentActionEnd_ < SDL_GetTicks()) {
		freeSequence(); // Free current surface
		currentActionEnd_ = 0; // Reset end time
		currentAction_ = getIdleAction();
	}

	// Refresh the sequence
	if (!sequence_) {
		refreshSequence();
	}

	// Modify cellx/celly based on the smooth move settings
	// Smooth move handling. 
	if (smoothMoveEnd != 0) {
		int moveProgress = smoothMoveTime - (smoothMoveEnd - SDL_GetTicks());
		if (moveProgress < 0 || moveProgress >= (int)smoothMoveTime) {
			smoothMoveEnd = 0;
		} else {
			if (moveProgress <= 0) {
				cellx += drawxoffset;
				celly += drawyoffset;
				frame = 0;
			} else {
				float factor = 1.0f - (float)moveProgress / (float)smoothMoveTime;
				cellx += (int)(factor * (float)drawxoffset);
				celly += (int)(factor * (float)drawyoffset);
				if (sequence_) {
					frame = (int)(sequence_->frameCount() * factor);
				}
			}
		}
	}

	// Draw
	if (sequence_) {
		// Skip to next frame
		if (smoothMoveEnd == 0 && nextFrame < SDL_GetTicks()) {
			if (++frame >= sequence_->frameCount()) {
				frame = 0;
			}
			nextFrame = SDL_GetTicks() + getFrameDelay();
		}
		
		// The anims facing right are generated by flipping the ones facing left
		bool flip = (direction_ > 0 && direction_ < 4);
		sequence_->draw(frame, cellx, celly, flip);
	}
}

bool cMobile::hitTest(int x, int y) {
	return false;
}

void cMobile::updatePriority() {
	priority_ = z_ + 2;
}

cMobile *Player = 0;
