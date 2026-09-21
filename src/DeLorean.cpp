#include "DeLorean.h"

#include <algorithm>


// ============================================================
// Setup
// ============================================================

bool DeLorean::setup(
	const std::string& path)
{
	loaded =
		model.load(
			path
		);


	if (!loaded)
	{
		ofLogError(
			"DeLorean"
		)
		<< "Failed to load: "
		<< path;

		return false;
	}


	// 自動サイズ変更をOFF
	model.setScaleNormalization(
		false
	);


	const aiScene* scene =
		model.getAssimpScene();


	if (
		!scene
		||
		!scene->mRootNode
	)
	{
		ofLogError(
			"DeLorean"
		)
		<< "Invalid Assimp scene.";

		loaded =
			false;

		return false;
	}


	// ========================================================
	// Blender Object名からNode取得
	// ========================================================

	const aiNode* bodyNode =
		findNode(
			scene->mRootNode,
			"body"
		);


	const aiNode* flNode =
		findNode(
			scene->mRootNode,
			"wheelFL"
		);


	const aiNode* frNode =
		findNode(
			scene->mRootNode,
			"wheelFR"
		);


	const aiNode* llNode =
		findNode(
			scene->mRootNode,
			"wheelLL"
		);


	const aiNode* lrNode =
		findNode(
			scene->mRootNode,
			"wheelLR"
		);


	if (
		!bodyNode
		||
		!flNode
		||
		!frNode
		||
		!llNode
		||
		!lrNode
	)
	{
		ofLogError(
			"DeLorean"
		)
		<< "Required nodes not found.";

		loaded =
			false;

		return false;
	}


	// ========================================================
	// Nodeが持つMesh indexを保存
	// ========================================================

	collectMeshIndices(
		bodyNode,
		bodyIndices
	);


	collectMeshIndices(
		flNode,
		wheelFLIndices
	);


	collectMeshIndices(
		frNode,
		wheelFRIndices
	);


	collectMeshIndices(
		llNode,
		wheelLLIndices
	);


	collectMeshIndices(
		lrNode,
		wheelLRIndices
	);


	// ========================================================
	// 初期Matrix保存
	// ========================================================

	baseMatrices.resize(
		model.getMeshCount()
	);


	for (
		unsigned int i = 0;
		i < model.getMeshCount();
		++i
	)
	{
		baseMatrices[i] =
			model
			.getMeshHelper(i)
			.matrix;
	}


	// ========================================================
	// Debug
	// ========================================================

	ofLogNotice(
		"DeLorean"
	)
	<< "body meshes: "
	<< bodyIndices.size();


	ofLogNotice(
		"DeLorean"
	)
	<< "wheelFL meshes: "
	<< wheelFLIndices.size();


	ofLogNotice(
		"DeLorean"
	)
	<< "wheelFR meshes: "
	<< wheelFRIndices.size();


	ofLogNotice(
		"DeLorean"
	)
	<< "wheelLL meshes: "
	<< wheelLLIndices.size();


	ofLogNotice(
		"DeLorean"
	)
	<< "wheelLR meshes: "
	<< wheelLRIndices.size();


	return true;
}



// ============================================================
// Render scale
// ============================================================

void DeLorean::setRenderScale(
	float scale)
{
	renderScale =
		std::max(
			scale,
			0.0001f
		);
}



// ============================================================
// World position
// ============================================================

void DeLorean::setWorldOffset(
	const glm::vec3& offset)
{
	worldOffset =
		offset;
}



// ============================================================
// Find node
// ============================================================

const aiNode* DeLorean::findNode(
	const aiNode* node,
	const std::string& name) const
{
	if (!node)
	{
		return nullptr;
	}


	if (
		name
		==
		node->mName.C_Str()
	)
	{
		return node;
	}


	for (
		unsigned int i = 0;
		i < node->mNumChildren;
		++i
	)
	{
		const aiNode* found =
			findNode(
				node->mChildren[i],
				name
			);


		if (found)
		{
			return found;
		}
	}


	return nullptr;
}



// ============================================================
// Mesh index
// ============================================================

void DeLorean::collectMeshIndices(
	const aiNode* node,
	std::vector<unsigned int>& indices)
{
	indices.clear();


	if (!node)
	{
		return;
	}


	for (
		unsigned int i = 0;
		i < node->mNumMeshes;
		++i
	)
	{
		indices.push_back(
			node->mMeshes[i]
		);
	}
}



// ============================================================
// Reset
// ============================================================

void DeLorean::resetMatrices()
{
	for (
		unsigned int i = 0;
		i < baseMatrices.size();
		++i
	)
	{
		model
			.getMeshHelper(i)
			.matrix
			=
			baseMatrices[i];
	}
}



// ============================================================
// Part transform
// ============================================================

void DeLorean::applyPartTransform(
	const std::vector<unsigned int>& indices,
	const glm::mat4& localTransform)
{
	for (
		const auto index
		:
		indices
	)
	{
		if (
			index
			>=
			baseMatrices.size()
		)
		{
			continue;
		}


		model
			.getMeshHelper(index)
			.matrix
			=
			baseMatrices[index]
			*
			localTransform;
	}
}



// ============================================================
// Update
// ============================================================

void DeLorean::update(
	const VehiclePose& pose)
{
	if (!loaded)
	{
		return;
	}


	// 毎フレームBlender初期姿勢から開始
	resetMatrices();


	rootLift =
		pose.rootLift;


	// ========================================================
	// BODY
	// ========================================================

	glm::mat4 bodyTransform(
		1.0f
	);


	// VehicleAnimatorはworld-unit基準なので、
	// GLB local unitへ戻す
	const float localHeave =
		pose.bodyHeave
		/
		renderScale;


	// --------------------------------------------------------
	// Suspension
	//
	// +bodyHeave = 沈む
	// --------------------------------------------------------

	bodyTransform =
		glm::translate(
			bodyTransform,

			glm::vec3(
				0.0f,
				-localHeave,
				0.0f
			)
		);


	// --------------------------------------------------------
	// Yaw
	// --------------------------------------------------------

	bodyTransform =
		glm::rotate(
			bodyTransform,

			glm::radians(
				pose.bodyYaw
			),

			glm::vec3(
				0.0f,
				1.0f,
				0.0f
			)
		);


	// --------------------------------------------------------
	// Pitch
	// --------------------------------------------------------

	bodyTransform =
		glm::rotate(
			bodyTransform,

			glm::radians(
				pose.bodyPitch
			),

			glm::vec3(
				1.0f,
				0.0f,
				0.0f
			)
		);


	// --------------------------------------------------------
	// Roll
	// --------------------------------------------------------

	bodyTransform =
		glm::rotate(
			bodyTransform,

			glm::radians(
				pose.bodyRoll
			),

			glm::vec3(
				0.0f,
				0.0f,
				1.0f
			)
		);


	applyPartTransform(
		bodyIndices,
		bodyTransform
	);


	// ========================================================
	// Wheel FL
	// ========================================================

	glm::mat4 flTransform(
		1.0f
	);


	flTransform =
		glm::rotate(
			flTransform,

			glm::radians(
				pose.steerFL
			),

			glm::vec3(
				0.0f,
				1.0f,
				0.0f
			)
		);


	applyPartTransform(
		wheelFLIndices,
		flTransform
	);


	// ========================================================
	// Wheel FR
	// ========================================================

	glm::mat4 frTransform(
		1.0f
	);


	frTransform =
		glm::rotate(
			frTransform,

			glm::radians(
				pose.steerFR
			),

			glm::vec3(
				0.0f,
				1.0f,
				0.0f
			)
		);


	applyPartTransform(
		wheelFRIndices,
		frTransform
	);


	// Rear wheelsは現時点では
	// Blenderの初期姿勢のまま。
}



// ============================================================
// Draw
// ============================================================

void DeLorean::draw()
{
	if (!loaded)
	{
		return;
	}

	ofPushMatrix();

	ofTranslate(
		worldOffset.x,
		worldOffset.y + rootLift,
		worldOffset.z
	);

	// ==============================
	// Blender → OF 座標補正
	// ==============================

	// 上下を直す
	ofRotateZDeg(180.0f);

	// 前後を直す
	ofRotateYDeg(180.0f);

	// ==============================

	ofScale(
		renderScale,
		renderScale,
		renderScale
	);

	ofSetColor(255);

	model.drawFaces();

	ofPopMatrix();
}
