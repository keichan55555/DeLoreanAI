#include "TTSEngine.h"

#include <algorithm>
#include <vector>
#include <cmath>


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
	ready = false;

	try
	{
		voiceStylesDir = voiceStylesDirectory;

		tts = loadTextToSpeech(
			env,
			onnxDir,
			false
		);

		if (!tts)
		{
			ofLogError()
				<< "TTSEngine: failed to load TTS models.";

			return false;
		}

		if (!setVoice(defaultVoice))
		{
			return false;
		}

		ready = true;

		ofLogNotice()
			<< "TTSEngine ready. Voice = "
			<< currentVoice;

		return true;
	}
	catch (const std::exception& e)
	{
		ofLogError()
			<< "TTSEngine setup error: "
			<< e.what();

		return false;
	}
}


// --------------------------------------------------------------
bool TTSEngine::setVoice(
	const std::string& voiceName)
{
	try
	{
		const std::string path =
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

		currentVoice = voiceName;

		ofLogNotice()
			<< "TTS voice = "
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

	if (text.empty())
	{
		ofLogWarning()
			<< "TTSEngine: empty text.";

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

		const int sampleRate =
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
			result.wav.begin() + wavLength
		);
		
		// ----------------------------------------------------------
		// 発話アニメーション用の音量エンベロープを作る
		// 約20msごとのRMSを計算
		// ----------------------------------------------------------

		lastEnvelope.clear();

		const std::size_t windowSize =
			std::max<std::size_t>(
				1,
				static_cast<std::size_t>(
					sampleRate * 0.02f
				)
			);

		float maxRms = 0.0f;


		for (
			std::size_t start = 0;
			start < wavOut.size();
			start += windowSize
		)
		{
			const std::size_t end =
				std::min(
					start + windowSize,
					wavOut.size()
				);


			double sumSquares = 0.0;


			for (
				std::size_t i = start;
				i < end;
				++i
			)
			{
				const float sample =
					wavOut[i];

				sumSquares +=
					sample * sample;
			}


			const std::size_t count =
				end - start;


			const float rms =
				count > 0
				? static_cast<float>(
					std::sqrt(
						sumSquares /
						static_cast<double>(count)
					)
				)
				: 0.0f;


			lastEnvelope.push_back(rms);

			maxRms =
				std::max(
					maxRms,
					rms
				);
		}


		// 0～1へ正規化
		if (maxRms > 0.00001f)
		{
			for (float& value : lastEnvelope)
			{
				value /= maxRms;

				value =
					ofClamp(
						value,
						0.0f,
						1.0f
					);
			}
		}

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
