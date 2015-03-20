#include "BatchassApp.h"

void BatchassApp::prepareSettings(Settings* settings)
{
	// parameters
	mParameterBag = ParameterBag::create();
	// utils
	mBatchass = Batchass::create(mParameterBag);
	mBatchass->log("start");

	mBatchass->getWindowsResolution();

	settings->setWindowSize(mParameterBag->mMainWindowWidth, mParameterBag->mMainWindowHeight);
	// Setting an unrealistically high frame rate effectively
	// disables frame rate limiting
	//settings->setFrameRate(10000.0f);
	settings->setFrameRate(60.0f);
	settings->setResizable(true);
	settings->setWindowPos(Vec2i(mParameterBag->mMainWindowX, mParameterBag->mMainWindowY));

#ifdef _DEBUG

#else
	settings->setBorderless();
#endif  // _DEBUG
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

	// setup shaders and textures
	mBatchass->setup();
	// setup the main window and associated draw function
	mMainWindow = getWindow();
	mMainWindow->setTitle("Batchass");
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

	// Setup the MinimalUI user interface
	mUI = UI::create(mParameterBag, mBatchass->getShadersRef(), mBatchass->getTexturesRef(), mMainWindow);
	mUI->setup();
	// set ui window and io events callbacks
	//ui::setWindow(getWindow());
	// set ui window and io events callbacks
	//ui::connectWindow(getWindow());
	//ui::initialize();
	ui::initialize();

	// midi
	setupMidi();
	mSeconds = 0;
	// if AutoLayout, create render window on the 2nd screen
	if (mParameterBag->mAutoLayout)
	{
		createRenderWindow();
	}
	mBatchass->tapTempo(true);
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
	stringstream ss;
	ss << "ctrl: " << name << " value: " << newValue << " normalized: " << normalizedValue;
	ss << " midi port: " << msg.port << " ch: " << msg.channel << " status: " << msg.status << std::endl;
	mLogMsg = ss.str();
	mOSC->updateAndSendOSCFloatMessage(controlType, name, normalizedValue, msg.channel);
	// CHECK why? mOSC->sendOSCFloatMessage(controlType, name, normalizedValue, msg.channel);
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
	// must be first to avoid gl matrices to change
	// draw from Spout receivers
	mSpout->draw();
	// draw the fbos
	mBatchass->getTexturesRef()->draw();

	gl::setViewport(getWindowBounds());
	//gl::setViewport(mBatchass->getTexturesRef()->getFbo(0).getBounds());
	//Area mViewportArea = Area(0, -300, mParameterBag->mRenderWidth, mParameterBag->mRenderHeight);
	//gl::setViewport(mViewportArea);

	//gl::setMatricesWindow(mParameterBag->mMainDisplayWidth, mParameterBag->mMainDisplayHeight, true);// mParameterBag->mOriginUpperLeft);
	gl::setMatricesWindow(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight);// , false);

	//gl::setMatricesWindow(getWindowSize());
	gl::clear(ColorAf(0.0f, 0.0f, 0.0f, 0.0f));
	gl::color(ColorAf(1.0f, 1.0f, 1.0f, 1.0f));
	// draw preview
	if (getElapsedFrames() % mParameterBag->mUIRefresh == 0)
	{
		Rectf rect = Rectf(50, 40, 50 + mParameterBag->mPreviewFboWidth, 40 + mParameterBag->mPreviewFboHeight);
		if (mParameterBag->mPreviewEnabled)
		{
			// select drawing mode 
			switch (mParameterBag->mMode)
			{
			case MODE_AUDIO:
				mAudio->draw();
				gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mAudioFboIndex), rect);
				break;
			case MODE_WARP:
				mWarpings->draw();
				break;
			case MODE_SPHERE:
				mSphere->draw();
				gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mSphereFboIndex), rect);
				break;
			case MODE_MESH:
				if (mMeshes->isSetup())
				{
					mMeshes->draw();
					gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mMeshFboIndex), rect);
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
				//gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mMixFboIndex), rect);
				gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mMixFboIndex));
				break;
			}
		}
	}
	else
	{
		if (mParameterBag->mMode == MODE_MESH){ if (mMeshes->isSetup()) mMeshes->draw(); }
	}
	gl::setViewport(getWindowBounds());
	gl::setMatricesWindow(getWindowSize());
	gl::setMatricesWindow(mParameterBag->mFboWidth, mParameterBag->mFboHeight, true);
	if (!removeUI) mUI->draw();

	gl::setViewport(getWindowBounds());
	gl::setMatricesWindow(getWindowSize());

	// draw textures and fbos 		
	/*margin = 10;
	int texY = 10;
	int fboY = 10 + mParameterBag->mPreviewFboHeight;

	// previews
	//gl::setMatricesWindow(mParameterBag->mFboWidth, mParameterBag->mFboHeight, true);
	for (int i = 0; i < mBatchass->getTexturesRef()->getTextureCount(); i++)
	{
	gl::draw(mBatchass->getTexturesRef()->getTexture(i), Rectf(i* mParameterBag->mPreviewFboWidth, texY, (i + 1) * mParameterBag->mPreviewFboWidth + margin, texY + mParameterBag->mPreviewFboHeight));
	}
	for (int i = 0; i < mBatchass->getTexturesRef()->getFboCount(); i++)
	{
	gl::draw(mBatchass->getTexturesRef()->getFboTexture(i), Rectf(i* mParameterBag->mPreviewFboWidth, fboY, (i + 1) * mParameterBag->mPreviewFboWidth + margin, fboY + mParameterBag->mPreviewFboHeight));
	}*/
	gl::setViewport(getWindowBounds());
	gl::setMatricesWindow(getWindowSize());

	//imgui
	static float f = 0.0f;

	static bool showTextures = true, showTest = false, showMidi = false, showFbos = true, showTheme = false, showAudio = true, showShaders = true, showOSC = false, showFps = true, showWS = true;
	//ui::NewFrame();
	// our theme variables
	static float WindowPadding[2] = { 4, 2 };
	static float WindowMinSize[2] = { 160, 80 };
	static float FramePadding[2] = { 4, 4 };
	static float ItemSpacing[2] = { 10, 5 };
	static float ItemInnerSpacing[2] = { 5, 5 };

	static float WindowFillAlphaDefault = 0.35;
	static float WindowRounding = 7;
	static float TreeNodeSpacing = 22;
	static float ColumnsMinSpacing = 50;
	static float ScrollBarWidth = 12;

	ui::GetStyle().FramePadding = ImVec2(2, 2);
	/*ImVec4 color0 = ImVec4(0.431373, 0.160784, 0.372549, 1);
	ImVec4 color1 = ImVec4(0.2, 0.0392157, 0.0392157, 1);
	ImVec4 color2 = ImVec4(0.317647, 0.184314, 0.2, 1);
	ImVec4 color3 = ImVec4(1, 0.647059, 1, 1);
	ImVec4 color4 = ImVec4(0.741176, 0.0941176, 1, 1);
	ui::setThemeColor(color0, color1, color2, color3, color4);
	\"panelColor\":\"0x44282828\",  \"defaultBackgroundColor\":\"0xFF0d0d0d\", \"defaultNameColor\":\"0xFF90a5b6\", \"defaultStrokeColor\":\"0xFF282828\", \"activeStrokeColor\":\"0xFF919ea7\" }");
	*/

	if (showTest) ui::ShowTestWindow();

#pragma region Global

	// start a new window
	ui::Begin("Global", NULL, ImVec2(500, 200));
	{
		if (ui::CollapsingHeader("Panels", "11", true, true))
		{
			// Checkbox
			ui::Checkbox("Audio", &showAudio);
			ui::SameLine();
			ui::Checkbox("WebSockets", &showWS);
			ui::SameLine();
			ui::Checkbox("Shada", &showShaders);
			ui::SameLine();
			ui::Checkbox("FPS", &showFps);
			ui::Checkbox("OSC", &showOSC);
			ui::SameLine();
			ui::Checkbox("MIDI", &showMidi);
			ui::SameLine();
			ui::Checkbox("Test", &showTest);
			ui::SameLine();
			ui::Checkbox("Editor", &showTheme);
			if (ui::Button("Save Params")) { mParameterBag->save(); }

		}
		if (ui::CollapsingHeader("Mode", NULL, true, true))
		{
			static int mode = mParameterBag->mMode;
			ui::RadioButton("Mix", &mode, MODE_MIX); ui::SameLine();
			ui::RadioButton("Audio", &mode, MODE_AUDIO); ui::SameLine();
			ui::RadioButton("Sphere", &mode, MODE_SPHERE); ui::SameLine();
			ui::RadioButton("Warp", &mode, MODE_WARP); ui::SameLine();
			ui::RadioButton("Mesh", &mode, MODE_MESH);
			if (mParameterBag->mMode != mode) changeMode(mode);
		}
		if (ui::CollapsingHeader("Render Window", NULL, true, true))
		{
			if (ui::Button("Create")) { createRenderWindow(); }
			ui::SameLine();
			if (ui::Button("Delete")) { deleteRenderWindows(); }
			ui::SameLine();
			if (ui::Button("Preview")) { mParameterBag->mPreviewEnabled = !mParameterBag->mPreviewEnabled; }
			ui::SameLine();
			if (ui::Button("Debug")) { mParameterBag->iDebug = !mParameterBag->iDebug; }
		}
		if (ui::CollapsingHeader("Effects", NULL, true, true))
		{
			if (ui::Button("chromatic")) { mParameterBag->controlValues[15] = !mParameterBag->controlValues[15]; }
			ui::SameLine();
			if (ui::Button("origin up left")) { mParameterBag->mOriginUpperLeft = !mParameterBag->mOriginUpperLeft; }
			ui::SameLine();
			if (ui::Button("repeat")) { mParameterBag->iRepeat = !mParameterBag->iRepeat; }
			ui::SameLine();
			if (ui::Button("45 glitch")) { mParameterBag->controlValues[45] = !mParameterBag->controlValues[45]; }

			if (ui::Button("46 toggle")) { mParameterBag->controlValues[46] = !mParameterBag->controlValues[46]; }
			ui::SameLine();
			if (ui::Button("47 vignette")) { mParameterBag->controlValues[47] = !mParameterBag->controlValues[47]; }
			ui::SameLine();
			if (ui::Button("48 invert")) { mParameterBag->controlValues[48] = !mParameterBag->controlValues[48]; }
			ui::SameLine();
			if (ui::Button("greyscale")) { mParameterBag->iGreyScale = !mParameterBag->iGreyScale; }
			ui::SameLine();
			if (ui::Button("instant black"))
			{
				mParameterBag->controlValues[1] = mParameterBag->controlValues[2] = mParameterBag->controlValues[3] = mParameterBag->controlValues[4] = 0.0;
				mParameterBag->controlValues[5] = mParameterBag->controlValues[6] = mParameterBag->controlValues[7] = mParameterBag->controlValues[8] = 0.0;
			}
		}
		if (ui::CollapsingHeader("Animation", NULL, true, true))
		{

			ui::SliderInt("mUIRefresh", &mParameterBag->mUIRefresh, 1, 255);
			int ctrl;
			stringstream aParams;
			aParams << "{\"anim\" :[{\"name\" : 0,\"value\" : " << getElapsedFrames() << "}"; // TimeStamp

			// ratio
			ctrl = 11;
			if (ui::Button("a")) { mBatchass->lockRatio(); }
			ui::SameLine();
			if (ui::Button("t")) { mBatchass->tempoRatio(); }
			ui::SameLine();
			if (ui::Button("x")) { mBatchass->resetRatio(); }
			ui::SameLine();

			static float ratio = mParameterBag->controlValues[ctrl];
			if (ui::SliderFloat("ratio/min/max", &ratio, mBatchass->minRatio, mBatchass->maxRatio))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << ratio << "}";
				mParameterBag->controlValues[ctrl] = ratio;
			}
			// exposure
			ctrl = 14;
			if (ui::Button("a")) { mBatchass->lockExposure(); }
			ui::SameLine();
			if (ui::Button("t")) { mBatchass->tempoExposure(); }
			ui::SameLine();
			if (ui::Button("x")) { mBatchass->resetExposure(); }
			ui::SameLine();
			static float exposure = mParameterBag->controlValues[ctrl];
			if (ui::SliderFloat("exposure", &exposure, mBatchass->minExposure, mBatchass->maxExposure))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << exposure << "}";
				mParameterBag->controlValues[ctrl] = exposure;
			}
			// zoom
			ctrl = 13;
			if (ui::Button("a")) { mBatchass->lockZoom(); }
			ui::SameLine();
			if (ui::Button("t")) { mBatchass->tempoZoom(); }
			ui::SameLine();
			if (ui::Button("x")) { mBatchass->resetZoom(); }
			ui::SameLine();
			static float zoom = mParameterBag->controlValues[ctrl];
			if (ui::SliderFloat("zoom", &zoom, mBatchass->minZoom, mBatchass->maxZoom))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << zoom << "}";
				mParameterBag->controlValues[ctrl] = zoom;
			}
			// z position
			ctrl = 9;
			if (ui::Button("a")) { mBatchass->lockZPos(); }
			ui::SameLine();
			if (ui::Button("t")) { mBatchass->tempoZPos(); }
			ui::SameLine();
			if (ui::Button("x")) { mBatchass->resetZPos(); }
			ui::SameLine();
			static float zPosition = mParameterBag->controlValues[ctrl];
			if (ui::SliderFloat("zPosition", &zPosition, mBatchass->minZPos, mBatchass->maxZPos))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << zPosition << "}";
				mParameterBag->controlValues[ctrl] = zPosition;
			}

			// rotation speed
			ctrl = 19;
			if (ui::Button("a")) { mBatchass->lockRotationSpeed(); }
			ui::SameLine();
			if (ui::Button("t")) { mBatchass->tempoRotationSpeed(); }
			ui::SameLine();
			if (ui::Button("x")) { mBatchass->resetRotationSpeed(); }
			ui::SameLine();
			static float rotationSpeed = mParameterBag->controlValues[ctrl];
			if (ui::SliderFloat("rotationSpeed", &rotationSpeed, mBatchass->minRotationSpeed, mBatchass->maxRotationSpeed))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << rotationSpeed << "}";
				mParameterBag->controlValues[ctrl] = rotationSpeed;
			}
			// blend modes
			ctrl = 15;
			static float blendmode = mParameterBag->controlValues[ctrl];
			if (ui::SliderFloat("blendmode", &blendmode, 0.0f, 27.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << blendmode << "}";
				mParameterBag->controlValues[ctrl] = blendmode;
			}
			// steps
			ctrl = 16;
			static float steps = mParameterBag->controlValues[ctrl];
			if (ui::SliderFloat("steps", &steps, 1.0f, 128.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << steps << "}";
				mParameterBag->controlValues[ctrl] = steps;
			}
			// pixelate
			ctrl = 20;
			static float pixelate = mParameterBag->controlValues[ctrl];
			if (ui::SliderFloat("pixelate", &pixelate, 0.01f, 1.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << pixelate << "}";
				mParameterBag->controlValues[ctrl] = pixelate;
			}
			// iPreviewCrossfade
			ctrl = 17;
			static float previewCrossfade = mParameterBag->controlValues[ctrl];
			if (ui::SliderFloat("previewCrossfade", &previewCrossfade, 0.01f, 1.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << previewCrossfade << "}";
				mParameterBag->controlValues[ctrl] = previewCrossfade;
			}
			// crossfade
			ctrl = 18;
			static float crossfade = mParameterBag->controlValues[ctrl];
			if (ui::SliderFloat("crossfade", &crossfade, 0.01f, 1.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << crossfade << "}";
				mParameterBag->controlValues[ctrl] = crossfade;
			}

			aParams << "]}";
			string strAParams = aParams.str();
			if (strAParams.length() > 60)
			{
				mWebSockets->write(strAParams);
			}
		}
		if (ui::CollapsingHeader("Colors", NULL, true, true))
		{
			stringstream sParams;
			sParams << "{\"colors\" :[{\"name\" : 0,\"value\" : " << getElapsedFrames() << "}"; // TimeStamp
			// foreground color
			static float color[4] = { mParameterBag->controlValues[1], mParameterBag->controlValues[2], mParameterBag->controlValues[3], mParameterBag->controlValues[4] };
			ui::ColorEdit4("f", color);

			for (int i = 0; i < 4; i++)
			{
				if (mParameterBag->controlValues[i + 1] != color[i])
				{
					sParams << ",{\"name\" : " << i + 1 << ",\"value\" : " << color[i] << "}";
					mParameterBag->controlValues[i + 1] = color[i];
					stringstream s;
					s << mParameterBag->controlValues[1];// << "," << mParameterBag->controlValues[2] << "," << mParameterBag->controlValues[3] << "," << mParameterBag->controlValues[4];
					string strColor = s.str();
					mWebSockets->write(strColor);
					if (i == 0) mOSC->sendOSCColorMessage("/fr", mParameterBag->controlValues[1]);
					if (i == 1) mOSC->sendOSCColorMessage("/fg", mParameterBag->controlValues[2]);
					if (i == 2) mOSC->sendOSCColorMessage("/fb", mParameterBag->controlValues[3]);
					if (i == 3) mOSC->sendOSCColorMessage("/fa", mParameterBag->controlValues[4]);


				}

			}

			//ui::SameLine();
			//ui::TextColored(ImVec4(mParameterBag->controlValues[1], mParameterBag->controlValues[2], mParameterBag->controlValues[3], mParameterBag->controlValues[4]), "fg color");

			// background color
			static float backcolor[4] = { mParameterBag->controlValues[5], mParameterBag->controlValues[6], mParameterBag->controlValues[7], mParameterBag->controlValues[8] };
			ui::ColorEdit4("g", backcolor);
			for (int i = 0; i < 4; i++)
			{
				if (mParameterBag->controlValues[i + 5] != backcolor[i])
				{
					sParams << ",{\"name\" : " << i + 5 << ",\"value\" : " << backcolor[i] << "}";
					mParameterBag->controlValues[i + 5] = backcolor[i];
				}

			}

			//ui::SameLine();
			//ui::TextColored(ImVec4(mParameterBag->controlValues[5], mParameterBag->controlValues[6], mParameterBag->controlValues[7], mParameterBag->controlValues[8]), "bg color");

			sParams << "]}";
			string strParams = sParams.str();
			/*if (strParams.length() > 60)
			{
			mWebSockets->write(strParams);
			}*/

		}

	}
	ui::End();
#pragma endregion Global
#pragma region shaders

	if (showShaders)
	{
		ui::Begin("shaders", NULL, ImVec2(300, 300));
		{

			ui::BeginChild("shadas", ImVec2(0, 450), true);
			ui::Columns(4, "data", true);

			for (int i = 0; i < mBatchass->getShadersRef()->getCount(); i++)
			{
				//char buf[32];
				//sprintf_s(buf, "s %d", i);
				if (ui::Button(mBatchass->getShadersRef()->getShaderName(i).c_str()))
				{
					//setCurrentFbo mParameterBag->mCurrentFboLibraryIndex = aIndex;
				}
				ui::NextColumn();
				char buf[32];
				sprintf_s(buf, "L%d", i);
				if (ui::Button(buf))
				{
					mParameterBag->mLeftFragIndex = i;
				}
				ui::NextColumn();
				sprintf_s(buf, "R%d", i);
				if (ui::Button(buf))
				{
					mParameterBag->mRightFragIndex = i;
				}
				ui::NextColumn();
				sprintf_s(buf, "P%d", i);
				if (ui::Button(buf))
				{
					mParameterBag->mPreviewFragIndex = i;
				}
				ui::NextColumn();
				//ui::Separator();
			}
			ui::EndChild();
			ui::Columns(1);
		}
		ui::End();
	}

#pragma endregion shaders
#pragma region fbos

	if (showFbos)
	{
		ui::Begin("fbos", NULL, ImVec2(300, 150));
		{

			ui::BeginChild("fbo", ImVec2(0, 150), true);
			ui::Columns(2, "data", true);

			for (int i = 0; i < mBatchass->getTexturesRef()->getFboCount(); i++)
			{
				char buf[32];
				sprintf_s(buf, "fbo %d", i);
				if (ui::Button(buf))
				{
					//setCurrentFbo 
				}
				ui::NextColumn();
				sprintf_s(buf, "F%d", i);
				if (ui::Button(buf))
				{
					mBatchass->getTexturesRef()->flipFbo(i);
				}
				ui::NextColumn();
				//ui::Separator();
			}
			ui::EndChild();
			ui::Columns(1);
		}
		ui::End();
	}

#pragma endregion fbos
#pragma region textures

	if (showTextures)
	{
		ui::Begin("textures", NULL, ImVec2(300, 150));
		{
			ui::BeginChild("texture", ImVec2(0, 150), true);
			ui::Columns(2, "data", true);

			for (int i = 0; i < mBatchass->getTexturesRef()->getTextureCount(); i++)
			{
				char buf[32];
				sprintf_s(buf, "texture %d", i);
				if (ui::Button(buf))
				{
					mParameterBag->iChannels[0] = i;
				}
				ui::NextColumn();
				sprintf_s(buf, "F%d", i);
				if (ui::Button(buf))
				{
					mBatchass->getTexturesRef()->flipTexture(i);
				}
				ui::NextColumn();
				//ui::Separator();
			}
			ui::EndChild();
			ui::Columns(1);
		}
		ui::End();
	}

#pragma endregion textures
#pragma region MIDI

	// MIDI window
	if (showMidi)
	{
		ui::Begin("MIDI", NULL, ImVec2(300, 300));
		{
			if (ui::CollapsingHeader("MidiIn", "20", true, true))
			{
				ui::Columns(2, "data", true);
				ui::Text("Name"); ui::NextColumn();
				ui::Text("Connect"); ui::NextColumn();
				ui::Separator();

				for (int i = 0; i < mMidiInputs.size(); i++)
				{
					ui::Text(mMidiInputs[i].portName.c_str()); ui::NextColumn();
					char buf[32];
					if (mMidiInputs[i].isConnected)
					{
						sprintf_s(buf, "Disconnect %d", i);
					}
					else
					{
						sprintf_s(buf, "Connect %d", i);
					}

					if (ui::Button(buf))
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
					ui::NextColumn();
					ui::Separator();
				}
				ui::Columns(1);

			}
			if (ui::CollapsingHeader("Log", "21", true, true))
			{
				static ImGuiTextBuffer log;
				static int lines = 0;
				ui::Text("Buffer contents: %d lines, %d bytes", lines, log.size());
				if (ui::Button("Clear")) { log.clear(); lines = 0; }
				//ui::SameLine();

				if (newLogMsg)
				{
					newLogMsg = false;
					log.append(mLogMsg.c_str());
					lines++;
					if (lines > 5) { log.clear(); lines = 0; }
				}
				ui::BeginChild("Log");
				ui::TextUnformatted(log.begin(), log.end());
				ui::EndChild();
			}
		}
		ui::End();
	}
#pragma endregion MIDI
#pragma region OSC

	if (showOSC)
	{
		ui::Begin("OSC router", NULL, ImVec2(300, 300));
		{
			ui::Text("Sending to host %s", mParameterBag->mOSCDestinationHost.c_str());
			ui::SameLine();
			ui::Text(" on port %d", mParameterBag->mOSCDestinationPort);
			ui::Text("Sending to 2nd host %s", mParameterBag->mOSCDestinationHost2.c_str());
			ui::SameLine();
			ui::Text(" on port %d", mParameterBag->mOSCDestinationPort2);
			ui::Text(" Receiving on port %d", mParameterBag->mOSCReceiverPort);

			static char str0[128] = "/live/play";
			static int i0 = 0;
			static float f0 = 0.0f;
			ui::InputText("address", str0, IM_ARRAYSIZE(str0));
			ui::InputInt("track", &i0);
			ui::InputFloat("clip", &f0, 0.01f, 1.0f);
			if (ui::Button("Send")) { mOSC->sendOSCIntMessage(str0, i0); }

			static ImGuiTextBuffer OSClog;
			static int lines = 0;
			if (ui::Button("Clear")) { OSClog.clear(); lines = 0; }
			ui::SameLine();
			ui::Text("Buffer contents: %d lines, %d bytes", lines, OSClog.size());

			if (mParameterBag->newOSCMsg)
			{
				mParameterBag->newOSCMsg = false;
				OSClog.append(mParameterBag->OSCMsg.c_str());
				lines++;
				if (lines > 5) { OSClog.clear(); lines = 0; }
			}
			ui::BeginChild("OSClog");
			ui::TextUnformatted(OSClog.begin(), OSClog.end());
			ui::EndChild();
		}
		ui::End();
	}
#pragma endregion OSC
#pragma region WebSockets
	if (showWS)
	{
		ui::Begin("WebSockets", NULL, ImVec2(300, 300));
		{
			if (mParameterBag->mIsWebSocketsServer)
			{
				ui::Text("Server %s", mParameterBag->mWebSocketsHost.c_str());
				ui::SameLine();
			}
			else
			{
				ui::Text("Client %s", mParameterBag->mWebSocketsHost.c_str());
				ui::SameLine();
			}
			ui::Text(" on port %d", mParameterBag->mWebSocketsPort);
			if (ui::Button("Send"))
			{
				mSeconds = (int)getElapsedSeconds();
				stringstream s;
				s << mSeconds;
				mWebSockets->write(s.str());
			}
			static ImGuiTextBuffer WSlog;
			static int lines = 0;
			if (ui::Button("Clear")) { WSlog.clear(); lines = 0; }
			ui::SameLine();
			ui::Text("Buffer contents: %d lines, %d bytes", lines, WSlog.size());

			if (mParameterBag->newWSMsg)
			{
				mParameterBag->newWSMsg = false;
				WSlog.append(mParameterBag->WSMsg.c_str());
				lines++;
				if (lines > 5) { WSlog.clear(); lines = 0; }
			}
			ui::BeginChild("WSlog");
			ui::TextUnformatted(WSlog.begin(), WSlog.end());
			ui::EndChild();
		}
		ui::End();
	}
#pragma endregion WebSockets
#pragma region Routing

	ui::Begin("Routing", NULL, ImVec2(300, 300));
	{
		ui::BeginChild("Warps routing", ImVec2(0, 300), true);
		ui::Text("Selected warp: %d", mParameterBag->selectedWarp);
		ui::Columns(4);
		ui::Text("ID"); ui::NextColumn();
		ui::Text("texIndex"); ui::NextColumn();
		ui::Text("texMode"); ui::NextColumn();
		ui::Text("active"); ui::NextColumn();
		ui::Separator();
		for (int i = 0; i < mParameterBag->MAX; i++)
		{
			ui::Text("%d", i); ui::NextColumn();
			ui::Text("%d", mParameterBag->mWarpFbos[i].textureIndex); ui::NextColumn();
			ui::Text("%d", mParameterBag->mWarpFbos[i].textureMode); ui::NextColumn();
			ui::Text("%d", mParameterBag->mWarpFbos[i].active); ui::NextColumn();

		}
		ui::Columns(1);
		ui::EndChild();
	}
	ui::End();
#pragma endregion Routing
#pragma region Audio

	// audio window
	if (showAudio)
	{
		ui::Begin("Audio", NULL, ImVec2(200, 100));
		{
			ui::Checkbox("Playing", &mParameterBag->mIsPlaying);
			ui::SameLine();
			ui::Text("Beat %d", mParameterBag->mBeat);
			ui::SameLine();
			ui::Text("Tempo %.2f", mParameterBag->mTempo);

			static ImVector<float> values; if (values.empty()) { values.resize(40); memset(&values.front(), 0, values.size()*sizeof(float)); }
			static int values_offset = 0;
			// audio maxVolume
			static float refresh_time = -1.0f;
			if (ui::GetTime() > refresh_time + 1.0f / 20.0f)
			{
				refresh_time = ui::GetTime();
				values[values_offset] = mParameterBag->maxVolume;
				values_offset = (values_offset + 1) % values.size();
			}
			if (mParameterBag->maxVolume > 240.0) ui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
			ui::PlotLines("Volume", &values.front(), (int)values.size(), values_offset, toString(mBatchass->formatFloat(mParameterBag->maxVolume)).c_str(), 0.0f, 255.0f, ImVec2(0, 30));
			if (mParameterBag->maxVolume > 240.0) ui::PopStyleColor();

			ui::SliderFloat("mult factor", &mParameterBag->mAudioMultFactor, 0.01f, 10.0f);

			static int fftSize = mAudio->getFftSize();
			if (ui::SliderInt("fft size", &fftSize, 1, 1024))
			{
				mAudio->setFftSize(fftSize);
			}
			static int windowSize = mAudio->getWindowSize();
			if (ui::SliderInt("window size", &windowSize, 1, 1024))
			{
				mAudio->setWindowSize(windowSize);
			}
			/*for (int a = 0; a < MAX; a++)
			{
			if (mOSC->tracks[a] != "default.glsl") ui::Button(mOSC->tracks[a].c_str());
			}*/

		}
		ui::End();
	}
#pragma endregion Audio
#pragma region FPS
	// fps window
	if (showFps)
	{
		ui::Begin("Fps", NULL, ImVec2(100, 100));
		{
			static ImVector<float> values; if (values.empty()) { values.resize(100); memset(&values.front(), 0, values.size()*sizeof(float)); }
			static int values_offset = 0;
			static float refresh_time = -1.0f;
			if (ui::GetTime() > refresh_time + 1.0f / 6.0f)
			{
				refresh_time = ui::GetTime();
				values[values_offset] = mParameterBag->iFps;
				values_offset = (values_offset + 1) % values.size();
			}
			if (mParameterBag->iFps < 12.0) ui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
			ui::PlotLines("FPS", &values.front(), (int)values.size(), values_offset, mParameterBag->sFps.c_str(), 0.0f, 300.0f, ImVec2(0, 30));
			if (mParameterBag->iFps < 12.0) ui::PopStyleColor();
			ui::Text("mouse %d", ui::GetIO().MouseDown[0]);

		}

		ui::End();

	}
#pragma endregion FPS

	//ui::Render();

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
	gl::setMatricesWindow(mParameterBag->mFboWidth, mParameterBag->mFboHeight);// , false);
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
void BatchassApp::saveThumb()
{
	string filename;
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
		filename = mBatchass->getShadersRef()->getFragFileName() + ".png";
		writeImage(getAssetPath("") / "thumbs" / filename, mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mCurrentPreviewFboIndex));
		mBatchass->log("saved:" + filename);

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
		int rtn = mBatchass->getShadersRef()->loadPixelFragmentShader(mFile);
		if (rtn > -1 && rtn < mBatchass->getShadersRef()->getCount())
		{
			mParameterBag->controlValues[13] = 1.0f;
			// send content via OSC
			/*fs::path fr = mFile;
			string name = "unknown";
			if (mFile.find_last_of("\\") != std::string::npos) name = mFile.substr(mFile.find_last_of("\\") + 1);
			if (fs::exists(fr))
			{

			std::string fs = loadString(loadFile(mFile));
			mOSC->sendOSCStringMessage("/fs", 0, fs, name);
			}*/
			// save thumb
			timeline().apply(&mTimer, 1.0f, 1.0f).finishFn([&]{ saveThumb(); });
		}
	}
	if (!loaded && ext == "fs")
	{
		//do not try to load by other ways
		loaded = true;
		//mShaders->incrementPreviewIndex();
		mBatchass->getShadersRef()->loadFragmentShader(mPath);
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
		ui::Shutdown();
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
	mParameterBag->sFps = toString(floor(mParameterBag->iFps));
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
	mUI->update();
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
	if (mParameterBag->mMode == MODE_AUDIO) mAudio->mouseDown(event);
	
}

void BatchassApp::mouseDrag(MouseEvent event)
{
	if (mParameterBag->mMode == MODE_WARP) mWarpings->mouseDrag(event);
	if (mParameterBag->mMode == MODE_MESH) mMeshes->mouseDrag(event);
	if (mParameterBag->mMode == MODE_AUDIO) mAudio->mouseDrag(event);
}

void BatchassApp::mouseUp(MouseEvent event)
{
	if (mParameterBag->mMode == MODE_WARP) mWarpings->mouseUp(event);
	if (mParameterBag->mMode == MODE_AUDIO) mAudio->mouseUp(event);

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
		case ci::app::KeyEvent::KEY_n:
			changeMode(MODE_MIX);
			break;
		case ci::app::KeyEvent::KEY_a:
			changeMode(MODE_AUDIO);
			break;
		case ci::app::KeyEvent::KEY_s:
			changeMode(MODE_SPHERE);
			break;
		case ci::app::KeyEvent::KEY_w:
			changeMode(MODE_WARP);
			break;
		case ci::app::KeyEvent::KEY_m:
			changeMode(MODE_MESH);
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
			mUI->toggleVisibility();
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
			ui::Shutdown();
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
void BatchassApp::changeMode(int newMode)
{
	if (mParameterBag->mMode != newMode)
	{
		mParameterBag->controlValues[4] = 1.0f;
		mParameterBag->controlValues[8] = 1.0f;

		mParameterBag->mPreviousMode = mParameterBag->mMode;
		switch (mParameterBag->mPreviousMode)
		{
		case 5: //mesh
			mParameterBag->iLight = false;
			mParameterBag->controlValues[19] = 0.0; //reset rotation
			break;
		}
		mParameterBag->mMode = newMode;
		switch (newMode)
		{
		case 4: //sphere
			mParameterBag->mCamPosXY = Vec2f(-155.6, -87.3);
			mParameterBag->mCamEyePointZ = -436.f;
			mParameterBag->controlValues[5] = mParameterBag->controlValues[6] = mParameterBag->controlValues[7] = 0;
			break;
		case 5: //mesh
			mParameterBag->controlValues[19] = 1.0; //reset rotation
			mParameterBag->mRenderPosXY = Vec2f(0.0, 0.0);
			mParameterBag->mCamEyePointZ = -56.f;
			mParameterBag->controlValues[5] = mParameterBag->controlValues[6] = mParameterBag->controlValues[7] = 0;
			mParameterBag->currentSelectedIndex = 5;
			mParameterBag->iLight = true;
			break;
		}
	}
}

CINDER_APP_BASIC(BatchassApp, RendererGl)
