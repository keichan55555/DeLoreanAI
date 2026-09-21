#include "WhisperEngine.h"
#include <whisper/whisper.h>

#include <algorithm>
#include <cctype>
#include <thread>


WhisperEngine::~WhisperEngine()
{
	clear();
}


// --------------------------------------------------------------
void WhisperEngine::clear()
{
	if (ctx != nullptr)
	{
		whisper_free(ctx);
		ctx = nullptr;
	}

	ready = false;
}


// --------------------------------------------------------------
bool WhisperEngine::setup(
	const std::string& modelPath)
{
	clear();

	ofLogNotice()
		<< "Loading Whisper model: "
		<< modelPath;


	whisper_context_params cparams =
		whisper_context_default_params();


	// Apple SiliconではMetalを使用
	cparams.use_gpu = true;

	// 対応していれば高速化
	cparams.flash_attn = true;


	ctx =
		whisper_init_from_file_with_params(
			modelPath.c_str(),
			cparams
		);


	if (ctx == nullptr)
	{
		ofLogError()
			<< "Failed to load Whisper model.";

		return false;
	}


	ready = true;


	ofLogNotice()
		<< "Whisper ready.";

	ofLogNotice()
		<< "Multilingual: "
		<< (
			whisper_is_multilingual(ctx)
			? "yes"
			: "no"
		);


	return true;
}


// --------------------------------------------------------------
std::string WhisperEngine::transcribe(
	const std::vector<float>& pcm16kMono,
	const std::string& language)
{
	if (!ready || ctx == nullptr)
	{
		ofLogError()
			<< "WhisperEngine is not ready.";

		return "";
	}


	if (pcm16kMono.empty())
	{
		ofLogWarning()
			<< "Whisper received empty audio.";

		return "";
	}


	// Whisper標準の推論設定
	whisper_full_params params =
		whisper_full_default_params(
			WHISPER_SAMPLING_GREEDY
		);


	params.print_progress =
		false;

	params.print_realtime =
		false;

	params.print_timestamps =
		false;

	params.print_special =
		false;


	params.translate =
		false;


	params.no_context =
		true;

	params.no_timestamps =
		true;


	params.single_segment =
		true;


	// 日本語・英語の自動判定にも対応
	params.language =
		language.c_str();

	params.detect_language =
		(language == "auto");


	// CPU側で使うスレッド数
	const unsigned int hardwareThreads =
		std::thread::hardware_concurrency();


	params.n_threads =
		std::max(
			1,
			std::min(
				4,
				static_cast<int>(
					hardwareThreads
				)
			)
		);


	params.temperature =
		0.0f;


	ofLogNotice()
		<< "Whisper transcription started. Samples = "
		<< pcm16kMono.size();


	const int result =
		whisper_full(
			ctx,
			params,
			pcm16kMono.data(),
			static_cast<int>(
				pcm16kMono.size()
			)
		);


	if (result != 0)
	{
		ofLogError()
			<< "whisper_full failed: "
			<< result;

		return "";
	}


	std::string text;


	const int segmentCount =
		whisper_full_n_segments(
			ctx
		);


	for (
		int i = 0;
		i < segmentCount;
		++i
	)
	{
		const char* segment =
			whisper_full_get_segment_text(
				ctx,
				i
			);


		if (segment != nullptr)
		{
			text += segment;
		}
	}


	text =
		trim(text);


	ofLogNotice()
		<< "WHISPER: "
		<< text;


	return text;
}


// --------------------------------------------------------------
std::string WhisperEngine::trim(
	const std::string& text)
{
	auto first =
		std::find_if_not(
			text.begin(),
			text.end(),

			[](unsigned char c)
			{
				return std::isspace(c);
			}
		);


	auto last =
		std::find_if_not(
			text.rbegin(),
			text.rend(),

			[](unsigned char c)
			{
				return std::isspace(c);
			}
		).base();


	if (first >= last)
	{
		return "";
	}


	return std::string(
		first,
		last
	);
}
