#pragma once

#include "VertexArray.h"

#include "cereal/cereal.hpp"
#include "cereal/access.hpp"
#include "Utilities/FileUtils.h"
#include "Core/Application.h"
#include "Renderer/Asset.h"

class Mesh : public Asset
{
public:
	Mesh() = default;
	Mesh(const std::filesystem::path& filepath);
	Mesh(Ref<VertexArray> vertexArray, std::string name);
	~Mesh() = default;
	void LoadModel(Ref<VertexArray> vertexArray, std::string name);
	virtual bool Load(const std::filesystem::path& filepath) override;

	const std::filesystem::path& GetFilepath() const { return m_Filepath; }

	const Ref<VertexArray> GetVertexArray() const { return m_VertexArray; }
private:
	Ref<VertexArray> m_VertexArray;
	std::string m_Name;
	std::filesystem::path m_Filepath;
};