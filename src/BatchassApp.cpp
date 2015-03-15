#include "BatchassApp.h"

void BatchassApp::prepareSettings(Settings* settings)
{
	// parameters
	mParameterBag = ParameterBag::create();
	// utils
	mBatchass = Batchass::create(mParameterBag);
	mBatchass->log("start");
	// if AutoLayout, try to position the window on the 2nd screen
	if (mParameterBag->mAutoLayout)
	{
		mBatchass->getWindowsResolution();
	}

	settings->setWindowSize(mParameterBag->mMainWindowWidth, mParameterBag->mMainWindowHeight);
	// Setting an unrealistically high frame rate effectively
	// disables frame rate limiting
	//settings->setFrameRate(10000.0f);
	settings->setFrameRate(60.0f);
	settings->setResizable(true);
	settings->setWindowPos(Vec2i(mParameterBag->mMainWindowX, mParameterBag->mMainWindowY));
}


void BatchassApp::setup()
{
	mBatchass->log("setup");
	mIsShutDown = false;
	removeUI = false;
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

	// setup sahders and textures
	mBatchass->setup();
	// setup the main window and associated draw function
	mMainWindow = getWindow();
	mMainWindow->setTitle("Reymenta ShadaMixa");
	mMainWindow->connectDraw(&BatchassApp::drawMain, this);
	mMainWindow->connectClose(&BatchassApp::shutdown, this);
	// instanciate the audio class
	mAudio = AudioWrapper::create(mParameterBag, mBatchass->getTexturesRef());
	// instanciate the warp wrapper class
	mWarpings = WarpWrapper::create(mParameterBag, mBatchass->getTexturesRef(), mBatchass->getShadersRef());
	// instanciate the Meshes class
	mMeshes = Meshes::create(mParameterBag, mBatchass->getTexturesRef());
	// instanciate the PointSphere class
	mSphere = PointSphere::create(mParameterBag, mBatchass->getTexturesRef(), mBatchass->getShadersRef());
	// instanciate the spout class
	mSpout = SpoutWrapper::create(mParameterBag, mBatchass->getTexturesRef());

	mTimer = 0.0f;
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
void BatchassApp::createRenderWindow()
{
	deleteRenderWindows();
	mBatchass->getWindowsResolution();

	mParameterBag->iResolution.x = mParameterBag->mRenderWidth;
	mParameterBag->iResolution.y = mParameterBag->mRenderHeight;
	mParameterBag->mRenderResolution = Vec2i(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight);

	mBatchass->log("createRenderWindow, resolution:" + toString(mParameterBag->iResolution.x) + "x" + toString(mParameterBag->iResolution.y));

	string windowName = "render";

	WindowRef	mRenderWindow;
	mRenderWindow = createWindow(Window::Format().size(mParameterBag->iResolution.x, mParameterBag->iResolution.y));

	// create instance of the window and store in vector
	WindowMngr rWin = WindowMngr(windowName, mParameterBag->mRenderWidth, mParameterBag->mRenderHeight, mRenderWindow);
	allRenderWindows.push_back(rWin);

	mRenderWindow->setBorderless();
	mParameterBag->mRenderResoXY = Vec2f(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight);
	mRenderWindow->connectDraw(&BatchassApp::drawRender, this);
	mParameterBag->mRenderPosXY = Vec2i(mParameterBag->mRenderX, mParameterBag->mRenderY);//20141214 was 0
	mRenderWindow->setPos(mParameterBag->mRenderX, mParameterBag->mRenderY);

}
void BatchassApp::deleteRenderWindows()
{
	for (auto wRef : allRenderWindows) DestroyWindow((HWND)wRef.mWRef->getNative());
	allRenderWindows.clear();
}
void BatchassApp::drawMain()
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

			ImGui::PlotLines("FPS", &values.front(), (int)values.size(), values_offset, mParameterBag->sFps.c_str(), 0.0f, 300.0f, ImVec2(0, 30));
		}
		ImGui::End();
	}

	ImGui::Render();

	mSpout->draw();
	// draw the fbos
	mBatchass->getTexturesRef()->draw();
	//if (mParameterBag->mUIRefresh < 1.0) mParameterBag->mUIRefresh = 1.0;
	//if (getElapsedFrames() % (int)(mParameterBag->mUIRefresh + 0.1) == 0)
	if (getElapsedFrames() % mParameterBag->mUIRefresh == 0)
	{
		//gl::clear();
		//gl::setViewport(getWindowBounds());
		//20140703 gl::setMatricesWindow(getWindowSize());
		gl::setMatricesWindow(mParameterBag->mFboWidth, mParameterBag->mFboHeight, true);// mParameterBag->mOriginUpperLeft);

		gl::color(ColorAf(1.0f, 1.0f, 1.0f, 1.0f));
		if (mParameterBag->mPreviewEnabled)
		{
			// select drawing mode 
			switch (mParameterBag->mMode)
			{
				/*case MODE_NORMAL:
				gl::setMatricesWindow(mParameterBag->mPreviewFboWidth, mParameterBag->mPreviewFboHeight, mParameterBag->mOriginUpperLeft);
				gl::draw(mTextures->getFboTexture(mParameterBag->mCurrentPreviewFboIndex));
				break;
				case MODE_MIX:
				gl::draw(mTextures->getFboTexture(mParameterBag->mMixFboIndex));
				break;*/
			case MODE_AUDIO:
				mAudio->draw();
				gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mAudioFboIndex));
				break;
			case MODE_WARP:
				mWarpings->draw();
				break;
			case MODE_SPHERE:
				mSphere->draw();
				gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mSphereFboIndex));
				break;
			case MODE_MESH:
				if (mMeshes->isSetup())
				{
					mMeshes->draw();
					gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mMeshFboIndex));
				}
				break;
			case MODE_KINECT:
				for (int i = 0; i < 20; i++)
				{
					gl::drawLine(Vec2f(mOSC->skeleton[i].x, mOSC->skeleton[i].y), Vec2f(mOSC->skeleton[i].z, mOSC->skeleton[i].w));
					gl::drawSolidCircle(Vec2f(mOSC->skeleton[i].x, mOSC->skeleton[i].y), 5.0f, 16);
				}
				break;
			default:
				gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mMixFboIndex));
				break;
			}
		}
	}
	else
	{
		if (mParameterBag->mMode == MODE_MESH){ if (mMeshes->isSetup()) mMeshes->draw(); }
	}
	//if (!removeUI) mUI->draw();
	//mUserInterface->draw();

	gl::disableAlphaBlending();
}
void BatchassApp::drawRender()
{
	// clear
	gl::clear();
	// shaders			
	gl::setViewport(getWindowBounds());
	gl::enableAlphaBlending();
	//20140703 gl::setMatricesWindow(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight, mParameterBag->mOriginUpperLeft);//NEW 20140620, needed?
	gl::setMatricesWindow(mParameterBag->mFboWidth, mParameterBag->mFboHeight, true);// mParameterBag->mOriginUpperLeft);
	switch (mParameterBag->mMode)
	{
	case MODE_AUDIO:
		gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mAudioFboIndex));
		break;
	case MODE_WARP:
		mWarpings->draw();
		break;
	case MODE_SPHERE:
		gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mSphereFboIndex));
		break;
	case MODE_MESH:
		gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mMeshFboIndex));
		break;
	default:
		gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mMixFboIndex));
		break;
	}

	gl::disableAlphaBlending();
}
void BatchassApp::save()
{
	string filename = mBatchass->getShadersRef()->getMiddleFragFileName() + ".png";
	//string fpsFilename = ci::toString((int)getAverageFps()) + "-fps-" + mShaders->getMiddleFragFileName() + ".json";
	try
	{
		/*if (mParameterBag->iDebug)
		{
		JsonTree node = JsonTree();
		node.pushBack(JsonTree("RenderResolution width", mParameterBag->mRenderResolution.x));
		node.pushBack(JsonTree("RenderResolution height", mParameterBag->mRenderResolution.y));
		fs::path localFile = getAssetPath("") / "fps" / fpsFilename;
		node.write(localFile);
		log->logTimedString("saved:" + fpsFilename);
		Area area = Area(0, mParameterBag->mMainWindowHeight, mParameterBag->mPreviewWidth, mParameterBag->mMainWindowHeight - mParameterBag->mPreviewHeight);
		writeImage(getAssetPath("") / "thumbs" / filename, copyWindowSurface(area));*/
		writeImage(getAssetPath("") / "thumbs" / filename, mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mCurrentPreviewFboIndex));
		mBatchass->log("saved:" + filename);
		//}
	}
	catch (const std::exception &e)
	{
		mBatchass->log("unable to save:" + filename + string(e.what()));
	}
}
void BatchassApp::keyUp(KeyEvent event)
{
	if (mParameterBag->mMode == MODE_WARP) mWarpings->keyUp(event);
}

void BatchassApp::fileDrop(FileDropEvent event)
{
	bool loaded = false;
	string ext = "";
	// use the last of the dropped files
	boost::filesystem::path mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	if (mFile.find_last_of(".") != std::string::npos) ext = mFile.substr(mFile.find_last_of(".") + 1);
	//mParameterBag->currentSelectedIndex = (int)(event.getX() / 80);//76+margin mParameterBag->mPreviewWidth);
	mBatchass->log(mFile + " dropped, currentSelectedIndex:" + toString(mParameterBag->currentSelectedIndex) + " x: " + toString(event.getX()) + " mPreviewWidth: " + toString(mParameterBag->mPreviewWidth));

	if (ext == "wav" || ext == "mp3")
	{
		//do not try to load by other ways
		loaded = true;
		mAudio->loadWaveFile(mFile);

	}
	if (ext == "png" || ext == "jpg")
	{
		//do not try to load by other ways
		loaded = true;
		//mTextures->loadImageFile(mParameterBag->currentSelectedIndex, mFile);
		mBatchass->getTexturesRef()->loadImageFile(1, mFile);
	}
	/*if (!loaded && ext == "frag")
	{
	//do not try to load by other ways
	loaded = true;
	//mShaders->incrementPreviewIndex();

	if (mShaders->loadPixelFrag(mFile))
	{
	mParameterBag->controlValues[13] = 1.0f;
	timeline().apply(&mTimer, 1.0f, 1.0f).finishFn([&]{ save(); });
	}
	if (mCodeEditor) mCodeEditor->fileDrop(event);
	}*/
	if (!loaded && ext == "glsl")
	{
		//do not try to load by other ways
		loaded = true;
		//mShaders->incrementPreviewIndex();
		//mUserInterface->mLibraryPanel->addShader(mFile);
		if (mBatchass->getShadersRef()->loadPixelFragmentShader(mFile))
		{
			mParameterBag->controlValues[13] = 1.0f;
			// send content via OSC
			fs::path fr = mFile;
			string name = "unknown";
			if (mFile.find_last_of("\\") != std::string::npos) name = mFile.substr(mFile.find_last_of("\\") + 1);
			if (fs::exists(fr))
			{
				std::string fs = loadString(loadFile(mFile));
				mOSC->sendOSCStringMessage("/fs", 0, fs, name);
			}
			// save thumb
			timeline().apply(&mTimer, 1.0f, 1.0f).finishFn([&]{ save(); });
		}
	}
	if (!loaded && ext == "fs")
	{
		//do not try to load by other ways
		loaded = true;
		//mShaders->incrementPreviewIndex();
		mBatchass->getShadersRef()->loadFragmentShader(mPath);
		timeline().apply(&mTimer, 1.0f, 1.0f).finishFn([&]{ save(); });
	}
	if (!loaded && ext == "patchjson")
	{
		// try loading patch
		//do not try to load by other ways
		loaded = true;
		try
		{
			JsonTree patchjson;
			try
			{
				patchjson = JsonTree(loadFile(mFile));
				mParameterBag->mCurrentFilePath = mFile;
			}
			catch (cinder::JsonTree::Exception exception)
			{
				mBatchass->log("patchjsonparser exception " + mFile + ": " + exception.what());

			}
			//Assets
			int i = 1; // 0 is audio
			JsonTree jsons = patchjson.getChild("assets");
			for (JsonTree::ConstIter jsonElement = jsons.begin(); jsonElement != jsons.end(); ++jsonElement)
			{
				string jsonFileName = jsonElement->getChild("filename").getValue<string>();
				int channel = jsonElement->getChild("channel").getValue<int>();
				if (channel < mBatchass->getTexturesRef()->getTextureCount())
				{
					mBatchass->log("asset filename: " + jsonFileName);
					mBatchass->getTexturesRef()->setTexture(channel, jsonFileName);
				}
				i++;
			}

		}
		catch (...)
		{
			mBatchass->log("patchjson parsing error: " + mFile);
		}
	}
	if (!loaded && ext == "txt")
	{
		//do not try to load by other ways
		loaded = true;
		// try loading shader parts
		if (mBatchass->getShadersRef()->loadTextFile(mFile))
		{

		}
	}

	if (!loaded && ext == "")
	{
		//do not try to load by other ways
		loaded = true;
		// try loading image sequence from dir
		//mTextures->createFromDir(mFile + "/");

	}
	mParameterBag->isUIDirty = true;
}

void BatchassApp::shutdown()
{
	if (!mIsShutDown)
	{
		mIsShutDown = true;
		mBatchass->log("shutdown");
		deleteRenderWindows();
		// save warp settings
		mWarpings->save();
		// save params
		mParameterBag->save();
		//mUI->shutdown();
		if (mMeshes->isSetup()) mMeshes->shutdown();
		// not implemented mShaders->shutdownLoader();
		// close spout
		mSpout->shutdown();
		quit();
	}
}

void BatchassApp::update()
{
	mWebSockets->update();
	mOSC->update();
	mParameterBag->iFps = getAverageFps();
	mParameterBag->sFps = toString(floor(getAverageFps()));
	getWindow()->setTitle("(" + mParameterBag->sFps + " fps) Batchass");
	if (mParameterBag->iGreyScale)
	{
		mParameterBag->controlValues[1] = mParameterBag->controlValues[2] = mParameterBag->controlValues[3];
		mParameterBag->controlValues[5] = mParameterBag->controlValues[6] = mParameterBag->controlValues[7];
	}

	mParameterBag->iChannelTime[0] = getElapsedSeconds();
	mParameterBag->iChannelTime[1] = getElapsedSeconds() - 1;
	mParameterBag->iChannelTime[3] = getElapsedSeconds() - 2;
	mParameterBag->iChannelTime[4] = getElapsedSeconds() - 3;
	//
	if (mParameterBag->mUseTimeWithTempo)
	{
		mParameterBag->iGlobalTime = mParameterBag->iTempoTime*mParameterBag->iTimeFactor;
	}
	else
	{
		mParameterBag->iGlobalTime = getElapsedSeconds();
	}

	switch (mParameterBag->mMode)
	{
	case MODE_SPHERE:
		mSphere->update();
		break;
	case MODE_MESH:
		if (mMeshes->isSetup()) mMeshes->update();
		break;
	default:
		break;
	}
	mSpout->update();
	mBatchass->update();
	mWebSockets->update();
	mOSC->update();
	mAudio->update();
	if (mParameterBag->mWindowToCreate > 0)
	{
		// try to create the window only once
		int windowToCreate = mParameterBag->mWindowToCreate;
		mParameterBag->mWindowToCreate = NONE;
		switch (windowToCreate)
		{
		case RENDER_1:
			createRenderWindow();
			break;
		case RENDER_DELETE:
			deleteRenderWindows();
			break;
			/*case MIDI_IN:
				setupMidi();
				break;*/
		}
	}
	/*if (mSeconds != (int)getElapsedSeconds())
	{
	mSeconds = (int)getElapsedSeconds();
	stringstream s;
	s << mSeconds;
	mWebSockets->write(s.str());
	}*/
}

void BatchassApp::resize()
{
	mWarpings->resize();
}

void BatchassApp::mouseMove(MouseEvent event)
{
	if (mParameterBag->mMode == MODE_WARP) mWarpings->mouseMove(event);

}

void BatchassApp::mouseDown(MouseEvent event)
{
	if (mParameterBag->mMode == MODE_WARP) mWarpings->mouseDown(event);
	if (mParameterBag->mMode == MODE_MESH) mMeshes->mouseDown(event);

}

void BatchassApp::mouseDrag(MouseEvent event)
{
	if (mParameterBag->mMode == MODE_WARP) mWarpings->mouseDrag(event);
	if (mParameterBag->mMode == MODE_MESH) mMeshes->mouseDrag(event);

}

void BatchassApp::mouseUp(MouseEvent event)
{
	if (mParameterBag->mMode == MODE_WARP) mWarpings->mouseUp(event);

}

void BatchassApp::keyDown(KeyEvent event)
{
	if (mParameterBag->mMode == MODE_WARP)
	{
		mWarpings->keyDown(event);
	}
	else
	{

		switch (event.getCode())
		{
		case ci::app::KeyEvent::KEY_a:
			mParameterBag->mMode = MODE_AUDIO;
			break;
		case ci::app::KeyEvent::KEY_s:
			mParameterBag->mMode = MODE_SPHERE;
			break;
		case ci::app::KeyEvent::KEY_w:
			mParameterBag->mMode = MODE_WARP;
			break;
		case ci::app::KeyEvent::KEY_m:
			mParameterBag->mMode = MODE_MESH;
			break;
		case ci::app::KeyEvent::KEY_o:
			mParameterBag->mOriginUpperLeft = !mParameterBag->mOriginUpperLeft;
			break;
		case ci::app::KeyEvent::KEY_g:
			mParameterBag->iGreyScale = !mParameterBag->iGreyScale;
			break;
		case ci::app::KeyEvent::KEY_p:
			mParameterBag->mPreviewEnabled = !mParameterBag->mPreviewEnabled;
			break;
		case ci::app::KeyEvent::KEY_v:
			mParameterBag->controlValues[48] = !mParameterBag->controlValues[48];
			break;
		case ci::app::KeyEvent::KEY_f:
			if (allRenderWindows.size() > 0) allRenderWindows[0].mWRef->setFullScreen(!allRenderWindows[0].mWRef->isFullScreen());
			break;
		case ci::app::KeyEvent::KEY_x:
			removeUI = !removeUI;
			break;
		case ci::app::KeyEvent::KEY_c:
			if (mParameterBag->mCursorVisible)
			{
				hideCursor();
			}
			else
			{
				showCursor();
			}
			mParameterBag->mCursorVisible = !mParameterBag->mCursorVisible;
			break;
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
	}
	//mWebSockets->write("yo");
}


CINDER_APP_BASIC(BatchassApp, RendererGl)
