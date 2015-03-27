/*
Copyright (c) 2014, Bruce Lane - Martin Blasko All rights reserved.
This code is intended for use with the Cinder C++ library: http://libcinder.org

This file is part of Cinder-MIDI.

Cinder-MIDI is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Cinder-MIDI is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cinder-MIDI.  If not, see <http://www.gnu.org/licenses/>.
*/

// don't forget to add winmm.lib to the linker

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "OSCSender.h"
#include "MidiIn.h"
#include "MidiMessage.h"
#include "MidiConstants.h"
#include <list>

// window manager
#include "WindowMngr.h"
// UserInterface
#include "CinderImGui.h"
// parameters
#include "ParameterBag.h"
// OSC
#include "OSCWrapper.h"
// json
#include "JSONWrapper.h"
// WebSockets
#include "WebSocketsWrapper.h"
// warps
#include "WarpWrapper.h"
// audio
#include "AudioWrapper.h"
// meshes
#include "Meshes.h"
// point sphere
#include "PointSphere.h"
// spout
#include "SpoutWrapper.h"
// UI
//#include "UI.h"
// Utils
#include "Batchass.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Reymenta;
//using namespace MinimalUI;

#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))
struct AppConsole
{
	char                  InputBuf[256];
	ImVector<char*>       Items;
	bool                  ScrollToBottom;
	ImVector<char*>       History;
	int                   HistoryPos;    // -1: new line, 0..History.size()-1 browsing history.
	ImVector<const char*> Commands;

	AppConsole()
	{
		ClearLog();
		HistoryPos = -1;
		Commands.push_back("HELP");
		Commands.push_back("HISTORY");
		Commands.push_back("CLEAR");
		Commands.push_back("CLASSIFY");  // "classify" is here to provide an example of "C"+[tab] completing to "CL" and displaying matches.
	}
	~AppConsole()
	{
		ClearLog();
		for (size_t i = 0; i < Items.size(); i++)
			ImGui::MemFree(History[i]);
	}

	void    ClearLog()
	{
		for (size_t i = 0; i < Items.size(); i++)
			ImGui::MemFree(Items[i]);
		Items.clear();
		ScrollToBottom = true;
	}

	void    AddLog(const char* fmt, ...)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		ui::ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);
		va_end(args);
		Items.push_back(ui::ImStrdup(buf));
		ScrollToBottom = true;
	}

	void    Run(const char* title, bool* opened)
	{
		ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiSetCond_FirstUseEver);
		if (!ImGui::Begin(title, opened))
		{
			ImGui::End();
			return;
		}

		ImGui::TextWrapped("This example implements a console with basic coloring, completion and history. A more elaborate implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
		ImGui::TextWrapped("Enter 'HELP' for help, press TAB to use text completion.");

		// TODO: display from bottom
		// TODO: clip manually

		if (ImGui::SmallButton("Add Dummy Text")) { AddLog("%d some text", Items.size()); AddLog("some more text"); AddLog("display very important message here!"); } ImGui::SameLine();
		if (ImGui::SmallButton("Add Dummy Error")) AddLog("[error] something went wrong"); ImGui::SameLine();
		if (ImGui::SmallButton("Clear")) ClearLog();
		//static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

		ImGui::Separator();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		static ImGuiTextFilter filter;
		filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
		//if (ImGui::IsItemHovered()) ImGui::SetKeyboardFocusHere(-1); // Auto focus on hover
		ImGui::PopStyleVar();
		ImGui::Separator();

		// Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
		// NB- if you have thousands of entries this approach may be too inefficient. You can seek and display only the lines that are visible - CalcListClipping() is a helper to compute this information.
		// If your items are of variable size you may want to implement code similar to what CalcListClipping() does. Or split your data into fixed height items to allow random-seeking into your list.
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() * 2));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		for (size_t i = 0; i < Items.size(); i++)
		{
			const char* item = Items[i];
			if (!filter.PassFilter(item))
				continue;
			ImVec4 col(1, 1, 1, 1); // A better implement may store a type per-item. For the sample let's just parse the text.
			if (strstr(item, "[error]")) col = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
			else if (strncmp(item, "# ", 2) == 0) col = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::TextUnformatted(item);
			ImGui::PopStyleColor();
		}
		if (ScrollToBottom)
			ImGui::SetScrollPosHere();
		ScrollToBottom = false;
		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::Separator();

		// Command-line
		if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
		{
			char* input_end = InputBuf + strlen(InputBuf);
			while (input_end > InputBuf && input_end[-1] == ' ') input_end--; *input_end = 0;
			if (InputBuf[0])
				ExecCommand(InputBuf);
			strcpy(InputBuf, "");
		}

		// Demonstrate keeping auto focus on the input box
		if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
			ImGui::SetKeyboardFocusHere(-1); // Auto focus

		ImGui::End();
	}

	void    ExecCommand(const char* command_line)
	{
		AddLog("# %s\n", command_line);

		// Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
		HistoryPos = -1;
		for (int i = (int)History.size() - 1; i >= 0; i--)
			if (ui::ImStricmp(History[i], command_line) == 0)
			{
				ImGui::MemFree(History[i]);
				History.erase(History.begin() + i);
				break;
			}
		History.push_back(ui::ImStrdup(command_line));

		// Process command
		if (ui::ImStricmp(command_line, "CLEAR") == 0)
		{
			ClearLog();
		}
		else if (ui::ImStricmp(command_line, "HELP") == 0)
		{
			AddLog("Commands:");
			for (size_t i = 0; i < Commands.size(); i++)
				AddLog("- %s", Commands[i]);
		}
		else if (ui::ImStricmp(command_line, "HISTORY") == 0)
		{
			for (size_t i = History.size() >= 10 ? History.size() - 10 : 0; i < History.size(); i++)
				AddLog("%3d: %s\n", i, History[i]);
		}
		else
		{
			AddLog("Unknown command: '%s'\n", command_line);
		}
	}

	static int TextEditCallbackStub(ImGuiTextEditCallbackData* data)
	{
		AppConsole* console = (AppConsole*)data->UserData;
		return console->TextEditCallback(data);
	}

	int     TextEditCallback(ImGuiTextEditCallbackData* data)
	{
		//AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
		switch (data->EventFlag)
		{
		case ImGuiInputTextFlags_CallbackCompletion:
		{
			// Example of TEXT COMPLETION

			// Locate beginning of current word
			const char* word_end = data->Buf + data->CursorPos;
			const char* word_start = word_end;
			while (word_start > data->Buf)
			{
				const char c = word_start[-1];
				if (ui::ImCharIsSpace(c) || c == ',' || c == ';')
					break;
				word_start--;
			}

			// Build a list of candidates
			ImVector<const char*> candidates;
			for (size_t i = 0; i < Commands.size(); i++)
				if (ui::ImStrnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
					candidates.push_back(Commands[i]);

			if (candidates.size() == 0)
			{
				// No match
				AddLog("No match for \"%.*s\"!\n", word_end - word_start, word_start);
			}
			else if (candidates.size() == 1)
			{
				// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0]);
				data->InsertChars(data->CursorPos, " ");
			}
			else
			{
				// Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display "CLEAR" and "CLASSIFY"
				int match_len = (int)(word_end - word_start);
				for (;;)
				{
					int c = 0;
					bool all_candidates_matches = true;
					for (size_t i = 0; i < candidates.size() && all_candidates_matches; i++)
						if (i == 0)
							c = toupper(candidates[i][match_len]);
						else if (c != toupper(candidates[i][match_len]))
							all_candidates_matches = false;
					if (!all_candidates_matches)
						break;
					match_len++;
				}

				if (match_len > 0)
				{
					data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
					data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
				}

				// List matches
				AddLog("Possible matches:\n");
				for (size_t i = 0; i < candidates.size(); i++)
					AddLog("- %s\n", candidates[i]);
			}

			break;
		}
		case ImGuiInputTextFlags_CallbackHistory:
		{
			// Example of HISTORY
			const int prev_history_pos = HistoryPos;
			if (data->EventKey == ImGuiKey_UpArrow)
			{
				if (HistoryPos == -1)
					HistoryPos = (int)(History.size() - 1);
				else if (HistoryPos > 0)
					HistoryPos--;
			}
			else if (data->EventKey == ImGuiKey_DownArrow)
			{
				if (HistoryPos != -1)
					if (++HistoryPos >= (int)History.size())
						HistoryPos = -1;
			}

			// A better implementation would preserve the data on the current input line along with cursor position.
			if (prev_history_pos != HistoryPos)
			{
				ui::ImFormatString(data->Buf, data->BufSize, "%s", (HistoryPos >= 0) ? History[HistoryPos] : "");
				data->BufDirty = true;
				data->CursorPos = data->SelectionStart = data->SelectionEnd = (int)strlen(data->Buf);
			}
		}
		}
		return 0;
	}
};
struct midiInput
{
	string			portName;
	bool			isConnected;
};
class BatchassApp : public AppBasic {
public:
	void prepareSettings(Settings* settings);
	void setup();
	void update();
	void keyDown(KeyEvent event);
	void keyUp(KeyEvent event);
	void fileDrop(FileDropEvent event);
	void shutdown();
	void resize();
	void mouseMove(MouseEvent event);
	void mouseDown(MouseEvent event);
	void mouseDrag(MouseEvent event);
	void mouseUp(MouseEvent event);
	void saveThumb();
	//! Override to receive window activate events
	void	activate();
	//! Override to receive window deactivate events
	void	deactivate();
private:
	// parameters
	ParameterBagRef				mParameterBag;
	// osc
	OSCRef						mOSC;
	// json
	JSONWrapperRef				mJson;
	// WebSockets
	WebSocketsRef				mWebSockets;
	// warps
	WarpWrapperRef				mWarpings;
	// audio
	AudioWrapperRef				mAudio;
	// mesh helper
	MeshesRef					mMeshes;
	// point sphere
	PointSphereRef				mSphere;
	// spout
	SpoutWrapperRef				mSpout;
	// utils
	BatchassRef					mBatchass;
	// minimalUI
	//UIRef						mUI;
	// timeline
	Anim<float>					mTimer;
	// midi
	vector<midiInput>			mMidiInputs;
	void						setupMidi();
	void						midiListener(midi::Message msg);
	// midi inputs: couldn't make a vector
	midi::Input					mMidiIn0;
	midi::Input					mMidiIn1;
	midi::Input					mMidiIn2;
	midi::Input					mMidiIn3;
	// log
	string						mLogMsg;
	bool						newLogMsg;
	// misc
	int							mSeconds;
	// windows
	WindowRef					mMainWindow;
	//void						windowManagement();
	void						drawMain();
	bool						mIsShutDown;
	// render
	void						createRenderWindow();
	void						deleteRenderWindows();
	vector<WindowMngr>			allRenderWindows;
	void						drawRender();
	void						createUIWindow();
	bool						removeUI;
	void						changeMode(int newMode);
	// modes, should be the same as in ParameterBag
	static const int			MODE_MIX = 1;
	static const int			MODE_AUDIO = 2;
	static const int			MODE_WARP = 3;
	static const int			MODE_SPHERE = 4;
	static const int			MODE_MESH = 5;
	static const int			MODE_KINECT = 6;
	// windows to create, should be the same as in ParameterBag
	static const int			NONE = 0;
	static const int			RENDER_1 = 1;
	static const int			RENDER_DELETE = 5;
	static const int			MIDI_IN = 6;

	float color[4];
	float backcolor[4];
	// imgui
	// mPreviewFboWidth 80 mPreviewFboHeight 60 margin 10 inBetween 15
	int w;
	int h;
	int xPos;
	int yPos;
	int largeW;
	int largeH;
	int							margin;
	int							inBetween;

	float f = 0.0f;
	char buf[32];

	bool showConsole, showGlobal, showSlidas, showWarps, showTextures, showTest, showRouting, showMidi, showFbos, showTheme, showAudio, showShaders, showOSC, showFps, showWS;

	// From imgui by Omar Cornut
	void ShowAppConsole(bool* opened);

};