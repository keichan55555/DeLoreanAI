#include "VehicleAnimator.h"

#include <algorithm>
#include <cmath>


// ============================================================
// Spring1D
// ============================================================

void Spring1D::update(
	float target,
	float dt)
{
	// フレーム落ち時の暴走防止
	dt =
		std::min(
			dt,
			1.0f / 30.0f
		);

	const float omega =
		TWO_PI
		* frequencyHz;


	const float acceleration =
		omega
		* omega
		* (target - value)

		-

		2.0f
		* dampingRatio
		* omega
		* velocity;


	velocity +=
		acceleration
		* dt;


	value +=
		velocity
		* dt;
}


void Spring1D::impulse(
	float velocityChange)
{
	velocity +=
		velocityChange;
}


void Spring1D::reset(
	float newValue)
{
	value =
		newValue;

	velocity =
		0.0f;
}



// ============================================================
// VehicleAnimator
// ============================================================

void VehicleAnimator::setup()
{
	// ========================================================
	// BODY suspension
	// ========================================================

	heave.frequencyHz =
		3.0f;

	heave.dampingRatio =
		0.42f;


	pitch.frequencyHz =
		2.8f;

	pitch.dampingRatio =
		0.48f;


	roll.frequencyHz =
		2.6f;

	roll.dampingRatio =
		0.46f;


	yaw.frequencyHz =
		2.2f;

	yaw.dampingRatio =
		0.58f;


	// ========================================================
	// Steering
	// ========================================================

	steerFL.frequencyHz =
		4.0f;

	steerFL.dampingRatio =
		0.62f;


	steerFR.frequencyHz =
		4.0f;

	steerFR.dampingRatio =
		0.62f;


	// ========================================================
	// Variation
	// ========================================================

	thinkingSeed =
		ofRandom(
			0.0f,
			1000.0f
		);


	personalitySide =
		ofRandom(1.0f) < 0.5f
		? -1.0f
		: 1.0f;
}



// ============================================================
// Mode
// ============================================================

void VehicleAnimator::setMode(
	VehicleMode newMode)
{
	mode =
		newMode;
}



// ============================================================
// Reaction
// ============================================================

void VehicleAnimator::trigger(
	VehicleReaction newReaction,
	float intensity)
{
	reaction =
		newReaction;


	reactionIntensity =
		ofClamp(
			intensity,
			0.0f,
			1.0f
		);


	personalitySide =
		ofRandom(1.0f) < 0.5f
		? -1.0f
		: 1.0f;


	nextPhase(
		ReactionPhase::Anticipation
	);
}



// ============================================================
// Update
// ============================================================

void VehicleAnimator::update(
	float dt)
{
	if (dt <= 0.0f)
	{
		return;
	}


	dt =
		std::min(
			dt,
			1.0f / 30.0f
		);


	const float now =
		ofGetElapsedTimef();


	phaseTime +=
		dt;


	// ========================================================
	// 基本Pose
	// ========================================================

	float targetHeave =
		0.0f;

	float targetPitch =
		0.0f;

	float targetRoll =
		0.0f;

	float targetYaw =
		0.0f;

	float targetSteerFL =
		0.0f;

	float targetSteerFR =
		0.0f;


	// ========================================================
	// 常時状態
	// ========================================================

	updateModeTargets(
		now,

		targetHeave,

		targetPitch,
		targetRoll,
		targetYaw,

		targetSteerFL,
		targetSteerFR
	);
	
	// ========================================================
	// Speaking
	// ========================================================

	if (mode == VehicleMode::Speaking)
	{
		targetHeave +=
			std::sin(now * 10.0f) * 0.35f;

		targetPitch +=
			std::sin(now * 7.0f) * 0.25f;

		targetRoll +=
			std::sin(now * 5.0f) * 0.18f;
	}

	// ========================================================
	// 一時リアクション
	// ========================================================

	updateReaction(
		dt,

		targetHeave,

		targetPitch,
		targetRoll,
		targetYaw,

		targetSteerFL,
		targetSteerFR
	);


	// ========================================================
	// Gravity
	// ========================================================

	updateGravity(
		dt
	);


	// ========================================================
	// Spring
	// ========================================================

	heave.update(
		targetHeave,
		dt
	);


	pitch.update(
		targetPitch,
		dt
	);


	roll.update(
		targetRoll,
		dt
	);


	yaw.update(
		targetYaw,
		dt
	);


	steerFL.update(
		targetSteerFL,
		dt
	);


	steerFR.update(
		targetSteerFR,
		dt
	);


	// ========================================================
	// Pose output
	// ========================================================

	pose.rootLift =
		rootLift;


	pose.bodyHeave =
		heave.value;


	pose.bodyPitch =
		pitch.value;


	pose.bodyRoll =
		roll.value;


	pose.bodyYaw =
		yaw.value;


	pose.steerFL =
		steerFL.value;


	pose.steerFR =
		steerFR.value;
}



// ============================================================
// 常時モーション
// ============================================================

void VehicleAnimator::updateModeTargets(
	float now,

	float& targetHeave,

	float& targetPitch,
	float& targetRoll,
	float& targetYaw,

	float& targetSteerFL,
	float& targetSteerFR)
{
	// sin波ではなくNoiseを使い、
	// 機械的な周期運動を避ける


	const float slowNoise =
		ofSignedNoise(
			thinkingSeed,
			now * 0.22f
		);


	const float mediumNoise =
		ofSignedNoise(
			thinkingSeed + 100.0f,
			now * 0.47f
		);


	const float tinyNoise =
		ofSignedNoise(
			thinkingSeed + 200.0f,
			now * 0.9f
		);


	switch (mode)
	{
		// ====================================================
		// IDLE
		// ====================================================

		case VehicleMode::Idle:
		{
			targetHeave +=
				slowNoise
				* 0.6f;


			targetPitch +=
				mediumNoise
				* 0.25f;


			targetRoll +=
				tinyNoise
				* 0.35f;


			targetYaw +=
				slowNoise
				* 0.25f;


			targetSteerFL +=
				slowNoise
				* 1.2f;


			targetSteerFR +=
				slowNoise
				* 1.2f;


			break;
		}


		// ====================================================
		// LISTENING
		// ====================================================

		case VehicleMode::Listening:
		{
			// 少し前のめり

			targetHeave +=
				1.5f;


			targetPitch -=
				2.8f;


			targetRoll +=
				mediumNoise
				* 0.3f;


			targetYaw +=
				slowNoise
				* 0.5f;


			targetSteerFL +=
				slowNoise
				* 2.0f;


			targetSteerFR +=
				slowNoise
				* 2.0f;


			break;
		}


		// ====================================================
		// THINKING
		// ====================================================

		case VehicleMode::Thinking:
		{
			const float lookDirection =
				ofSignedNoise(
					thinkingSeed + 400.0f,
					now * 0.32f
				);


			targetPitch +=
				1.0f;


			targetYaw +=
				lookDirection
				* 5.5f;


			targetRoll +=
				lookDirection
				* 1.3f;


			targetSteerFL +=
				lookDirection
				* 9.0f;


			targetSteerFR +=
				lookDirection
				* 9.0f;


			break;
		}
	}
}



// ============================================================
// Reaction
// ============================================================

void VehicleAnimator::updateReaction(
	float dt,

	float& targetHeave,

	float& targetPitch,
	float& targetRoll,
	float& targetYaw,

	float& targetSteerFL,
	float& targetSteerFR)
{
	if (
		reaction
		==
		VehicleReaction::None
	)
	{
		return;
	}


	const float e =
		reactionIntensity;


	// ========================================================
	// HAPPY
	// ========================================================

	if (
		reaction
		==
		VehicleReaction::Happy
	)
	{
		switch (phase)
		{
			// ------------------------------------------------
			// 予備動作
			// ------------------------------------------------

			case ReactionPhase::Anticipation:
			{
				// ジャンプ前に沈み込む

				targetHeave +=
					10.0f
					* e;


				targetPitch +=
					2.0f
					* e;


				targetRoll +=
					personalitySide
					* 1.3f
					* e;


				targetSteerFL +=
					personalitySide
					* 5.0f
					* e;


				targetSteerFR +=
					personalitySide
					* 5.0f
					* e;


				if (
					phaseTime
					>
					0.14f
				)
				{
					// サスペンション解放

					heave.impulse(
						-95.0f
						* e
					);


					pitch.impulse(
						-30.0f
						* e
					);


					roll.impulse(
						personalitySide
						* 22.0f
						* e
					);


					steerFL.impulse(
						personalitySide
						* 80.0f
						* e
					);


					steerFR.impulse(
						-personalitySide
						* 60.0f
						* e
					);


					launch(
						155.0f
						+
						65.0f
						* e
					);


					nextPhase(
						ReactionPhase::Active
					);
				}

				break;
			}


			// ------------------------------------------------
			// 空中
			// ------------------------------------------------

			case ReactionPhase::Active:
			{
				if (airborne)
				{
					targetPitch -=
						1.5f
						* e;
				}


				if (
					!airborne
					&&
					phaseTime > 0.18f
				)
				{
					nextPhase(
						ReactionPhase::Recovery
					);
				}

				break;
			}


			// ------------------------------------------------
			// 回復
			// ------------------------------------------------

			case ReactionPhase::Recovery:
			{
				if (
					phaseTime
					>
					0.65f
				)
				{
					finishReaction();
				}

				break;
			}


			default:
				break;
		}
	}


	// ========================================================
	// SURPRISED
	// ========================================================

	else if (
		reaction
		==
		VehicleReaction::Surprised
	)
	{
		switch (phase)
		{
			// ------------------------------------------------
			// 短い予備動作
			// ------------------------------------------------

			case ReactionPhase::Anticipation:
			{
				targetHeave +=
					2.0f
					* e;


				if (
					phaseTime
					>
					0.045f
				)
				{
					// ビクッと後ろへ

					pitch.impulse(
						85.0f
						* e
					);


					heave.impulse(
						-45.0f
						* e
					);


					roll.impulse(
						personalitySide
						* 12.0f
						* e
					);


					// 前輪を左右に開く

					steerFL.impulse(
						-135.0f
						* e
					);


					steerFR.impulse(
						135.0f
						* e
					);


					// 強い驚きなら少し浮く

					if (e > 0.55f)
					{
						launch(
							55.0f
							+
							45.0f
							* e
						);
					}


					nextPhase(
						ReactionPhase::Active
					);
				}

				break;
			}


			// ------------------------------------------------

			case ReactionPhase::Active:
			{
				targetPitch +=
					3.0f
					* e;


				targetSteerFL -=
					7.0f
					* e;


				targetSteerFR +=
					7.0f
					* e;


				if (
					(
						!airborne
						&&
						phaseTime > 0.28f
					)
					||
					phaseTime > 0.8f
				)
				{
					nextPhase(
						ReactionPhase::Recovery
					);
				}

				break;
			}


			// ------------------------------------------------

			case ReactionPhase::Recovery:
			{
				if (
					phaseTime
					>
					0.6f
				)
				{
					finishReaction();
				}

				break;
			}


			default:
				break;
		}
	}
}



// ============================================================
// Gravity
// ============================================================

void VehicleAnimator::updateGravity(
	float dt)
{
	if (!airborne)
	{
		return;
	}


	verticalVelocity -=
		gravity
		* dt;


	rootLift +=
		verticalVelocity
		* dt;


	// ========================================================
	// Ground collision
	// ========================================================

	if (
		rootLift
		<=
		0.0f
	)
	{
		const float impactSpeed =
			std::abs(
				verticalVelocity
			);


		rootLift =
			0.0f;


		verticalVelocity =
			0.0f;


		airborne =
			false;


		land(
			impactSpeed
		);
	}
}



// ============================================================
// Landing
// ============================================================

void VehicleAnimator::land(
	float impactSpeed)
{
	// タイヤが接地した後、
	// BODYだけが慣性で沈む

	heave.impulse(
		impactSpeed
		* 0.085f
	);


	// 少し左右非対称にする

	roll.impulse(
		ofRandom(
			-impactSpeed
			* 0.013f,

			 impactSpeed
			* 0.013f
		)
	);


	pitch.impulse(
		ofRandom(
			-impactSpeed
			* 0.010f,

			 impactSpeed
			* 0.010f
		)
	);


	// 前輪にも小さな反動

	steerFL.impulse(
		ofRandom(
			-impactSpeed
			* 0.025f,

			 impactSpeed
			* 0.025f
		)
	);


	steerFR.impulse(
		ofRandom(
			-impactSpeed
			* 0.025f,

			 impactSpeed
			* 0.025f
		)
	);
}



// ============================================================
// Launch
// ============================================================

void VehicleAnimator::launch(
	float speed)
{
	if (airborne)
	{
		return;
	}


	verticalVelocity =
		speed;


	airborne =
		true;
}



// ============================================================
// Phase
// ============================================================

void VehicleAnimator::nextPhase(
	ReactionPhase next)
{
	phase =
		next;


	phaseTime =
		0.0f;
}



// ============================================================
// Finish
// ============================================================

void VehicleAnimator::finishReaction()
{
	reaction =
		VehicleReaction::None;


	phase =
		ReactionPhase::None;


	phaseTime =
		0.0f;
}
