#include "PropertiesPanel.h"

#include "IconsFontAwesome6.h"
#include "IconsMaterialDesign.h"
#include "Fonts/IconsMaterialDesignIcons.h"
#include "Fonts/Fonts.h"
#include "MainDockSpace.h"

#include "Engine.h"

#include "ImGui/ImGuiTransform.h"
#include "ImGui/ImGuiFileEdit.h"
#include "ImGui/ImGuiTextureEdit.h"
#include "ImGui/ImGuiAssetEdit.h"
#include "ImGui/ImGuiVectorEdit.h"

#include "Viewers/ViewerManager.h"
#include "FileSystem/Directory.h"

#define Dirty(x) if(x) SceneManager::CurrentScene()->MakeDirty()

PropertiesPanel::PropertiesPanel(bool* show, Ref<HierarchyPanel> hierarchyPanel, Ref<TilemapEditor> tilemapEditor)
	:Layer("Properties"), m_Show(show), m_HierarchyPanel(hierarchyPanel), m_TilemapEditor(tilemapEditor)
{
}

void PropertiesPanel::OnAttach()
{
	m_DefaultMaterial = Material::GetDefaultMaterial();

	m_DefaultPhysMaterial = PhysicsMaterial::GetDefaultPhysicsMaterial();
}

void PropertiesPanel::OnUpdate(float deltaTime)
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

	if (ImGui::Begin(ICON_FA_SCREWDRIVER_WRENCH" Properties", m_Show))
	{
		if (ImGui::IsWindowFocused())
		{
			MainDockSpace::SetFocussedWindow(this);
		}

		Entity entity = m_HierarchyPanel->GetSelectedEntity();
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
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Asset", ImGuiDragDropFlags_AcceptPeekOnly))
				{
					std::filesystem::path* file = (std::filesystem::path*)payload->Data;

					if (ViewerManager::GetFileType(*file) == FileType::MESH)
					{
						if (ImGui::AcceptDragDropPayload("Asset", ImGuiDragDropFlags_None))
						{
							StaticMeshComponent comp = entity.GetOrAddComponent<StaticMeshComponent>();
							comp.mesh->Load(*file);
						}
					}
					else if (file->extension() == ".lua")
					{
						if (ImGui::AcceptDragDropPayload("Asset", ImGuiDragDropFlags_None))
						{
							LuaScriptComponent comp = entity.GetOrAddComponent<LuaScriptComponent>();
							comp.absoluteFilepath = *file;
							comp.ParseScript(entity);
						}
					}

					CLIENT_DEBUG(file->string());
				}
				ImGui::EndDragDropTarget();
			}
		}
		else if (SceneManager::IsSceneLoaded())
		{
			Dirty(ImGui::Vector("Gravity Scale", SceneManager::CurrentScene()->GetGravity(), ImGui::GetContentRegionAvail().x));
		}
	}
	ImGui::End();
}

void PropertiesPanel::DrawComponents(Entity entity)
{
	// Name------------------------------------------------------------------------------------------------------------------
	if (entity.HasComponent<NameComponent>())
	{
		auto& name = entity.GetComponent<NameComponent>().name;

		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		std::strncpy(buffer, name.c_str(), sizeof(buffer));

		if (ImGui::InputText("Entity Name##", buffer, sizeof(buffer)))
		{
			name = std::string(buffer);
			SceneManager::CurrentScene()->MakeDirty();
		}
	}

	// Transform------------------------------------------------------------------------------------------------------------
	DrawComponent<TransformComponent>(ICON_MDI_AXIS_ARROW" Transform", entity, [&](auto& transform)
		{
			if (ImGui::Transform(transform.position, transform.rotation, transform.scale))
			{
				if (entity.HasComponent<RigidBody2DComponent>())
				{
					entity.GetComponent<RigidBody2DComponent>().SetTransform(transform.position, transform.rotation.z);
				}
				SceneManager::CurrentScene()->MakeDirty();
			}
		}, false);

	// Sprite--------------------------------------------------------------------------------------------------------------
	DrawComponent<SpriteComponent>(ICON_FA_IMAGE" Sprite", entity, [&](auto& sprite)
		{
			float* tint[4] = { &sprite.tint.r, &sprite.tint.g, &sprite.tint.b, &sprite.tint.a };
			float* tilingFactor = &sprite.tilingFactor;

			Dirty(ImGui::ColorEdit4("Tint", tint[0]));

			if (sprite.texture)
			{
				if (ImGui::Button("Pixel Perfect"))
				{
					if (sprite.texture)
					{
						entity.GetTransform().scale.x = (float)sprite.texture->GetWidth() / 16.0f;
						entity.GetTransform().scale.y = (float)sprite.texture->GetHeight() / 16.0f;
					}
					SceneManager::CurrentScene()->MakeDirty();
				}
				ImGui::Tooltip("Set Scale to pixel perfect scaling");
			}

			Dirty(ImGui::Texture2DEdit("Texture", sprite.texture));

			if (sprite.texture)
			{
				Dirty(ImGui::DragFloat("Tiling Factor", tilingFactor, 0.1f, 0.0f, 100.0f));

				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					*tilingFactor = 1.0f;
					SceneManager::CurrentScene()->MakeDirty();
				}
			}
		});

	// Animated Sprite--------------------------------------------------------------------------------------------------------------
	DrawComponent<AnimatedSpriteComponent>(ICON_FA_IMAGE" Animated Sprite", entity, [&](auto& sprite)
		{
			float* tint[4] = { &sprite.tint.r, &sprite.tint.g, &sprite.tint.b, &sprite.tint.a };

			Dirty(ImGui::ColorEdit4("Tint", tint[0]));

			std::string tilesetName;
			if (sprite.spriteSheet)
			{
				if (ImGui::Button("Pixel Perfect"))
				{
					if (sprite.spriteSheet)
					{
						entity.GetTransform().scale.x = (float)sprite.spriteSheet->GetSubTexture()->GetSpriteWidth() / 16.0f;
						entity.GetTransform().scale.y = (float)sprite.spriteSheet->GetSubTexture()->GetSpriteHeight() / 16.0f;
						SceneManager::CurrentScene()->MakeDirty();
					}
				}
				ImGui::Tooltip("Set Scale to pixel perfect scaling");
				tilesetName = sprite.spriteSheet->GetFilepath().filename().string();

				static std::filesystem::file_time_type currentFileTime;

				std::filesystem::file_time_type lastWrittenTime = std::filesystem::last_write_time(sprite.spriteSheet->GetFilepath());

				if (lastWrittenTime != currentFileTime)
				{
					sprite.spriteSheet->Reload();
					currentFileTime = lastWrittenTime;
					sprite.spriteSheet->SelectAnimation(sprite.spriteSheet->GetCurrentAnimation());
				}
			}

			if (ImGui::BeginCombo("SpriteSheet", tilesetName.c_str()))
			{
				for (std::filesystem::path& file : Directory::GetFilesRecursive(Application::GetOpenDocumentDirectory(), ViewerManager::GetExtensions(FileType::SPRITESHEET)))
				{
					const bool is_selected = false;
					if (ImGui::Selectable(file.filename().string().c_str(), is_selected))
					{
						sprite.spriteSheet = AssetManager::GetAsset<SpriteSheet>(file);
						SceneManager::CurrentScene()->MakeDirty();
					}
					ImGui::Tooltip(file.string().c_str());
				}
				ImGui::EndCombo();
			}
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Asset", ImGuiDragDropFlags_AcceptPeekOnly))
				{
					std::filesystem::path* file = (std::filesystem::path*)payload->Data;

					for (std::string& ext : ViewerManager::GetExtensions(FileType::SPRITESHEET))
					{
						if (file->extension().string() == ext)
						{
							if (ImGui::AcceptDragDropPayload("Asset", ImGuiDragDropFlags_None))
								sprite.spriteSheet = AssetManager::GetAsset<SpriteSheet>(*file);
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
			if (sprite.spriteSheet)
			{
				ImGui::SameLine();

				if (ImGui::Button(ICON_FA_PEN_TO_SQUARE"##AnimatedSprite"))
				{
					ViewerManager::OpenViewer(sprite.spriteSheet->GetFilepath());
				}
				ImGui::Tooltip("Edit Tileset");
			}

			if (sprite.spriteSheet)
			{
				if (ImGui::BeginCombo("Animation", sprite.spriteSheet->GetCurrentAnimation().c_str()))
				{
					for (auto&& [name, animation] : sprite.spriteSheet->GetAnimations())
					{
						if (ImGui::Selectable(name.c_str()))
						{
							sprite.SelectAnimation(name);
							SceneManager::CurrentScene()->MakeDirty();
						}
					}
					ImGui::EndCombo();
				}
			}
		});

	// Circle Renderer------------------------------------------------------------------------------------------------------------------
	DrawComponent<CircleRendererComponent>(ICON_FA_CIRCLE" Circle Renderer", entity, [](auto& circleRenderer)
		{
			float* colour[4] = { &circleRenderer.colour.r, &circleRenderer.colour.g, &circleRenderer.colour.b, &circleRenderer.colour.a };
			Dirty(ImGui::ColorEdit4("Colour", colour[0]));
			Dirty(ImGui::DragFloat("Thickness", &circleRenderer.thickness, 0.025f, 0.0f, 1.0f));
			Dirty(ImGui::DragFloat("Fade", &circleRenderer.fade, 0.00025f, 0.0f, 1.0f));
		});

	// Tilemap ------------------------------------------------------------------------------------------------------------------------
	DrawComponent<TilemapComponent>(ICON_FA_BORDER_ALL" Tilemap", entity, [=](auto& tilemap)
		{
			//m_TilemapEditor->OnImGuiRender(tilemap);

			if(tilemap.tileset)
			{
				static std::filesystem::file_time_type currentFileTime;

				if (std::filesystem::exists(tilemap.tileset->GetFilepath()))
				{
					std::filesystem::file_time_type lastWrittenTime = std::filesystem::last_write_time(tilemap.tileset->GetFilepath());

					if (lastWrittenTime != currentFileTime)
					{
						tilemap.tileset->Reload();
						tilemap.Rebuild();
						currentFileTime = lastWrittenTime;
					}
				}
			}

			if (ImGui::Combo("Orientation", (int*)&tilemap.orientation,
				"Orthogonal\0"
				"Isometric\0"
				"Isometric (staggered)\0"
				"Hexagonal (staggered)"))
			{
				tilemap.Rebuild();
				SceneManager::CurrentScene()->MakeDirty();
			}

			int tilesWide = tilemap.tilesWide;
			if (ImGui::DragInt("Width", &tilesWide, 1.0f, 0, 1000))
			{
				tilemap.tilesWide = tilesWide;

				for (auto& row : tilemap.tiles)
				{
					row.resize(tilesWide);
				}
				tilemap.Rebuild();
				SceneManager::CurrentScene()->MakeDirty();
			}

			int tilesHigh = tilemap.tilesHigh;
			if (ImGui::DragInt("Height", &tilesHigh, 1.0f, 0, 1000))
			{
				tilemap.tiles.resize(tilesHigh);
				if (tilemap.tilesHigh < (uint32_t)tilesHigh)
				{
					for (size_t i = tilemap.tilesHigh; i < tilesHigh; i++)
					{
						tilemap.tiles[i].resize(tilemap.tilesWide);
					}
				}
				tilemap.tilesHigh = tilesHigh;

				tilemap.Rebuild();
				SceneManager::CurrentScene()->MakeDirty();
			}
			std::string tilesetName;
			if (tilemap.tileset)
				tilesetName = tilemap.tileset->GetFilepath().filename().string();

			if (ImGui::BeginCombo("Tileset", tilesetName.c_str()))
			{
				for (std::filesystem::path& file : Directory::GetFilesRecursive(Application::GetOpenDocumentDirectory(), ViewerManager::GetExtensions(FileType::TILESET)))
				{
					if (ImGui::Selectable(file.filename().string().c_str()))
					{
						tilemap.tileset = AssetManager::GetAsset<Tileset>(file);
						tilemap.Rebuild();
						SceneManager::CurrentScene()->MakeDirty();
					}
					ImGui::Tooltip(file.string().c_str());
				}
				ImGui::EndCombo();
			}
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Asset", ImGuiDragDropFlags_AcceptPeekOnly))
				{
					std::filesystem::path* file = (std::filesystem::path*)payload->Data;

					for (std::string& ext : ViewerManager::GetExtensions(FileType::TILESET))
					{
						if (file->extension().string() == ext)
						{
							if (ImGui::AcceptDragDropPayload("Asset", ImGuiDragDropFlags_None))
							{
								tilemap.tileset = AssetManager::GetAsset<Tileset>(*file);
								tilemap.material->AddTexture(tilemap.tileset->GetSubTexture()->GetTexture(), 0);
								SceneManager::CurrentScene()->MakeDirty();
							}
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
			if (tilemap.tileset)
			{
				ImGui::SameLine();
				if (ImGui::Button(ICON_FA_PEN_TO_SQUARE))
				{
					ViewerManager::OpenViewer(tilemap.tileset->GetFilepath());
				}
			}

			if (ImGui::Button("Edit Tilemap"))
				m_TilemapEditor->Show();
		});

	// Static Mesh------------------------------------------------------------------------------------------------------------
	DrawComponent<StaticMeshComponent>(ICON_FA_SHAPES" Static Mesh", entity, [=](auto& staticMesh)
		{
			std::filesystem::path meshFilepath;
			if (staticMesh.mesh)
				meshFilepath = staticMesh.mesh->GetFilepath();

			if (ImGui::FileSelect("Static Mesh", meshFilepath, FileType::MESH))
			{
				staticMesh.mesh = AssetManager::GetAsset<StaticMesh>(meshFilepath);
				SceneManager::CurrentScene()->MakeDirty();
			}
			for (size_t i = 0; i < staticMesh.materialOverrides.size(); ++i)
			{
				Ref<Material> material = AssetManager::GetAsset<Material>(
					std::filesystem::absolute(Application::GetOpenDocumentDirectory() / staticMesh.materialOverrides[i]));

				if (ImGui::AssetEdit<Material>(std::string("Material " + std::to_string(i)).c_str(), material, staticMesh.mesh->GetMeshes()[i]->GetMaterial(), FileType::MATERIAL))
					staticMesh.materialOverrides[i] = FileUtils::RelativePath(material->GetFilepath(), Application::GetOpenDocumentDirectory()).string();
			}
		});

	// Camera------------------------------------------------------------------------------------------------------------
	DrawComponent<CameraComponent>(ICON_FA_VIDEO" Camera", entity, [](auto& cameraComp)
		{
			auto& camera = cameraComp.Camera;

			Dirty(ImGui::Checkbox("Primary", &cameraComp.Primary));
			Dirty(ImGui::Checkbox("Fixed Aspect Ratio", &cameraComp.FixedAspectRatio));

			SceneCamera::ProjectionType projectionType = camera.GetProjectionType();
			if (ImGui::Combo("Projection", (int*)&projectionType,
				"Perspective\0"
				"Orthographic"))
			{
				camera.SetProjection(projectionType);
				SceneManager::CurrentScene()->MakeDirty();
			}

			switch (camera.GetProjectionType())
			{
			case SceneCamera::ProjectionType::perspective:
			{
				float fov = (float)RadToDeg(camera.GetVerticalFov());
				if (ImGui::DragFloat("FOV", &fov, 1.0f, 1.0f, 180.0f))
				{
					camera.SetVerticalFov((float)DegToRad(fov));
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					camera.SetVerticalFov((float)PI * 0.5f);
					SceneManager::CurrentScene()->MakeDirty();
				}

				float nearClip = camera.GetPerspectiveNear();
				if (ImGui::DragFloat("Near Clip##Perspective", &nearClip, 1.0f, 0.001f, 10000.0f))
				{
					camera.SetPerspectiveNear(nearClip);
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					camera.SetPerspectiveNear(1.0f);
					SceneManager::CurrentScene()->MakeDirty();
				}

				float farClip = camera.GetPerspectiveFar();
				if (ImGui::DragFloat("Far Clip##Perspective", &farClip, 1.0f, 0.001f, 10000.0f))
				{
					camera.SetPerspectiveFar(farClip);
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					camera.SetPerspectiveFar(1000.0f);
					SceneManager::CurrentScene()->MakeDirty();
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
					SceneManager::CurrentScene()->MakeDirty();
				}

				float nearClip = camera.GetOrthoNear();
				if (ImGui::DragFloat("Near Clip##Orthographic", &nearClip))
				{
					camera.SetOrthoNear(nearClip);
					SceneManager::CurrentScene()->MakeDirty();
				}

				float farClip = camera.GetOrthoFar();
				if (ImGui::DragFloat("Far Clip##Orthographic", &farClip))
				{
					camera.SetOrthoFar(farClip);
					SceneManager::CurrentScene()->MakeDirty();
				}

				camera.RecalculateProjection();
				break;
			}
			}
		});

	// Primitive--------------------------------------------------------------------------------------------------------------
	DrawComponent<PrimitiveComponent>(ICON_FA_SHAPES" Primitive", entity, [=](auto& primitive)
		{
			if (ImGui::Combo("Shape", (int*)&primitive.type,
				"Cube\0"
				"Sphere\0"
				"Plane\0"
				"Cylinder\0"
				"Cone\0"
				"Torus\0"))
			{
				primitive.needsUpdating = true;
				SceneManager::CurrentScene()->MakeDirty();
			}

			switch (primitive.type)
			{
				int tempInt;
			case PrimitiveComponent::Shape::Cube:
				if (ImGui::DragFloat("Width##cube", &primitive.cubeWidth, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::DragFloat("Height##cube", &primitive.cubeHeight, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::DragFloat("Depth##cube", &primitive.cubeDepth, 0.1f, 0.0f))
					primitive.needsUpdating = true;
				break;
			case PrimitiveComponent::Shape::Sphere:
				if (ImGui::DragFloat("Radius##Sphere", &primitive.sphereRadius, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				tempInt = primitive.sphereLongitudeLines;
				if (ImGui::DragInt("Longitude Lines##Sphere", &tempInt, 1.0f, 3, 600))
				{
					primitive.sphereLongitudeLines = tempInt;
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				tempInt = primitive.sphereLatitudeLines;
				if (ImGui::DragInt("Latitude Lines##Sphere", &tempInt, 1.0f, 3, 600))
				{
					primitive.sphereLatitudeLines = tempInt;
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				break;
			case PrimitiveComponent::Shape::Plane:
				if (ImGui::DragFloat("Width##Plane", &primitive.planeWidth, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::DragFloat("Length##Plane", &primitive.planeLength, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				tempInt = primitive.planeWidthLines;
				if (ImGui::DragInt("Width Lines##Plane", &tempInt, 1.0f, 2, 1000))
				{
					primitive.planeWidthLines = tempInt;
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				tempInt = primitive.planeLengthLines;
				if (ImGui::DragInt("Length Lines##Plane", &tempInt, 1.0f, 2, 1000))
				{
					primitive.planeLengthLines = tempInt;
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::DragFloat("Tile U##Plane", &primitive.planeTileU, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::DragFloat("Tile V##Plane", &primitive.planeTileV, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				break;
			case PrimitiveComponent::Shape::Cylinder:
				if (ImGui::DragFloat("Bottom Radius##Cylinder", &primitive.cylinderBottomRadius, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::DragFloat("Top Radius##Cylinder", &primitive.cylinderTopRadius, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::DragFloat("Height##Cylinder", &primitive.cylinderHeight, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				tempInt = primitive.cylinderSliceCount;
				if (ImGui::DragInt("Slice Count##Cylinder", &tempInt, 1.0f, 3, 600))
				{
					primitive.cylinderSliceCount = tempInt;
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				tempInt = primitive.cylinderStackCount;
				if (ImGui::DragInt("Stack Count##Cylinder", &tempInt, 1.0f, 1, 600))
				{
					primitive.cylinderStackCount = tempInt;
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				break;
			case PrimitiveComponent::Shape::Cone:
				if (ImGui::DragFloat("Bottom Radius##Cone", &primitive.coneBottomRadius, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::DragFloat("Height##Cone", &primitive.coneHeight, 0.1f, 0.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				tempInt = primitive.coneSliceCount;
				if (ImGui::DragInt("Slice Count##Cone", &tempInt, 1.0f, 3, 600))
				{
					primitive.coneSliceCount = tempInt;
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				tempInt = primitive.coneStackCount;
				if (ImGui::DragInt("Stack Count##Cone", &tempInt, 1.0f, 1, 600))
				{
					primitive.coneStackCount = tempInt;
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				break;
			case PrimitiveComponent::Shape::Torus:
				if (ImGui::DragFloat("Outer Radius##Torus", &primitive.torusOuterRadius, 0.1f, 0.0f, 100.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				if (ImGui::DragFloat("Inner Radius##Torus", &primitive.torusInnerRadius, 0.1f, 0.0f, 100.0f))
				{
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				tempInt = primitive.torusSliceCount;
				if (ImGui::DragInt("Slice Count##Torus", &tempInt, 1.0f, 3, 600))
				{
					primitive.torusSliceCount = tempInt;
					primitive.needsUpdating = true;
					SceneManager::CurrentScene()->MakeDirty();
				}
				break;
			default:
				break;
			}
			Dirty(ImGui::AssetEdit<Material>("Material", primitive.material, m_DefaultMaterial, FileType::MATERIAL));
		});

	// Text Component ------------------------------------------------------------------------------------------------------------
	DrawComponent<TextComponent>(ICON_FA_FONT" Text", entity, [](auto& text)
		{
			static char inputBuffer[1024] = "";
			memset(inputBuffer, 0, sizeof(inputBuffer));
			for (int i = 0; i < text.text.length(); i++)
			{
				inputBuffer[i] = text.text[i];
			}

			if (ImGui::InputTextMultiline("Text", inputBuffer, sizeof(inputBuffer)))
			{
				text.text = inputBuffer;
				SceneManager::CurrentScene()->MakeDirty();
			}

			if (ImGui::AssetEdit<Font>("Font", text.font, Font::GetDefaultFont(), FileType::FONT))
			{
				SceneManager::CurrentScene()->MakeDirty();
			}
			Dirty(ImGui::DragFloat("Max Width", &text.maxWidth, 0.001f, 0.0f, 100.0f, "%.2f"));
			float* colour[4] = { &text.colour.r, &text.colour.g, &text.colour.b, &text.colour.a };
			Dirty(ImGui::ColorEdit4("Colour", colour[0]));
		});

	// Rigid Body 2D--------------------------------------------------------------------------------------------------------------
	DrawComponent<RigidBody2DComponent>(ICON_FA_BASEBALL" Rigid Body 2D", entity, [](auto& rigidBody2D)
		{
			Dirty(ImGui::Combo("Body Type", (int*)&rigidBody2D.type,
				"Static\0"
				"Kinematic\0"
				"Dynamic\0"));

			if (rigidBody2D.type == RigidBody2DComponent::BodyType::DYNAMIC)
			{
				Dirty(ImGui::Checkbox("Fixed Rotation", &rigidBody2D.fixedRotation));
				Dirty(ImGui::DragFloat("Gravity Scale", &rigidBody2D.gravityScale, 0.01f, -1.0f, 2.0f));
				Dirty(ImGui::DragFloat("Angular Damping", &rigidBody2D.angularDamping, 0.01f, 0.0f, 1.0f));
				Dirty(ImGui::DragFloat("Linear Damping", &rigidBody2D.linearDamping, 0.01f, 0.0f, 1.0f));
			}
		});

	// Box Collider 2D--------------------------------------------------------------------------------------------------------------
	DrawComponent<BoxCollider2DComponent>(ICON_FA_VECTOR_SQUARE" Box Collider 2D", entity, [=](auto& boxCollider2D)
		{
			Dirty(ImGui::Vector("Offset", boxCollider2D.offset));
			Dirty(ImGui::Vector("Size", boxCollider2D.size));
			Dirty(ImGui::AssetEdit<PhysicsMaterial>("Physics Material", boxCollider2D.physicsMaterial, m_DefaultPhysMaterial, FileType::PHYSICSMATERIAL));
		});

	// Circle Collider 2D--------------------------------------------------------------------------------------------------------------
	DrawComponent<CircleCollider2DComponent>(ICON_MDI_CIRCLE_OUTLINE" Circle Collider 2D", entity, [=](auto& circleCollider2D)
		{
			Dirty(ImGui::Vector("Offset", circleCollider2D.offset));
			Dirty(ImGui::DragFloat("Radius", &circleCollider2D.radius, 0.01f, 0.0f, 10.0f));
			Dirty(ImGui::AssetEdit<PhysicsMaterial>("Physics Material", circleCollider2D.physicsMaterial, m_DefaultPhysMaterial, FileType::PHYSICSMATERIAL));
		});

	// Polygon Collider 2D ------------------------------------------------------------------------------------------------------------
	DrawComponent<PolygonCollider2DComponent>(ICON_FA_DRAW_POLYGON" Polygon Collider 2D", entity, [=](auto& polygonCollider2D)
		{
			Dirty(ImGui::Vector("Offset", polygonCollider2D.offset));

			if (ImGui::TreeNode("Vertices"))
			{
				if (ImGui::Button(ICON_FA_PLUS"##AddVertex"))
				{
					polygonCollider2D.vertices.push_back(Vector2f());
					SceneManager::CurrentScene()->MakeDirty();
				}
				size_t i = 0;
				while (i < polygonCollider2D.vertices.size())
				{
					std::string deleteStr = ICON_FA_TRASH_CAN "##" + std::to_string(i);
					if (ImGui::Button(deleteStr.c_str()))
					{
						polygonCollider2D.vertices.erase(polygonCollider2D.vertices.begin() + i);
						continue;
					}

					ImGui::SameLine();
					std::string labelStr = std::to_string(i);
					Dirty(ImGui::Vector(labelStr.c_str(), polygonCollider2D.vertices[i]));

					i++;
				}
				ImGui::TreePop();
			}

			Dirty(ImGui::AssetEdit<PhysicsMaterial>("Physics Material", polygonCollider2D.physicsMaterial, m_DefaultPhysMaterial, FileType::PHYSICSMATERIAL));

			if (polygonCollider2D.vertices.size() < 3)
			{
				Colour textColour(Colours::YELLOW);
				ImGui::TextColored(ImVec4(textColour.r, textColour.g, textColour.b, textColour.a), "No Polygon Collider Created!");
			}
		});

	// Capsule Collider ---------------------------------------------------------------------------------------------------------------
	DrawComponent<CapsuleCollider2DComponent>(ICON_FA_CAPSULES" Capsule Collider 2D", entity, [=](auto& capsuleCollider2D)
		{
			Dirty(ImGui::Vector("Offset", capsuleCollider2D.offset));

			Dirty(ImGui::DragFloat("Radius", &capsuleCollider2D.radius, 0.01f, 0.0f, 10.0f));

			Dirty(ImGui::DragFloat("Height", &capsuleCollider2D.height, 0.01f, 0.0f, 10.0f));

			Dirty(ImGui::Combo("Direction", (int*)&capsuleCollider2D.direction,
				"Vertical\0"
				"Horizontal\0"));

			Dirty(ImGui::AssetEdit<PhysicsMaterial>("Physics Material", capsuleCollider2D.physicsMaterial, m_DefaultPhysMaterial, FileType::PHYSICSMATERIAL));
		});

	// Point Light --------------------------------------------------------------------------------------------------------------------
	DrawComponent<PointLightComponent>(ICON_FA_LIGHTBULB" Point Light", entity, [](auto& pointLight)
		{
			float* colour[4] = { &pointLight.colour.r, &pointLight.colour.g, &pointLight.colour.b, &pointLight.colour.a };
			Dirty(ImGui::ColorEdit4("Colour", colour[0]));
			Dirty(ImGui::Checkbox("Cast Shadows", &pointLight.castsShadows));
			Dirty(ImGui::DragFloat("Range", &pointLight.range, 0.1f, 0.0f, 1000.0f, "%.2f"));
			Dirty(ImGui::DragFloat("Attenuation", &pointLight.attenuation, 0.001f, 0.0f, 1.0f));
		});

	// Behaviour Tree -----------------------------------------------------------------------------------------------------------------
	DrawComponent<BehaviourTreeComponent>(ICON_FA_DIAGRAM_PROJECT" Behaviour Tree", entity, [](auto& behaviourTree)
		{
			//ImGui::FileSelect("Behaviour Tree", ai.)
		});

	// State Machine ------------------------------------------------------------------------------------------------------------------
	DrawComponent<StateMachineComponent>(ICON_FA_DIAGRAM_PROJECT" State Machine", entity, [](auto& stateMachine)
		{
			//ImGui::FileSelect("State Machine", );
		});

	DrawComponent<BillboardComponent>(ICON_FA_SIGN_HANGING" Billboard", entity, [](auto& billboard)
		{
			//combo
			Dirty(ImGui::Combo("Orientation", (int*)&billboard.orientation,
				"World Up\0"
				"Camera\0"));
			Dirty(ImGui::Combo("Position", (int*)&billboard.position,
				"World\0"
				"Camera\0"));
			if(billboard.position == BillboardComponent::Position::Camera)
				Dirty(ImGui::Vector("Screen Position", billboard.screenPosition, 0.0f));
		});

	// Lua Script ---------------------------------------------------------------------------------------------------------------------
	DrawComponent<LuaScriptComponent>(ICON_FA_FILE_CODE" Lua Script", entity, [](auto& luaScript)
		{
			if (ImGui::BeginCombo("##luaScript", luaScript.absoluteFilepath.filename().string().c_str()))
			{
				for (std::filesystem::path& file : Directory::GetFilesRecursive(Application::GetOpenDocumentDirectory(), ViewerManager::GetExtensions(FileType::SCRIPT)))
				{
					const bool is_selected = false;
					if (ImGui::Selectable(file.filename().string().c_str(), is_selected))
					{
						luaScript.absoluteFilepath = std::filesystem::absolute(Application::GetOpenDocumentDirectory() / file);
						SceneManager::CurrentScene()->MakeDirty();
						break;
					}
					ImGui::Tooltip(file.string().c_str());
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_PEN_TO_SQUARE"##LuaScript"))
			{
				ViewerManager::OpenViewer(luaScript.absoluteFilepath);
			}
			ImGui::Tooltip("Edit script");
		});
}

void PropertiesPanel::DrawAddComponent(Entity entity)
{
	ImGui::Dummy({ 0,0 });
	float width = ImGui::GetContentRegionAvail().x;
	ImGui::SameLine((width / 2.0f) - (ImGui::CalcTextSize("Add Component").x / 2.0f));
	if (ImGui::Button("Add Component"))
	{
		ImGui::OpenPopup("Components");
	}

	if (ImGui::BeginPopup("Components"))
	{
		AddComponentMenuItem<SpriteComponent>(ICON_FA_IMAGE" Sprite", entity);
		AddComponentMenuItem<AnimatedSpriteComponent>(ICON_FA_IMAGE" Animated Sprite", entity);
		AddComponentMenuItem<CircleRendererComponent>(ICON_FA_CIRCLE" Circle Renderer", entity);
		AddComponentMenuItem<TilemapComponent>(ICON_FA_BORDER_ALL" Tilemap", entity);
		AddComponentMenuItem<StaticMeshComponent>(ICON_FA_SHAPES" Static Mesh", entity);
		AddComponentMenuItem<CameraComponent>(ICON_FA_VIDEO" Camera", entity);
		AddComponentMenuItem<PrimitiveComponent>(ICON_FA_SHAPES" Primitive", entity);
		AddComponentMenuItem<TextComponent>(ICON_FA_FONT" Text", entity);
		AddComponentMenuItem<RigidBody2DComponent>(ICON_FA_BASEBALL" Rigid Body 2D", entity);
		AddComponentMenuItem<BoxCollider2DComponent>(ICON_FA_VECTOR_SQUARE" Box Collider 2D", entity);
		AddComponentMenuItem<CircleCollider2DComponent>(ICON_MDI_CIRCLE_OUTLINE" Circle Collider 2D", entity);
		AddComponentMenuItem<PolygonCollider2DComponent>(ICON_FA_DRAW_POLYGON" Polygon Collider 2D", entity);
		AddComponentMenuItem<CapsuleCollider2DComponent>(ICON_FA_CAPSULES" Capsule Collider 2D", entity);
		AddComponentMenuItem<BehaviourTreeComponent>(ICON_FA_DIAGRAM_PROJECT" Behaviour Tree", entity);
		AddComponentMenuItem<StateMachineComponent>(ICON_FA_DIAGRAM_PROJECT" State Machine", entity);
		AddComponentMenuItem<BillboardComponent>(ICON_FA_SIGN_HANGING" Billboard", entity);
		AddComponentMenuItem<PointLightComponent>(ICON_FA_LIGHTBULB" Point Light", entity);

		std::vector<std::filesystem::path> scripts = Directory::GetFilesRecursive(Application::GetOpenDocumentDirectory(), ViewerManager::GetExtensions(FileType::SCRIPT));

		if (ImGui::BeginMenu(ICON_FA_FILE_CODE" Lua Script", !entity.HasComponent<LuaScriptComponent>() && scripts.size() > 0))
		{
			for (std::filesystem::path& file : scripts)
			{
				if (ImGui::MenuItem(file.filename().string().c_str()))
				{
					entity.AddComponent<LuaScriptComponent>(file);
					break;
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}
}