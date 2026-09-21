#pragma once

#include "ofMain.h"
#include <llama/llama.h>

#include <string>

class LLMEngine
{
public:
	LLMEngine() = default;
	~LLMEngine();

	bool setup(
		const std::string& modelPath
	);

	std::string generate(
		const std::string& userPrompt,
		int maxTokens = 128
	);

	bool isReady() const
	{
		return ready;
	}

private:
	llama_model* model = nullptr;
	llama_context* ctx = nullptr;
	llama_sampler* sampler = nullptr;

	const llama_vocab* vocab = nullptr;

	bool ready = false;

	void clear();
};
