#pragma once

#include "ofMain.h"

#include "VehicleAnimator.h"
#include "DeLorean.h"
#include "LLMEngine.h"
#include <future>
#include <chrono>
#include "TTSEngine.h"

struct AIReply
{
	std::string speech = "";
	std::string reaction = "none";
	float intensity = 0.0f;
	bool valid = false;
};


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
	
	// ==========================
	// LLM
	// ==========================

	LLMEngine llm;

	std::string aiResponse;

	std::future<std::string> llmFuture;

	bool llmGenerating = false;

	AIReply parseAIReply(
		const std::string& rawText
	);

	void askDeLorean(
		const std::string& userText
	);

	// ========================================================
	// Drawing
	// ========================================================

	void drawGround();


	// ========================================================
	// UI
	// ========================================================

	std::string modeText() const;

	std::string reactionText() const;
	
	
	// ========================================================
	// TTS
	// ========================================================
	
	TTSEngine tts;

	ofSoundPlayer voicePlayer;

	std::future<bool> ttsFuture;

	bool ttsGenerating = false;

	std::string ttsOutputPath;
	
	void speakText(
		const std::string& text,
		const std::string& language
	);
	
	std::string detectTTSLanguage(
		const std::string& text
	);
	
	VehicleReaction pendingReaction =
		VehicleReaction::None;

	float pendingReactionIntensity = 0.0f;

	bool hasPendingReaction = false;
	bool wasVoicePlaying = false;
	
	std::vector<float> speechEnvelope;
};
