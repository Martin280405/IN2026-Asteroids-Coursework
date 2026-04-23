#include "Asteroid.h"
#include "Asteroids.h"
#include "Animation.h"
#include "AnimationManager.h"
#include "GameUtil.h"
#include "GameWindow.h"
#include "GameWorld.h"
#include "GameDisplay.h"
#include "Spaceship.h"
#include "BoundingShape.h"
#include "BoundingSphere.h"
#include "GUILabel.h"
#include "Explosion.h"

// PUBLIC INSTANCE CONSTRUCTORS ///////////////////////////////////////////////

Asteroids::Asteroids(int argc, char* argv[])
	: GameSession(argc, argv)
{
	mLevel = 0;
	mAsteroidCount = 0;
	gameStarted = false;
}

Asteroids::~Asteroids(void)
{
}

// PUBLIC INSTANCE METHODS ////////////////////////////////////////////////////

void Asteroids::Start()
{
	shared_ptr<Asteroids> thisPtr = shared_ptr<Asteroids>(this);

	mGameWorld->AddListener(thisPtr.get());
	mGameWindow->AddKeyboardListener(thisPtr);
	mGameWorld->AddListener(&mScoreKeeper);
	mScoreKeeper.AddListener(thisPtr);

	GLfloat ambient_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat diffuse_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_light);
	glEnable(GL_LIGHT0);

	AnimationManager::GetInstance().CreateAnimationFromFile("explosion", 64, 1024, 64, 64, "explosion_fs.png");
	AnimationManager::GetInstance().CreateAnimationFromFile("asteroid1", 128, 8192, 128, 128, "asteroid1_fs.png");
	AnimationManager::GetInstance().CreateAnimationFromFile("spaceship", 128, 128, 128, 128, "spaceship_fs.png");

	// GUI
	CreateGUI();
	mGameOverLabel->SetText("PRESS ENTER TO START");
	mGameOverLabel->SetVisible(true);

	mGameWorld->AddListener(&mPlayer);
	mPlayer.AddListener(thisPtr);

	GameSession::Start();
}

void Asteroids::Stop()
{
	GameSession::Stop();
}

// KEYBOARD ////////////////////////////////////////////////////

void Asteroids::OnKeyPressed(uchar key, int x, int y)
{
	// 🚀 START GAME ON ENTER
	if (!gameStarted && key == 13)
	{
		gameStarted = true;

		mGameOverLabel->SetVisible(false);
		mScoreLabel->SetVisible(true);
		mLivesLabel->SetVisible(true);

		shared_ptr<GameObject> ship = CreateSpaceship();
		mGameWorld->AddObject(ship);

		CreateAsteroids(10);
		return;
	}

	if (!gameStarted || !mSpaceship) return;

	if (key == ' ')
	{
		mSpaceship->Shoot();
	}
}

void Asteroids::OnKeyReleased(uchar key, int x, int y) {}

void Asteroids::OnSpecialKeyPressed(int key, int x, int y)
{
	// START GAME
	if (!gameStarted && key == 13)
	{
		gameStarted = true;

		mGameOverLabel->SetVisible(false);
		mScoreLabel->SetVisible(true);
		mLivesLabel->SetVisible(true);

		shared_ptr<GameObject> ship = CreateSpaceship();
		mGameWorld->AddObject(ship);

		CreateAsteroids(10);
		return;
	}

	if (!gameStarted || !mSpaceship) return;

	switch (key)
	{
	case GLUT_KEY_UP: mSpaceship->Thrust(10); break;
	case GLUT_KEY_LEFT: mSpaceship->Rotate(90); break;
	case GLUT_KEY_RIGHT: mSpaceship->Rotate(-90); break;
	}
}

void Asteroids::OnSpecialKeyReleased(int key, int x, int y)
{
	if (!gameStarted || !mSpaceship) return;

	switch (key)
	{
	case GLUT_KEY_UP: mSpaceship->Thrust(0); break;
	case GLUT_KEY_LEFT: mSpaceship->Rotate(0); break;
	case GLUT_KEY_RIGHT: mSpaceship->Rotate(0); break;
	}
}

// GAME WORLD ////////////////////////////////////////////////////

void Asteroids::OnObjectRemoved(GameWorld* world, shared_ptr<GameObject> object)
{
	if (object->GetType() == GameObjectType("Asteroid"))
	{
		shared_ptr<GameObject> explosion = CreateExplosion();
		explosion->SetPosition(object->GetPosition());
		explosion->SetRotation(object->GetRotation());
		mGameWorld->AddObject(explosion);

		mAsteroidCount--;

		if (mAsteroidCount <= 0)
		{
			SetTimer(500, START_NEXT_LEVEL);
		}
	}
}

// TIMER ////////////////////////////////////////////////////

void Asteroids::OnTimer(int value)
{
	if (value == CREATE_NEW_PLAYER)
	{
		mSpaceship->Reset();
		mGameWorld->AddObject(mSpaceship);
	}

	if (value == START_NEXT_LEVEL)
	{
		mLevel++;
		CreateAsteroids(10 + 2 * mLevel);
	}

	if (value == SHOW_GAME_OVER)
	{
		mGameOverLabel->SetVisible(true);
	}
}

// OBJECT CREATION ////////////////////////////////////////////////////

shared_ptr<GameObject> Asteroids::CreateSpaceship()
{
	mSpaceship = make_shared<Spaceship>();

	mSpaceship->SetBoundingShape(make_shared<BoundingSphere>(mSpaceship->GetThisPtr(), 4.0f));
	mSpaceship->SetBulletShape(make_shared<Shape>("bullet.shape"));

	Animation* anim = AnimationManager::GetInstance().GetAnimationByName("spaceship");

	shared_ptr<Sprite> sprite =
		make_shared<Sprite>(anim->GetWidth(), anim->GetHeight(), anim);

	mSpaceship->SetSprite(sprite);
	mSpaceship->SetScale(0.1f);
	mSpaceship->Reset();

	return mSpaceship;
}

void Asteroids::CreateAsteroids(const uint num_asteroids)
{
	mAsteroidCount = num_asteroids;

	for (uint i = 0; i < num_asteroids; i++)
	{
		Animation* anim = AnimationManager::GetInstance().GetAnimationByName("asteroid1");

		shared_ptr<Sprite> sprite =
			make_shared<Sprite>(anim->GetWidth(), anim->GetHeight(), anim);

		sprite->SetLoopAnimation(true);

		shared_ptr<GameObject> asteroid = make_shared<Asteroid>();

		asteroid->SetBoundingShape(make_shared<BoundingSphere>(asteroid->GetThisPtr(), 10.0f));
		asteroid->SetSprite(sprite);
		asteroid->SetScale(0.2f);

		mGameWorld->AddObject(asteroid);
	}
}

// GUI ////////////////////////////////////////////////////

void Asteroids::CreateGUI()
{
	mGameDisplay->GetContainer()->SetBorder(GLVector2i(10, 10));

	mScoreLabel = make_shared<GUILabel>("Score: 0");
	mScoreLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_TOP);

	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mScoreLabel),
		GLVector2f(0.0f, 1.0f)
	);

	mLivesLabel = make_shared<GUILabel>("Lives: 3");
	mLivesLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_BOTTOM);

	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mLivesLabel),
		GLVector2f(0.0f, 0.0f)
	);

	mGameOverLabel = make_shared<GUILabel>("GAME OVER");
	mGameOverLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mGameOverLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mGameOverLabel->SetVisible(false);

	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mGameOverLabel),
		GLVector2f(0.5f, 0.5f)
	);

	// Hide UI initially
	mScoreLabel->SetVisible(false);
	mLivesLabel->SetVisible(false);
}

// SCORE ////////////////////////////////////////////////////

void Asteroids::OnScoreChanged(int score)
{
	std::ostringstream msg;
	msg << "Score: " << score;
	mScoreLabel->SetText(msg.str());
}

// PLAYER ////////////////////////////////////////////////////

void Asteroids::OnPlayerKilled(int lives_left)
{
	shared_ptr<GameObject> explosion = CreateExplosion();

	explosion->SetPosition(mSpaceship->GetPosition());
	explosion->SetRotation(mSpaceship->GetRotation());

	mGameWorld->AddObject(explosion);

	std::ostringstream msg;
	msg << "Lives: " << lives_left;
	mLivesLabel->SetText(msg.str());

	if (lives_left > 0)
	{
		SetTimer(1000, CREATE_NEW_PLAYER);
	}
	else
	{
		SetTimer(500, SHOW_GAME_OVER);
	}
}

// EXPLOSION ////////////////////////////////////////////////////

shared_ptr<GameObject> Asteroids::CreateExplosion()
{
	Animation* anim = AnimationManager::GetInstance().GetAnimationByName("explosion");

	shared_ptr<Sprite> sprite =
		make_shared<Sprite>(anim->GetWidth(), anim->GetHeight(), anim);

	sprite->SetLoopAnimation(false);

	shared_ptr<GameObject> explosion = make_shared<Explosion>();

	explosion->SetSprite(sprite);
	explosion->Reset();

	return explosion;
}