#include "EditorApp.h"

#include "MainDockSpace.h"
#include "ProjectsStartScreen.h"
#include "Renderer/RenderCommand.h"

Editor::Editor(const WindowProps& props)
	:Application(props)
{
	RenderCommand::SetClearColour(Colours::GREY);

	if (Application::GetOpenDocument().empty())
	{
		PushOverlay(new ProjectsStartScreen());
		CLIENT_INFO("No project has been opened");
	}

	PushOverlay(new MainDockSpace());

	Application::GetWindow().SetIcon(GetWorkingDirectory() / "resources" / "Icons" / "Logo.png");
}

void Editor::OnUpdate()
{
	RenderCommand::Clear();
}
