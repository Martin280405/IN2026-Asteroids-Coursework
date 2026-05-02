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
	mHardMode = false;  // NEW
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

	
	CreateGUI();

	mGameWorld->AddListener(&mPlayer);
	mPlayer.AddListener(thisPtr);

	CreateAsteroids(10);

	GameSession::Start();
}

void Asteroids::Stop()
{
	GameSession::Stop();
}



void Asteroids::OnKeyPressed(uchar key, int x, int y)
{
	if (!gameStarted)
	{
		
		if (key == 13)
		{
			StartGame();  
			return;
		}
		
		if (key == 'd' || key == 'D')  
		{
			mHardMode = !mHardMode;
			if (mHardMode)
				mDifficultyLabel->SetText("Difficulty: HARD (power-ups ON)  | Press D to change | Press Enter to Start");
			else
				mDifficultyLabel->SetText("Difficulty: EASY (power-ups OFF) | Press D to change | Press Enter to Start");
		
		}
		if (key == 'i' || key == 'I')  
		{
			bool currentlyVisible = mInstructionsLabel->GetVisible();
			mInstructionsLabel->SetVisible(!currentlyVisible);
		
		
		}
		return;
	}

	if (!mSpaceship) return;

	if (key == ' ')
	{
		mSpaceship->Shoot();
	}
}

void Asteroids::OnKeyReleased(uchar key, int x, int y) {}

void Asteroids::OnSpecialKeyPressed(int key, int x, int y)
{
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



shared_ptr<GameObject> Asteroids::CreateSpaceship()
{
	mSpaceship = make_shared<Spaceship>();

	mSpaceship->SetBoundingShape(make_shared<BoundingSphere>(mSpaceship->GetThisPtr(), 4.0f));
	mSpaceship->SetBulletShape(make_shared<Shape>("bullet.shape"));

	Animation* anim = AnimationManager::GetInstance().GetAnimationByName("spaceship");
	shared_ptr<Sprite> sprite = make_shared<Sprite>(anim->GetWidth(), anim->GetHeight(), anim);

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
		shared_ptr<Sprite> sprite = make_shared<Sprite>(anim->GetWidth(), anim->GetHeight(), anim);
		sprite->SetLoopAnimation(true);

		shared_ptr<GameObject> asteroid = make_shared<Asteroid>();
		asteroid->SetBoundingShape(make_shared<BoundingSphere>(asteroid->GetThisPtr(), 10.0f));
		asteroid->SetSprite(sprite);
		asteroid->SetScale(0.2f);

		mGameWorld->AddObject(asteroid);
	}
}



void Asteroids::CreateGUI()
{
	mGameDisplay->GetContainer()->SetBorder(GLVector2i(10, 10));

	
	mScoreLabel = make_shared<GUILabel>("Score: 0");
	mScoreLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_TOP);
	mScoreLabel->SetVisible(false);  
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mScoreLabel), GLVector2f(0.0f, 1.0f));

	
	mLivesLabel = make_shared<GUILabel>("Lives: 3");
	mLivesLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_BOTTOM);
	mLivesLabel->SetVisible(false);  
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mLivesLabel), GLVector2f(0.0f, 0.0f));

	
	mGameOverLabel = make_shared<GUILabel>("GAME OVER");
	mGameOverLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mGameOverLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mGameOverLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mGameOverLabel), GLVector2f(0.5f, 0.5f));

	
	mStartLabel = make_shared<GUILabel>("ASTEROIDS  |  Press I for Instructions  |  Press Enter to Start");
	mStartLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mStartLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mStartLabel), GLVector2f(0.5f, 0.6f));

	
	mDifficultyLabel = make_shared<GUILabel>("Difficulty: EASY (power-ups OFF) | Press D to change | Press Enter to Start");
	mDifficultyLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mDifficultyLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mDifficultyLabel), GLVector2f(0.5f, 0.5f));

	mInstructionsLabel = make_shared<GUILabel>(
		"HOW TO PLAY:  UP ARROW = Thrust  |  LEFT/RIGHT ARROW = Rotate  |  SPACE = Shoot  |  D = Difficulty  |  ENTER = Start");
	mInstructionsLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstructionsLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstructionsLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mInstructionsLabel), GLVector2f(0.5f, 0.35f));
}


void Asteroids::OnScoreChanged(int score)
{
	std::ostringstream msg;
	msg << "Score: " << score;
	mScoreLabel->SetText(msg.str());
}



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



shared_ptr<GameObject> Asteroids::CreateExplosion()
{
	Animation* anim = AnimationManager::GetInstance().GetAnimationByName("explosion");
	shared_ptr<Sprite> sprite = make_shared<Sprite>(anim->GetWidth(), anim->GetHeight(), anim);
	sprite->SetLoopAnimation(false);

	shared_ptr<GameObject> explosion = make_shared<Explosion>();
	explosion->SetSprite(sprite);
	explosion->Reset();

	return explosion;
}



void Asteroids::StartGame()
{
	gameStarted = true;
	mStartLabel->SetVisible(false);
	mDifficultyLabel->SetVisible(false);
	mInstructionsLabel->SetVisible(false);
	mScoreLabel->SetVisible(true);
	mLivesLabel->SetVisible(true);
	mGameWorld->AddObject(CreateSpaceship());
	CreateAsteroids(10);
}