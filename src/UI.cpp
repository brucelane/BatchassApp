#include "UI.h"

using namespace Reymenta;
/*
4:3 w h
btn: 48 36
tex: 76 57
pvw: 156 88
*/
UI::UI(ParameterBagRef aParameterBag, ShadersRef aShadersRef, TexturesRef aTexturesRef, WindowRef aWindow)
{
	mParameterBag = aParameterBag;
	mShaders = aShadersRef;
	mTextures = aTexturesRef;
	mWindow = aWindow;

	mCbMouseDown = mWindow->getSignalMouseDown().connect(0, std::bind(&UI::mouseDown, this, std::placeholders::_1));
	mCbKeyDown = mWindow->getSignalKeyDown().connect(0, std::bind(&UI::keyDown, this, std::placeholders::_1));

	mVisible = true;
	mSetupComplete = false;
	mTimer = 0.0f;
	// tempo
	counter = 0;
	//startTime = timer.getSeconds();
	//currentTime = timer.getSeconds();
	mParameterBag->iDeltaTime = 60 / mParameterBag->mTempo;
	previousTime = 0.0f;
	beatIndex = 0;
	//timer.start();
}

UIRef UI::create(ParameterBagRef aParameterBag, ShadersRef aShadersRef, TexturesRef aTexturesRef, app::WindowRef aWindow)
{
	return shared_ptr<UI>(new UI(aParameterBag, aShadersRef, aTexturesRef, aWindow));
}

void UI::setup()
{
	// load custom fonts (I do this once, in my UserInterface class)
	// UI fonts
	mParameterBag->mLabelFont = Font(loadAsset("HelveticaNeue.ttf"), 14 * 2);
	mParameterBag->mSmallLabelFont = Font(loadAsset("HelveticaNeue.ttf"), 12 * 2);
	//mParameterBag->mIconFont = Font(loadResource(RES_GLYPHICONS_REGULAR), 18 * 2);
	mParameterBag->mHeaderFont = Font(loadAsset("HelveticaNeueUltraLight.ttf"), 24 * 2);
	mParameterBag->mBodyFont = Font(loadAsset("Garamond.ttf"), 19 * 2);
	mParameterBag->mFooterFont = Font(loadAsset("GaramondItalic.ttf"), 14 * 2);

	setupMiniControl();

	mSetupComplete = true;
}
void UI::setupMiniControl()
{
	mMiniControl = UIController::create("{ \"depth\":100, \"width\":1400, \"height\":50, \"fboNumSamples\":0, \"panelColor\":\"0x44282828\", \"defaultBackgroundColor\":\"0xFF0d0d0d\", \"defaultNameColor\":\"0xFF90a5b6\", \"defaultStrokeColor\":\"0xFF282828\", \"activeStrokeColor\":\"0xFF919ea7\" }");
	mMiniControl->DEFAULT_UPDATE_FREQUENCY = 12;
	mMiniControl->setFont("label", mParameterBag->mLabelFont);
	mMiniControl->setFont("smallLabel", mParameterBag->mSmallLabelFont);
	mMiniControl->setFont("icon", mParameterBag->mIconFont);
	mMiniControl->setFont("header", mParameterBag->mHeaderFont);
	mMiniControl->setFont("body", mParameterBag->mBodyFont);
	mMiniControl->setFont("footer", mParameterBag->mFooterFont);
	mPanels.push_back(mMiniControl);

	// tempo
	/*tempoMvg = mMiniControl->addMovingGraphButton("tempo", &mParameterBag->iTempoTime, std::bind(&UI::tapTempo, this, std::placeholders::_1), "{ \"clear\":false,\"width\":76, \"min\":0.0, \"max\":2.001 }");
	sliderTimeFactor = mMiniControl->addSlider("time", &mParameterBag->iTimeFactor, "{ \"min\":0.01, \"max\":32.0, \"clear\":false }");
	for (int i = 0; i < 8; i++)
	{
		mMiniControl->addButton(toString(i), std::bind(&UI::setTimeFactor, this, i, std::placeholders::_1), "{ \"clear\":false, \"width\":9, \"stateless\":false, \"group\":\"timefactor\", \"exclusive\":true }");
	}
	mMiniControl->addButton("time\ntempo", std::bind(&UI::toggleUseTimeWithTempo, this, std::placeholders::_1), "{ \"clear\":false, \"width\":56, \"stateless\":false}");
	*/
	// Color Sliders
	mMiniControl->addLabel("Draw color", "{ \"clear\":false }");

	// fr
	sliderRed = mMiniControl->addToggleSlider("R", &mParameterBag->controlValues[1], "a", std::bind(&UI::lockFR, this, std::placeholders::_1), "{ \"width\":36, \"clear\":false, \"handleVisible\":false, \"vertical\":true, \"nameColor\":\"0xEEFF0000\" }", "{ \"width\":9, \"stateless\":false, \"group\":\"fr\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("t", std::bind(&UI::tempoFR, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"fr\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("x", std::bind(&UI::resetFR, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"fr\", \"exclusive\":true, \"pressed\":true, \"clear\":false }");

	// fg
	sliderGreen = mMiniControl->addToggleSlider("G", &mParameterBag->controlValues[2], "a", std::bind(&UI::lockFG, this, std::placeholders::_1), "{ \"width\":36, \"clear\":false, \"handleVisible\":false, \"vertical\":true, \"nameColor\":\"0xEE00FF00\" }", "{ \"width\":9, \"stateless\":false, \"group\":\"fg\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("t", std::bind(&UI::tempoFG, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"fg\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("x", std::bind(&UI::resetFG, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"fg\", \"exclusive\":true, \"pressed\":true, \"clear\":false }");
	// fb
	sliderBlue = mMiniControl->addToggleSlider("B", &mParameterBag->controlValues[3], "a", std::bind(&UI::lockFB, this, std::placeholders::_1), "{ \"width\":36, \"clear\":false, \"handleVisible\":false, \"vertical\":true, \"nameColor\":\"0xEE0000FF\" }", "{ \"width\":9, \"stateless\":false, \"group\":\"fb\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("t", std::bind(&UI::tempoFB, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"fb\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("x", std::bind(&UI::resetFB, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"fb\", \"exclusive\":true, \"pressed\":true, \"clear\":false }");
	// fa
	sliderAlpha = mMiniControl->addToggleSlider("A", &mParameterBag->controlValues[4], "a", std::bind(&UI::lockFA, this, std::placeholders::_1), "{ \"width\":36, \"clear\":false, \"handleVisible\":false, \"vertical\":true, \"nameColor\":\"0xFFFFFFFF\" }", "{ \"width\":9, \"stateless\":false, \"group\":\"fa\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("t", std::bind(&UI::tempoFA, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"fa\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("x", std::bind(&UI::resetFA, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"fa\", \"exclusive\":true, \"pressed\":true, \"clear\":false }");

	mMiniControl->addLabel("Back color", "{ \"clear\":false }");
	//br
	sliderBackgroundRed = mMiniControl->addToggleSlider("R", &mParameterBag->controlValues[5], "a", std::bind(&UI::lockBR, this, std::placeholders::_1), "{ \"width\":36, \"clear\":false, \"handleVisible\":false, \"vertical\":true, \"nameColor\":\"0xEEFF0000\" }", "{ \"width\":9, \"stateless\":false, \"group\":\"br\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("t", std::bind(&UI::tempoBR, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"br\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("x", std::bind(&UI::resetBR, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"br\", \"exclusive\":true, \"pressed\":true, \"clear\":false }");
	//bg
	sliderBackgroundGreen = mMiniControl->addToggleSlider("G", &mParameterBag->controlValues[6], "a", std::bind(&UI::lockBG, this, std::placeholders::_1), "{ \"width\":36, \"clear\":false, \"handleVisible\":false, \"vertical\":true, \"nameColor\":\"0xEE00FF00\" }", "{ \"width\":9, \"stateless\":false, \"group\":\"bg\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("t", std::bind(&UI::tempoBG, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"bg\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("x", std::bind(&UI::resetBG, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"bg\", \"exclusive\":true, \"pressed\":true, \"clear\":false }");
	//bb
	sliderBackgroundBlue = mMiniControl->addToggleSlider("B", &mParameterBag->controlValues[7], "a", std::bind(&UI::lockBB, this, std::placeholders::_1), "{ \"width\":36, \"clear\":false, \"handleVisible\":false, \"vertical\":true, \"nameColor\":\"0xEE0000FF\" }", "{ \"width\":9, \"stateless\":false, \"group\":\"bb\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("t", std::bind(&UI::tempoBB, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"bb\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("x", std::bind(&UI::resetBB, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"bb\", \"exclusive\":true, \"pressed\":true, \"clear\":false }");
	//ba
	sliderBackgroundAlpha = mMiniControl->addToggleSlider("A", &mParameterBag->controlValues[8], "a", std::bind(&UI::lockBA, this, std::placeholders::_1), "{ \"width\":36, \"clear\":false, \"handleVisible\":false, \"vertical\":true, \"nameColor\":\"0xFFFFFFFF\" }", "{ \"width\":9, \"stateless\":false, \"group\":\"ba\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("t", std::bind(&UI::tempoBA, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"ba\", \"exclusive\":true, \"clear\":false }");
	mMiniControl->addButton("x", std::bind(&UI::resetBA, this, std::placeholders::_1), "{ \"width\":9, \"stateless\":false, \"group\":\"ba\", \"exclusive\":true, \"pressed\":true }");



	// Textures select/layers
	// Button Group
	/*for (int i = 0; i < mTextures->getTextureCount(); i++)
	{
		buttonLayer[i] = mMiniControl->addButton(toString(i), std::bind(&UI::setLayer, this, i, std::placeholders::_1), "{ \"clear\":false, \"width\":48, \"stateless\":false, \"group\":\"layer\", \"exclusive\":true }");
	}
	mMiniControl->addButton("Black", std::bind(&UI::InstantBlack, this, std::placeholders::_1), "{ \"width\":72 }");
	// 2D Sliders
	sliderLeftRenderXY = mMiniControl->addSlider2D("LeftXY", &mParameterBag->mLeftRenderXY, "{ \"clear\":false, \"minX\":-0.5, \"maxX\":0.5, \"minY\":-0.5, \"maxY\":0.5, \"width\":" + toString(mParameterBag->mPreviewWidth) + " }");
	sliderRightRenderXY = mMiniControl->addSlider2D("RightXY", &mParameterBag->mRightRenderXY, "{ \"clear\":false, \"minX\":-0.5, \"maxX\":0.5, \"minY\":-0.5, \"maxY\":0.5, \"width\":" + toString(mParameterBag->mPreviewWidth) + " }");
	sliderMixRenderXY = mMiniControl->addSlider2D("MixXY", &mParameterBag->mPreviewRenderXY, "{ \"clear\":false, \"minX\":-0.5, \"maxX\":0.5, \"minY\":-0.5, \"maxY\":0.5, \"width\":" + toString(mParameterBag->mPreviewWidth) + " }");
	sliderPreviewRenderXY = mMiniControl->addSlider2D("PreviewFragXY", &mParameterBag->mPreviewFragXY, "{ \"clear\":false, \"minX\":-0.5, \"maxX\":0.5, \"minY\":-0.5, \"maxY\":0.5, \"width\":" + toString(mParameterBag->mPreviewWidth) + " }");

	sliderRenderXY = mMiniControl->addSlider2D("renderXY", &mParameterBag->mRenderXY, "{ \"clear\":false, \"minX\":-2.0, \"maxX\":2.0, \"minY\":-2.0, \"maxY\":2.0, \"width\":" + toString(mParameterBag->mPreviewWidth) + " }");

	string posXY = toString(mParameterBag->mFboWidth) + ", \"minY\":" + toString(mParameterBag->mFboHeight) + ", \"maxY\":0.0, \"width\":" + toString(mParameterBag->mPreviewWidth) + " }";
	sliderRenderPosXY = mMiniControl->addSlider2D("renderPosXY", &mParameterBag->mRenderPosXY, "{ \"minX\":0.0, \"maxX\":" + posXY);*/
}

void UI::setTimeFactor(const int &aTimeFactor, const bool &pressed)
{
	if (pressed)
	{
		switch (aTimeFactor)
		{
		case 0:
			mParameterBag->iTimeFactor = 0.0001;
			break;
		case 1:
			mParameterBag->iTimeFactor = 0.125;
			break;
		case 2:
			mParameterBag->iTimeFactor = 0.25;
			break;
		case 3:
			mParameterBag->iTimeFactor = 0.5;
			break;
		case 4:
			mParameterBag->iTimeFactor = 0.75;
			break;
		case 5:
			mParameterBag->iTimeFactor = 1.0;
			break;
		case 6:
			mParameterBag->iTimeFactor = 2.0;
			break;
		case 7:
			mParameterBag->iTimeFactor = 4.0;
			break;
		case 8:
			mParameterBag->iTimeFactor = 16.0;
			break;
		default:
			mParameterBag->iTimeFactor = 1.0;
			break;

		}
	}
}

/*void UI::setLayer(const int &aLayer, const bool &pressed)
{
	if (pressed)
	{
		mParameterBag->currentSelectedIndex = aLayer;
		mParameterBag->iChannels[0] = aLayer;
		mParameterBag->isUIDirty = true;
	}
}*/

void UI::draw()
{
	// normal alpha blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (mVisible)
	{
		for (auto & panel : mPanels) panel->draw();		
	}
	// needed because of what the ping pong fbo is doing, at least
	gl::disableAlphaBlending();

}
string UI::formatNumber(float f)
{
	f *= 100;
	f = (float)((int)f);
	return toString(f);
}
void UI::update()
{

	for (auto & panel : mPanels) panel->update();

	if (getElapsedFrames() % mParameterBag->mUIRefresh * mParameterBag->mUIRefresh * mParameterBag->mUIRefresh == 0)
	{
		if (mVisible)
		{
			/*if (!mParameterBag->mOptimizeUI)
			{
				tempoMvg->setName(toString(floor(mParameterBag->mTempo)) + "bpm\n" + toString(floor(mParameterBag->iDeltaTime * 1000)) + "ms " + formatNumber(mParameterBag->iTempoTime));
				
				sliderTimeFactor->setName(formatNumber(mParameterBag->iTimeFactor));
			}*/
			if (mParameterBag->controlValues[12] == 0.0) mParameterBag->controlValues[12] = 0.01;

			// iColor slider
			sliderRed->setBackgroundColor(ColorA(mParameterBag->controlValues[1], 0, 0));
			sliderGreen->setBackgroundColor(ColorA(0, mParameterBag->controlValues[2], 0));
			sliderBlue->setBackgroundColor(ColorA(0, 0, mParameterBag->controlValues[3]));
			sliderAlpha->setBackgroundColor(ColorA(mParameterBag->controlValues[1], mParameterBag->controlValues[2], mParameterBag->controlValues[3], mParameterBag->controlValues[4]));
			// iBackColor sliders
			sliderBackgroundRed->setBackgroundColor(ColorA(mParameterBag->controlValues[5], 0, 0));
			sliderBackgroundGreen->setBackgroundColor(ColorA(0, mParameterBag->controlValues[6], 0));
			sliderBackgroundBlue->setBackgroundColor(ColorA(0, 0, mParameterBag->controlValues[7]));
			sliderBackgroundAlpha->setBackgroundColor(ColorA(mParameterBag->controlValues[5], mParameterBag->controlValues[6], mParameterBag->controlValues[7], mParameterBag->controlValues[8]));

			// other sliders
			//labelXY->setName(toString(int(mParameterBag->mRenderXY.x * 100) / 100) + "x" + toString(int(mParameterBag->mRenderXY.y * 100) / 100));
			//labelPosXY->setName("mouse " + toString(floor(mParameterBag->mRenderPosXY.x)) + "x" + toString(floor(mParameterBag->mRenderPosXY.y)));

			// fps
			//fpsMvg->setName(toString(floor(mParameterBag->iFps)) + " fps");

			/*if (mParameterBag->isUIDirty)
			{
				// do this once
				mParameterBag->isUIDirty = false;
			}
			for (int i = 0; i < mTextures->getTextureCount(); i++)
			{
				buttonLayer[i]->setBackgroundTexture(mTextures->getTexture(i));
			}
			sliderLeftRenderXY->setBackgroundTexture(mTextures->getFboTexture(mParameterBag->mLeftFboIndex));
			sliderRightRenderXY->setBackgroundTexture(mTextures->getFboTexture(mParameterBag->mRightFboIndex));
			sliderMixRenderXY->setBackgroundTexture(mTextures->getFboTexture(mParameterBag->mMixFboIndex));
			sliderPreviewRenderXY->setBackgroundTexture(mTextures->getFboTexture(mParameterBag->mCurrentPreviewFboIndex));
			sliderRenderXY->setBackgroundTexture(mTextures->getFboTexture(mParameterBag->mAudioFboIndex));
			sliderRenderPosXY->setBackgroundTexture(mTextures->getFboTexture(mParameterBag->mMixFboIndex));*/

		}
	}
}
void UI::tempoFR(const bool &pressed)
{
	mParameterBag->tFR = pressed;
	if (!pressed) resetFR(pressed);
}
void UI::resetFR(const bool &pressed)
{
	mParameterBag->mLockFR = mParameterBag->tFR = false;
}
void UI::tempoFG(const bool &pressed)
{
	mParameterBag->tFG = pressed;
	if (!pressed) resetFG(pressed);
}
void UI::resetFG(const bool &pressed)
{
	mParameterBag->mLockFG = mParameterBag->tFG = false;
}
void UI::tempoFB(const bool &pressed)
{
	mParameterBag->tFB = pressed;
	if (!pressed) resetFB(pressed);
}
void UI::resetFB(const bool &pressed)
{
	mParameterBag->mLockFB = mParameterBag->tFB = false;
}
void UI::tempoFA(const bool &pressed)
{
	mParameterBag->tFA = pressed;
	if (!pressed) resetFA(pressed);
}
void UI::resetFA(const bool &pressed)
{
	mParameterBag->mLockFA = mParameterBag->tFA = false;
}
void UI::tempoBR(const bool &pressed)
{
	mParameterBag->tBR = pressed;
	if (!pressed) resetBR(pressed);
}
void UI::resetBR(const bool &pressed)
{
	mParameterBag->mLockBR = mParameterBag->tBR = false;
}
void UI::tempoBG(const bool &pressed)
{
	mParameterBag->tBG = pressed;
	if (!pressed) resetBG(pressed);
}
void UI::resetBG(const bool &pressed)
{
	mParameterBag->mLockBG = mParameterBag->tBG = false;
}
void UI::tempoBB(const bool &pressed)
{
	mParameterBag->tBB = pressed;
	if (!pressed) resetBB(pressed);
}
void UI::resetBB(const bool &pressed)
{
	mParameterBag->mLockBB = mParameterBag->tBB = false;
}
void UI::tempoBA(const bool &pressed)
{
	mParameterBag->tBA = pressed;
	if (!pressed) resetBA(pressed);
}
void UI::resetBA(const bool &pressed)
{
	mParameterBag->mLockBA = mParameterBag->tBA = false;
}

void UI::resize()
{
	for (auto & panel : mPanels) panel->resize();
}

void UI::mouseDown(MouseEvent &event)
{
}

void UI::keyDown(KeyEvent &event)
{
	/*switch (event.getChar())
	{
	// toggle params & mouse
	case 'h':
	toggleVisibility();
	break;
	}*/
}

void UI::show()
{
	mVisible = true;
	AppBasic::get()->showCursor();
}

void UI::hide()
{
	mVisible = false;
	//AppBasic::get()->hideCursor();
}
/*void UI::InstantBlack(const bool &pressed)
{
	mParameterBag->controlValues[1] = mParameterBag->controlValues[2] = mParameterBag->controlValues[3] = mParameterBag->controlValues[4] = 0.0;
	mParameterBag->controlValues[5] = mParameterBag->controlValues[6] = mParameterBag->controlValues[7] = mParameterBag->controlValues[8] = 0.0;

}*/

void UI::toggleUseTimeWithTempo(const bool &pressed)
{
	mParameterBag->mUseTimeWithTempo = pressed;
}

// tempo
/*void UI::tapTempo(const bool &pressed)
{
	startTime = currentTime = timer.getSeconds();

	timer.stop();
	timer.start();

	// check for out of time values - less than 50% or more than 150% of from last "TAP and whole time budder is going to be discarded....
	if (counter > 2 && (buffer.back() * 1.5 < currentTime || buffer.back() * 0.5 > currentTime))
	{
		buffer.clear();
		counter = 0;
		averageTime = 0;
	}
	if (counter >= 1)
	{
		buffer.push_back(currentTime);
		calculateTempo();
	}
	counter++;
}
void UI::calculateTempo()
{
	// NORMAL AVERAGE
	double tAverage = 0;
	for (int i = 0; i < buffer.size(); i++)
	{
		tAverage += buffer[i];
	}
	averageTime = (double)(tAverage / buffer.size());
	mParameterBag->iDeltaTime = averageTime;
	mParameterBag->mTempo = 60 / averageTime;
}*/
