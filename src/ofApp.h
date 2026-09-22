#pragma once

#include "ofMain.h"

#include "VehicleAnimator.h"
#include "DeLorean.h"
#include "LLMEngine.h"
#include <future>
#include <chrono>
#include "TTSEngine.h"
#include "WhisperEngine.h"
#include <atomic>
#include <mutex>
#include <vector>

struct AIReply
{
	std::string speech = "";
	std::string reaction = "none";
	float intensity = 0.0f;
	bool valid = false;
};

struct ConversationTurn
{
	std::string user;
	std::string assistant;
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

	void audioIn(
		ofSoundBuffer& input
	);

	void startRecording();

	void stopRecording();

	std::vector<float> resampleTo16k(
		const std::vector<float>& input,
		int inputSampleRate
	);
	
	std::string buildConversationHistory() const;

	void addConversationTurn(
		const std::string& user,
		const std::string& assistant
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
	
	std::vector<ConversationTurn>
		conversationHistory;

	static constexpr std::size_t
		maxConversationTurns = 3;
	
	std::string currentUserText;

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
	
	VehicleReaction pendingReaction =
		VehicleReaction::None;

	float pendingReactionIntensity = 0.0f;

	bool hasPendingReaction = false;
	bool wasVoicePlaying = false;
	
	std::vector<float> speechEnvelope;
	
	// ========================================================
	// Whisper
	// ========================================================
	
	WhisperEngine whisper;
	
	ofSoundStream microphoneStream;

	std::atomic<bool> isRecording
	{
		false
	};

	std::mutex microphoneMutex;

	std::vector<float> recordedAudio;

	int microphoneSampleRate =
		48000;

	std::future<std::string>
		whisperFuture;

	bool whisperGenerating =
		false;
	
};
