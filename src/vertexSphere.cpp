#include "VertexSphere.h"
using namespace Reymenta;

VertexSphere::VertexSphere(ParameterBagRef aParameterBag, TexturesRef aTexturesRef, ShadersRef aShadersRef)
{
	mParameterBag = aParameterBag;
	mShaders = aShadersRef;
	mTextures = aTexturesRef;

	//mRotationSpeed = 0.010f;
	mAngle = 0.0f;
	mAxis = Vec3f(0.0f, 1.0f, 0.0f);
	mQuat = Quatf(mAxis, mAngle);
	mRotate = true;
	mShader = gl::GlslProg(loadAsset("mShader.vert"), loadAsset("mShader.frag"));
	mTexture = gl::Texture(loadImage(loadAsset("nebula.jpg")));
	mTexture.setWrap(GL_REPEAT, GL_REPEAT);
	mTexture.setMinFilter(GL_NEAREST);
	mTexture.setMagFilter(GL_NEAREST);

	gl::enableDepthRead();
}

void VertexSphere::update()
{
	//if (mRotate)
	//{
		mAngle += mParameterBag->controlValues[19];
	//}
	mQuat.set(mAxis, mAngle);
}

void VertexSphere::draw()
{
	mTextures->getFbo(mParameterBag->mVertexSphereFboIndex).bindFramebuffer();
	gl::clear();

	mTexture.enableAndBind();
	mShader.bind();
	mShader.uniform("normScale", (mMouse.x) / 5.0f);
	mShader.uniform("colorMap", 0);
	mShader.uniform("displacementMap", 0);
	gl::pushModelView();
	gl::translate(Vec3f(0.5f*getWindowWidth(), 0.5f*getWindowHeight(), 0));
	gl::rotate(mQuat);
	gl::drawSphere(Vec3f(0, 0, 0), 200, 500);
	gl::popModelView();
	mShader.unbind();
	mTexture.unbind();
	mTextures->getFbo(mParameterBag->mVertexSphereFboIndex).unbindFramebuffer();
	/*
	mTextures->getFbo(mParameterBag->mVertexSphereFboIndex).bindFramebuffer();
	gl::clear();
	gl::setViewport(mTextures->getFbo(mParameterBag->mVertexSphereFboIndex).getBounds());


	mTextures->getTexture(mParameterBag->mVertexSphereTextureIndex).enableAndBind();
	
	mShaders->getVertexSphereShader()->bind();

	mShaders->getVertexSphereShader()->uniform("normScale", mParameterBag->maxVolume);
	mShaders->getVertexSphereShader()->uniform("colorMap", 0);
	mShaders->getVertexSphereShader()->uniform("displacementMap", 0);
	gl::pushModelView();
	gl::translate(Vec3f(0.5f*mParameterBag->mRenderWidth, 0.5f*mParameterBag->mRenderHeight, 0));
	gl::rotate(mQuat);
	gl::drawSphere(Vec3f(0, 0, 0), 140, 500);
	gl::popModelView();
	mShaders->getVertexSphereShader()->unbind();
	
	mTextures->getTexture(mParameterBag->mVertexSphereTextureIndex).unbind();

	mTextures->getFbo(mParameterBag->mVertexSphereFboIndex).unbindFramebuffer();
	*/
}
