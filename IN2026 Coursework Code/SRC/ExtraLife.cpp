#include "ExtraLife.h"
#include "GameWorld.h"
#include <stdlib.h>

ExtraLife::ExtraLife() : GameObject("ExtraLife")
{
	mAngle = rand() % 360;
	mRotation = 0;
	mPosition.x = rand() / 2;
	mPosition.y = rand() / 2;
	mPosition.z = 0.0;
	mVelocity.x = 5.0 * cos(DEG2RAD * mAngle);
	mVelocity.y = 5.0 * sin(DEG2RAD * mAngle);
	mVelocity.z = 0.0;
}

ExtraLife::~ExtraLife(void) {}

void ExtraLife::Update(int t)
{
	GameObject::Update(t);
}