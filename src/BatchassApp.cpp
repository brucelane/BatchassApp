/*
TODO
- warp select mix fbo texture
- flip horiz
- check flip H and V (spout also)
- sort fbo names and indexes (warps only 4 or 5 inputs)
- spout texture 10 create shader 10.glsl(ThemeFromBrazil) iChannel0
- warpwrapper handle texture mode 0 for spout (without fbo)
*/

#include "BatchassApp.h"

void BatchassApp::prepareSettings(Settings* settings)
{
	int w;
	// parameters
	mParameterBag = ParameterBag::create();
	// utils
	mBatchass = Batchass::create(mParameterBag);
	mBatchass->log("start");
	// start profiling
	auto start = Clock::now();

	w = mBatchass->getWindowsResolution();

	settings->setWindowSize(mParameterBag->mMainWindowWidth, mParameterBag->mMainWindowHeight);
	// Setting an unrealistically high frame rate effectively
	// disables frame rate limiting
	//settings->setFrameRate(10000.0f);
	settings->setFrameRate(60.0f);
	settings->setWindowPos(Vec2i(w - mParameterBag->mMainWindowWidth, 0));

	settings->setResizable(false);
	settings->setBorderless();

	// if mStandalone, put on the 2nd screen
	if (mParameterBag->mStandalone)
	{
		settings->setWindowSize(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight);
		settings->setWindowPos(Vec2i(mParameterBag->mRenderX, mParameterBag->mRenderY));
		settings->setBorderless();
		settings->setResizable(false);
	}
	auto end = Clock::now();
	auto msdur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	auto nsdur = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	//std::cout << msdur.count() << "ms, " << nsdur.count() << "µs" << std::endl;
	mBatchass->log("prepareSettings: " + toString(msdur.count()));
}

void BatchassApp::setup()
{
	// start profiling
	auto start = Clock::now();
	//0SetWindowPos
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
	mWebSockets = WebSockets::create(mParameterBag, mBatchass);

	// setup shaders and textures
	mBatchass->setup();
	//mParameterBag->mWarpFbos[0].textureIndex = 1;
	// setup the main window and associated draw function
	mMainWindow = getWindow();
	mMainWindow->setTitle("Batchass");
	mMainWindow->connectDraw(&BatchassApp::drawMain, this);
	mMainWindow->connectClose(&BatchassApp::shutdown, this);

	// instanciate the audio class
	mAudio = AudioWrapper::create(mParameterBag, mBatchass->getTexturesRef());
	// instanciate the Meshes class
	mMeshes = Meshes::create(mParameterBag, mBatchass->getTexturesRef());
	// instanciate the PointSphere class
	mSphere = PointSphere::create(mParameterBag, mBatchass->getTexturesRef(), mBatchass->getShadersRef());
	// instanciate the VertexSphere class
	mVertexSphere = VertexSphere::create(mParameterBag, mBatchass->getTexturesRef(), mBatchass->getShadersRef());
	// instanciate the ABP class
	mABP = ABP::create(mParameterBag, mBatchass->getTexturesRef());
	// instanciate the spout class
	mSpout = SpoutWrapper::create(mParameterBag, mBatchass->getTexturesRef());
	// instanciate the console class
	mConsole = AppConsole::create(mParameterBag, mBatchass);

	mTimer = 0.0f;

	newLogMsg = false;
	auto end = Clock::now();
	auto mididur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	mBatchass->log("setup before: " + toString(mididur.count()));
	start = Clock::now();

	// Setup the MinimalUI user interface
	//mUI = UI::create(mParameterBag, mBatchass->getShadersRef(), mBatchass->getTexturesRef(), mMainWindow);
	//mUI->setup();
	mouseGlobal = false;
	// imgui
	margin = 3;
	inBetween = 3;
	// mPreviewFboWidth 80 mPreviewFboHeight 60 margin 10 inBetween 15 mPreviewWidth = 160;mPreviewHeight = 120;
	w = mParameterBag->mPreviewFboWidth + margin;
	h = mParameterBag->mPreviewFboHeight * 3;
	largeW = (mParameterBag->mPreviewFboWidth + margin) * 4;
	largeH = (mParameterBag->mPreviewFboHeight + margin) * 5;
	largePreviewW = mParameterBag->mPreviewWidth + margin;
	largePreviewH = (mParameterBag->mPreviewHeight + margin) * 2.2;
	displayHeight = mParameterBag->mMainDisplayHeight - 50;
	static float f = 0.0f;

	showConsole = showGlobal = showTextures = showAudio = showWS = showMidi = showChannels = showShaders = true;
	showTest = showTheme = showOSC = showFbos = false;

	// set ui window and io events callbacks
	ui::connectWindow(getWindow());
	ui::initialize();

	// midi
	setupMidi();
	mSeconds = 0;
	// RTE mBatchass->getShadersRef()->setupLiveShader();
	mBatchass->tapTempo();
	// end profiling
	end = Clock::now();
	auto msdur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	auto nsdur = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	//std::cout << msdur.count() << "ms, " << nsdur.count() << "µs" << std::endl;
	mBatchass->log("setup: " + toString(msdur.count()));

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
				if (mBatchass->getWarpsRef()->isEditModeEnabled())
				{
					mBatchass->getWarpsRef()->draw();
				}

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
			case MODE_VERTEXSPHERE:
				mVertexSphere->draw();
				gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mVertexSphereFboIndex), rect);
				break;
			case MODE_ABP:
				//mABP->draw();
				//gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mABPFboIndex), rect);
				break;
			default:
				//gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mMixFboIndex));
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

	//imgui
	if (removeUI || mBatchass->getWarpsRef()->isEditModeEnabled())
	{
		return;
	}

	gl::setViewport(getWindowBounds());
	gl::setMatricesWindow(getWindowSize());
	xPos = margin;
	yPos = margin;
	const char* textureNames[] = { "audio", "img1", "img2", "img3", "4pvwFbo", "5mixFbo", "6leftFbo", "7rightFbo", "8warp1Fbo", "9warp2Fbo", "10spout", "11LiveFbo" };
	const char* fboNames[] = { "mix", "left", "right", "warp1", "warp2", "preview", "abp", "live", "sphere", "mesh", "audio", "vtxsphere" };
	const char* warpInputs[] = { "mix", "left", "right", "warp1", "warp2", "preview", "abp", "live" };

#pragma region style
	// our theme variables
	ImGuiStyle& style = ui::GetStyle();
	style.WindowRounding = 4;
	style.WindowPadding = ImVec2(3, 3);
	style.FramePadding = ImVec2(2, 2);
	style.ItemSpacing = ImVec2(3, 3);
	style.ItemInnerSpacing = ImVec2(3, 3);
	style.WindowMinSize = ImVec2(w, mParameterBag->mPreviewFboHeight);
	style.Alpha = 0.6f;
	style.Colors[ImGuiCol_Text] = ImVec4(0.89f, 0.92f, 0.94f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.38f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.4f, 0.21f, 0.21f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_ComboBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.99f, 0.22f, 0.22f, 0.50f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.65f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8f, 0.35f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_Column] = ImVec4(0.04f, 0.04f, 0.04f, 0.22f);
	style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.65f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.8f, 0.35f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.9f, 0.45f, 0.45f, 1.00f);
	style.Colors[ImGuiCol_CloseButton] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
	style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.65f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.8f, 0.35f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.65f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.8f, 0.35f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	style.Colors[ImGuiCol_TooltipBg] = ImVec4(0.65f, 0.25f, 0.25f, 1.00f);
#pragma endregion style

#pragma region mix
	// left/warp1 fbo
	ui::SetNextWindowSize(ImVec2(largePreviewW, largePreviewH), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
	ui::Begin("Left/warp1 fbo");
	{
		ui::PushItemWidth(mParameterBag->mPreviewFboWidth);
		ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.1f, 0.6f, 0.6f));
		ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.1f, 0.7f, 0.7f));
		ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.1f, 0.8f, 0.8f));

		sprintf_s(buf, "FV##f%d", 40);
		if (mParameterBag->mMode == mParameterBag->MODE_WARP)
		{
			ui::Image((void*)mBatchass->getTexturesRef()->getFboTextureId(mParameterBag->mWarp1FboIndex), Vec2i(mParameterBag->mPreviewWidth, mParameterBag->mPreviewHeight));
			if (ui::Button(buf)) mBatchass->getTexturesRef()->flipFbo(mParameterBag->mWarp1FboIndex);
			if (ui::IsItemHovered()) ui::SetTooltip("Flip vertically");
			// renderXY mouse
			ui::SliderFloat("W1RdrX", &mParameterBag->mWarp1RenderXY.x, 0.0f, 1.0f);
			ui::SliderFloat("W1RdrY", &mParameterBag->mWarp1RenderXY.y, 0.0f, 1.0f);

			ui::Columns(4);
			ui::Text("ID"); ui::NextColumn();
			ui::Text("idx"); ui::NextColumn();
			ui::Text("mode"); ui::NextColumn();
			ui::Text("actv"); ui::NextColumn();
			ui::Separator();
			for (int i = 0; i < mParameterBag->mWarpFbos.size() - 1; i++)
			{
				if (mParameterBag->mWarpFbos[i].textureIndex == 3)
				{
					ui::Text("%d", i); ui::NextColumn();
					ui::Text("%d", mParameterBag->mWarpFbos[i].textureIndex); ui::NextColumn();
					ui::Text("%d", mParameterBag->mWarpFbos[i].textureMode); ui::NextColumn();
					ui::Text("%d", mParameterBag->mWarpFbos[i].active); ui::NextColumn();

				}

			}
			ui::Columns(1);

		}
		else
		{
			ui::Image((void*)mBatchass->getTexturesRef()->getFboTextureId(mParameterBag->mLeftFboIndex), Vec2i(mParameterBag->mPreviewWidth, mParameterBag->mPreviewHeight));
			if (ui::Button(buf)) mBatchass->getTexturesRef()->flipFbo(mParameterBag->mLeftFboIndex);
			if (ui::IsItemHovered()) ui::SetTooltip("Flip vertically");
			// renderXY mouse
			ui::SliderFloat("LeftRdrX", &mParameterBag->mLeftRenderXY.x, 0.0f, 1.0f);
			ui::SliderFloat("LeftRdrY", &mParameterBag->mLeftRenderXY.y, 0.0f, 1.0f);

			ui::Columns(4);
			ui::Text("ID"); ui::NextColumn();
			ui::Text("idx"); ui::NextColumn();
			ui::Text("mode"); ui::NextColumn();
			ui::Text("actv"); ui::NextColumn();
			ui::Separator();
			for (int i = 0; i < mParameterBag->mWarpFbos.size() - 1; i++)
			{
				if (mParameterBag->mWarpFbos[i].textureIndex == 4)
				{
					ui::Text("%d", i); ui::NextColumn();
					ui::Text("%d", mParameterBag->mWarpFbos[i].textureIndex); ui::NextColumn();
					ui::Text("%d", mParameterBag->mWarpFbos[i].textureMode); ui::NextColumn();
					ui::Text("%d", mParameterBag->mWarpFbos[i].active); ui::NextColumn();

				}

			}
			ui::Columns(1);
		}
		ui::PopStyleColor(3);
		ui::PopItemWidth();
	}
	ui::End();
	xPos += largePreviewW + margin;

	// right/warp2 fbo
	ui::SetNextWindowSize(ImVec2(largePreviewW, largePreviewH), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
	ui::Begin("Right/warp2 fbo");
	{
		ui::PushItemWidth(mParameterBag->mPreviewFboWidth);

		ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.1f, 0.6f, 0.6f));
		ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.1f, 0.7f, 0.7f));
		ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.1f, 0.8f, 0.8f));

		sprintf_s(buf, "FV##f%d", 41);
		if (mParameterBag->mMode == mParameterBag->MODE_WARP)
		{
			ui::Image((void*)mBatchass->getTexturesRef()->getFboTextureId(mParameterBag->mWarp2FboIndex), Vec2i(mParameterBag->mPreviewWidth, mParameterBag->mPreviewHeight));
			if (ui::Button(buf)) mBatchass->getTexturesRef()->flipFbo(mParameterBag->mWarp2FboIndex);
			if (ui::IsItemHovered()) ui::SetTooltip("Flip vertically");
			// renderXY mouse
			ui::SliderFloat("W2RdrX", &mParameterBag->mWarp2RenderXY.x, 0.0f, 1.0f);
			ui::SliderFloat("W2RdrY", &mParameterBag->mWarp2RenderXY.y, 0.0f, 1.0f);
		}
		else
		{
			ui::Image((void*)mBatchass->getTexturesRef()->getFboTextureId(mParameterBag->mRightFboIndex), Vec2i(mParameterBag->mPreviewWidth, mParameterBag->mPreviewHeight));
			if (ui::Button(buf)) mBatchass->getTexturesRef()->flipFbo(mParameterBag->mRightFboIndex);
			if (ui::IsItemHovered()) ui::SetTooltip("Flip vertically");
			// renderXY mouse
			ui::SliderFloat("RightRdrX", &mParameterBag->mRightRenderXY.x, 0.0f, 1.0f);
			ui::SliderFloat("RightRdrY", &mParameterBag->mRightRenderXY.y, 0.0f, 1.0f);
		}
		ui::PopStyleColor(3);
		ui::PopItemWidth();
	}
	ui::End();
	xPos += largePreviewW + margin;

	// mix fbo
	ui::SetNextWindowSize(ImVec2(largePreviewW, largePreviewH), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
	ui::Begin("Mix fbo");
	{
		ui::PushItemWidth(mParameterBag->mPreviewFboWidth);


		ui::Image((void*)mBatchass->getTexturesRef()->getFboTextureId(mParameterBag->mMixFboIndex), Vec2i(mParameterBag->mPreviewWidth, mParameterBag->mPreviewHeight));
		ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.1f, 0.6f, 0.6f));
		ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.1f, 0.7f, 0.7f));
		ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.1f, 0.8f, 0.8f));

		sprintf_s(buf, "FV##f%d", 42);
		if (ui::Button(buf)) mBatchass->getTexturesRef()->flipFbo(mParameterBag->mMixFboIndex);
		if (ui::IsItemHovered()) ui::SetTooltip("Flip vertically");
		// crossfade
		if (ui::SliderFloat("Xfade", &mParameterBag->controlValues[18], 0.01f, 1.0f))
		{
		}
		// renderXY mouse
		ui::SliderFloat("RdrX", &mParameterBag->mRenderXY.x, 0.0f, 1.0f);
		ui::SliderFloat("RdrY", &mParameterBag->mRenderXY.y, 0.0f, 1.0f);
		ui::PopStyleColor(3);
		ui::PopItemWidth();

	}
	ui::End();
	xPos += largePreviewW + margin;

	// preview fbo
	ui::SetNextWindowSize(ImVec2(largePreviewW, largePreviewH), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
	ui::Begin("Preview fbo");
	{
		ui::PushItemWidth(mParameterBag->mPreviewFboWidth);

		ui::Image((void*)mBatchass->getTexturesRef()->getFboTextureId(mParameterBag->mCurrentPreviewFboIndex), Vec2i(mParameterBag->mPreviewWidth, mParameterBag->mPreviewHeight));
		ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.1f, 0.6f, 0.6f));
		ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.1f, 0.7f, 0.7f));
		ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.1f, 0.8f, 0.8f));

		sprintf_s(buf, "FV##f%d", 39);
		if (ui::Button(buf)) mBatchass->getTexturesRef()->flipFbo(mParameterBag->mCurrentPreviewFboIndex);
		if (ui::IsItemHovered()) ui::SetTooltip("Flip vertically"); ui::SameLine();
		if (mParameterBag->mPreviewEnabled)
		{
			sprintf_s(buf, "On##pvwe");
		}
		else
		{
			sprintf_s(buf, "Off##pvwe");
		}
		mParameterBag->mPreviewEnabled ^= ui::Button(buf);
		ui::PopStyleColor(3);
		ui::Text(mBatchass->getTexturesRef()->getPreviewTime());

		ui::PopItemWidth();

	}
	ui::End();
	xPos += largePreviewW + margin;
#pragma endregion mix
	// console
	ui::SetNextWindowSize(ImVec2(largePreviewW, largePreviewH), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
	if (showConsole) ShowAppConsole(&showConsole);
	xPos += largePreviewW + margin;
#pragma region channels
	if (showChannels)
	{
		static bool popupTexture_open = false;
		static int selectedChn = -1;
		static int selectedTex = -1;

		ui::SetNextWindowSize(ImVec2(w * 2, largePreviewH), ImGuiSetCond_Once);
		ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);

		ui::Begin("Channels");
		{
			ui::Columns(2);
			ui::Text("Chn"); ui::NextColumn();
			ui::Text("Tex"); ui::NextColumn();

			ui::Separator();
			for (int i = 0; i < mParameterBag->iChannels.size() - 1; i++)
			{
				ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
				ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
				ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
				ui::Text("c%d", i); ui::NextColumn();
				sprintf_s(buf, "%d", mParameterBag->iChannels[i]);
				if (ImGui::Button(buf))
				{
					popupTexture_open = true;
					selectedChn = i;
				}
				ui::NextColumn();
				ui::PopStyleColor(3);
			}
			ui::Columns(1);
		}
		ui::End();

		ImGui::SameLine();
		if (selectedTex == -1)
		{
			ImGui::Text("<None>");
		}
		else
		{
			sprintf_s(buf, "%d", mParameterBag->iChannels[selectedTex]);
			ImGui::Text(buf);
		}
		if (popupTexture_open)
		{
			//sprintf_s(buf, "##wtpopup", selectedChn);
			ImGui::BeginPopup(&popupTexture_open);
			for (size_t i = 0; i < mBatchass->getTexturesRef()->getTextureCount(); i++)
			{
				sprintf_s(buf, "%d##chntpopup", i);
				if (ImGui::Selectable(buf, false))
				{
					selectedTex = i;
					if (selectedTex > -1) mBatchass->assignTextureToChannel(selectedTex, selectedChn);
					// reinit
					popupTexture_open = false;
					selectedChn = -1;
					selectedTex = -1;
				}
			}
			ImGui::EndPopup();

		}
		xPos += w * 2 + margin;
	}
#pragma endregion channels
#pragma region WebSockets
	if (showWS)
	{
		ui::SetNextWindowSize(ImVec2(largePreviewW + 20, largePreviewH), ImGuiSetCond_Once);
		ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
		ui::Begin("WebSockets");
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
			if (ui::Button("Connect"))
			{
				mWebSockets->clientConnect();
			}
			ui::SameLine();
			if (ui::Button("Ping"))
			{
				mWebSockets->ping();
			}
			ui::SameLine();
			if (ui::Button("Send"))
			{
				mSeconds = (int)getElapsedSeconds();
				stringstream s;
				s << mSeconds;
				mWebSockets->write(s.str());
			}
			static ImGuiTextBuffer WSlog;
			static int lines = 0;
			if (ui::Button("Clear##ws")) { WSlog.clear(); lines = 0; }
			ui::SameLine();
			ui::Text("Content: %d l, %d b", lines, WSlog.size());

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
		yPos += largePreviewH + margin;
	}
#pragma endregion WebSockets

#pragma region Audio

	// audio window
	if (showAudio)
	{
		ui::SetNextWindowSize(ImVec2(largePreviewW + 20, largePreviewH), ImGuiSetCond_Once);
		ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
		ui::Begin("Audio##ap");
		{
			ui::Text("Beat %d", mParameterBag->mBeat);
			ui::SameLine();
			ui::Checkbox("Playing", &mParameterBag->mIsPlaying);

			ui::Text("Tempo %.2f", mParameterBag->mTempo);
			if (ui::Button("Tap tempo")) { mBatchass->tapTempo(); }
			ui::SameLine();
			if (ui::Button("Time tempo")) { mParameterBag->mUseTimeWithTempo = !mParameterBag->mUseTimeWithTempo; }

			//void Batchass::setTimeFactor(const int &aTimeFactor)
			ui::SliderFloat("time x", &mParameterBag->iTimeFactor, 0.0001f, 32.0f, "%.1f");

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

			ui::SliderFloat("mult x", &mParameterBag->mAudioMultFactor, 0.01f, 10.0f);
			ImGui::PlotHistogram("Histogram", mAudio->getSmallSpectrum(), 7, 0, NULL, 0.0f, 255.0f, ImVec2(0, 30));

			if (mParameterBag->maxVolume > 240.0) ui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
			ui::PlotLines("Volume", &values.front(), (int)values.size(), values_offset, toString(mBatchass->formatFloat(mParameterBag->maxVolume)).c_str(), 0.0f, 255.0f, ImVec2(0, 30));
			if (mParameterBag->maxVolume > 240.0) ui::PopStyleColor();
		}
		ui::End();

		yPos += largePreviewH + margin;
	}
#pragma endregion Audio

#pragma region MIDI

	// MIDI window
	if (showMidi)
	{
		ui::SetNextWindowSize(ImVec2(largePreviewW + 20, largePreviewH), ImGuiSetCond_Once);
		ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
		ui::Begin("MIDI");
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
		xPos += largePreviewW + 20 + margin;
		yPos = margin;

	}
#pragma endregion MIDI
#pragma region Global

	ui::SetNextWindowSize(ImVec2(largeW, displayHeight), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
	sprintf_s(buf, "Fps %c %d###fps", "|/-\\"[(int)(ImGui::GetTime() / 0.25f) & 3], (int)mParameterBag->iFps);
	ui::Begin(buf);
	{
		ImGui::PushItemWidth(mParameterBag->mPreviewFboWidth);
		// mode
		static int mode = mParameterBag->mMode;

		const char* modes[] = { "Mix", "Warp", "Audio", "Sphere", "Mesh", "Live", "ABP", "VertexSphere" };
		ui::Combo("Mode", &mode, modes, IM_ARRAYSIZE(modes));

		if (mParameterBag->mMode != mode) mBatchass->changeMode(mode);
		// fps
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
		if (ui::CollapsingHeader("Panels", "11", true, true))
		{
			// Checkbox
			ui::Checkbox("Tex", &showTextures);
			ui::SameLine();
			ui::Checkbox("Fbos", &showFbos);
			ui::SameLine();
			ui::Checkbox("Shada", &showShaders);
			ui::SameLine();
			ui::Checkbox("Audio", &showAudio);
			ui::SameLine();
			ui::Checkbox("WebSox", &showWS);

			ui::Checkbox("Cmd", &showConsole);
			ui::SameLine();
			ui::Checkbox("OSC", &showOSC);
			ui::SameLine();
			ui::Checkbox("MIDI", &showMidi);
			ui::SameLine();
			ui::Checkbox("Test", &showTest);
			if (ui::Button("Save Params"))
			{
				// save warp settings
				mBatchass->getWarpsRef()->save("warps1.xml");
				// save params
				mParameterBag->save();
			}
		}
		if (ui::CollapsingHeader("Render Window", NULL, true, true))
		{
			ui::SliderInt("RenderX", &mParameterBag->mRenderX, 0, 3000); ui::SameLine();
			ui::SliderInt("RenderWidth", &mParameterBag->mRenderWidth, 1024, 3840); ui::SameLine();
			ui::SliderInt("RenderHeight", &mParameterBag->mRenderHeight, 600, 1280);
			if (ui::Button("Create")) { createRenderWindow(); }
			ui::SameLine();
			if (ui::Button("Delete")) { deleteRenderWindows(); }
			ui::SameLine();
			//if (ui::Button("Preview")) { mParameterBag->mPreviewEnabled = !mParameterBag->mPreviewEnabled; }
			mParameterBag->mPreviewEnabled ^= ui::Button("Preview");
			ui::SameLine();
			mParameterBag->iDebug ^= ui::Button("Debug");
			ui::SameLine();
			mParameterBag->mRenderThumbs ^= ui::Button("Thumbs");

		}
		if (ui::CollapsingHeader("Mouse", NULL, true, true))
		{
			ui::Text("Mouse Position: (%.1f,%.1f)", ui::GetIO().MousePos.x, ui::GetIO().MousePos.y); ui::SameLine();
			ui::Text("Mouse %d", ui::GetIO().MouseDown[0]); ui::SameLine();
			mouseGlobal ^= ui::Button("mouse gbl");
			if (mouseGlobal)
			{
				mParameterBag->mRenderPosXY.x = ui::GetIO().MousePos.x; ui::SameLine();
				mParameterBag->mRenderPosXY.y = ui::GetIO().MousePos.y;
				mParameterBag->iMouse.z = ui::GetIO().MouseDown[0];
			}
			else
			{
				ui::SameLine();
				mParameterBag->iMouse.z = ui::Button("mouse click");
			}
			ui::SliderFloat("MouseX", &mParameterBag->mRenderPosXY.x, 0, mParameterBag->mFboWidth);
			ui::SliderFloat("MouseY", &mParameterBag->mRenderPosXY.y, 0, 2048);// mParameterBag->mFboHeight);

		}
		if (ui::CollapsingHeader("Effects", NULL, true, true))
		{
			mParameterBag->mOriginUpperLeft ^= ui::Button("upleft");
			ui::SameLine();
			mParameterBag->iRepeat ^= ui::Button("repeat");
			ui::SameLine();
			if (ui::Button("glitch")) { mParameterBag->controlValues[45] = !mParameterBag->controlValues[45]; }

			if (ui::Button("toggle")) { mParameterBag->controlValues[46] = !mParameterBag->controlValues[46]; }
			ui::SameLine();
			if (ui::Button("vignette")) { mParameterBag->controlValues[47] = !mParameterBag->controlValues[47]; }
			ui::SameLine();
			if (ui::Button("invert")) { mParameterBag->controlValues[48] = !mParameterBag->controlValues[48]; }

			mParameterBag->iGreyScale ^= ui::Button("greyscale");
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

			// iChromatic
			ctrl = 10;
			if (ui::Button("a##chromatic")) { mBatchass->lockChromatic(); }
			ui::SameLine();
			if (ui::Button("t##chromatic")) { mBatchass->tempoChromatic(); }
			ui::SameLine();
			if (ui::Button("x##chromatic")) { mBatchass->resetChromatic(); }
			ui::SameLine();
			if (ui::SliderFloat("chromatic/min/max", &mParameterBag->controlValues[ctrl], mBatchass->minChromatic, mBatchass->maxChromatic))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mParameterBag->controlValues[ctrl] << "}";
			}

			// ratio
			ctrl = 11;
			if (ui::Button("a##ratio")) { mBatchass->lockRatio(); }
			ui::SameLine();
			if (ui::Button("t##ratio")) { mBatchass->tempoRatio(); }
			ui::SameLine();
			if (ui::Button("x##ratio")) { mBatchass->resetRatio(); }
			ui::SameLine();
			if (ui::SliderFloat("ratio/min/max", &mParameterBag->controlValues[ctrl], mBatchass->minRatio, mBatchass->maxRatio))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mParameterBag->controlValues[ctrl] << "}";
			}
			// exposure
			ctrl = 14;
			if (ui::Button("a##exposure")) { mBatchass->lockExposure(); }
			ui::SameLine();
			if (ui::Button("t##exposure")) { mBatchass->tempoExposure(); }
			ui::SameLine();
			if (ui::Button("x##exposure")) { mBatchass->resetExposure(); }
			ui::SameLine();
			if (ui::DragFloat("exposure", &mParameterBag->controlValues[ctrl], 0.1f, mBatchass->minExposure, mBatchass->maxExposure))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mParameterBag->controlValues[ctrl] << "}";
			}

			// zoom
			ctrl = 13;
			if (ui::Button("a##zoom"))
			{
				mBatchass->lockZoom();
			}
			ui::SameLine();
			if (ui::Button("t##zoom")) { mBatchass->tempoZoom(); }
			ui::SameLine();
			if (ui::Button("x##zoom")) { mBatchass->resetZoom(); }
			ui::SameLine();
			if (ui::SliderFloat("zoom", &mParameterBag->controlValues[ctrl], mBatchass->minZoom, mBatchass->maxZoom))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mParameterBag->controlValues[ctrl] << "}";
			}
			// z position
			ctrl = 9;
			if (ui::Button("a##zpos")) { mBatchass->lockZPos(); }
			ui::SameLine();
			if (ui::Button("t##zpos")) { mBatchass->tempoZPos(); }
			ui::SameLine();
			if (ui::Button("x##zpos")) { mBatchass->resetZPos(); }
			ui::SameLine();
			if (ui::SliderFloat("zPosition", &mParameterBag->controlValues[ctrl], mBatchass->minZPos, mBatchass->maxZPos))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mParameterBag->controlValues[ctrl] << "}";
			}

			// rotation speed 
			ctrl = 19;
			if (ui::Button("a##rotationspeed")) { mBatchass->lockRotationSpeed(); }
			ui::SameLine();
			if (ui::Button("t##rotationspeed")) { mBatchass->tempoRotationSpeed(); }
			ui::SameLine();
			if (ui::Button("x##rotationspeed")) { mBatchass->resetRotationSpeed(); }
			ui::SameLine();
			if (ui::SliderFloat("rotationSpeed", &mParameterBag->controlValues[ctrl], mBatchass->minRotationSpeed, mBatchass->maxRotationSpeed))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mParameterBag->controlValues[ctrl] << "}";
			}
			// blend modes
			ui::SliderInt("blendmode", &mParameterBag->iBlendMode, 0, 28);

			// steps
			ctrl = 16;
			if (ui::SliderFloat("steps", &mParameterBag->controlValues[ctrl], 1.0f, 128.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mParameterBag->controlValues[ctrl] << "}";
			}
			// pixelate
			ctrl = 15;
			if (ui::SliderFloat("pixelate", &mParameterBag->controlValues[ctrl], 0.01f, 1.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mParameterBag->controlValues[ctrl] << "}";
			}
			// trixels
			ctrl = 20;
			if (ui::SliderFloat("trixels", &mParameterBag->controlValues[ctrl], 0.00f, 1.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mParameterBag->controlValues[ctrl] << "}";
			}
			// trixels
			ctrl = 21;
			if (ui::SliderFloat("grid", &mParameterBag->controlValues[ctrl], 0.00f, 60.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mParameterBag->controlValues[ctrl] << "}";
			}

			aParams << "]}";
			string strAParams = aParams.str();
			if (strAParams.length() > 60)
			{
				mWebSockets->write(strAParams);
			}
		}
		ui::PopItemWidth();
		if (ui::CollapsingHeader("Colors", NULL, true, true))
		{
			stringstream sParams;
			bool colorChanged = false;
			sParams << "{\"colors\" :[{\"name\" : 0,\"value\" : " << getElapsedFrames() << "}"; // TimeStamp
			// foreground color
			color[0] = mParameterBag->controlValues[1];
			color[1] = mParameterBag->controlValues[2];
			color[2] = mParameterBag->controlValues[3];
			color[3] = mParameterBag->controlValues[4];
			ui::ColorEdit4("f", color);

			for (int i = 0; i < 4; i++)
			{
				if (mParameterBag->controlValues[i + 1] != color[i])
				{
					sParams << ",{\"name\" : " << i + 1 << ",\"value\" : " << color[i] << "}";
					mParameterBag->controlValues[i + 1] = color[i];
					if (i == 0) mOSC->sendOSCColorMessage("/fr", mParameterBag->controlValues[1]);
					if (i == 1) mOSC->sendOSCColorMessage("/fg", mParameterBag->controlValues[2]);
					if (i == 2) mOSC->sendOSCColorMessage("/fb", mParameterBag->controlValues[3]);
					if (i == 3) mOSC->sendOSCColorMessage("/fa", mParameterBag->controlValues[4]);
					colorChanged = true;
				}
			}
			if (colorChanged)
			{
				char col[8];
				int r = mParameterBag->controlValues[1] * 255;
				int g = mParameterBag->controlValues[2] * 255;
				int b = mParameterBag->controlValues[3] * 255;
				ui::ImFormatString(col, IM_ARRAYSIZE(col), "#%02X%02X%02X", r, g, b);
				mWebSockets->write(col);
			}
			// background color
			backcolor[0] = mParameterBag->controlValues[5];
			backcolor[1] = mParameterBag->controlValues[6];
			backcolor[2] = mParameterBag->controlValues[7];
			backcolor[3] = mParameterBag->controlValues[8];
			ui::ColorEdit4("g", backcolor);
			for (int i = 0; i < 4; i++)
			{
				if (mParameterBag->controlValues[i + 5] != backcolor[i])
				{
					sParams << ",{\"name\" : " << i + 5 << ",\"value\" : " << backcolor[i] << "}";
					mParameterBag->controlValues[i + 5] = backcolor[i];
				}

			}

			sParams << "]}";
			string strParams = sParams.str();
			/*if (strParams.length() > 60)
			{
			mWebSockets->write(strParams);
			}*/

		}

		if (ui::CollapsingHeader("Camera", NULL, true, true))
		{
			ui::SliderFloat("Pos.x", &mParameterBag->mRenderPosXY.x, 0.0f, mParameterBag->mRenderWidth);
			ui::SliderFloat("Pos.y", &mParameterBag->mRenderPosXY.y, 0.0f, mParameterBag->mRenderHeight);
			float eyeZ = mParameterBag->mCamera.getEyePoint().z;
			if (ui::SliderFloat("Eye.z", &eyeZ, -500.0f, 1.0f))
			{
				Vec3f eye = mParameterBag->mCamera.getEyePoint();
				eye.z = eyeZ;
				mParameterBag->mCamera.setEyePoint(eye);
			}
			ui::SliderFloat("ABP Bend", &mParameterBag->mBend, -20.0f, 20.0f);

		}


	}
	ui::End();

#pragma endregion Global
	// next line
	xPos = margin;
	yPos += largePreviewH + margin;

#pragma region warps
	if (mParameterBag->mMode == MODE_WARP)
	{
		static bool popup_open = false;
		static int selected_index = -1;
		static int selected_fbo = -1;

		for (int i = 0; i < mBatchass->getWarpsRef()->getWarpsCount(); i++)
		{
			sprintf_s(buf, "Warp %d", i);
			ui::SetNextWindowSize(ImVec2(w, h));
			ui::Begin(buf);
			{
				ui::SetWindowPos(ImVec2((i * (w + inBetween)) + margin, yPos));
				ui::PushID(i);
				ui::Image((void*)mBatchass->getTexturesRef()->getFboTextureId(mParameterBag->mWarpFbos[i].textureIndex), Vec2i(mParameterBag->mPreviewFboWidth, mParameterBag->mPreviewFboHeight));
				ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
				ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
				ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(i / 7.0f, 0.8f, 0.8f));

				sprintf_s(buf, "In:%s##wm%d", warpInputs[mParameterBag->mWarpFbos[i].textureIndex], i);
				if (ImGui::Button(buf))
				{
					popup_open = true;
					selected_index = i;
				}
				ui::PopStyleColor(3);
				ui::PopID();
			}
			ui::End();
		}
		ImGui::SameLine();
		ImGui::Text(selected_fbo == -1 ? "<None>" : warpInputs[selected_fbo]);
		if (popup_open)
		{
			// needed? sprintf_s(buf, "##wmpopup", selected_index);
			ImGui::BeginPopup(&popup_open);
			for (size_t i = 0; i < IM_ARRAYSIZE(warpInputs); i++)
			{
				if (ImGui::Selectable(warpInputs[i], false))
				{
					selected_fbo = i;
					if (selected_fbo > -1) mBatchass->assignFboToWarp(selected_index, selected_fbo);
					// reinit
					popup_open = false;
					selected_index = -1;
					selected_fbo = -1;
				}
			}
			ImGui::EndPopup();

		}
		yPos += h + margin;
	}
#pragma endregion warps
#pragma region textures
	if (showTextures)
	{
		for (int i = 0; i < mBatchass->getTexturesRef()->getTextureCount(); i++)
		{
			ui::SetNextWindowSize(ImVec2(w, h));
			ui::SetNextWindowPos(ImVec2((i * (w + inBetween)) + margin, yPos));
			ui::Begin(textureNames[i], NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
			{
				ui::PushID(i);
				ui::Image((void*)mBatchass->getTexturesRef()->getTexture(i).getId(), Vec2i(mParameterBag->mPreviewFboWidth, mParameterBag->mPreviewFboHeight));
				ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
				ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
				ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
				sprintf_s(buf, "FV##t%d", i);
				if (ui::Button(buf)) mBatchass->getTexturesRef()->flipTexture(i);
				if (ui::IsItemHovered()) ui::SetTooltip("Flip vertically");
				ui::PopStyleColor(3);
				ui::PopID();
			}
			ui::End();
		}
		yPos += h + margin;
	}
#pragma endregion textures
#pragma region library
	if (showShaders)
	{

		static ImGuiTextFilter filter;
		ui::Text("Filter usage:\n"
			"  \"\"         display all lines\n"
			"  \"xxx\"      display lines containing \"xxx\"\n"
			"  \"xxx,yyy\"  display lines containing \"xxx\" or \"yyy\"\n"
			"  \"-xxx\"     hide lines containing \"xxx\"");
		filter.Draw();


		for (int i = 0; i < mBatchass->getShadersRef()->getCount(); i++)
		{
			if (filter.PassFilter(mBatchass->getShadersRef()->getShaderName(i).c_str()))
				ui::BulletText("%s", mBatchass->getShadersRef()->getShaderName(i).c_str());
		}

		xPos = margin;
		for (int i = 0; i < mBatchass->getShadersRef()->getCount(); i++)
		{
			if (filter.PassFilter(mBatchass->getShadersRef()->getShaderName(i).c_str()))
			{

				sprintf_s(buf, "%s##lsh%d", mBatchass->getShadersRef()->getShaderName(i).c_str(), i);
				ui::SetNextWindowSize(ImVec2(w, h));
				ui::SetNextWindowPos(ImVec2(xPos + margin, yPos));
				ui::Begin(buf, NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
				{
					xPos += w + inBetween;
					if (xPos > mParameterBag->MAX * w * 1.8)
					{
						xPos = margin;
						yPos += h + margin;
					}

					ui::PushID(i);
					ui::Image((void*)mBatchass->getTexturesRef()->getShaderThumbTextureId(i), Vec2i(mParameterBag->mPreviewFboWidth, mParameterBag->mPreviewFboHeight));

					//ui::Columns(2, "lr", false);
					// left
					if (mParameterBag->mLeftFragIndex == i)
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 1.0f, 0.5f));
					}
					else
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 0.1f, 0.1f));

					}
					ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.0f, 0.7f, 0.7f));
					ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.0f, 0.8f, 0.8f));
					sprintf_s(buf, "L##s%d", i);
					if (ui::Button(buf)) mParameterBag->mLeftFragIndex = i;
					if (ui::IsItemHovered()) ui::SetTooltip("Set shader to left");
					ui::PopStyleColor(3);
					//ui::NextColumn();
					ui::SameLine();
					// right
					if (mParameterBag->mRightFragIndex == i)
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.3f, 1.0f, 0.5f));
					}
					else
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 0.1f, 0.1f));
					}
					ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.3f, 0.7f, 0.7f));
					ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.3f, 0.8f, 0.8f));
					sprintf_s(buf, "R##s%d", i);
					if (ui::Button(buf)) mParameterBag->mRightFragIndex = i;
					if (ui::IsItemHovered()) ui::SetTooltip("Set shader to right");
					ui::PopStyleColor(3);
					//ui::NextColumn();
					ui::SameLine();

					// warp1
					if (mParameterBag->mWarp1FragIndex == i)
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.16f, 1.0f, 0.5f));
					}
					else
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 0.1f, 0.1f));
					}
					ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.16f, 0.7f, 0.7f));
					ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.16f, 0.8f, 0.8f));
					sprintf_s(buf, "W1##s%d", i);
					if (ui::Button(buf)) mParameterBag->mWarp1FragIndex = i;
					if (ui::IsItemHovered()) ui::SetTooltip("Set warp 1 shader");
					ui::PopStyleColor(3);
					//ui::NextColumn();
					ui::SameLine();

					// warp2
					if (mParameterBag->mWarp2FragIndex == i)
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.77f, 1.0f, 0.5f));
					}
					else
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 0.1f, 0.1f));
					}
					ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.77f, 0.7f, 0.7f));
					ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.77f, 0.8f, 0.8f));
					sprintf_s(buf, "W2##s%d", i);
					if (ui::Button(buf)) mParameterBag->mWarp2FragIndex = i;
					if (ui::IsItemHovered()) ui::SetTooltip("Set warp 2 shader");
					ui::PopStyleColor(3);
					//ui::NextColumn();
					ui::SameLine();
					// preview
					if (mParameterBag->mPreviewFragIndex == i)
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.6f, 1.0f, 0.5f));
					}
					else
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 0.1f, 0.1f));
					}
					ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.6f, 0.7f, 0.7f));
					ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.6f, 0.8f, 0.8f));
					sprintf_s(buf, "P##s%d", i);
					if (ui::Button(buf)) mParameterBag->mPreviewFragIndex = i;
					if (ui::IsItemHovered()) ui::SetTooltip("Preview shader");
					ui::PopStyleColor(3);
					//ui::NextColumn();

					ui::PopID();

					//ui::Columns(1);
				}
				ui::End();
			} // if filtered

		} // for
		xPos = margin;
		yPos += h + margin;
	}
#pragma endregion library
#pragma region fbos

	if (showFbos)
	{
		for (int i = 0; i < mBatchass->getTexturesRef()->getFboCount(); i++)
		{
			ui::SetNextWindowSize(ImVec2(w, h));
			ui::SetNextWindowPos(ImVec2((i * (w + inBetween)) + margin, yPos));
			ui::Begin(fboNames[i], NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
			{
				//if (i > 0) ui::SameLine();
				ui::PushID(i);
				ui::Image((void*)mBatchass->getTexturesRef()->getFboTextureId(i), Vec2i(mParameterBag->mPreviewFboWidth, mParameterBag->mPreviewFboHeight));
				ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
				ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
				ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(i / 7.0f, 0.8f, 0.8f));

				sprintf_s(buf, "FV##f%d", i);
				if (ui::Button(buf)) mBatchass->getTexturesRef()->flipFbo(i);
				if (ui::IsItemHovered()) ui::SetTooltip("Flip vertically");

				ui::PopStyleColor(3);
				ui::PopID();
			}
			ui::End();
		}
		yPos += h + margin;
	}
#pragma endregion fbos

#pragma region OSC

	if (showOSC)
	{
		ui::SetNextWindowSize(ImVec2(largeW, largeH), ImGuiSetCond_Once);
		ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
		ui::Begin("OSC router");
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
		xPos += largeW + margin;
	}
#pragma endregion OSC

	if (showTest)
	{
		ui::ShowTestWindow();
		ui::ShowStyleEditor();

	}
	gl::disableAlphaBlending();
}
void BatchassApp::drawRender()
{
	// clear
	gl::clear();
	// shaders			
	//gl::setViewport(getWindowBounds());
	gl::setViewport(allRenderWindows[0].mWRef->getBounds());
	gl::enableAlphaBlending();
	//20140703 gl::setMatricesWindow(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight, mParameterBag->mOriginUpperLeft);//NEW 20140620, needed?
	gl::setMatricesWindow(mParameterBag->mFboWidth, mParameterBag->mFboHeight);// , false);
	switch (mParameterBag->mMode)
	{
	case MODE_AUDIO:
		gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mAudioFboIndex));
		break;
	case MODE_WARP:
		mBatchass->getWarpsRef()->draw();
		break;
	case MODE_SPHERE:
		gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mSphereFboIndex));
		break;
	case MODE_MESH:
		gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mMeshFboIndex));
		break;
	case MODE_VERTEXSPHERE:
		gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mVertexSphereFboIndex));
		break;
	case MODE_ABP:
		gl::draw(mBatchass->getTexturesRef()->getFboTexture(mParameterBag->mABPFboIndex));
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
		filename = mBatchass->getShadersRef()->getFragFileName() + ".jpg";
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
	if (mParameterBag->mMode == mParameterBag->MODE_WARP) mBatchass->getWarpsRef()->keyUp(event);
}

void BatchassApp::fileDrop(FileDropEvent event)
{
	int index;
	string ext = "";
	// use the last of the dropped files
	boost::filesystem::path mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	int dotIndex = mFile.find_last_of(".");
	int slashIndex = mFile.find_last_of("\\");

	if (dotIndex != std::string::npos && dotIndex > slashIndex) ext = mFile.substr(mFile.find_last_of(".") + 1);
	index = (int)(event.getX() / (margin + mParameterBag->mPreviewFboWidth + inBetween));// +1;
	//mBatchass->log(mFile + " dropped, currentSelectedIndex:" + toString(mParameterBag->currentSelectedIndex) + " x: " + toString(event.getX()) + " PreviewFboWidth: " + toString(mParameterBag->mPreviewFboWidth));

	if (ext == "wav" || ext == "mp3")
	{
		mAudio->loadWaveFile(mFile);
	}
	else if (ext == "png" || ext == "jpg")
	{
		if (index < 1) index = 1;
		if (index > 3) index = 3;
		//mTextures->loadImageFile(mParameterBag->currentSelectedIndex, mFile);
		mBatchass->getTexturesRef()->loadImageFile(index, mFile);
	}
	else if (ext == "glsl")
	{
		//mShaders->incrementPreviewIndex();
		//mUserInterface->mLibraryPanel->addShader(mFile);

		if (index < 3) index = 3;
		int rtn = mBatchass->getShadersRef()->loadPixelFragmentShaderAtIndex(mFile, index);
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
	else if (ext == "fs")
	{
		//mShaders->incrementPreviewIndex();
		mBatchass->getShadersRef()->loadFragmentShader(mPath);
	}
	else if (ext == "xml")
	{
		mBatchass->getWarpsRef()->loadWarps(mFile);
	}
	else if (ext == "patchjson")
	{
		// try loading patch
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
	else if (ext == "txt")
	{
		// try loading shader parts
		if (mBatchass->getShadersRef()->loadTextFile(mFile))
		{

		}
	}
	else if (ext == "")
	{
		// try loading image sequence from dir
		//mTextures->createFromDir(mFile + "/");
		mBatchass->getShadersRef()->createThumbsFromDir(mFile + "/");
	}
	/*if (!loaded && ext == "frag")
	{

	//mShaders->incrementPreviewIndex();

	if (mShaders->loadPixelFrag(mFile))
	{
	mParameterBag->controlValues[13] = 1.0f;
	timeline().apply(&mTimer, 1.0f, 1.0f).finishFn([&]{ save(); });
	}
	if (mCodeEditor) mCodeEditor->fileDrop(event);
	}*/
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
		mBatchass->getWarpsRef()->save();
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
	case MODE_VERTEXSPHERE:
		mVertexSphere->update();
		break;

	default:
		break;
	}
	mSpout->update();
	mBatchass->update();
	mABP->update();
	mAudio->update();
	//mUI->update();
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
}

void BatchassApp::resize()
{
	mBatchass->getWarpsRef()->resize();
	/*ui::disconnectWindow(mMainWindow);
	ui::connectWindow(mMainWindow);
	ui::initialize();*/
}

void BatchassApp::mouseMove(MouseEvent event)
{
	if (mParameterBag->mMode == mParameterBag->MODE_WARP) mBatchass->getWarpsRef()->mouseMove(event);
}

void BatchassApp::mouseDown(MouseEvent event)
{
	if (mParameterBag->mMode == mParameterBag->MODE_WARP) mBatchass->getWarpsRef()->mouseDown(event);
	if (mParameterBag->mMode == mParameterBag->MODE_MESH) mMeshes->mouseDown(event);
	if (mParameterBag->mMode == mParameterBag->MODE_AUDIO) mAudio->mouseDown(event);
}

void BatchassApp::mouseDrag(MouseEvent event)
{
	if (mParameterBag->mMode == mParameterBag->MODE_WARP) mBatchass->getWarpsRef()->mouseDrag(event);
	if (mParameterBag->mMode == mParameterBag->MODE_MESH) mMeshes->mouseDrag(event);
	if (mParameterBag->mMode == mParameterBag->MODE_AUDIO) mAudio->mouseDrag(event);
}

void BatchassApp::mouseUp(MouseEvent event)
{
	if (mParameterBag->mMode == mParameterBag->MODE_WARP) mBatchass->getWarpsRef()->mouseUp(event);
	if (mParameterBag->mMode == mParameterBag->MODE_AUDIO) mAudio->mouseUp(event);
}
void BatchassApp::mouseWheel(MouseEvent event)
{
	if (mParameterBag->mMode == mParameterBag->MODE_MESH) mMeshes->mouseWheel(event);
}
void BatchassApp::keyDown(KeyEvent event)
{
	if (mParameterBag->mMode == mParameterBag->MODE_WARP)
	{
		mBatchass->getWarpsRef()->keyDown(event);
	}
	else
	{

		switch (event.getCode())
		{
		case ci::app::KeyEvent::KEY_n:
			mBatchass->changeMode(mParameterBag->MODE_MIX);
			break;
		case ci::app::KeyEvent::KEY_a:
			mBatchass->changeMode(mParameterBag->MODE_AUDIO);
			break;
		case ci::app::KeyEvent::KEY_s:
			if (event.isControlDown())
			{
				// save warp settings
				mBatchass->getWarpsRef()->save("warps2.xml");
				// save params
				mParameterBag->save();
			}
			else
			{
				mBatchass->changeMode(mParameterBag->MODE_SPHERE);
			}
			break;
		case ci::app::KeyEvent::KEY_w:
			mBatchass->changeMode(mParameterBag->MODE_WARP);
			break;
		case ci::app::KeyEvent::KEY_m:
			mBatchass->changeMode(mParameterBag->MODE_MESH);
			break;
		case ci::app::KeyEvent::KEY_o:
			mParameterBag->mOriginUpperLeft = !mParameterBag->mOriginUpperLeft;
			break;
		case ci::app::KeyEvent::KEY_r:
			mParameterBag->controlValues[1] += 0.2;
			if (mParameterBag->controlValues[1] > 0.9) mParameterBag->controlValues[1] = 0.0;
			break;
		case ci::app::KeyEvent::KEY_g:
			mParameterBag->controlValues[2] += 0.2;
			if (mParameterBag->controlValues[2] > 0.9) mParameterBag->controlValues[2] = 0.0;
			break;
		case ci::app::KeyEvent::KEY_b:
			mParameterBag->controlValues[3] += 0.2;
			if (mParameterBag->controlValues[3] > 0.9) mParameterBag->controlValues[3] = 0.0;
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
		case ci::app::KeyEvent::KEY_h:
			if (mParameterBag->mCursorVisible)
			{
				removeUI = true;
				hideCursor();
			}
			else
			{
				removeUI = false;
				showCursor();
			}
			mParameterBag->mCursorVisible = !mParameterBag->mCursorVisible;
			break;
		case ci::app::KeyEvent::KEY_ESCAPE:
			mParameterBag->save();
			//mBatchass->shutdownLoader(); // Not used yet(loading shaders in a different thread
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
}

// From imgui by Omar Cornut
void BatchassApp::ShowAppConsole(bool* opened)
{
	mConsole->Run("Console", opened);
}
CINDER_APP_BASIC(BatchassApp, RendererGl)
