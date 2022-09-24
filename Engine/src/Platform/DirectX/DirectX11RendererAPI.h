#pragma once

#include "Renderer/RendererAPI.h"

#include <d3d11.h>

class DirectX11RendererAPI : public RendererAPI
{
public:
	virtual ~DirectX11RendererAPI();
	virtual bool Init() override;
	virtual void SetClearColour(const Colour& colour) override;
	virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
	virtual void Clear() override;

	virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount, bool backFaceCull = false, DrawMode drawMode = DrawMode::FILL) override;
	virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;
private:
	D3D11_VIEWPORT m_Viewport;

	Colour m_ClearColour;

	ID3D11RasterizerState* m_RSWireFrame;
	ID3D11RasterizerState* m_RSFill;
	ID3D11RasterizerState* m_RSPoints;
	ID3D11RasterizerState* m_RSCullNone;
};