#include "stdafx.h"
#include "OpenGLVertexArray.h"

#include <GLAD/glad.h>

// Temporary function
static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
{
	switch (type)
	{
	case ShaderDataType::Float:		return GL_FLOAT;
	case ShaderDataType::Float2:	return GL_FLOAT;
	case ShaderDataType::Float3:	return GL_FLOAT;
	case ShaderDataType::Float4:	return GL_FLOAT;
	case ShaderDataType::Mat3:		return GL_FLOAT;
	case ShaderDataType::Mat4:		return GL_FLOAT;
	case ShaderDataType::Int:		return GL_INT;
	case ShaderDataType::Int2:		return GL_INT;
	case ShaderDataType::Int3:		return GL_INT;
	case ShaderDataType::Int4:		return GL_INT;
	case ShaderDataType::Bool:		return GL_BOOL;
	default:
		break;
	}

	CORE_ASSERT(false, "Shader Data type not recognised");
	return 0;
}

OpenGLVertexArray::OpenGLVertexArray()
{
	PROFILE_FUNCTION();
	glGenVertexArrays(1, &m_RendererID);
}

OpenGLVertexArray::~OpenGLVertexArray()
{
	PROFILE_FUNCTION();
	glDeleteVertexArrays(1, &m_RendererID);
}

void OpenGLVertexArray::Bind() const
{
	PROFILE_FUNCTION();
	glBindVertexArray(m_RendererID);
}

void OpenGLVertexArray::UnBind() const
{
	PROFILE_FUNCTION();
	glBindVertexArray(0);
}

void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
{
	PROFILE_FUNCTION();
	glBindVertexArray(m_RendererID);
	vertexBuffer->Bind();

	CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer Layout has no elements");

	uint32_t index = 0;
	const BufferLayout& layout = vertexBuffer->GetLayout();
	for (const BufferElement& element : layout)
	{
		glEnableVertexAttribArray(m_VertexBufferIndex);
		glVertexAttribPointer(m_VertexBufferIndex, element.Count(),
			ShaderDataTypeToOpenGLBaseType(element.Type),
			element.Normalized ? GL_TRUE : GL_FALSE,
			layout.GetStride(),
			(const void*)element.Offset);
		m_VertexBufferIndex++;
	}

	m_VertexBuffers.push_back(vertexBuffer);
}

void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
{
	PROFILE_FUNCTION();
	glBindVertexArray(m_RendererID);
	indexBuffer->Bind();

	m_IndexBuffer = indexBuffer;
}

const std::vector<Ref<VertexBuffer>>& OpenGLVertexArray::GetVertexBuffers() const
{
	return m_VertexBuffers;
}

const Ref<IndexBuffer>& OpenGLVertexArray::GetIndexBuffer() const
{
	return m_IndexBuffer;
}
