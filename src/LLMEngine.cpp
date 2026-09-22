#include "LLMEngine.h"

#include <vector>


LLMEngine::~LLMEngine()
{
	clear();
}


// ============================================================
// Setup
// ============================================================

bool LLMEngine::setup(
	const std::string& modelPath)
{
	clear();

	// CPU / Metal backendを読み込む
	ggml_backend_load_all();


	// ========================================================
	// Model
	// ========================================================

	llama_model_params modelParams =
		llama_model_default_params();

	// Apple SiliconではできるだけGPUへ
	modelParams.n_gpu_layers = 99;

	model =
		llama_model_load_from_file(
			modelPath.c_str(),
			modelParams
		);

	if (!model)
	{
		ofLogError("LLMEngine")
			<< "Failed to load model: "
			<< modelPath;

		return false;
	}


	vocab =
		llama_model_get_vocab(
			model
		);

	if (!vocab)
	{
		ofLogError("LLMEngine")
			<< "Failed to get vocab.";

		clear();

		return false;
	}


	// ========================================================
	// Context
	// ========================================================

	llama_context_params ctxParams =
		llama_context_default_params();

	ctxParams.n_ctx = 2048;
	ctxParams.n_batch = 2048;

	ctx =
		llama_init_from_model(
			model,
			ctxParams
		);

	if (!ctx)
	{
		ofLogError("LLMEngine")
			<< "Failed to create context.";

		clear();

		return false;
	}


	// ========================================================
	// Sampler
	// ========================================================

	sampler =
		llama_sampler_chain_init(
			llama_sampler_chain_default_params()
		);

	llama_sampler_chain_add(
		sampler,
		llama_sampler_init_temp(
			0.25f
		)
	);

	llama_sampler_chain_add(
		sampler,
		llama_sampler_init_dist(
			LLAMA_DEFAULT_SEED
		)
	);


	ready = true;


	// モデル名表示
	char description[512];

	llama_model_desc(
		model,
		description,
		sizeof(description)
	);

	ofLogNotice("LLMEngine")
		<< "Model loaded: "
		<< description;


	return true;
}


// ============================================================
// Generate
// ============================================================

std::string LLMEngine::generate(
	const std::string& userPrompt,
	int maxTokens)
{
	if (!ready)
	{
		return "";
	}
	
	// ========================================================
	// Qwen3.5 chat prompt
	//
	// <think></think> を先に閉じることで
	// reasoningを出力せず回答本文から生成させる
	// ========================================================

	std::string prompt;

	prompt +=
		"<|im_start|>system\n"
		"You are a friendly talking DeLorean car character. "
		"Respond naturally and briefly. "
		"Do not explain your reasoning."
		"<|im_end|>\n";

	prompt +=
		"<|im_start|>user\n";

	prompt +=
		userPrompt;

	prompt +=
		"<|im_end|>\n";

	prompt +=
		"<|im_start|>assistant\n"
		"<think>\n\n</think>\n\n"
		"{";
	
	
	// ========================================================
	// 前回のcontextをクリア
	// ========================================================

	llama_memory_clear(
		llama_get_memory(ctx),
		true
	);

	llama_sampler_reset(
		sampler
	);


	// ========================================================
	// Tokenize
	// ========================================================

	int nTokens =
		-llama_tokenize(
			vocab,
			prompt.c_str(),
			prompt.size(),
			nullptr,
			0,
			true,
			true
		);

	if (nTokens <= 0)
	{
		ofLogError("LLMEngine")
			<< "Could not calculate token count.";

		return "";
	}


	std::vector<llama_token>
		tokens(nTokens);


	int result =
		llama_tokenize(
			vocab,
			prompt.c_str(),
			prompt.size(),
			tokens.data(),
			tokens.size(),
			true,
			true
		);

	if (result < 0)
	{
		ofLogError("LLMEngine")
			<< "Tokenization failed.";

		return "";
	}


	// ========================================================
	// Prompt decode
	// ========================================================

	llama_batch batch =
		llama_batch_get_one(
			tokens.data(),
			tokens.size()
		);


	if (
		llama_decode(
			ctx,
			batch
		) != 0
	)
	{
		ofLogError("LLMEngine")
			<< "Prompt decode failed.";

		return "";
	}


	// ========================================================
	// Generate
	// ========================================================

	std::string response = "{";


	for (
		int i = 0;
		i < maxTokens;
		++i
	)
	{
		llama_token token =
			llama_sampler_sample(
				sampler,
				ctx,
				-1
			);


		if (
			llama_vocab_is_eog(
				vocab,
				token
			)
		)
		{
			break;
		}


		char buffer[512];


		int length =
			llama_token_to_piece(
				vocab,
				token,
				buffer,
				sizeof(buffer),
				0,
				true
			);


		if (length < 0)
		{
			break;
		}


		response.append(
			buffer,
			length
		);


		llama_batch nextBatch =
			llama_batch_get_one(
				&token,
				1
			);


		if (
			llama_decode(
				ctx,
				nextBatch
			) != 0
		)
		{
			break;
		}
	}


	return response;
}


// ============================================================
// Cleanup
// ============================================================

void LLMEngine::clear()
{
	ready = false;


	if (sampler)
	{
		llama_sampler_free(
			sampler
		);

		sampler = nullptr;
	}


	if (ctx)
	{
		llama_free(
			ctx
		);

		ctx = nullptr;
	}


	if (model)
	{
		llama_model_free(
			model
		);

		model = nullptr;
	}


	vocab = nullptr;
}
