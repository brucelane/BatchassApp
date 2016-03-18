#include "ABP.h"

using namespace Reymenta;

ABP::ABP(ParameterBagRef aParameterBag, TexturesRef aTexturesRef)
{
	mParameterBag = aParameterBag;
	mTextures = aTexturesRef;
	// neRenderer
	x = mParameterBag->mRenderWidth / 2;
	y = mParameterBag->mRenderHeight / 2;

	mZoom = 0.3f;
	mXYVector = Vec2f(1.0,1.0);
	mRepetitions = 1;
	mShape = 0;
	mZPosition = 0.0f;
	mRotation = 2.5f;
	mSize = 1.0f;
	mMotionVector = 0.2f;
	mBend = 0.1f;
	mLockZ = false;
	mLockRepetitions = false;
	mLockRotation = false;
	mLockSize = false;
	mLockMotionVector = false;
	mLockBend = false;
	mGlobalMode = false;
	mColorFactor = 0.06;
	mR = 0.5f;
	mG = 0.0f;
	mB = 0.8f;
	mA = 1.0f;
	// init one brick
	addBrick(true);
	alreadyCreated = false;
	// gl setup
	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::enableAlphaBlending();

	mCam.lookAt(Vec3f(0.0f, CAMERA_Y_RANGE.first, 0.0f), Vec3f(0.0f, 0.0f, 0.0f)); 

}
void ABP::addBrick(const bool &pressed)
{
	brick newBrick;
	newBrick.r = mR;
	newBrick.g = mG;
	newBrick.b = mB;
	newBrick.a = mA;
	newBrick.vx = mXYVector.x;
	newBrick.vy = mXYVector.y;
	newBrick.shape = mShape;
	newBrick.size = mSize;
	newBrick.rotation = mRotation;
	newBrick.motionVector = mMotionVector;
	bricks.push_back(newBrick);
	mXYVector.x = Rand::randFloat(-2.0, 2.0);
	mXYVector.y = Rand::randFloat(-2.0, 2.0);
}
void ABP::updateBricks()
{
	float new_x;
	float new_y;
	float bendFactor;
	float volumeFactor;
	mBend = mParameterBag->mBend ;
	/*if (mParameterBag->liveMeter > 0.7)
	{
		float between08and1 = mParameterBag->liveMeter - 0.7;
		volumeFactor = lmap<float>(between08and1, 0.0, 0.3, 0.1, 0.8);
	}
	else
	{
		volumeFactor = 0.01;
	}*/
	volumeFactor = mParameterBag->controlValues[14];
	mTextures->getFbo(mParameterBag->mABPFboIndex).bindFramebuffer();
	gl::clear();
	gl::setViewport(mTextures->getFbo(mParameterBag->mABPFboIndex).getBounds());

	CameraPersp cam(mTextures->getFbo(mParameterBag->mABPFboIndex).getWidth(), mTextures->getFbo(mParameterBag->mABPFboIndex).getHeight(), 60.0f);
	cam.setPerspective(60, mTextures->getFbo(mParameterBag->mABPFboIndex).getAspectRatio(), 1, 1000);
	cam.lookAt(Vec3f(2.8f, 1.8f, -2.8f), Vec3f(0.0f, 0.0f, 0.0f));
	gl::setMatrices(cam);
	gl::color(Color::white());
	gl::scale(Vec3f(1.0f, 1.0f, 1.0f) * mZoom);
	gl::rotate(mRotation);
	for (int i = 0; i < bricks.size(); i++)
	{
		if (mGlobalMode)
		{
			r = mR;
			g = mG;
			b = mB;
			a = mA;
			shape = mShape;
		}
		else
		{
			r = bricks[i].r;
			g = bricks[i].g;
			b = bricks[i].b;
			a = bricks[i].a;
			shape = bricks[i].shape;
		}
		rotation = bricks[i].rotation++;
		distance = mMotionVector * mSize;
		bendFactor = 0.0f;
		//save state to restart translation from center point
		gl::pushMatrices();
		for (int j = 0; j < mRepetitions; j++)
		{
			r -= mColorFactor / volumeFactor;
			g -= mColorFactor / volumeFactor;
			b -= mColorFactor / volumeFactor;
			a -= mColorFactor / volumeFactor;
			gl::color(r, g, b, a);
			new_x = sin(rotation*0.01745329251994329576923690768489) * distance;
			new_y = cos(rotation*0.01745329251994329576923690768489) * distance;

			x = new_x + bricks[i].vx;
			y = new_y + bricks[i].vy;
			bendFactor += mBend / 10.0f;
			gl::translate(x, y, mZPosition + bendFactor);

			if (shape == 0)
			{
				gl::drawSphere(Vec3f(0.0f, 0.0f, 0.0f), bricks[i].size * mSize, 16);
			}
			if (shape == 1)
			{
				gl::drawCube(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(bricks[i].size * mSize, bricks[i].size * mSize, bricks[i].size * mSize));
			}
			if (shape == 2)
			{
				gl::drawSolidCircle(Vec2f(0.0f, 0.0f), bricks[i].size * mSize);
			}
			if (shape == 3)
			{
				Vec2f dupa[3] = { Vec2f(0, bricks[i].size * mSize), Vec2f(-bricks[i].size * mSize, -bricks[i].size * mSize), Vec2f(-bricks[i].size * mSize, -bricks[i].size * mSize) };
				gl::drawSolidTriangle(dupa);
			}
		}
		gl::popMatrices();

	}

	mTextures->getFbo(mParameterBag->mABPFboIndex).unbindFramebuffer();

	gl::color(Color::white());
}
void ABP::update()
{
	mZPosition = mLockZ ? sin(getElapsedFrames() / 100.0f) : mZPosition;
	mRotation = mLockRotation ? sin(getElapsedFrames() / 100.0f)*4.0f : mRotation;
	mSize = mLockSize ? sin(getElapsedFrames() / 100.0f) + 0.7f : mSize;
	mMotionVector = mLockMotionVector ? sin(getElapsedFrames() / 50.0f) : mMotionVector;
	//mRotationMatrix *= rotate(0.06f, normalize(vec3(0.16666f, 0.333333f, 0.666666f)));
	mRepetitions = mLockRepetitions ? (sin(getElapsedFrames() / 100.0f) + 1) * 20 : mRepetitions;
	mBend = mLockBend ? sin(getElapsedFrames() / 100.0f) * 10.0f : mBend;
	if (mParameterBag->iBeat < 64)
	{
		mRepetitions = (mParameterBag->iBeat / 8) + 1;
	}
	else
	{

		if (mParameterBag->iBeat % 8 == 0)
		{
			if (bricks.size() < 20 && !alreadyCreated)
			{
				addBrick(false);
				alreadyCreated = true;
			}

		}
		else
		{
			alreadyCreated = false;
		}
		if (mParameterBag->iBeat % 8 == 0)
		{
			if (mParameterBag->iBeat > 92)
			{
				mGlobalMode = true;
				mR = 1.0f;
				mB = 0.0f;
			}
			if (mParameterBag->iBeat > 280 && mParameterBag->iBeat < 292)
			{
				mShape = 1;
			}
			if (mParameterBag->iBeat > 316)
			{
				mRotation += 0.2;
				if (mRotation > 6.35) mRotation = 0;
			}
		}
		else
		{
			mR = 0.6f;
			mB = 0.9f;
			if (mParameterBag->iBeat > 510 && mParameterBag->iBeat % 2 == 0)
			{
				mRotation += 0.5 + mParameterBag->controlValues[12]; //was 0.2
				if (mRotation > 6.35) mRotation = 0;
			}
		}
	}
	updateBricks();
	// move the camera up and down on Y
	mCam.lookAt(Vec3f(0.0f, CAMERA_Y_RANGE.first + abs(sin(getElapsedSeconds() / 4)) * (CAMERA_Y_RANGE.second - CAMERA_Y_RANGE.first), 0.0f), Vec3f(0.0f, 0.0f, 0.0f));

}