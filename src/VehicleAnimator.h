#pragma once

#include "ofMain.h"


// ============================================================
// 常時状態
// ============================================================

enum class VehicleMode
{
	Idle,
	Listening,
	Thinking,
	Speaking
};


// ============================================================
// 一時リアクション
// ============================================================

enum class VehicleReaction
{
	None,
	Happy,
	Surprised
};


// ============================================================
// Animator -> DeLorean に渡す姿勢
// ============================================================

struct VehiclePose
{
	// 車全体のジャンプ
	float rootLift = 0.0f;

	// Bodyだけのサスペンション変位
	//
	// +
	// ↓
	// BODYが沈む
	float bodyHeave = 0.0f;

	float bodyPitch = 0.0f;
	float bodyRoll  = 0.0f;
	float bodyYaw   = 0.0f;

	// 前輪
	float steerFL = 0.0f;
	float steerFR = 0.0f;
};


// ============================================================
// 1自由度バネ
// ============================================================

struct Spring1D
{
	float value = 0.0f;
	float velocity = 0.0f;

	float frequencyHz = 3.0f;
	float dampingRatio = 0.55f;

	void update(
		float target,
		float dt
	);

	void impulse(
		float velocityChange
	);

	void reset(
		float newValue = 0.0f
	);
};


// ============================================================
// VehicleAnimator
// ============================================================

class VehicleAnimator
{
public:

	void setup();

	void update(
		float dt
	);

	void setMode(
		VehicleMode newMode
	);

	void trigger(
		VehicleReaction reaction,
		float intensity = 1.0f
	);

	const VehiclePose&
	getPose() const
	{
		return pose;
	}

	VehicleMode
	getMode() const
	{
		return mode;
	}

	VehicleReaction
	getReaction() const
	{
		return reaction;
	}

	bool
	isAirborne() const
	{
		return airborne;
	}
	
	void setSpeechAmplitude(float amplitude)
	{
		speechAmplitude =
			ofClamp(
				amplitude,
				0.0f,
				1.0f
			);
	}


private:

	enum class ReactionPhase
	{
		None,

		// 予備動作
		Anticipation,

		// メイン動作
		Active,

		// 収束
		Recovery
	};


	// ========================================================
	// 出力
	// ========================================================

	VehiclePose pose;


	// ========================================================
	// 状態
	// ========================================================

	VehicleMode mode =
		VehicleMode::Idle;

	VehicleReaction reaction =
		VehicleReaction::None;

	ReactionPhase phase =
		ReactionPhase::None;


	// ========================================================
	// Body physics
	// ========================================================

	Spring1D heave;

	Spring1D pitch;
	Spring1D roll;
	Spring1D yaw;


	// ========================================================
	// Wheels
	// ========================================================

	Spring1D steerFL;
	Spring1D steerFR;


	// ========================================================
	// Jump physics
	// ========================================================

	float rootLift = 0.0f;

	float verticalVelocity =
		0.0f;

	bool airborne =
		false;

	float gravity =
		600.0f;


	// ========================================================
	// Reaction
	// ========================================================

	float reactionIntensity =
		1.0f;

	float phaseTime =
		0.0f;


	// ========================================================
	// Personality / variation
	// ========================================================

	float personalitySide =
		1.0f;

	float thinkingSeed =
		0.0f;


	// ========================================================
	// Internal
	// ========================================================

	void updateModeTargets(
		float now,

		float& targetHeave,

		float& targetPitch,
		float& targetRoll,
		float& targetYaw,

		float& targetSteerFL,
		float& targetSteerFR
	);


	void updateReaction(
		float dt,

		float& targetHeave,

		float& targetPitch,
		float& targetRoll,
		float& targetYaw,

		float& targetSteerFL,
		float& targetSteerFR
	);


	void updateGravity(
		float dt
	);


	void land(
		float impactSpeed
	);


	void launch(
		float speed
	);


	void nextPhase(
		ReactionPhase next
	);


	void finishReaction();
	
	
	float speechAmplitude = 0.0f;
	float smoothedSpeechAmplitude = 0.0f;
};
