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

	/*mTexture = gl::Texture(loadImage(loadAsset("vertexsphere.jpg")));//ndf.jpg
	mTexture.setWrap(GL_REPEAT, GL_REPEAT);
	mTexture.setMinFilter(GL_NEAREST);
	mTexture.setMagFilter(GL_NEAREST);
	sTexture = gl::Texture(loadImage(loadAsset("vertexsphere.jpg")));*/

	//gl::enableDepthRead();
}

/*void VertexSphere::mouseDown(MouseEvent event)
{
	mRotate = !mRotate;
}*/
void VertexSphere::update(){

	//if (mRotate)
	//{
		mAngle += mParameterBag->controlValues[19];
	//}
	mQuat.set(mAxis, mAngle);
}

void VertexSphere::draw()
{
	// vertex sphere
	mTextures->getTexture(mParameterBag->mVertexSphereTextureIndex).setWrap(GL_REPEAT, GL_REPEAT);
	mTextures->getTexture(mParameterBag->mVertexSphereTextureIndex).setMinFilter(GL_NEAREST);
	mTextures->getTexture(mParameterBag->mVertexSphereTextureIndex).setMagFilter(GL_NEAREST);

	mTextures->getFbo(mParameterBag->mVertexSphereFboIndex).bindFramebuffer();
	gl::clear();
	gl::enableDepthRead();
	gl::setViewport(mTextures->getFbo(mParameterBag->mVertexSphereFboIndex).getBounds());

	mTextures->getTexture(mParameterBag->mVertexSphereTextureIndex).enableAndBind();
	mShaders->getVertexSphereShader()->bind();
	/*if (maxVolume > 0)
	{*/
	mShaders->getVertexSphereShader()->uniform("normScale", mParameterBag->maxVolume);
	//}
	//else
	//{
	//	mShader.uniform("normScale", (mMouse.x) / 5.0f);// (mMouse.x)
	//}
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
}
