#pragma once

#include "ofMain.h"

#include <string>
#include <vector>


// whisper.hをここではincludeしない
struct whisper_context;


class WhisperEngine
{
public:
	WhisperEngine() = default;
	~WhisperEngine();

	bool setup(
		const std::string& modelPath
	);

	std::string transcribe(
		const std::vector<float>& pcm16kMono,
		const std::string& language = "auto"
	);

	bool isReady() const
	{
		return ready;
	}


private:
	whisper_context* ctx = nullptr;

	bool ready = false;

	void clear();

	static std::string trim(
		const std::string& text
	);
};
