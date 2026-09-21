#pragma once

#include "ofMain.h"

#include "VehicleAnimator.h"
#include "DeLorean.h"


class ofApp
	:
	public ofBaseApp
{
public:

	void setup();
	void update();
	void draw();

	void keyPressed(
		int key
	);


private:

	// ========================================================
	// Camera
	// ========================================================

	ofEasyCam cam;


	// ========================================================
	// Character
	// ========================================================

	VehicleAnimator animator;

	DeLorean car;


	// ========================================================
	// Drawing
	// ========================================================

	void drawGround();


	// ========================================================
	// UI
	// ========================================================

	std::string modeText() const;

	std::string reactionText() const;
};
