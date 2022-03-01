#pragma once

#include "math/Vector3f.h"

#include "cereal/cereal.hpp"

struct TransformComponent
{
	Vector3f position = Vector3f(0.0f, 0.0f, 0.0f);
	Vector3f rotation = Vector3f(0.0f, 0.0f, 0.0f);
	Vector3f scale = Vector3f(1.0f, 1.0f, 1.0f);

	TransformComponent() = default;
	TransformComponent(const TransformComponent&) = default;
	TransformComponent(const Vector3f & position)
		:position(position) {}
	TransformComponent(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale)
		:position(position), rotation(rotation), scale(scale) {}

	Matrix4x4 GetWorldMatrix()
	{
		return m_WorldMatrix;
	}

	Matrix4x4 GetLocalMatrix()
	{
		return Matrix4x4::Translate(position) * Matrix4x4::Rotate(Quaternion(rotation)) * Matrix4x4::Scale(scale);
	}

	void SetWorldMatrix(Matrix4x4 parentMatrix) 
	{ 
		m_ParentMatrix = parentMatrix;
		m_WorldMatrix = parentMatrix * GetLocalMatrix();
	}

private:
	Matrix4x4 m_WorldMatrix;
	Matrix4x4 m_ParentMatrix;

	friend cereal::access;
	template<typename Archive>
	void serialize(Archive& archive)
	{
		archive(cereal::make_nvp("Position", position));
		archive(cereal::make_nvp("Rotation", rotation));
		archive(cereal::make_nvp("Scale", scale));
	}
};