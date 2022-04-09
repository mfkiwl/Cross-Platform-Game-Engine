#include "stdafx.h"
#include "LuaBindings.h"

#include "Logging/Logger.h"
#include "Logging/Instrumentor.h"
#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/MouseButtonCodes.h"
#include "Core/Joysticks.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/SceneManager.h"
#include "Scene/Components/Components.h"
#include "Utilities/StringUtils.h"

template<typename Component>
void RegisterComponent(sol::state& state)
{
	std::string name = type_name<Component>().data();
	name.pop_back();
	sol::usertype<Component> component_type = state.new_usertype<Component>(name);
	auto entity_Type = state["Entity"].get_or_create<sol::usertype<Entity>>();
	entity_Type.set_function("Add" + name, static_cast<Component & (Entity::*)()>(&Entity::AddComponent<Component>));
	entity_Type.set_function("Remove" + name, &Entity::RemoveComponent<Component>);
	entity_Type.set_function("Has" + name, &Entity::HasComponent<Component>);
	entity_Type.set_function("GetOrAdd" + name, &Entity::GetOrAddComponent<Component>);
	entity_Type.set_function("Get" + name, &Entity::GetComponent<Component>);
}

template<typename... Component>
void RegisterAllComponents(sol::state& state)
{
	(RegisterComponent<Component>(state), ...);
}

namespace Lua
{
	void BindLogging(sol::state& state)
	{
		PROFILE_FUNCTION();

		sol::table log = state.create_table("Log");

		log.set_function("Trace", [&](sol::this_state s, std::string_view message)
		{
			CLIENT_TRACE(message);
		});

		log.set_function("Info", [&](sol::this_state s, std::string_view message)
		{
			CLIENT_INFO(message);
		});

		log.set_function("Debug", [&](sol::this_state s, std::string_view message)
		{
			CLIENT_DEBUG(message);
		});

		log.set_function("Warn", [&](sol::this_state s, std::string_view message)
		{
			CLIENT_WARN(message);
		});

		log.set_function("Error", [&](sol::this_state s, std::string_view message)
		{
			CLIENT_ERROR(message);
		});

		log.set_function("Critical", [&](sol::this_state s, std::string_view message)
		{
			CLIENT_CRITICAL(message);
		});
	}

	//--------------------------------------------------------------------------------------------------------------

	void BindApp(sol::state& state)
	{
		PROFILE_FUNCTION();

		sol::table application = state.create_table("App");

		application.set_function("ShowImGui", &Application::ShowImGui);
		application.set_function("ToggleImGui", &Application::ToggleImGui);

		application.set_function("GetFixedUpdateInterval", [&](sol::this_state s) -> float
		{
			return Application::Get().GetFixedUpdateInterval();
		});
	}

	//--------------------------------------------------------------------------------------------------------------

	void ChangeScene(std::string sceneFilepath)
	{
		SceneManager::ChangeScene(std::filesystem::path(sceneFilepath));
	}

	Entity FindEntity(std::string_view name)
	{
		auto view = SceneManager::CurrentScene()->GetRegistry().view<NameComponent>();
		for (auto entity : view)
		{
			auto [nameComp] = view.get(entity);
			if (name == nameComp.name)
				return Entity(entity, SceneManager::CurrentScene());
		}
	}

	void BindScene(sol::state& state)
	{
		PROFILE_FUNCTION();

		state.set_function("ChangeScene", &ChangeScene);
		state.set_function("FindEntity", &FindEntity);

		sol::usertype<Scene> scene_type = state.new_usertype<Scene>("Scene");
		scene_type.set_function("CreateEntity", static_cast<Entity(Scene::*)(const std::string&)>(&Scene::CreateEntity));
		scene_type.set_function("GetName", &Scene::GetSceneName);
		scene_type.set_function("SetName", &Scene::SetSceneName);
		scene_type.set_function("GetPrimaryCamera", &Scene::GetPrimaryCameraEntity);
	}

	//--------------------------------------------------------------------------------------------------------------

	void PhysicsApplyImpulse2D(Vector2f impulse)
	{

	}

	void BindEntity(sol::state& state)
	{
		PROFILE_FUNCTION();

		sol::usertype<Entity> entity_type = state.new_usertype<Entity>("Entity");
		entity_type.set_function("IsValid", &Entity::IsValid);
		entity_type.set_function("GetName", &Entity::GetName);
		entity_type.set_function("SetName", &Entity::SetName);
		entity_type.set_function("AddChild", &Entity::AddChild);

		RegisterAllComponents<COMPONENTS>(state);

		auto transform_type = state["TransformComponent"].get_or_create<sol::usertype<TransformComponent>>();
		transform_type["Position"] = &TransformComponent::position;
		transform_type["Rotation"] = &TransformComponent::rotation;
		transform_type["Scale"] = &TransformComponent::scale;
		transform_type.set_function("GetWorldPosition", &TransformComponent::GetWorldPosition);

		sol::usertype<SceneCamera> sceneCamera_type = state.new_usertype<SceneCamera>("Camera");
		sceneCamera_type.set_function("SetOrthographic", &SceneCamera::SetOrthographic);
		sceneCamera_type.set_function("SetPerspective", &SceneCamera::SetPerspective);
		sceneCamera_type.set_function("SetAspectRatio", &SceneCamera::SetAspectRatio);
		sceneCamera_type.set_function("GetAspectRatio", &SceneCamera::GetAspectRatio);
		sceneCamera_type.set_function("GetOrthoNear", &SceneCamera::GetOrthoNear);
		sceneCamera_type.set_function("SetOrthoNear", &SceneCamera::GetOrthoNear);
		sceneCamera_type.set_function("GetOrthoFar", &SceneCamera::GetOrthoFar);
		sceneCamera_type.set_function("SetOrthoFar", &SceneCamera::SetOrthoFar);
		sceneCamera_type.set_function("GetOrthoSize", &SceneCamera::GetOrthoSize);
		sceneCamera_type.set_function("SetOrthoSize", &SceneCamera::SetOrthoSize);
		sceneCamera_type.set_function("GetPerspectiveNear", &SceneCamera::GetPerspectiveNear);
		sceneCamera_type.set_function("SetPerspectiveNear", &SceneCamera::SetPerspectiveNear);
		sceneCamera_type.set_function("GetPerspectiveFar", &SceneCamera::GetPerspectiveFar);
		sceneCamera_type.set_function("SetFov", &SceneCamera::SetVerticalFov);
		sceneCamera_type.set_function("SetFov", &SceneCamera::SetVerticalFov);

		auto camera_type = state["CameraComponent"].get_or_create<sol::usertype<CameraComponent>>();
		camera_type["Camera"] = &CameraComponent::Camera;
		camera_type["Primary"] = &CameraComponent::Primary;
		camera_type["FixedAspectRatio"] = &CameraComponent::FixedAspectRatio;

		auto sprite_type = state["SpriteComponent"].get_or_create<sol::usertype<SpriteComponent>>();
		sprite_type["Tint"] = &SpriteComponent::tint;
		sprite_type["Texture"] = &SpriteComponent::texture;
		sprite_type["TilingFactor"] = &SpriteComponent::tilingFactor;

		std::initializer_list<std::pair<sol::string_view, int>> rigidBodyTypesItems =
		{
			{ "Static", (int)RigidBody2DComponent::BodyType::STATIC },
			{ "Kinematic", (int)RigidBody2DComponent::BodyType::KINEMATIC },
			{ "Dynamic", (int)RigidBody2DComponent::BodyType::DYNAMIC }
		};
		state.new_enum("BodyType", rigidBodyTypesItems);

		auto rigidBody2D_type = state["RigidBody2DComponent"].get_or_create<sol::usertype<RigidBody2DComponent>>();
		rigidBody2D_type["Type"] = &RigidBody2DComponent::type;
		rigidBody2D_type["FixedRotation"] = &RigidBody2DComponent::fixedRotation;
		rigidBody2D_type["GravityScale"] = &RigidBody2DComponent::gravityScale;
		rigidBody2D_type["AngularDamping"] = &RigidBody2DComponent::angularDamping;
		rigidBody2D_type["LinearDamping"] = &RigidBody2DComponent::linearDamping;
		
		auto boxCollider2D_type = state["BoxCollider2DComponent"].get_or_create<sol::usertype<BoxCollider2DComponent>>();
		boxCollider2D_type["Offset"] = &BoxCollider2DComponent::Offset;
		boxCollider2D_type["Size"] = &BoxCollider2DComponent::Size;
		boxCollider2D_type["Desity"] = &BoxCollider2DComponent::Density;
		boxCollider2D_type["Friction"] = &BoxCollider2DComponent::Friction;
		boxCollider2D_type["Restitution"] = &BoxCollider2DComponent::Restitution;
		
		auto circleCollider2D_type = state["CircleCollider2DComponent"].get_or_create<sol::usertype<CircleCollider2DComponent>>();
		circleCollider2D_type["Offset"] = &CircleCollider2DComponent::Offset;
		circleCollider2D_type["Radius"] = &CircleCollider2DComponent::Radius;
		circleCollider2D_type["Desity"] = &CircleCollider2DComponent::Density;
		circleCollider2D_type["Friction"] = &CircleCollider2DComponent::Friction;
		circleCollider2D_type["Restitution"] = &CircleCollider2DComponent::Restitution;
		
		auto circleRenderer_type = state["CircleRendererComponent"].get_or_create<sol::usertype<CircleRendererComponent>>();
		circleRenderer_type["Colour"] = &CircleRendererComponent::colour;
		circleRenderer_type["Radius"] = &CircleRendererComponent::Radius;
		circleRenderer_type["Thickness"] = &CircleRendererComponent::Thickness;
		circleRenderer_type["Fade"] = &CircleRendererComponent::Fade;
	}

	//--------------------------------------------------------------------------------------------------------------

	void BindInput(sol::state& state)
	{
		PROFILE_FUNCTION();

		sol::table input = state.create_table("Input");

		input.set_function("IsKeyPressed", [&](char c) -> bool
		{
			return Input::IsKeyPressed((int)c);
		});
		input.set_function("IsMouseButtonPressed", &Input::IsMouseButtonPressed);
		input.set_function("GetMousePos", &Input::GetMousePos);

		std::initializer_list<std::pair<sol::string_view, int>> mouseItems = {
			{ "Left", MOUSE_BUTTON_LEFT },
			{ "Right", MOUSE_BUTTON_RIGHT },
			{ "Middle", MOUSE_BUTTON_MIDDLE },
		};
		state.new_enum("MouseButton", mouseItems);

		std::initializer_list<std::pair<sol::string_view, int>> joystickItems = {
			{ "A", GAMEPAD_BUTTON_A },
			{ "B", GAMEPAD_BUTTON_B },
			{ "X", GAMEPAD_BUTTON_X },
			{ "Y", GAMEPAD_BUTTON_Y },
			{ "LeftBumper", GAMEPAD_BUTTON_LEFT_BUMPER },
			{ "RightBumper", GAMEPAD_BUTTON_LEFT_BUMPER },
			{ "Back", GAMEPAD_BUTTON_BACK },
			{ "Start", GAMEPAD_BUTTON_START },
			{ "Guide",GAMEPAD_BUTTON_GUIDE },
			{ "LeftThumbStick", GAMEPAD_BUTTON_LEFT_THUMB },
			{ "RightThumbStick", GAMEPAD_BUTTON_RIGHT_THUMB },
			{ "Up", GAMEPAD_BUTTON_DPAD_UP },
			{ "Right", GAMEPAD_BUTTON_DPAD_RIGHT },
			{ "Down", GAMEPAD_BUTTON_DPAD_DOWN },
			{ "Left", GAMEPAD_BUTTON_DPAD_LEFT },
			{ "Cross", GAMEPAD_BUTTON_CROSS },
			{ "Circle", GAMEPAD_BUTTON_CIRCLE },
			{ "Square", GAMEPAD_BUTTON_SQUARE },
			{ "Triangle", GAMEPAD_BUTTON_TRIANGLE }
		};
		state.new_enum("JoystickButton", joystickItems);

		std::initializer_list<std::pair<sol::string_view, int>> joystickAxisItems =
		{
			{ "LeftX", GAMEPAD_AXIS_LEFT_X },
			{ "LeftY", GAMEPAD_AXIS_LEFT_Y },
			{ "RightX", GAMEPAD_AXIS_RIGHT_X },
			{ "RightY", GAMEPAD_AXIS_RIGHT_Y },
			{ "LeftTrigger", GAMEPAD_AXIS_LEFT_TRIGGER },
			{ "RightTrigger", GAMEPAD_AXIS_RIGHT_TRIGGER }
		};
		state.new_enum("JoystickAxis", joystickAxisItems);

		input.set_function("GetJoyStickCount", &Joysticks::GetJoystickCount);
		input.set_function("IsJoystickButtonPressed", &Input::IsJoystickButtonPressed);
		input.set_function("GetJoystickAxis", &Input::GetJoystickAxis);
	}

	//--------------------------------------------------------------------------------------------------------------

	void BindMath(sol::state& state)
	{
		PROFILE_FUNCTION();

		sol::usertype<Vector2f> vector2_type = state.new_usertype<Vector2f>(
			"Vec2",
			sol::constructors<Vector2f(float, float)>(),
			"x", &Vector2f::x,
			"y", &Vector2f::y,
			sol::meta_function::addition, [](const Vector2f& a, const Vector2f& b) { return a + b; },
			sol::meta_function::subtraction, [](const Vector2f& a, const Vector2f& b) { return a - b; }
		);

		vector2_type.set_function("Length", &Vector2f::Magnitude);
		vector2_type.set_function("SqrLength", &Vector2f::SqrMagnitude);
		vector2_type.set_function("Normalize", &Vector2f::Normalize);
		vector2_type.set_function("Clamp", &Vector2f::Clamp);
		vector2_type.set_function("Perpendicular", &Vector2f::Perpendicular);

		sol::usertype<Vector3f> vector3_type = state.new_usertype<Vector3f>(
			"Vec3",
			sol::constructors<Vector3f(float, float, float)>(),
			"x", &Vector3f::x,
			"y", &Vector3f::y,
			"z", &Vector3f::z,
			sol::meta_function::addition, [](const Vector3f& a, const Vector3f& b) { return a + b; },
			sol::meta_function::subtraction, [](const Vector3f& a, const Vector3f& b) { return a - b; }
		);

		vector3_type.set_function("Length", &Vector3f::Magnitude);
		vector3_type.set_function("SqrLength", &Vector3f::SqrMagnitude);
		vector3_type.set_function("Normalize", &Vector3f::Normalize);
	}
}