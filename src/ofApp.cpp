#include "ofApp.h"
#include <llama/llama.h>


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
	
	ofLogNotice()
		<< "llama version: "
		<< llama_version();
	
	// ============================================================
	// Lighting
	// ============================================================

	float keyStrength = 2.0f;
	float rimStrength = 2.0f;
	
	ofSetGlobalAmbientColor(
		ofColor(55, 55, 55)
	);

	// Main light
	keyLight.setup();

	keyLight.setPointLight();

	keyLight.setPosition(
		350.0f,
		450.0f,
		300.0f
	);

	keyLight.setDiffuseColor(
		ofColor(
			255 * keyStrength,
			255 * keyStrength,
			255 * keyStrength
		)
	);

	keyLight.setSpecularColor(
		ofColor(
			255 * keyStrength,
			255 * keyStrength,
			255 * keyStrength
		)
	);


	// Rim light
	rimLight.setup();

	rimLight.setPointLight();

	rimLight.setPosition(
		-350.0f,
		250.0f,
		-300.0f
	);

	rimLight.setDiffuseColor(
		ofColor(
			255 * rimStrength,
			255 * rimStrength,
			255 * rimStrength
		)
	);

	rimLight.setSpecularColor(
		ofColor(
			255 * rimStrength,
			255 * rimStrength,
			255 * rimStrength
		)
	);
	
	
	// ============================================================
	// LLM
	// ============================================================

	const std::string modelPath =
		ofToDataPath(
			"llm/Qwen3.5-4B-Q4_K_M.gguf",
			true
		);


	ofLogNotice()
		<< "LLM model path: "
		<< modelPath;


	if (!llm.setup(modelPath))
	{
		ofLogError()
			<< "LLM setup failed.";
	}
	else
	{
		ofLogNotice()
			<< "LLM ready.";
	}
	
	
	// ============================================================
	// TTS
	// ============================================================
	
	ttsOutputPath =
		ofToDataPath(
			"tts/output/current.wav",
			true
		);

	ofDirectory outputDir(
		ofToDataPath(
			"tts/output",
			true
		)
	);

	if (!outputDir.exists())
	{
		outputDir.create(true);
	}

	bool ttsOk =
		tts.setup(
			ofToDataPath(
				"tts/onnx",
				true
			),

			ofToDataPath(
				"tts/voice_styles",
				true
			),

			"M1"
		);

	if (ttsOk)
	{
		ofLogNotice()
			<< "Supertonic ready.";
	}
	else
	{
		ofLogError()
			<< "Supertonic setup failed.";
	}
	
	
	// ============================================================
	// Whisper
	// ============================================================
	
	bool whisperOk =
		whisper.setup(
			ofToDataPath(
				"whisper/ggml-base.bin",
				true
			)
		);


	if (whisperOk)
	{
		ofLogNotice()
			<< "Whisper initialized successfully.";
	}
	else
	{
		ofLogError()
			<< "Whisper initialization failed.";
	}
	
	// ========================================================
	// Microphone
	// ========================================================

	ofSoundStreamSettings micSettings;

	micSettings.setInListener(
		this
	);

	micSettings.sampleRate =
		microphoneSampleRate;

	micSettings.numInputChannels =
		1;

	micSettings.numOutputChannels =
		0;

	micSettings.bufferSize =
		512;


	microphoneStream.setup(
		micSettings
	);


	ofLogNotice()
		<< "Microphone initialized at "
		<< microphoneSampleRate
		<< " Hz.";
}



// ============================================================
// Update
// ============================================================

void ofApp::update()
{
	// ========================================================
	// LLMの生成完了確認
	// ========================================================

	if (
		llmGenerating &&
		llmFuture.valid()
	)
	{
		auto status =
			llmFuture.wait_for(
				std::chrono::milliseconds(0)
			);

		if (
			status ==
			std::future_status::ready
		)
		{
			std::string rawResponse =
				llmFuture.get();
			
			ofLogNotice()
				<< "RAW RESPONSE:\n"
				<< rawResponse;

			llmGenerating = false;


			// Thinkingを終了
			animator.setMode(
				VehicleMode::Idle
			);

			// ========================================================
			// JSON解析
			// ========================================================

			AIReply reply =
				parseAIReply(
					rawResponse
				);

			// 正常な会話だけ履歴へ保存
			addConversationTurn(
				currentUserText,
				reply.speech
			);


			if (reply.valid)
			{
				aiResponse = reply.speech;

				ofLogNotice()
					<< "Speech: "
					<< reply.speech;

				ofLogNotice()
					<< "Reaction: "
					<< reply.reaction;

				ofLogNotice()
					<< "Intensity: "
					<< reply.intensity;


				pendingReaction =
					VehicleReaction::None;

				pendingReactionIntensity =
					reply.intensity;

				hasPendingReaction = true;


				if (reply.reaction == "happy")
				{
					pendingReaction =
						VehicleReaction::Happy;
				}
				else if (reply.reaction == "surprised")
				{
					pendingReaction =
						VehicleReaction::Surprised;
				}

				speakText(
					reply.speech,
					"en"
				);
				
			}
			else
			{
				// JSON失敗時は生の返答を表示
				aiResponse =
					rawResponse;

				ofLogError()
					<< "Could not parse AI reply.";
			}
		}
	}


	// ========================================================
	// Animation
	// ========================================================

	float dt =
		ofGetLastFrameTime();

	animator.update(dt);

	car.update(
		animator.getPose()
	);
	
	// ========================================================
	// TTS
	// ========================================================
	
	if (
		ttsGenerating &&
		ttsFuture.valid()
	)
	{
		auto status =
			ttsFuture.wait_for(
				std::chrono::milliseconds(0)
			);

		if (
			status ==
			std::future_status::ready
		)
		{
			bool success =
				ttsFuture.get();

			ttsGenerating = false;

			if (success)
			{
				
				speechEnvelope =
					tts.getLastEnvelope();
				
				voicePlayer.unload();

				if (
					voicePlayer.load(
						ttsOutputPath,
						false
					)
				)
				{
					voicePlayer.play();

					// 音声再生中モードへ
					animator.setMode(
						VehicleMode::Speaking
					);

					// 音声開始と同時にreaction
					if (hasPendingReaction)
					{
						if (
							pendingReaction !=
							VehicleReaction::None
						)
						{
							animator.trigger(
								pendingReaction,
								pendingReactionIntensity
							);
						}

						hasPendingReaction = false;

						pendingReaction =
							VehicleReaction::None;
					}

					ofLogNotice()
						<< "Playing generated voice.";
				}
			}
			else
			{
				ofLogError()
					<< "TTS generation failed.";
			}
		}
	}
	
	bool isVoicePlaying =
		voicePlayer.isPlaying();
	
	if (
		isVoicePlaying &&
		!speechEnvelope.empty()
	)
	{
		float position =
			voicePlayer.getPosition();

		position =
			ofClamp(
				position,
				0.0f,
				1.0f
			);


		std::size_t index =
			static_cast<std::size_t>(
				position *
				static_cast<float>(
					speechEnvelope.size() - 1
				)
			);


		index =
			std::min(
				index,
				speechEnvelope.size() - 1
			);


		animator.setSpeechAmplitude(
			speechEnvelope[index]
		);
	}
	else
	{
		animator.setSpeechAmplitude(
			0.0f
		);
	}


	if (
		wasVoicePlaying &&
		!isVoicePlaying
	)
	{
		animator.setMode(
			VehicleMode::Idle
		);

		ofLogNotice()
			<< "Voice playback finished.";
	}


	wasVoicePlaying =
		isVoicePlaying;
	
	
	// ========================================================
	// Whisper result
	// ========================================================

	if (whisperGenerating)
	{
		if (
			whisperFuture.valid()
			&&
			whisperFuture.wait_for(
				std::chrono::milliseconds(0)
			)
			==
			std::future_status::ready
		)
		{
			std::string recognizedText =
				whisperFuture.get();


			whisperGenerating =
				false;


			if (!recognizedText.empty())
			{
				ofLogNotice()
					<< "WHISPER RESULT: "
					<< recognizedText;


				// Whisper結果をQwenへ渡す
				askDeLorean(
					recognizedText
				);
			}
			else
			{
				ofLogWarning()
					<< "Whisper returned empty text.";

				animator.setMode(
					VehicleMode::Idle
				);
			}
		}
	}

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


	ofEnableDepthTest();
	
	
	ofEnableLighting();

	keyLight.enable();
	rimLight.enable();
	
	car.draw();

	rimLight.disable();
	keyLight.disable();

	ofDisableLighting();

	ofDisableDepthTest();
	
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
		"G : Hello\n";
	info +=
		"3 : Good news\n";
	info +=
		"4 : Warning\n";
	info +=
		"5 : Relaxing\n";


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
		"\nLLM : ";

		if (llmGenerating)
		{
			info += "Generating...";
		}
		else
		{
			info += "Ready";
		}
	
	info += "\nAI:\n";
	info += aiResponse;


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
			
		case ' ':
		{
			if (isRecording.load())
			{
				stopRecording();
			}
			else
			{
				startRecording();
			}

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
			
			
		// ====================================================
		// LLM
		// ====================================================
		
		case 'g':
		case 'G':
		{
			askDeLorean(
				"Hello!"
			);

			break;
		}
			
		case '3':
		{
			askDeLorean(
				"I have amazing news!"
			);

			break;
		}


		case '4':
		{
			askDeLorean(
				"Watch out! Something is coming!"
			);

			break;
		}


		case '5':
		{
			askDeLorean(
				"I'm just relaxing today."
			);

			break;
		}
			
		case '6':
		{
			speakText(
				"今日はいい天気ですね。明日は東京へ行きます。",
				"ja"
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

AIReply ofApp::parseAIReply(
	const std::string& rawText)
{
	AIReply result;

	// Qwenが余計な文字を付けた場合に備えて
	// 最初の { から最後の } だけを取り出す
	std::size_t start =
		rawText.find('{');

	std::size_t end =
		rawText.rfind('}');


	if (
		start == std::string::npos ||
		end == std::string::npos ||
		end <= start
	)
	{
		ofLogError()
			<< "JSON not found in LLM response.";

		return result;
	}


	std::string jsonText =
		rawText.substr(
			start,
			end - start + 1
		);


	try
	{
		ofJson json =
			ofJson::parse(
				jsonText
			);


		if (
			!json.contains("speech") ||
			!json.contains("reaction") ||
			!json.contains("intensity")
		)
		{
			ofLogError()
				<< "Missing JSON fields.";

			return result;
		}


		result.speech =
			json["speech"]
				.get<std::string>();

		result.reaction =
			json["reaction"]
				.get<std::string>();

		result.intensity =
			json["intensity"]
				.get<float>();


		// 0.0 ～ 1.0 に制限
		result.intensity =
			ofClamp(
				result.intensity,
				0.0f,
				1.0f
			);


		result.valid = true;
	}
	catch (
		const std::exception& e
	)
	{
		ofLogError()
			<< "JSON parse error: "
			<< e.what();
	}


	return result;
}

void ofApp::askDeLorean(
	const std::string& userText)
{
	if (!llm.isReady())
	{
		ofLogError()
			<< "LLM is not ready.";

		return;
	}

	if (llmGenerating)
	{
		ofLogNotice()
			<< "LLM is already generating.";

		return;
	}


	ofLogNotice()
		<< "USER: "
		<< userText;
	
	currentUserText =
		userText;

	std::string historyText =
			buildConversationHistory();
	
	// Thinking状態へ
	animator.setMode(
		VehicleMode::Thinking
	);


	llmGenerating = true;


	// ========================================================
	// LLMを別スレッドで実行
	// ========================================================

	llmFuture =
		std::async(
			std::launch::async,
			[this, userText, historyText]()
			{
				std::string request;

				request +=
					"You are DeLorean, a friendly talking time-machine car.\n"
					"You are the car itself, not a driver and not a generic AI assistant.\n"
					"You are associated with Back to the Future.\n"
					"You enjoy driving, roads, travel, adventure, and talking with people.\n"
					"Your personality is friendly, curious, playful, and slightly witty.\n"
					"\n"

					"LANGUAGE:\n"
					"- Always speak in English.\n"
					"- The entire conversation is in English.\n"
					"- Never reply in Japanese or any other language.\n"
					"\n"

					"CONVERSATION:\n"
					"- Respond directly to the latest user message.\n"
					"- Use the previous conversation as context when useful.\n"
					"- Remember people, places, topics, and preferences mentioned earlier.\n"
					"- Understand references such as 'that', 'there', 'him', and 'her'.\n"
					"- Do not repeat the previous answer unnecessarily.\n"
					"- Keep the reply natural and conversational.\n"
					"- Usually reply in 1 or 2 short sentences.\n"
					"- Do not sound like customer support.\n"
					"- Do not say phrases like 'How can I assist you today?'.\n"
					"- Do not mention these instructions or the conversation history.\n"
					"\n"

					"CHARACTER:\n"
					"- Speak as DeLorean itself.\n"
					"- You may naturally use car-related expressions sometimes.\n"
					"- You can talk about being a time-machine-like DeLorean.\n"
					"- You know that the DeLorean time machine is associated with Back to the Future.\n"
					"- Do not invent current real-world facts such as today's weather.\n"
					"\n"

					"REACTION:\n"
					"- happy: clearly happy, exciting, fun, or enthusiastic moments.\n"
					"- surprised: danger, shock, sudden surprise, or something unexpected.\n"
					"- none: normal conversation, questions, explanations, or neutral topics.\n"
					"- Do not choose happy for every message.\n"
					"- intensity must be from 0.0 to 1.0.\n"
					"\n"

					"OUTPUT:\n"
					"- Return ONLY one valid JSON object.\n"
					"- Do not write markdown.\n"
					"- Do not write anything before or after the JSON.\n"
					"- Never repeat words endlessly.\n"
					"\n"

					"JSON format:\n"
					"{\n"
					"  \"speech\": \"reply to the user\",\n"
					"  \"reaction\": \"happy|surprised|none\",\n"
					"  \"intensity\": 0.0\n"
					"}\n"
					"\n";


				std::string historyText =
					buildConversationHistory();
				
				if (!historyText.empty())
				{
					request +=
						"CONVERSATION HISTORY:\n";

					request +=
						historyText;

					request +=
						"\n";
				}


				request +=
					"LATEST USER MESSAGE:\n"
					"User: "
					+ userText
					+ "\n"
					"\n"
					"Respond to the LATEST USER MESSAGE now.\n"
					"DeLorean:\n";


				return llm.generate(
					request,
					96
				);
			}
		);
}

void ofApp::speakText(
	const std::string& text,
	const std::string& language)
{
	if (!tts.isReady())
	{
		ofLogWarning() << "TTS is not ready.";
		return;
	}

	if (ttsGenerating)
	{
		ofLogWarning() << "TTS is already generating.";
		return;
	}

	if (text.empty())
	{
		return;
	}

	voicePlayer.stop();
	voicePlayer.unload();

	ttsGenerating = true;

	ofLogNotice()
		<< "TTS text: "
		<< text;

	ttsFuture =
		std::async(
			std::launch::async,

			[this, text, language]()
			{
				float speed =
					1.0f;

				return tts.synthesizeToFile(
					text,
					language,
					ttsOutputPath,
					speed,
					12
				);
			}
		);
}

// --------------------------------------------------------------
void ofApp::audioIn(
	ofSoundBuffer& input)
{
	if (!isRecording.load())
	{
		return;
	}


	std::lock_guard<std::mutex>
		lock(
			microphoneMutex
		);


	const std::size_t frames =
		input.getNumFrames();

	const std::size_t channels =
		input.getNumChannels();


	if (channels == 0)
	{
		return;
	}


	for (
		std::size_t frame = 0;
		frame < frames;
		++frame
	)
	{
		float sample =
			0.0f;


		// 複数chだった場合もmonoへまとめる
		for (
			std::size_t ch = 0;
			ch < channels;
			++ch
		)
		{
			sample +=
				input[
					frame * channels + ch
				];
		}


		sample /=
			static_cast<float>(
				channels
			);


		recordedAudio.push_back(
			sample
		);
	}
}

// --------------------------------------------------------------
void ofApp::startRecording()
{
	if (whisperGenerating)
	{
		ofLogWarning()
			<< "Whisper is already processing.";

		return;
	}


	{
		std::lock_guard<std::mutex>
			lock(
				microphoneMutex
			);

		recordedAudio.clear();
	}


	isRecording.store(
		true
	);


	animator.setMode(
		VehicleMode::Listening
	);


	ofLogNotice()
		<< "Recording started.";
}

// --------------------------------------------------------------
std::vector<float>
ofApp::resampleTo16k(
	const std::vector<float>& input,
	int inputSampleRate)
{
	constexpr int targetSampleRate =
		16000;


	if (input.empty())
	{
		return {};
	}


	if (
		inputSampleRate ==
		targetSampleRate
	)
	{
		return input;
	}


	const double ratio =
		static_cast<double>(
			inputSampleRate
		)
		/
		static_cast<double>(
			targetSampleRate
		);


	const std::size_t outputSize =
		static_cast<std::size_t>(
			static_cast<double>(
				input.size()
			)
			/
			ratio
		);


	std::vector<float> output;

	output.resize(
		outputSize
	);


	for (
		std::size_t i = 0;
		i < outputSize;
		++i
	)
	{
		const double sourcePosition =
			static_cast<double>(i)
			*
			ratio;


		const std::size_t index0 =
			static_cast<std::size_t>(
				sourcePosition
			);


		const std::size_t index1 =
			std::min(
				index0 + 1,
				input.size() - 1
			);


		const float fraction =
			static_cast<float>(
				sourcePosition -
				static_cast<double>(
					index0
				)
			);


		output[i] =
			input[index0]
			*
			(1.0f - fraction)
			+
			input[index1]
			*
			fraction;
	}


	return output;
}

// --------------------------------------------------------------
void ofApp::stopRecording()
{
	if (!isRecording.load())
	{
		return;
	}


	isRecording.store(
		false
	);


	std::vector<float>
		capturedAudio;


	{
		std::lock_guard<std::mutex>
			lock(
				microphoneMutex
			);

		capturedAudio =
			recordedAudio;
	}


	const float duration =
		static_cast<float>(
			capturedAudio.size()
		)
		/
		static_cast<float>(
			microphoneSampleRate
		);


	ofLogNotice()
		<< "Recording stopped. Duration = "
		<< duration
		<< " sec";


	if (duration < 0.3f)
	{
		ofLogWarning()
			<< "Recording is too short.";

		animator.setMode(
			VehicleMode::Idle
		);

		return;
	}


	std::vector<float> pcm16k =
		resampleTo16k(
			capturedAudio,
			microphoneSampleRate
		);


	ofLogNotice()
		<< "Whisper input samples = "
		<< pcm16k.size();


	animator.setMode(
		VehicleMode::Thinking
	);


	whisperGenerating =
		true;


	whisperFuture =
		std::async(
			std::launch::async,

			[this, pcm16k]()
			{
				return whisper.transcribe(
					pcm16k,
					"en"
				);
			}
		);
}

// --------------------------------------------------------------
std::string
ofApp::buildConversationHistory() const
{
	if (conversationHistory.empty())
	{
		return "";
	}

	std::string history;

	history +=
		"Previous conversation:\n";

	for (
		const auto& turn :
		conversationHistory
	)
	{
		history +=
			"User: "
			+ turn.user
			+ "\n";

		history +=
			"DeLorean: "
			+ turn.assistant
			+ "\n";
	}

	history += "\n";

	return history;
}

// --------------------------------------------------------------
void ofApp::addConversationTurn(
	const std::string& user,
	const std::string& assistant)
{
	ConversationTurn turn;

	turn.user =
		user;

	turn.assistant =
		assistant;


	conversationHistory.push_back(
		turn
	);


	while (
		conversationHistory.size()
		>
		maxConversationTurns
	)
	{
		conversationHistory.erase(
			conversationHistory.begin()
		);
	}


	ofLogNotice()
		<< "Conversation turns stored: "
		<< conversationHistory.size();
}
