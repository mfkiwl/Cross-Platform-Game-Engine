#include "ExampleLayer2D.h"
#include "ExampleLayer3D.h"
#include "GridLayer.h"
#include "ExampleApp.h"

ExampleGame::ExampleGame(const WindowProps& props)
	:Application(props)
{
	RenderCommand::SetClearColour(Colours::GREY);

	//PushLayer(new GridLayer());
	PushLayer(new ExampleLayer3D());
	PushLayer(new ExampleLayer2D());
	PushOverlay(new ImGuiLayer());

	Application::GetWindow().SetIcon("resources/Circle.png");

	Application::GetWindow().SetWindowMode(WindowMode::BORDERLESS, 256, 144);
}

void ExampleGame::OnUpdate()
{
	RenderCommand::Clear();
}
