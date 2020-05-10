#include "ImGuiContentExplorer.h"

#include "imgui/imgui.h"

#include "Renderer/Texture.h"

#include "Viewers/ViewerManager.h"

#include "Viewers/ImGuiScriptView.h"


std::filesystem::path ImGuiContentExplorer::GetPathForSplitPathIndex(int index)
{
	std::string path;
	for (int i = 0; i <= index; i++)
	{
		path += m_CurrentSplitPath[i];
		if (i != index)
			path += '\\';
	}

	return std::filesystem::path(path);
}

void ImGuiContentExplorer::CalculateBrowsingDataTableSizes(const ImVec2& childWindowSize)
{
	int approxNumEntriesPerColumn = 20;
	if (childWindowSize.y > 0)
	{
		int numLinesThatFit = (int)(childWindowSize.y / ImGui::GetTextLineHeightWithSpacing());
		if (numLinesThatFit <= 0)
			numLinesThatFit = 1;
		approxNumEntriesPerColumn = numLinesThatFit;
	}
	m_NumBrowsingColumns = std::ceil((float)m_TotalNumBrowsingEntries / (float)approxNumEntriesPerColumn);

	if (m_NumBrowsingColumns <= 0)
	{
		m_NumBrowsingColumns = 1;
		m_NumBrowsingEntriesPerColumn = approxNumEntriesPerColumn;
		return;
	}

	if (m_TotalNumBrowsingEntries % approxNumEntriesPerColumn > (approxNumEntriesPerColumn / 2))
		++m_NumBrowsingColumns;

	int maxNumBrowsingColumns = (childWindowSize.x > 0) ? (int)(childWindowSize.x / 100) : 6;

	if (maxNumBrowsingColumns < 1)
		maxNumBrowsingColumns = 1;

	if (m_NumBrowsingColumns > maxNumBrowsingColumns)
		m_NumBrowsingColumns = maxNumBrowsingColumns;

	m_NumBrowsingEntriesPerColumn = m_TotalNumBrowsingEntries / m_NumBrowsingColumns;

	if (m_TotalNumBrowsingEntries % m_NumBrowsingColumns != 0)
		++m_NumBrowsingEntriesPerColumn;
}


ImGuiContentExplorer::ImGuiContentExplorer(bool* show)
	:m_Show(show), Layer("ContentExplorer")
{
	m_TotalNumBrowsingEntries = 0;
	m_NumBrowsingColumns = 0;
	m_NumBrowsingEntriesPerColumn = 0;
}

void ImGuiContentExplorer::OnAttach()
{
	m_TextureLibrary.Load("resources/Icons/Folder.png");
	m_TextureLibrary.Load("resources/Icons/Arrow_Right.png");
	m_TextureLibrary.Load("resources/Icons/Arrow_Left.png");
	m_TextureLibrary.Load("resources/Icons/Arrow_Up.png");
}

void ImGuiContentExplorer::OnImGuiRender()
{
	if (!*m_Show)
	{
		return;
	}

	static char inputBuffer[1024] = "";

	if (m_ForceRescan)
	{
		m_ForceRescan = false;

		if (m_CurrentPath == "")
		{
			m_CurrentPath = std::filesystem::absolute(".");
		}

		//set the input buffer as the current path
		memset(inputBuffer, 0, sizeof(inputBuffer));
		for (int i = 0; i < m_CurrentPath.string().length(); i++)
		{
			inputBuffer[i] = m_CurrentPath.string()[i];
		}

		m_Dirs = Directory::GetDirectories(m_CurrentPath, m_SortingMode);
		m_Files = Directory::GetFiles(m_CurrentPath, m_SortingMode);
	}
	ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Content Explorer", m_Show))
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		ImVec4 dummyButtonColour(0.0f, 0.0f, 0.0f, 0.5f);

		//----------------------------------------------------------------------------------------------------
		// History ('<', '>', '^')
		bool historyBackClicked = false;
		bool historyForwardClicked = false;
		bool levelUpClicked = false;

		ImGui::PushID("historyDirectoriesID");
		{

			const bool historyCanGoBack = m_History.CanGoBack();
			const bool historyCanGoForward = m_History.CanGoForward();

			if (!historyCanGoBack)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, dummyButtonColour);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, dummyButtonColour);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, dummyButtonColour);
			}
			historyBackClicked = ImGui::ImageButton(m_TextureLibrary.Get("resources/Icons/Arrow_Left.png"), { 16,16 }, 0, ImGui::GetStyle().Colors[2]);

			if (!historyCanGoBack) {
				ImGui::PopStyleColor(3);
			}

			if (!historyCanGoForward)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, dummyButtonColour);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, dummyButtonColour);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, dummyButtonColour);
			}
			ImGui::SameLine();
			historyForwardClicked = ImGui::ImageButton(m_TextureLibrary.Get("resources/Icons/Arrow_Right.png"), { 16,16 }, 0, ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);

			if (!historyCanGoForward) {
				ImGui::PopStyleColor(3);
			}
		}
		ImGui::PopID();

		if (historyBackClicked || historyForwardClicked)
		{
			if (historyBackClicked)
			{
				if (m_History.GoBack())
					m_ForceRescan = true;
			}
			else if (historyForwardClicked)
			{
				if (m_History.GoForward())
					m_ForceRescan = true;
			}

			m_CurrentPath = *m_History.GetCurrentFolder();
		}
		//----------------------------------------------------------------------------------------------------
		// Manual Location text entry
		const std::filesystem::path* fi = m_History.GetCurrentFolder();

		// Edit Location CheckButton
		bool editlocationInputTextReturnPressed = false;
		{
			bool mustValidateInputPath = false;
			ImGui::PushStyleColor(ImGuiCol_Button, m_EditLocationCheckButtonPressed ? dummyButtonColour : style.Colors[ImGuiCol_Button]);
			ImGui::SameLine();
			if (ImGui::ImageButton(m_TextureLibrary.Get("resources/Icons/Folder.png"), { 16,16 }, 0, ImGui::GetStyle().Colors[ImGuiCol_WindowBg]))
			{
				m_EditLocationCheckButtonPressed = !m_EditLocationCheckButtonPressed;
				if (m_EditLocationCheckButtonPressed)
				{
					ImGui::SetKeyboardFocusHere();
				}
			}

			ImGui::PopStyleColor();


			if (m_EditLocationCheckButtonPressed)
			{
				ImGui::SameLine();
				editlocationInputTextReturnPressed = ImGui::InputText("##EditLocationInputText", inputBuffer, sizeof(inputBuffer), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);

				ImGui::SetKeyboardFocusHere();
				if (editlocationInputTextReturnPressed)
				{
					mustValidateInputPath = true;
				}
				else ImGui::Separator();
			}

			if (mustValidateInputPath)
			{
				try
				{
					if (std::filesystem::exists(inputBuffer))
					{
						m_CurrentPath = inputBuffer;

						if (std::filesystem::is_regular_file(inputBuffer))
						{
							m_CurrentPath.remove_filename();
							std::string path = m_CurrentPath.string();
							path.pop_back();
							m_CurrentPath = path;
						}
						m_CurrentSplitPath = SplitString(m_CurrentPath.string(), '\\');
						m_EditLocationCheckButtonPressed = false;
						m_ForceRescan = true;
						m_History.SwitchTo(m_CurrentPath);
					}
				}
				catch (const std::exception& e)
				{
					CLIENT_ERROR(e.what());
				}
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Split path control
		if (!m_EditLocationCheckButtonPressed && !editlocationInputTextReturnPressed)
		{
			bool mustSwitchSplitPath = false;

			//Split Path
			// tab:
			{
				const int numTabs = (int)m_CurrentSplitPath.size();

				int pushpop = 0;

				for (int t = 0; t < numTabs; t++)
				{
					if (t == numTabs - 1)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, dummyButtonColour);
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, dummyButtonColour);
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, dummyButtonColour);
					}

					ImGui::PushID(t);
					ImGui::SameLine();

					const bool pressed = ImGui::Button(m_CurrentSplitPath[t].c_str());
					if (t != numTabs - 1)
					{
						ImGui::SameLine();
						ImGui::Text(">");
					}
					ImGui::PopID();

					if (pressed)
					{
						if (t == numTabs - 1)
						{
							m_EditLocationCheckButtonPressed = true;
							ImGui::PopStyleColor(3);
							continue;
						}

						m_History.SwitchTo(GetPathForSplitPathIndex(t));
						m_ForceRescan = true;

						m_CurrentPath = *m_History.GetCurrentFolder();
					}
					if (t == numTabs - 1) {
						ImGui::PopStyleColor(3);
					}
				}

				m_CurrentSplitPath = SplitString(m_CurrentPath.string(), '\\');

				ImGui::Separator();
			}
		}

		//----------------------------------------------------------------------------------------------------
		// MAIN BROWSING WINDOW
		//----------------------------------------------------------------------------------------------------
		{
			if (ImGui::BeginChild("BrowsingFrame"))
			{
				m_TotalNumBrowsingEntries = m_Dirs.size() + m_Files.size();
				CalculateBrowsingDataTableSizes(ImGui::GetWindowSize());

				ImGui::Columns(m_NumBrowsingColumns);

				static int id;
				ImGui::PushID(&id);
				int cntEntries = 0;
				//Directories -------------------------------------------------------------------------------
				if (m_Dirs.size() > 0)
				{
					for (int i = 0; i < m_Dirs.size(); i++)
					{
						std::string dirName = SplitString(m_Dirs[i].string(), '\\').back();
						if (ImGui::SmallButton(dirName.c_str()))
						{
							//change dirctory on click
							m_History.SwitchTo(m_Dirs[i]);
							m_CurrentPath = *m_History.GetCurrentFolder();
							m_ForceRescan = true;
						}
						++cntEntries;
						//TODO:: switch on view zoom
						if (cntEntries == m_NumBrowsingEntriesPerColumn)
						{
							cntEntries = 0;
							ImGui::NextColumn();
						}
					}
				}

				//Files -----------------------------------------------------------------------------------
				if (m_Files.size() > 0)
				{
					for (int i = 0; i < m_Files.size(); i++)
					{
						if (ImGui::SmallButton(m_Files[i].filename().string().c_str()))
						{
							ViewerManager::OpenViewer(ViewerManager::GetAssetViewer(m_Files[i]));
							//open file on click

							static bool showTextureView = true;
							//Application::Get().AddLayer(new ImGuiTextureView(&showTextureView, m_Files[i]));
							Application::Get().AddOverlay(new ImGuiScriptView(&showTextureView, m_Files[i]));
						}
					}
				}

				ImGui::PopID();
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();
}

std::string ImGuiContentExplorer::FileDialog(const wchar_t* title, const wchar_t* filter)
{
#ifdef __WINDOWS__
	wchar_t filename[MAX_PATH];

	OPENFILENAME ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = L"Text Files\0*.txt\0Any File\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"Select a file path";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (!GetOpenFileName(&ofn))
	{
		switch (CommDlgExtendedError())
		{
		case CDERR_DIALOGFAILURE:		ENGINE_ERROR("CDERR_DIALOGFAILURE");   break;
		case CDERR_FINDRESFAILURE:		ENGINE_ERROR("CDERR_FINDRESFAILURE");  break;
		case CDERR_INITIALIZATION:		ENGINE_ERROR("CDERR_INITIALIZATION");  break;
		case CDERR_LOADRESFAILURE:		ENGINE_ERROR("CDERR_LOADRESFAILURE");  break;
		case CDERR_LOADSTRFAILURE:		ENGINE_ERROR("CDERR_LOADSTRFAILURE");  break;
		case CDERR_LOCKRESFAILURE:		ENGINE_ERROR("CDERR_LOCKRESFAILURE");  break;
		case CDERR_MEMALLOCFAILURE:		ENGINE_ERROR("CDERR_MEMALLOCFAILURE"); break;
		case CDERR_MEMLOCKFAILURE:		ENGINE_ERROR("CDERR_MEMLOCKFAILURE");  break;
		case CDERR_NOHINSTANCE:			ENGINE_ERROR("CDERR_NOHINSTANCE");     break;
		case CDERR_NOHOOK:				ENGINE_ERROR("CDERR_NOHOOK");          break;
		case CDERR_NOTEMPLATE:			ENGINE_ERROR("CDERR_NOTEMPLATE");      break;
		case CDERR_STRUCTSIZE:			ENGINE_ERROR("CDERR_STRUCTSIZE");      break;
		case FNERR_BUFFERTOOSMALL:		ENGINE_ERROR("FNERR_BUFFERTOOSMALL");  break;
		case FNERR_INVALIDFILENAME:		ENGINE_ERROR("FNERR_INVALIDFILENAME"); break;
		case FNERR_SUBCLASSFAILURE:		ENGINE_ERROR("FNERR_SUBCLASSFAILURE"); break;
		default: ENGINE_INFO("File open cancelled.");
		}
	}

	std::wstring ws(filename);
	return std::string(ws.begin(), ws.end());
#endif // __WINDOWS__

	return std::string();
}