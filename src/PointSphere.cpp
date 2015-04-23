// from https://gist.github.com/paulhoux/6443497 
#include "PointSphere.h"

using namespace Reymenta;

PointSphere::PointSphere(ParameterBagRef aParameterBag, TexturesRef aTexturesRef, ShadersRef aShadersRef)
{
	mParameterBag = aParameterBag;
	mShaders = aShadersRef;
	mTextures = aTexturesRef;
	// instanciate the logger class
	log = Logger::create("PointSphereLog.txt");
	log->logTimedString("PointSphere constructor");

	multFactor = 1000;
	mRotation = 0.0f;

	mSphereAngle = 0.0f;
	mSphereAxis = Vec3f(0.0f, 1.0f, 0.0f);
	mSphereQuat = Quatf(mSphereAxis, mSphereAngle);

	mSphereShader = gl::GlslProg(loadAsset("pointsphere.vert"), loadAsset("pointsphere.frag"));
	mSphereTexture = gl::Texture(loadImage(loadAsset("reymenta.jpg")));
	mSphereTexture.setWrap(GL_REPEAT, GL_REPEAT);
	mSphereTexture.setMinFilter(GL_NEAREST);
	mSphereTexture.setMagFilter(GL_NEAREST);
}

void PointSphere::mouseMove(MouseEvent event)
{	
}
void PointSphere::mouseDown(MouseEvent event)
{
	// use the maya camera helper to control our camera
	//mMayaCam.setCurrentCam(mCamera);
	//mMayaCam.mouseDown(mParameterBag->mRenderPosXY);// event.getPos());
}
void PointSphere::mouseDrag(MouseEvent event)
{
// use the maya camera helper to control our camera
	//mMayaCam.mouseDrag(event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown());
	//mMayaCam.mouseDrag(mParameterBag->mRenderPosXY, event.isLeftDown(), event.isMiddleDown(), event.isRightDown());
	//mCamera = mMayaCam.getCamera();
}
void PointSphere::mouseUp(MouseEvent event)
{
}
void PointSphere::keyDown(KeyEvent event)
{
}

void PointSphere::keyUp(KeyEvent event)
{
}
void PointSphere::update()
{
	mSphereAngle += mParameterBag->controlValues[9];// controlValues[19];
	mSphereQuat.set(mSphereAxis, mSphereAngle);
	//aggressivity += 0.01;
}
void PointSphere::draw()
{
	mTextures->getFbo(mParameterBag->mSphereFboIndex).bindFramebuffer();
	gl::setViewport(mTextures->getFbo(mParameterBag->mSphereFboIndex).getBounds());
	gl::setMatrices(mParameterBag->mCamera);
	gl::clear(Color(mParameterBag->controlValues[5], mParameterBag->controlValues[6], mParameterBag->controlValues[7]));

	// enable depth buffer and setup camera
	gl::enableDepthRead();
	gl::enableDepthWrite();

	// draw our 'shapes'
	gl::color(Color::white());
	gl::enable(GL_TEXTURE_2D);
	mTextures->getFboTexture(mParameterBag->mMixFboIndex).bind(0);
	mSphereShader.bind();
	
	mSphereShader.uniform("normScale", float(mParameterBag->iFreqs[0]/10.0));
	mSphereShader.uniform("colorMap", 0);
	mSphereShader.uniform("displacementMap", 0);
	mSphereShader.uniform("iZoom", mParameterBag->controlValues[13]);
	float radius = 100.0f;

	for (int longitude = -180; longitude<180; longitude += 20)
	{
		for (int latitude = -80; latitude <= 80; latitude += 20)
		{
			Matrix44f transform;
			// turn according to position on Earth
			transform.rotate(Vec3f(0.0f, toRadians((float)longitude + (mParameterBag->controlValues[19])), 0.0f));
			transform.rotate(Vec3f(toRadians((float)-latitude + (mParameterBag->controlValues[9] * 20.0)), 0.0f, 0.0f));
			// place on surface of Earth
			transform.translate(Vec3f(0.0f, 0.0f, radius));

			// now that we have constructed our transform matrix,
			// multiply the current modelView matrix with it
			gl::pushModelView();
			gl::multModelView(transform);

			// now we can simply draw something at the origin, 
			// as if we haven't rotated at all
			//
			switch (mParameterBag->mMeshIndex)
			{
			case 0:
				gl::drawSphere(Vec3f::one(), mParameterBag->iFreqs[0]/10.0);
				break;
			case 1:
				gl::drawLine(Vec3f::one(), Vec3f::one()*mParameterBag->iFreqs[0]);
				break;
			case 2:
				gl::drawCoordinateFrame(10.f, 1.f, 1.f);
				break;
			default://case MESH_TYPE_SPHERE,
				gl::drawColorCube(Vec3f::one(), Vec3f::one()*mParameterBag->iFreqs[0]/10.0);
				break;
			}
			// restore the modelView matrix to undo the transformation,
			// so it doesn't interfere with the next shape
			gl::popModelView();
		}
	}
	mSphereShader.unbind();
	mTextures->getFboTexture(mParameterBag->mMixFboIndex).unbind();
	gl::disable(GL_TEXTURE_2D);

	// draw a translucent black sphere to make the shapes at the back appear darker
	gl::color(ColorA(0, 0, 0, 0.8f));
	gl::enableAlphaBlending();
	gl::enable(GL_CULL_FACE);
	gl::drawSphere(Vec3f::zero(), radius, 60);
	gl::disable(GL_CULL_FACE);
	gl::disableAlphaBlending();

	// disable depth buffer
	gl::disableDepthRead();
	gl::disableDepthWrite();
	mTextures->getFbo(mParameterBag->mSphereFboIndex).unbindFramebuffer();
}
PointSphere::~PointSphere()
{
	log->logTimedString("PointSphere destructor");
}


