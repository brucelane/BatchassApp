#include "BatchassApp.h"

void BatchassApp::prepareSettings(Settings* settings){
	settings->setFrameRate(12.0f);
	// parameters
	mParameterBag = ParameterBag::create();
	// utils
	mBatchass = Batchass::create(mParameterBag);
	// if AutoLayout, try to position the window on the 2nd screen
	if (mParameterBag->mAutoLayout)
	{
		mBatchass->getWindowsResolution();
	}

	settings->setWindowSize(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight);
	settings->setWindowPos(Vec2i(mParameterBag->mRenderX, mParameterBag->mRenderY));
}


void BatchassApp::setup()
{
	getWindow()->setTitle("Batchass");
	ci::app::App::get()->getSignalShutdown().connect([&]() {
		BatchassApp::shutdown();
	});
	// instanciate the OSC class
	mOSC = OSC::create(mParameterBag);
	// instanciate the json wrapper class
	mJson = JSONWrapper::create();

	// instanciate the WebSockets class
	mWebSockets = WebSockets::create(mParameterBag);
	mBatchass->setup();

	newLogMsg = false;

	// set ui window and io events callbacks
	ImGui::setWindow(getWindow());

	// midi
	setupMidi();
	mSeconds = 0;
}
void BatchassApp::setupMidi()
{
	stringstream ss;
	newLogMsg = true;
	ss << "setupMidi: ";

	if (mMidiIn0.getNumPorts() > 0)
	{
		mMidiIn0.listPorts();
		for (int i = 0; i < mMidiIn0.getNumPorts(); i++)
		{

			midiInput mIn;
			mIn.portName = mMidiIn0.mPortNames[i];
			mMidiInputs.push_back(mIn);
			if (mParameterBag->mMIDIOpenAllInputPorts)
			{
				if (i == 0)
				{
					mMidiIn0.openPort(i);
					mMidiIn0.midiSignal.connect(boost::bind(&BatchassApp::midiListener, this, boost::arg<1>::arg()));
				}
				if (i == 1)
				{
					mMidiIn1.openPort(i);
					mMidiIn1.midiSignal.connect(boost::bind(&BatchassApp::midiListener, this, boost::arg<1>::arg()));
				}
				if (i == 2)				{
					mMidiIn2.openPort(i);
					mMidiIn2.midiSignal.connect(boost::bind(&BatchassApp::midiListener, this, boost::arg<1>::arg()));
				}
				mMidiInputs[i].isConnected = true;
				ss << "Opening MIDI port " << i << " " << mMidiInputs[i].portName;
				mWebSockets->write(ss.str());
			}
			else
			{
				mMidiInputs[i].isConnected = false;
				ss << "Available MIDI port " << i << " " << mMidiIn0.mPortNames[i];
			}
		}
	}
	else
	{
		ss << "No MIDI Ports found!!!!" << std::endl;
	}
	ss << std::endl;
	mLogMsg = ss.str();
}
void BatchassApp::midiListener(midi::Message msg){
	float normalizedValue;
	stringstream ss;
	ss << "midi port: " << msg.port << " ch: " << msg.channel << " status: " << msg.status;
	string controlType = "unknown";

	newLogMsg = true;
	int name;
	int newValue;

	switch (msg.status)
	{
	case MIDI_CONTROL_CHANGE:
		controlType = "/cc";
		name = msg.control;
		newValue = msg.value;
		break;
	case MIDI_NOTE_ON:
		controlType = "/on";
		name = msg.pitch;
		newValue = msg.velocity;
		break;
	case MIDI_NOTE_OFF:
		controlType = "/off";
		name = msg.pitch;
		newValue = msg.velocity;
		break;
	default:
		break;
	}
	normalizedValue = lmap<float>(newValue, 0.0, 127.0, 0.0, 1.0);
	ss << " control: " << name << " value: " << newValue << " normalized: " << normalizedValue << std::endl;
	mLogMsg = ss.str();
	mOSC->updateAndSendOSCFloatMessage(controlType, name, normalizedValue, msg.channel);
	mOSC->sendOSCFloatMessage(controlType, name, normalizedValue, msg.channel);
	mWebSockets->write("{\"params\" :[{" + controlType);
}
/*void BatchassApp::shutdown()
{
	// save warp settings
	fs::path settings = getAssetPath("") / "warps.xml";
	Warp::writeSettings( mWarps, writeFile( settings ) );
}*/

void BatchassApp::update()
{
	mWebSockets->update();
	mOSC->update();
	getWindow()->setTitle("(" + toString(floor(getAverageFps())) + " fps) Batchass");
	/*if (mSeconds != (int)getElapsedSeconds())
	{
	mSeconds = (int)getElapsedSeconds();
	stringstream s;
	s << mSeconds;
	mWebSockets->write(s.str());
	}*/
}

void BatchassApp::draw()
{
	gl::clear(ColorAf(0.0f, 0.0f, 0.0f, 0.0f));

	gl::setViewport(getWindowBounds());
	gl::setMatricesWindow(getWindowSize());

	//imgui
	static float f = 0.0f;

	static bool showTest = false, showTheme = false, showAudio = true, showShaders = true, showOSC = true, showFps = true, showWS = true;
	ImGui::NewFrame();

	// start a new window
	ImGui::Begin("MIDI2OSC", NULL, ImVec2(500, 500));
	{
		// our theme variables
		static float WindowPadding[2] = { 25, 10 };
		static float WindowMinSize[2] = { 160, 80 };
		static float FramePadding[2] = { 4, 4 };
		static float ItemSpacing[2] = { 10, 5 };
		static float ItemInnerSpacing[2] = { 5, 5 };

		static float WindowFillAlphaDefault = 0.7;
		static float WindowRounding = 4;
		static float TreeNodeSpacing = 22;
		static float ColumnsMinSpacing = 50;
		static float ScrollBarWidth = 12;

		if (ImGui::CollapsingHeader("MidiIn", "0", true, true))
		{
			ImGui::Columns(2, "data", true);
			ImGui::Text("Name"); ImGui::NextColumn();
			ImGui::Text("Connect"); ImGui::NextColumn();
			ImGui::Separator();

			for (int i = 0; i < mMidiInputs.size(); i++)
			{
				ImGui::Text(mMidiInputs[i].portName.c_str()); ImGui::NextColumn();
				char buf[32];
				if (mMidiInputs[i].isConnected)
				{
					sprintf_s(buf, "Disconnect %d", i);
				}
				else
				{
					sprintf_s(buf, "Connect %d", i);
				}

				if (ImGui::Button(buf))
				{
					stringstream ss;
					if (mMidiInputs[i].isConnected)
					{
						if (i == 0)
						{
							mMidiIn0.closePort();
						}
						if (i == 1)
						{
							mMidiIn1.closePort();
						}
						if (i == 2)
						{
							mMidiIn2.closePort();
						}
						mMidiInputs[i].isConnected = false;
					}
					else
					{
						if (i == 0)
						{
							mMidiIn0.openPort(i);
							mMidiIn0.midiSignal.connect(boost::bind(&BatchassApp::midiListener, this, boost::arg<1>::arg()));
						}
						if (i == 1)
						{
							mMidiIn1.openPort(i);
							mMidiIn1.midiSignal.connect(boost::bind(&BatchassApp::midiListener, this, boost::arg<1>::arg()));
						}
						if (i == 2)
						{
							mMidiIn2.openPort(i);
							mMidiIn2.midiSignal.connect(boost::bind(&BatchassApp::midiListener, this, boost::arg<1>::arg()));
						}
						mMidiInputs[i].isConnected = true;
						ss << "Opening MIDI port " << i << " " << mMidiInputs[i].portName << std::endl;
					}
					mLogMsg = ss.str();
				}
				ImGui::NextColumn();
				ImGui::Separator();
			}
			ImGui::Columns(1);

		}

		if (ImGui::CollapsingHeader("Parameters", "1", true, true))
		{
			// Checkbox
			ImGui::Checkbox("Audio", &showAudio);
			ImGui::SameLine();
			ImGui::Checkbox("Shada", &showShaders);
			ImGui::SameLine();
			ImGui::Checkbox("Test", &showTest);
			ImGui::SameLine();
			ImGui::Checkbox("FPS", &showFps);
			ImGui::SameLine();
			ImGui::Checkbox("OSC", &showOSC);
			ImGui::SameLine();
			ImGui::Checkbox("WebSockets", &showWS);
			ImGui::SameLine();
			ImGui::Checkbox("Editor", &showTheme);
			if (ImGui::Button("Save")) { mParameterBag->save(); }

		}
		if (ImGui::CollapsingHeader("Log", "2", true, true))
		{
			static ImGuiTextBuffer log;
			static int lines = 0;
			ImGui::Text("Buffer contents: %d lines, %d bytes", lines, log.size());
			if (ImGui::Button("Clear")) { log.clear(); lines = 0; }
			//ImGui::SameLine();

			if (newLogMsg)
			{
				newLogMsg = false;
				log.append(mLogMsg.c_str());
				lines++;
				if (lines > 5) { log.clear(); lines = 0; }
			}
			ImGui::BeginChild("Log");
			ImGui::TextUnformatted(log.begin(), log.end());
			ImGui::EndChild();
		}
	}
	ImGui::End();

	if (showTest) ImGui::ShowTestWindow();

	if (showOSC)
	{
		ImGui::Begin("OSC router", NULL, ImVec2(300, 300));
		{
			ImGui::Text("Sending to host %s", mParameterBag->mOSCDestinationHost.c_str());
			ImGui::SameLine();
			ImGui::Text(" on port %d", mParameterBag->mOSCDestinationPort);
			ImGui::Text("Sending to 2nd host %s", mParameterBag->mOSCDestinationHost2.c_str());
			ImGui::SameLine();
			ImGui::Text(" on port %d", mParameterBag->mOSCDestinationPort2);
			ImGui::Text(" Receiving on port %d", mParameterBag->mOSCReceiverPort);

			static char str0[128] = "/live/play";
			static int i0 = 0;
			static float f0 = 0.0f;
			ImGui::InputText("address", str0, IM_ARRAYSIZE(str0));
			ImGui::InputInt("track", &i0);
			ImGui::InputFloat("clip", &f0, 0.01f, 1.0f);
			if (ImGui::Button("Send")) { mOSC->sendOSCIntMessage(str0, i0); }

			static ImGuiTextBuffer OSClog;
			static int lines = 0;
			if (ImGui::Button("Clear")) { OSClog.clear(); lines = 0; }
			ImGui::SameLine();
			ImGui::Text("Buffer contents: %d lines, %d bytes", lines, OSClog.size());

			if (mParameterBag->newOSCMsg)
			{
				mParameterBag->newOSCMsg = false;
				OSClog.append(mParameterBag->OSCMsg.c_str());
				lines++;
				if (lines > 5) { OSClog.clear(); lines = 0; }
			}
			ImGui::BeginChild("OSClog");
			ImGui::TextUnformatted(OSClog.begin(), OSClog.end());
			ImGui::EndChild();
		}
		ImGui::End();
	}

	if (showWS)
	{
		ImGui::Begin("WebSockets", NULL, ImVec2(300, 300));
		{
			if (mParameterBag->mIsWebSocketsServer)
			{
				ImGui::Text("Server %s", mParameterBag->mWebSocketsHost.c_str());
				ImGui::SameLine();
			}
			else
			{
				ImGui::Text("Client %s", mParameterBag->mWebSocketsHost.c_str());
				ImGui::SameLine();
			}
			ImGui::Text(" on port %d", mParameterBag->mWebSocketsPort);
			if (ImGui::Button("Send"))
			{
				mSeconds = (int)getElapsedSeconds();
				stringstream s;
				s << mSeconds;
				mWebSockets->write(s.str());
			}
			static ImGuiTextBuffer WSlog;
			static int lines = 0;
			if (ImGui::Button("Clear")) { WSlog.clear(); lines = 0; }
			ImGui::SameLine();
			ImGui::Text("Buffer contents: %d lines, %d bytes", lines, WSlog.size());

			if (mParameterBag->newWSMsg)
			{
				mParameterBag->newWSMsg = false;
				WSlog.append(mParameterBag->WSMsg.c_str());
				lines++;
				if (lines > 5) { WSlog.clear(); lines = 0; }
			}
			ImGui::BeginChild("WSlog");
			ImGui::TextUnformatted(WSlog.begin(), WSlog.end());
			ImGui::EndChild();
		}
		ImGui::End();
	}


	ImGui::Begin("UI", NULL, ImVec2(300, 300));
	{
		stringstream sParams;
		sParams << "{\"colors\" :[{\"name\" : 0,\"value\" : " << getElapsedFrames() << "}"; // TimeStamp
		// foreground color
		static float color[4] = { mParameterBag->controlValues[1], mParameterBag->controlValues[2], mParameterBag->controlValues[3], mParameterBag->controlValues[4] };
		ImGui::ColorEdit4("f", color);
		for (int i = 0; i < 4; i++)
		{
			if (mParameterBag->controlValues[i + 1] != color[i])
			{
				sParams << ",{\"name\" : " << i + 1 << ",\"value\" : " << color[i] << "}";
				mParameterBag->controlValues[i + 1] = color[i];
			}

		}
		//ImGui::SameLine();
		//ImGui::TextColored(ImVec4(mParameterBag->controlValues[1], mParameterBag->controlValues[2], mParameterBag->controlValues[3], mParameterBag->controlValues[4]), "fg color");

		// background color
		static float backcolor[4] = { mParameterBag->controlValues[5], mParameterBag->controlValues[6], mParameterBag->controlValues[7], mParameterBag->controlValues[8] };
		ImGui::ColorEdit4("g", backcolor);
		for (int i = 0; i < 4; i++)
		{
			if (mParameterBag->controlValues[i + 5] != backcolor[i])
			{
				sParams << ",{\"name\" : " << i + 5 << ",\"value\" : " << backcolor[i] << "}";
				mParameterBag->controlValues[i + 5] = backcolor[i];
			}

		}

		//ImGui::SameLine();
		//ImGui::TextColored(ImVec4(mParameterBag->controlValues[5], mParameterBag->controlValues[6], mParameterBag->controlValues[7], mParameterBag->controlValues[8]), "bg color");

		ImGui::BeginChild("Warps routing", ImVec2(0, 300), true);
		ImGui::Text("Selected warp: %d", mParameterBag->selectedWarp);
		ImGui::Columns(4);
		ImGui::Text("ID"); ImGui::NextColumn();
		ImGui::Text("texIndex"); ImGui::NextColumn();
		ImGui::Text("texMode"); ImGui::NextColumn();
		ImGui::Text("active"); ImGui::NextColumn();
		ImGui::Separator();
		for (int i = 0; i < mParameterBag->MAX; i++)
		{
			ImGui::Text("%d", i); ImGui::NextColumn();
			ImGui::Text("%d", mParameterBag->mWarpFbos[i].textureIndex); ImGui::NextColumn();
			ImGui::Text("%d", mParameterBag->mWarpFbos[i].textureMode); ImGui::NextColumn();
			ImGui::Text("%d", mParameterBag->mWarpFbos[i].active); ImGui::NextColumn();

		}
		ImGui::Columns(1);
		ImGui::EndChild();
		sParams << "]}";
		string strParams = sParams.str();
		if (strParams.length() > 60)
		{
			mWebSockets->write(strParams);
		}
		ImGui::End();
	}
	// audio window
	if (showAudio)
	{

		ImGui::Begin("Audio", NULL, ImVec2(200, 100));
		{
			ImGui::Checkbox("Playing", &mParameterBag->mIsPlaying);
			ImGui::SameLine();
			ImGui::Text("Beat %d", mParameterBag->mBeat);
			ImGui::SameLine();
			ImGui::Text("Tempo %.2f", mParameterBag->mTempo);

			static ImVector<float> values; if (values.empty()) { values.resize(40); memset(&values.front(), 0, values.size()*sizeof(float)); }
			static int values_offset = 0;
			// audio maxVolume
			static float refresh_time = -1.0f;
			if (ImGui::GetTime() > refresh_time + 1.0f / 20.0f)
			{
				refresh_time = ImGui::GetTime();
				values[values_offset] = mParameterBag->maxVolume;
				values_offset = (values_offset + 1) % values.size();
			}
			ImGui::PlotLines("Volume", &values.front(), (int)values.size(), values_offset, toString(mBatchass->formatFloat(mParameterBag->maxVolume)).c_str(), 0.0f, 1.0f, ImVec2(0, 30));

			/*for (int a = 0; a < MAX; a++)
			{
			if (mOSC->tracks[a] != "default.glsl") ImGui::Button(mOSC->tracks[a].c_str());
			}*/

		}
		ImGui::End();
	}
	// fps window
	if (showFps)
	{
		ImGui::Begin("Fps", NULL, ImVec2(100, 100));
		{
			static ImVector<float> values; if (values.empty()) { values.resize(100); memset(&values.front(), 0, values.size()*sizeof(float)); }
			static int values_offset = 0;
			static float refresh_time = -1.0f;
			if (ImGui::GetTime() > refresh_time + 1.0f / 6.0f)
			{
				refresh_time = ImGui::GetTime();
				values[values_offset] = getAverageFps();
				values_offset = (values_offset + 1) % values.size();
			}

			ImGui::PlotLines("FPS", &values.front(), (int)values.size(), values_offset, toString(floor(getAverageFps())).c_str(), 0.0f, 300.0f, ImVec2(0, 30));
		}
		ImGui::End();
	}

	ImGui::Render();
}

void BatchassApp::resize()
{

}

void BatchassApp::mouseMove( MouseEvent event )
{

}

void BatchassApp::mouseDown( MouseEvent event )
{

}

void BatchassApp::mouseDrag( MouseEvent event )
{

}

void BatchassApp::mouseUp( MouseEvent event )
{

}

void BatchassApp::keyDown( KeyEvent event )
{
	switch (event.getCode())
	{

	case ci::app::KeyEvent::KEY_ESCAPE:
		mParameterBag->save();
		//mBatchass->shutdownLoader(); // Not yet used (loading shaders in a different thread
		ImGui::Shutdown();
		mMidiIn0.closePort();
		mMidiIn1.closePort();
		mMidiIn2.closePort();
		quit();
		break;

	default:
		break;
	}
	//mWebSockets->write("yo");
}


CINDER_APP_BASIC( BatchassApp, RendererGl )
