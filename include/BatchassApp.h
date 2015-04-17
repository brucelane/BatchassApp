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
// audio
#include "AudioWrapper.h"
// meshes
#include "Meshes.h"
// point sphere
#include "PointSphere.h"
// Vertex sphere
#include "VertexSphere.h"
// spout
#include "SpoutWrapper.h"
// ABP
#include "Abp.h"
// UI
//#include "UI.h"
// Utils
#include "Batchass.h"
// Console
#include "AppConsole.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Reymenta;
//using namespace MinimalUI;

#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))

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
	void mouseWheel(MouseEvent event);
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
	// audio
	AudioWrapperRef				mAudio;
	// mesh helper
	MeshesRef					mMeshes;
	// point sphere
	PointSphereRef				mSphere;
	// spout
	SpoutWrapperRef				mSpout;
	//  Vertex Sphere
	VertexSphereRef				mVertexSphere;
	//  ABP
	ABPRef						mABP;
	// utils
	BatchassRef					mBatchass;
	// console
	AppConsoleRef				mConsole;
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
	// modes, should be the same as in ParameterBag
	static const int			MODE_MIX = 1;
	static const int			MODE_AUDIO = 2;
	static const int			MODE_WARP = 3;
	static const int			MODE_SPHERE = 4;
	static const int			MODE_MESH = 5;
	static const int			MODE_KINECT = 6;
	static const int			MODE_LIVE = 7;
	static const int			MODE_ABP = 8;
	static const int			MODE_VERTEXSPHERE = 9;
	// windows to create, should be the same as in ParameterBag
	static const int			NONE = 0;
	static const int			RENDER_1 = 1;
	static const int			RENDER_DELETE = 5;
	static const int			MIDI_IN = 6;

	float						color[4];
	float						backcolor[4];
	// imgui
	// mPreviewFboWidth 80 mPreviewFboHeight 60 margin 10 inBetween 15
	int							w;
	int							h;
	int							warpWidth;
	int							displayHeight;
	int							xPos;
	int							yPos;
	int							largeW;
	int							largeH;
	int							largePreviewW;
	int							largePreviewH;
	int							margin;
	int							inBetween;

	float						f = 0.0f;
	char						buf[64];

	bool showConsole, showGlobal, showTextures, showTest, showMidi, showFbos, showTheme, showAudio, showShaders, showOSC, showWS, showChannels;
	bool mouseGlobal;
	void ShowAppConsole(bool* opened);

};