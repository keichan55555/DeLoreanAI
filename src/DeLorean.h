#pragma once

#include "ofMain.h"

#include "ofxAssimpModelLoader.h"

#include "VehicleAnimator.h"

#include <assimp/scene.h>

#include <vector>


class DeLorean
{
public:

	bool setup(
		const std::string& path
	);


	void update(
		const VehiclePose& pose
	);


	void draw();


	bool isLoaded() const
	{
		return loaded;
	}


	// Blenderがmeter単位なら
	// 100くらいから試す
	void setRenderScale(
		float scale
	);


	void setWorldOffset(
		const glm::vec3& offset
	);


private:

	// ========================================================
	// Model
	// ========================================================

	ofxAssimpModelLoader model;

	bool loaded =
		false;


	// ========================================================
	// Node -> Mesh indices
	// ========================================================

	std::vector<unsigned int>
		bodyIndices;


	std::vector<unsigned int>
		wheelFLIndices;


	std::vector<unsigned int>
		wheelFRIndices;


	std::vector<unsigned int>
		wheelLLIndices;


	std::vector<unsigned int>
		wheelLRIndices;


	// ========================================================
	// Blender初期Transform
	// ========================================================

	std::vector<glm::mat4>
		baseMatrices;


	// ========================================================
	// World transform
	// ========================================================

	float renderScale =
		100.0f;


	glm::vec3 worldOffset =
		glm::vec3(
			0.0f
		);


	// 車全体ジャンプ
	float rootLift =
		0.0f;


	// ========================================================
	// Node utility
	// ========================================================

	const aiNode* findNode(
		const aiNode* node,
		const std::string& name
	) const;


	void collectMeshIndices(
		const aiNode* node,
		std::vector<unsigned int>& indices
	);


	// ========================================================
	// Matrix
	// ========================================================

	void resetMatrices();


	void applyPartTransform(
		const std::vector<unsigned int>& indices,
		const glm::mat4& localTransform
	);
};
