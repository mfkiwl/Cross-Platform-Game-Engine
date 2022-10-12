#pragma once

#include <filesystem>

#include "Core/Layer.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/StaticMesh.h"

#include "Renderer/FrameBuffer.h"
#include "imgui/imgui.h"

#include "ViewportCameraController.h"

class StaticMeshView :
	public Layer
{
public:
	StaticMeshView(bool* show, std::filesystem::path filepath);
	~StaticMeshView() = default;

	virtual void OnAttach() override;
	virtual void OnImGuiRender() override;
	virtual void OnUpdate(float deltaTime) override;
	virtual void OnFixedUpdate() override;
private:
	bool* m_Show;

	std::filesystem::path m_FilePath;

	std::string m_WindowName;

	Ref<StaticMesh> m_Mesh;

	bool m_WindowHovered = false;
	bool m_WindowFocussed = false;
	bool m_CursorDisabled;
	ImVec2 m_ViewportSize;
	Ref<FrameBuffer> m_Framebuffer;
	Vector2f m_RelativeMousePosition;

	ViewportCameraController m_CameraController;

	Ref<Mesh> m_GridMesh;
};
