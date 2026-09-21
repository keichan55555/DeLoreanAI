#include "TTSEngine.h"

#include <algorithm>


TTSEngine::TTSEngine()
:
env(
	ORT_LOGGING_LEVEL_WARNING,
	"DeLoreanTTS"
),
memoryInfo(
	Ort::MemoryInfo::CreateCpu(
		OrtAllocatorType::OrtArenaAllocator,
		OrtMemType::OrtMemTypeDefault
	)
)
{
}


// --------------------------------------------------------------
bool TTSEngine::setup(
	const std::string& onnxDir,
	const std::string& voiceStylesDirectory,
	const std::string& defaultVoice)
{
	try
	{
		voiceStylesDir =
			voiceStylesDirectory;


		// ONNXモデルロード
		tts =
			loadTextToSpeech(
				env,
				onnxDir,
				false
			);


		if (!tts)
		{
			ofLogError()
				<< "TTSEngine: "
				<< "Could not load TTS.";

			return false;
		}


		if (!setVoice(defaultVoice))
		{
			return false;
		}


		ready = true;


		ofLogNotice()
			<< "TTSEngine ready. Voice: "
			<< currentVoice;


		return true;
	}
	catch (const std::exception& e)
	{
		ofLogError()
			<< "TTSEngine setup error: "
			<< e.what();

		ready = false;

		return false;
	}
}


// --------------------------------------------------------------
bool TTSEngine::setVoice(
	const std::string& voiceName)
{
	try
	{
		std::string path =
			voiceStylesDir
			+ "/"
			+ voiceName
			+ ".json";


		Style loadedStyle =
			loadVoiceStyle(
				{ path },
				false
			);


		style =
			std::make_unique<Style>(
				std::move(loadedStyle)
			);


		currentVoice =
			voiceName;


		ofLogNotice()
			<< "TTS voice changed to: "
			<< currentVoice;


		return true;
	}
	catch (const std::exception& e)
	{
		ofLogError()
			<< "TTS voice load error: "
			<< e.what();

		return false;
	}
}


// --------------------------------------------------------------
bool TTSEngine::synthesizeToFile(
	const std::string& text,
	const std::string& language,
	const std::string& outputPath,
	float speed,
	int totalSteps)
{
	if (!ready || !tts || !style)
	{
		ofLogError()
			<< "TTSEngine is not ready.";

		return false;
	}


	try
	{
		auto result =
			tts->call(
				memoryInfo,
				text,
				language,
				*style,
				totalSteps,
				speed
			);


		if (
			result.wav.empty() ||
			result.duration.empty()
		)
		{
			ofLogError()
				<< "TTS generated empty audio.";

			clearTensorBuffers();

			return false;
		}


		int sampleRate =
			tts->getSampleRate();


		std::size_t wavLength =
			static_cast<std::size_t>(
				sampleRate *
				result.duration[0]
			);


		wavLength =
			std::min(
				wavLength,
				result.wav.size()
			);


		std::vector<float> wavOut(
			result.wav.begin(),
			result.wav.begin()
				+ wavLength
		);


		writeWavFile(
			outputPath,
			wavOut,
			sampleRate
		);


		clearTensorBuffers();


		ofLogNotice()
			<< "TTS saved: "
			<< outputPath;


		return true;
	}
	catch (const std::exception& e)
	{
		clearTensorBuffers();


		ofLogError()
			<< "TTS synthesis error: "
			<< e.what();


		return false;
	}
}
