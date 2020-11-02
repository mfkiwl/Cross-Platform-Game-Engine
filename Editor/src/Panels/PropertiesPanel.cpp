#include "PropertiesPanel.h"

#include "IconsFontAwesome5.h"
#include "MainDockSpace.h"

#include "ImGui/ImGuiTransform.h"

#include "Scene/SceneManager.h"

#include <cstring>

PropertiesPanel::PropertiesPanel(bool* show, HierarchyPanel* hierachyPanel)
	:Layer("Properties"), m_Show(show), m_HierachyPanel(hierachyPanel)
{
}

void PropertiesPanel::OnAttach()
{
}

void PropertiesPanel::OnFixedUpdate()
{
}

void PropertiesPanel::OnImGuiRender()
{
	PROFILE_FUNCTION();

	if (!*m_Show)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_FirstUseEver);

	if (ImGui::Begin(ICON_FA_TOOLS" Properties", m_Show))
	{
		if (ImGui::IsWindowFocused())
		{
			MainDockSpace::SetFocussedWindow(this);
		}

		Entity entity = m_HierachyPanel->GetSelectedEntity();
		if (entity)
		{
			ImGui::BeginGroup();
			DrawComponents(entity);
			ImGui::Separator();
			DrawAddComponent(entity);
			ImVec2 available = ImGui::GetContentRegionAvail();
			ImGui::Dummy(available);
			ImGui::EndGroup();
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Asset", ImGuiDragDropFlags_None))
				{
					std::filesystem::path* file = (std::filesystem::path*)payload->Data;

					if (file->extension() == ".staticmesh" && !entity.HasComponent<StaticMeshComponent>())
					{
						//TODO: Store the material in the mesh file
						Mesh mesh(*file);

						Material material(Shader::Create("NormalMap"));

						material.AddTexture(Texture2D::Create(Application::GetWorkingDirectory().string() + "\\resources\\UVChecker.png"), 0);

						entity.AddComponent<StaticMeshComponent>(mesh, material);
					}

					CLIENT_DEBUG(file->string());
				}
				ImGui::EndDragDropTarget();
			}
		}
	}
	ImGui::End();
}

void PropertiesPanel::DrawComponents(Entity entity)
{
	//Tag------------------------------------------------------------------------------------------------------------------
	if (entity.HasComponent<TagComponent>())
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;

		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		std::strncpy(buffer, tag.c_str(), sizeof(buffer));

		if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
		{
			tag = std::string(buffer);
			SceneManager::s_CurrentScene->MakeDirty();
		}
	}

	//Transform------------------------------------------------------------------------------------------------------------
	DrawComponent<TransformComponent>("Transform", entity, [](auto& transform)
		{
			ImGui::Transform(transform.Position, transform.Rotation, transform.Scale);
		}, false);


	//Sprite--------------------------------------------------------------------------------------------------------------
	DrawComponent<SpriteComponent>(ICON_FA_IMAGE" Sprite", entity, [](auto& sprite)
		{
			float* tint[4] = { &sprite.Tint.r, &sprite.Tint.g, &sprite.Tint.b, &sprite.Tint.a };

			if (ImGui::ColorEdit4("Tint", tint[0]))
			{
				SceneManager::s_CurrentScene->MakeDirty();
			}
		});

	//Static Mesh------------------------------------------------------------------------------------------------------------
	DrawComponent<StaticMeshComponent>(ICON_FA_SHAPES" Static Mesh", entity, [](auto& staticMesh)
		{
			//TODO: create static mesh properties

			/* static mesh properties */
		});

	//Native Script------------------------------------------------------------------------------------------------------------
	DrawComponent<NativeScriptComponent>(ICON_FA_FILE_CODE" Native Script", entity, [](auto& script)
		{
			ImGui::Text("%s", script.Name.c_str());
		});

	//Camera------------------------------------------------------------------------------------------------------------
	DrawComponent<CameraComponent>(ICON_FA_VIDEO" Camera", entity, [](auto& cameraComp)
		{
			auto& camera = cameraComp.Camera;

			ImGui::Checkbox("Primary", &cameraComp.Primary);
			ImGui::Checkbox("Fixed Aspect Ratio", &cameraComp.FixedAspectRatio);

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };

			const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];

			if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];

					if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
					{
						currentProjectionTypeString = projectionTypeStrings[i];
						camera.SetProjection((SceneCamera::ProjectionType)i);
						SceneManager::s_CurrentScene->MakeDirty();
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}


			switch (camera.GetProjectionType())
			{
			case SceneCamera::ProjectionType::perspective:
			{
				float fov = (float)RadToDeg(camera.GetVerticalFov());
				if (ImGui::DragFloat("FOV", &fov, 1.0f, 1.0f, 180.0f))
				{
					camera.SetVerticalFov((float)DegToRad(fov));
					SceneManager::s_CurrentScene->MakeDirty();
				}

				float nearClip = camera.GetPerspectiveNear();
				if (ImGui::DragFloat("Near Clip##Perspective", &nearClip, 1.0f, 0.001f, 10000.0f))
				{
					camera.SetPerspectiveNear(nearClip);
					SceneManager::s_CurrentScene->MakeDirty();
				}

				float farClip = camera.GetPerspectiveFar();
				if (ImGui::DragFloat("Far Clip##Perspective", &farClip, 1.0f, 0.001f, 10000.0f))
				{
					camera.SetPerspectiveFar(farClip);
					SceneManager::s_CurrentScene->MakeDirty();
				}

				camera.RecalculateProjection();
				break;
			}
			case SceneCamera::ProjectionType::orthographic:
			{
				float size = camera.GetOrthoSize();
				if (ImGui::DragFloat("Size", &size, 0.1f, 0.0f, 100.0f))
				{
					camera.SetOrthoSize(size);
					SceneManager::s_CurrentScene->MakeDirty();
				}

				float nearClip = camera.GetOrthoNear();
				if (ImGui::DragFloat("Near Clip##Orthographic", &nearClip))
				{
					camera.SetOrthoNear(nearClip);
					SceneManager::s_CurrentScene->MakeDirty();
				}

				float farClip = camera.GetOrthoFar();
				if (ImGui::DragFloat("Far Clip##Orthographic", &farClip))
				{
					camera.SetOrthoFar(farClip);
					SceneManager::s_CurrentScene->MakeDirty();
				}

				camera.RecalculateProjection();
				break;
			}
			}
		});

	//Primitive--------------------------------------------------------------------------------------------------------------
	DrawComponent<PrimitiveComponent>(ICON_FA_SHAPES" Primitive", entity, [](auto& primitive)
		{
			const char* shapeTypeStrings[] = { "Cube", "Sphere", "Plane", "Cylinder", "Cone", "Torus" };

			const char* currentShapeTypeString = shapeTypeStrings[(int)primitive.Type];

			if (ImGui::BeginCombo("Shape", currentShapeTypeString))
			{
				for (int i = 0; i < 6; i++)
				{
					bool isSelected = currentShapeTypeString == shapeTypeStrings[i];

					if (ImGui::Selectable(shapeTypeStrings[i], isSelected))
					{
						currentShapeTypeString = shapeTypeStrings[i];
						primitive.Type = (PrimitiveComponent::Shape)i;
						primitive.NeedsUpdating = true;
						SceneManager::s_CurrentScene->MakeDirty();
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}
			switch (primitive.Type)
			{
			case PrimitiveComponent::Shape::Cube:
				if (ImGui::DragFloat("Width##cube", &primitive.CubeWidth, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragFloat("Height##cube", &primitive.CubeHeight, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragFloat("Depth##cube", &primitive.CubeDepth, 0.1f, 0.0f))
					primitive.NeedsUpdating = true;
				break;
			case PrimitiveComponent::Shape::Sphere:
				if (ImGui::DragFloat("Radius##Sphere", &primitive.SphereRadius, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragInt("Longitude Lines##Sphere", &(int)primitive.SphereLongitudeLines, 1.0f, 3, 600))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragInt("Latitude Lines##Sphere", &(int)primitive.SphereLatitudeLines, 1.0f, 2, 600))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				break;
			case PrimitiveComponent::Shape::Plane:
				if (ImGui::DragFloat("Width##Plane", &primitive.PlaneWidth, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragFloat("Length##Plane", &primitive.PlaneLength, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragInt("Width Lines##Plane", &(int)primitive.PlaneWidthLines, 1.0f, 2, 1000))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragInt("Length Lines##Plane", &(int)primitive.PlaneLengthLines, 1.0f, 2, 1000))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragFloat("Tile U##Plane", &primitive.PlaneTileU, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragFloat("Tile V##Plane", &primitive.PlaneTileV, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				break;
			case PrimitiveComponent::Shape::Cylinder:
				if (ImGui::DragFloat("Bottom Radius##Cylinder", &primitive.CylinderBottomRadius, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragFloat("Top Radius##Cylinder", &primitive.CylinderTopRadius, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragFloat("Height##Cylinder", &primitive.CylinderHeight, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragInt("Slice Count##Cylinder", &(int)primitive.CylinderSliceCount, 1.0f, 3, 600))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragInt("Stack Count##Cylinder", &(int)primitive.CylinderStackCount, 1.0f, 1, 600))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				break;
			case PrimitiveComponent::Shape::Cone:
				if (ImGui::DragFloat("Bottom Radius##Cone", &primitive.ConeBottomRadius, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragFloat("Height##Cone", &primitive.ConeHeight, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragInt("Slice Count##Cone", &(int)primitive.ConeSliceCount, 1.0f, 3, 600))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragInt("Stack Count##Cone", &(int)primitive.ConeStackCount, 1.0f, 1, 600))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				break;
			case PrimitiveComponent::Shape::Torus:
				if (ImGui::DragFloat("Outer Radius##Torus", &primitive.TorusOuterRadius, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragFloat("Inner Radius##Torus", &primitive.TorusInnerRadius, 0.1f, 0.0f))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				if (ImGui::DragFloat("Slice Count##Torus", &primitive.TorusSliceCount, 1.0f, 3, 600))
				{
					primitive.NeedsUpdating = true;
					SceneManager::s_CurrentScene->MakeDirty();
				}
				break;
			default:
				break;
			}
		});
}

void PropertiesPanel::DrawAddComponent(Entity entity)
{
	ImGui::Dummy({ 0,0 });
	float width = ImGui::GetContentRegionAvailWidth();
	ImGui::SameLine((width / 2.0f) - (ImGui::CalcTextSize("Add Component").x / 2.0f));
	if (ImGui::Button("Add Component"))
	{
		ImGui::OpenPopup("Components");
	}

	if (ImGui::BeginPopup("Components"))
	{
		AddComponentMenuItem<CameraComponent>("Camera", entity);
		AddComponentMenuItem<SpriteComponent>("Sprite", entity);
		AddComponentMenuItem<StaticMeshComponent>("Static Mesh", entity);
		AddComponentMenuItem<PrimitiveComponent>("Primitive", entity);

		if (ImGui::BeginMenu("Native Script", !entity.HasComponent<NativeScriptComponent>() && Factory<ScriptableEntity>::GetMap()->size() > 0))
		{
			for (auto&& [key, value] : *Factory<ScriptableEntity>::GetMap())
			{
				if (ImGui::MenuItem(key.c_str()))
				{
					entity.AddComponent<NativeScriptComponent>().Bind(key);
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}
}
