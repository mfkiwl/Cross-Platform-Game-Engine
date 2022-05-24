#pragma once

#include "cereal/cereal.hpp"

#include "math/Vector2f.h"

struct CircleCollider2DComponent
{
	Vector2f offset = { 0.0f,0.0f };

	float radius = 0.5f;

	//TODO: create physics material
	float density = 1.0f;
	float friction = 0.5f;
	float restitution = 0.0f;

	void* RuntimeFixture = nullptr;

	CircleCollider2DComponent() = default;
	CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
private:
	friend cereal::access;
	template<typename Archive>
	void serialize(Archive& archive)
	{
		archive(cereal::make_nvp("Offset", offset));
		archive(cereal::make_nvp("Radius", radius));
		archive(cereal::make_nvp("Density", density));
		archive(cereal::make_nvp("Friction", friction));
		archive(cereal::make_nvp("Restitution", restitution));
	}
};