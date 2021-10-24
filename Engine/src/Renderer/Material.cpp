#include "stdafx.h"
#include "Material.h"

#include "TinyXml2/tinyxml2.h"
#include "Utilities/SerializationUtils.h"
#include "Utilities/FileUtils.h"

Material::Material(Ref<Shader> shader, Colour tint)
	:m_Shader(shader), m_Tint(tint)
{
}

/* ------------------------------------------------------------------------------------------------------------------ */

void Material::BindTextures() const
{
	for (auto&& [slot, texture] : m_Textures)
	{
		texture->Bind(slot);
	}
}

const Ref<Texture> Material::GetTexture(uint32_t slot) const
{
	if (m_Textures.find(slot) == m_Textures.end())
		return nullptr;
	return m_Textures.at(slot);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void Material::AddTexture(Ref<Texture> texture, uint32_t slot)
{
	m_Textures[slot] = texture;
}

/* ------------------------------------------------------------------------------------------------------------------ */

void Material::LoadMaterial(const std::filesystem::path& filepath)
{
	if (!std::filesystem::exists(filepath))
		return;

	m_Filepath = filepath;

	tinyxml2::XMLDocument doc;

	if (doc.LoadFile(filepath.string().c_str()) == tinyxml2::XML_SUCCESS)
	{
		tinyxml2::XMLElement* pRoot = doc.FirstChildElement("Material");

		if (!pRoot)
			ENGINE_ERROR("Could not read material file, no material node {0}", filepath);

		SerializationUtils::Decode(pRoot->FirstChildElement("Tint"), m_Tint);

		tinyxml2::XMLElement* pTexturesElement = pRoot->FirstChildElement("Textures");

		if (pTexturesElement)
		{
			tinyxml2::XMLElement* pTextureElement = pTexturesElement->FirstChildElement("Texture");

			while (pTextureElement)
			{
				const char* texturePath = pTextureElement->Attribute("Filepath");

				Ref<Texture> texture;
				if (texturePath)
					texture = Texture2D::Create(texturePath);
				else
				{
					texture = Texture2D::Create("");
				}

				uint32_t slot = 1;

				pTextureElement->QueryUnsignedAttribute("Slot", &slot);

				AddTexture(texture, slot);
				pTextureElement = pTextureElement->NextSiblingElement("Texture");
			}
		}
	}
}

/* ------------------------------------------------------------------------------------------------------------------ */

void Material::LoadMaterial()
{
	LoadMaterial(m_Filepath);
}

/* ------------------------------------------------------------------------------------------------------------------ */

bool Material::SaveMaterial(const std::filesystem::path& filepath) const
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* pRoot = doc.NewElement("Material");

	doc.InsertFirstChild(pRoot);

	SerializationUtils::Encode(pRoot->InsertNewChildElement("Tint"), m_Tint);

	tinyxml2::XMLElement* pTexturesElement = pRoot->InsertNewChildElement("Textures");

	for (auto&& [slot, texture] : m_Textures)
	{
		pTexturesElement->InsertNewChildElement("Texture");

		pTexturesElement->SetAttribute("Slot", slot);

		std::filesystem::path textureFilepath = FileUtils::relativePath(texture->GetFilepath(), Application::GetOpenDocumentDirectory());

		pTexturesElement->SetAttribute("Filepath", textureFilepath.string().c_str());
	}

	tinyxml2::XMLError error = doc.SaveFile(filepath.string().c_str());
	return error == tinyxml2::XML_SUCCESS;
}

/* ------------------------------------------------------------------------------------------------------------------ */

bool Material::SaveMaterial() const
{
	return SaveMaterial(m_Filepath);
}
