#include "WarpWrapper.h"

using namespace Reymenta;

WarpWrapper::WarpWrapper(ParameterBagRef aParameterBag, TexturesRef aTexturesRef, ShadersRef aShadersRef)
{
	mParameterBag = aParameterBag;
	mShaders = aShadersRef;
	mTextures = aTexturesRef;
	// instanciate the logger class
	log = Logger::create("WarpWrapperLog.txt");
	log->logTimedString("WarpWrapper constructor");

	mUseBeginEnd = false;
	// initialize warps
	fs::path settings = getAssetPath("") / "warps.xml";
	if (fs::exists(settings))
	{
		// load warp settings from file if one exists
		mWarps = Warp::readSettings(loadFile(settings));
	}
	else
	{
		// otherwise create a warp from scratch
		mWarps.push_back(WarpPerspectiveBilinear::create());
	}
	log->logTimedString("Warps size" + mWarps.size());
	mSrcArea = mTextures->getTexture(1).getBounds();
	/*for (int i = mWarps.size(); i < mTextures->getTextureCount(); i++)
	{
		mWarps.push_back(WarpPerspectiveBilinear::create());
	}*/
	// adjust the content size of the warps
	Warp::setSize(mWarps, mTextures->getFboTexture(mParameterBag->mMixFboIndex).getSize());

	gl::enableDepthRead();
	gl::enableDepthWrite();
}
void WarpWrapper::createWarp()
{
	// create a new warp
	mWarps.push_back(WarpPerspectiveBilinear::create());
}

void WarpWrapper::load()
{
	fs::path settings = getAssetPath("") / "warps.xml";
	Warp::writeSettings(mWarps, writeFile(settings));
}
void WarpWrapper::loadWarps(const std::string &filename)
{
	fs::path settings = filename;
	if (fs::exists(settings))
	{
		// load warp settings from file if one exists
		mWarps = Warp::readSettings(loadFile(settings));
	}
}
void WarpWrapper::save(const std::string &filename)
{
	fs::path settings = getAssetPath("") / filename;
	Warp::writeSettings(mWarps, writeFile(settings));
}

void WarpWrapper::resize()
{
	// tell the warps our window has been resized, so they properly scale up or down
	Warp::handleResize(mWarps);

}
void WarpWrapper::mouseMove(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseMove(mWarps, event))
	{
		// let your application perform its mouseMove handling here
	}
}
void WarpWrapper::mouseDown(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDown(mWarps, event))
	{
		// let your application perform its mouseDown handling here
	}
}
void WarpWrapper::mouseDrag(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDrag(mWarps, event))
	{
		// let your application perform its mouseDrag handling here
	}
}
void WarpWrapper::mouseUp(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseUp(mWarps, event))
	{
		// let your application perform its mouseUp handling here
	}
}
void WarpWrapper::keyDown(KeyEvent event)
{
	// pass this key event to the warp editor first
	if (!Warp::handleKeyDown(mWarps, event))
	{
		// warp editor did not handle the key, so handle it here
		switch (event.getCode())
		{
		case KeyEvent::KEY_f:
			// toggle full screen
			setFullScreen(!isFullScreen());
			break;
		case KeyEvent::KEY_w:
			// toggle warp edit mode
			Warp::enableEditMode(!Warp::isEditModeEnabled());
			break;
		case KeyEvent::KEY_a:
			// toggle drawing a random region of the image
			if (mSrcArea.getWidth() != mTextures->getFboTexture(mParameterBag->mMixFboIndex).getWidth() || mSrcArea.getHeight() != mTextures->getFboTexture(mParameterBag->mMixFboIndex).getHeight())
				mSrcArea = mTextures->getFboTexture(mParameterBag->mMixFboIndex).getBounds();
			else
			{
				int x1 = Rand::randInt(0, mTextures->getFboTexture(mParameterBag->mMixFboIndex).getWidth() - 150);
				int y1 = Rand::randInt(0, mTextures->getFboTexture(mParameterBag->mMixFboIndex).getHeight() - 150);
				int x2 = Rand::randInt(x1 + 150, mTextures->getFboTexture(mParameterBag->mMixFboIndex).getWidth());
				int y2 = Rand::randInt(y1 + 150, mTextures->getFboTexture(mParameterBag->mMixFboIndex).getHeight());
				mSrcArea = Area(x1, y1, x2, y2);
			}
			break;
		case KeyEvent::KEY_SPACE:
			// toggle drawing mode
			mUseBeginEnd = !mUseBeginEnd;
			break;
		}
	}
}

void WarpWrapper::keyUp(KeyEvent event)
{
	// pass this key event to the warp editor first
	if (!Warp::handleKeyUp(mWarps, event))
	{
		// let your application perform its keyUp handling here
	}
}
void WarpWrapper::draw()
{
	// start drawing into the FBO
	// all draw related commands after this will draw into the FBO
	/*mFbo.bindFramebuffer();

	// clear the FBO
	gl::clear();
	gl::color(Color::white());
	mTextures->video1.draw(mParameterBag->mRenderPosXY.x, mParameterBag->mRenderPosXY.y);
	// stop drawing into the FBO
	mFbo.unbindFramebuffer();*/

	/***********************************************
	* fbo2 begin	
	// we can draw it into a second FBO, applying the shader...
	// note that we have to set up the matrices for 2d drawing to the FBO not the screen
	mFbo2.bindFramebuffer();
	gl::setMatricesWindow(mFbo2.getSize());
	gl::setViewport(mFbo2.getBounds());
	gl::clear();
	gl::draw(mTextures->getTexture(0), Rectf(0, 0, mParameterBag->mRenderWidth, mParameterBag->mRenderHeight));
	mFbo2.unbindFramebuffer();	
	* fbo2 end
	*/
	// at this point the FBO contains our scene
	// we can draw it to the screen unmodified...
	// first we need to set the matrices and viewport for 2d drawing,
	// because we're drawing a 2d texture (the contents of the FBO) to the screen
	// the GL state is persistent across all framebuffers
	gl::setMatricesWindow(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight, mParameterBag->mOriginUpperLeft);
	mViewportArea = Area(0, 0, mParameterBag->mRenderWidth, mParameterBag->mRenderHeight);
	
	gl::setViewport(mViewportArea);
	// clear the window and set the drawing color to white
	gl::clear();
	gl::color(Color::white());
	gl::disableAlphaBlending();
	gl::disable(GL_TEXTURE_2D);

	int i = 0;
	// iterate over the warps and draw their content
	for (WarpConstIter itr = mWarps.begin(); itr != mWarps.end(); ++itr)
	{
		// create a readable reference to our warp, to prevent code like this: (*itr)->begin();
		WarpRef warp(*itr);

		// there are two ways you can use the warps:
		if (mUseBeginEnd)
		{
			// a) issue your draw commands between begin() and end() statements
			warp->begin();
			
			gl::draw(mTextures->getFboTexture(mParameterBag->mWarpFbos[i].textureIndex), mSrcArea, warp->getBounds());

			warp->end();
		}
		else
		{
			// b) simply draw a texture on them (ideal for video)

			if (i < mParameterBag->mWarpCount)
			{
				warp->draw(mTextures->getFboTexture(mParameterBag->mWarpFbos[i].textureIndex), mTextures->getFboTexture(mParameterBag->mWarpFbos[i].textureIndex).getBounds());// was i
			}
		}
		i++;
	}
}
WarpWrapper::~WarpWrapper()
{
	log->logTimedString("WarpWrapper destructor");
}


