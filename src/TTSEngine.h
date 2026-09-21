#pragma once

#include "ofMain.h"
#include "helper.h"

#include <memory>
#include <string>
#include <vector>


class TTSEngine
{
public:
	TTSEngine();

	bool setup(
		const std::string& onnxDir,
		const std::string& voiceStylesDir,
		const std::string& defaultVoice = "M1"
	);

	bool setVoice(
		const std::string& voiceName
	);

	bool synthesizeToFile(
		const std::string& text,
		const std::string& language,
		const std::string& outputPath,
		float speed = 1.05f,
		int totalSteps = 8
	);

	bool isReady() const
	{
		return ready;
	}

	const std::string& getCurrentVoice() const
	{
		return currentVoice;
	}
	
	const std::vector<float>& getLastEnvelope() const
	{
		return lastEnvelope;
	}


private:
	Ort::Env env;
	Ort::MemoryInfo memoryInfo;

	std::unique_ptr<TextToSpeech> tts;
	std::unique_ptr<Style> style;

	std::string voiceStylesDir;
	std::string currentVoice;

	bool ready = false;
	
	std::vector<float> lastEnvelope;
};
