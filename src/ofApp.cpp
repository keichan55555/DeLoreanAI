#include "ofApp.h"


// ============================================================
// Setup
// ============================================================

void ofApp::setup()
{
	ofSetFrameRate(
		60
	);


	ofSetVerticalSync(
		true
	);


	ofEnableDepthTest();

	ofEnableAntiAliasing();


	// ========================================================
	// Animator
	// ========================================================

	animator.setup();


	// ========================================================
	// DeLorean
	// ========================================================

	const bool loaded =
		car.setup(
			"models/DeLorean.glb"
		);


	if (!loaded)
	{
		ofLogError()
			<< "Could not setup DeLorean.";
	}


	// Blenderで1m = 1なら
	// まず100から試す
	car.setRenderScale(
		100.0f
	);


	car.setWorldOffset(
		glm::vec3(
			0.0f,
			0.0f,
			0.0f
		)
	);


	// ========================================================
	// Camera
	// ========================================================

	cam.setPosition(
		450.0f,
		280.0f,
		520.0f
	);


	cam.lookAt(
		glm::vec3(
			0.0f,
			70.0f,
			0.0f
		)
	);
}



// ============================================================
// Update
// ============================================================

void ofApp::update()
{
	const float dt =
		ofGetLastFrameTime();


	// 物理アニメーション計算
	animator.update(
		dt
	);


	// 計算結果を実モデルへ反映
	car.update(
		animator.getPose()
	);
}



// ============================================================
// Draw
// ============================================================

void ofApp::draw()
{
	ofBackground(
		22
	);


	ofEnableDepthTest();


	// ========================================================
	// 3D
	// ========================================================

	cam.begin();


	drawGround();


	car.draw();


	// 必要なら座標確認
	//
	// ofDrawAxis(100);


	cam.end();


	// ========================================================
	// UI
	// ========================================================

	ofDisableDepthTest();


	std::string info;


	info +=
		"SPACE : Idle\n";


	info +=
		"L     : Listening\n";


	info +=
		"T     : Thinking\n\n";


	info +=
		"1     : Happy\n";


	info +=
		"2     : Surprised\n\n";


	info +=
		"Mode     : "
		+
		modeText()
		+
		"\n";


	info +=
		"Reaction : "
		+
		reactionText()
		+
		"\n";


	info +=
		"Airborne : ";


	info +=
		animator.isAirborne()
		?
		"YES"
		:
		"NO";


	ofSetColor(
		255
	);


	ofDrawBitmapStringHighlight(
		info,
		20,
		30
	);
}



// ============================================================
// Ground
// ============================================================

void ofApp::drawGround()
{
	const float step =
		50.0f;


	const int count =
		12;


	ofSetColor(
		65
	);


	// X方向
	for (
		int i = -count;
		i <= count;
		++i
	)
	{
		const float p =
			i
			* step;


		ofDrawLine(
			-count * step,
			0.0f,
			p,

			 count * step,
			0.0f,
			p
		);


		// Z方向
		ofDrawLine(
			p,
			0.0f,
			-count * step,

			p,
			0.0f,
			 count * step
		);
	}
}



// ============================================================
// Keyboard
// ============================================================

void ofApp::keyPressed(
	int key)
{
	switch (key)
	{
		// ====================================================
		// Idle
		// ====================================================

		case ' ':
		{
			animator.setMode(
				VehicleMode::Idle
			);

			break;
		}


		// ====================================================
		// Listening
		// ====================================================

		case 'l':
		case 'L':
		{
			animator.setMode(
				VehicleMode::Listening
			);

			break;
		}


		// ====================================================
		// Thinking
		// ====================================================

		case 't':
		case 'T':
		{
			animator.setMode(
				VehicleMode::Thinking
			);

			break;
		}


		// ====================================================
		// Happy
		// ====================================================

		case '1':
		{
			animator.trigger(
				VehicleReaction::Happy,
				0.9f
			);

			break;
		}


		// ====================================================
		// Surprised
		// ====================================================

		case '2':
		{
			animator.trigger(
				VehicleReaction::Surprised,
				0.9f
			);

			break;
		}
	}
}



// ============================================================
// UI text
// ============================================================

std::string ofApp::modeText() const
{
	switch (
		animator.getMode()
	)
	{
		case VehicleMode::Idle:
			return "Idle";


		case VehicleMode::Listening:
			return "Listening";


		case VehicleMode::Thinking:
			return "Thinking";
	}


	return "Unknown";
}


std::string ofApp::reactionText() const
{
	switch (
		animator.getReaction()
	)
	{
		case VehicleReaction::None:
			return "None";


		case VehicleReaction::Happy:
			return "Happy";


		case VehicleReaction::Surprised:
			return "Surprised";
	}


	return "Unknown";
}
