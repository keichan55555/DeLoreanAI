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
	// LLM
	// ============================================================

	const std::string modelPath =
		ofToDataPath(
			"llm/Qwen3.5-0.8B-Q4_0.gguf",
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
	
//	ttsOutputPath =
//		ofToDataPath(
//			"tts/output/current.wav",
//			true
//		);
//
//
//	bool ttsOk =
//		tts.setup(
//			ofToDataPath(
//				"tts/onnx",
//				true
//			),
//
//			ofToDataPath(
//				"tts/voice_styles",
//				true
//			),
//
//			"M1"
//		);
//
//
//	if (ttsOk)
//	{
//		ofLogNotice()
//			<< "Supertonic ready.";
//	}
//	else
//	{
//		ofLogError()
//			<< "Supertonic setup failed.";
//	}
	
	
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


			if (reply.valid)
			{
				aiResponse =
					reply.speech;


				ofLogNotice()
					<< "Speech: "
					<< reply.speech;

				ofLogNotice()
					<< "Reaction: "
					<< reply.reaction;

				ofLogNotice()
					<< "Intensity: "
					<< reply.intensity;


				// ====================================================
				// Reaction
				// ====================================================

				if (
					reply.reaction ==
					"happy"
				)
				{
					animator.trigger(
						VehicleReaction::Happy,
						reply.intensity
					);
				}
				else if (
					reply.reaction ==
					"surprised"
				)
				{
					animator.trigger(
						VehicleReaction::Surprised,
						reply.intensity
					);
				}
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
	
//	if (
//		ttsGenerating &&
//		ttsFuture.valid()
//	)
//	{
//		auto status =
//			ttsFuture.wait_for(
//				std::chrono::milliseconds(0)
//			);
//
//
//		if (
//			status ==
//			std::future_status::ready
//		)
//		{
//			bool success =
//				ttsFuture.get();
//
//
//			ttsGenerating = false;
//
//
//			if (success)
//			{
//				voicePlayer.unload();
//
//
//				if (
//					voicePlayer.load(
//						ttsOutputPath
//					)
//				)
//				{
//					voicePlayer.play();
//				}
//				else
//				{
//					ofLogError()
//						<< "Could not load generated WAV.";
//				}
//			}
//		}
//	}
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


	car.draw();


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
		// ====================================================
		// Idle
		// ====================================================

		case ' ':
		{
			animator.setMode(
				VehicleMode::Idle
			);

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
			
//		case '6':
//		{
//			if (
//				tts.isReady() &&
//				!ttsGenerating
//			)
//			{
//				ttsGenerating = true;
//
//
//				ttsFuture =
//					std::async(
//						std::launch::async,
//
//						[this]()
//						{
//							return
//								tts.synthesizeToFile(
//									"こんにちは！今日も一緒にドライブしよう！",
//									"ja",
//									ttsOutputPath,
//									1.05f
//								);
//						}
//					);
//			}
//
//			break;
//		}
		
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

			[this, userText]()
			{
				std::string request;

				request +=
					"The user says: \"" + userText + "\"\n\n";

				request +=
					"You are a talking DeLorean character.\n"
					"Choose the reaction based on the MEANING of the user's message.\n\n"

					"Reaction rules:\n"
					"- happy: clearly good news, praise, excitement, celebration, success\n"
					"- surprised: danger, warning, sudden unexpected event, shock, urgent situation\n"
					"- none: calm, neutral, ordinary conversation, relaxing, factual statements\n\n"

					"Intensity rules:\n"
					"- 0.1 to 0.3: weak feeling\n"
					"- 0.4 to 0.6: medium feeling\n"
					"- 0.7 to 1.0: strong feeling\n\n"

					"Examples:\n"
					"User: I have amazing news!\n"
					"reaction = happy, intensity = 0.9\n\n"

					"User: Watch out! Something is coming!\n"
					"reaction = surprised, intensity = 0.9\n\n"

					"User: I'm just relaxing today.\n"
					"reaction = none, intensity = 0.1\n\n"

					"Now respond to the actual user message.\n"
					"Complete the JSON object only.\n"
					"Required fields:\n"
					"\"speech\": a short natural reply,\n"
					"\"reaction\": exactly one of \"happy\", \"surprised\", \"none\",\n"
					"\"intensity\": a number from 0.0 to 1.0.\n"
					"Do not use unescaped double quotes inside speech.";


				return llm.generate(
					request
				);
			}
		);
}
