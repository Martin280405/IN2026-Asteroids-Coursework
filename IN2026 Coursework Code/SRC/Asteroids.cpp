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
#include "ExtraLife.h"
#include <algorithm>

Asteroids::Asteroids(int argc, char* argv[])
	: GameSession(argc, argv)
{
	mLevel = 0;
	mAsteroidCount = 0;
	mGameState = MENU;
	mHardMode = false;
	mCurrentName = "";
}

Asteroids::~Asteroids(void)
{
}

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
	AnimationManager::GetInstance().CreateAnimationFromFile("extralife", 128, 128, 128, 128, "extralife_fs.png");

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
	if (mGameState == MENU)
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
				mDifficultyLabel->SetText("Difficulty: HARD (power-ups ON) | Press D to change | Press Enter to Start");
			else
				mDifficultyLabel->SetText("Difficulty: EASY (power-ups OFF) | Press D to change | Press Enter to Start");
		}
		if (key == 'i' || key == 'I')
		{
			bool currentlyVisible = mInstructionsLabel->GetVisible();
			mInstructionsLabel->SetVisible(!currentlyVisible);
		}
		if (key == 'h' || key == 'H')
		{
			ShowHighScores();
		}
		return;
	}

	if (mGameState == ENTER_NAME)
	{
		if (key == 13)
		{
			if (mCurrentName.empty()) mCurrentName = "AAA";
			ScoreEntry entry;
			entry.name = mCurrentName;
			entry.score = mScoreKeeper.GetScore();
			mHighScores.push_back(entry);
			std::sort(mHighScores.begin(), mHighScores.end(),
				[](const ScoreEntry& a, const ScoreEntry& b) {
					return a.score > b.score;
				});
			if (mHighScores.size() > 5) mHighScores.resize(5);
			mCurrentName = "";
			mEnterNameLabel->SetVisible(false);
			mGameOverLabel->SetVisible(false);
			mGameState = MENU;
			mStartLabel->SetVisible(true);
			mDifficultyLabel->SetVisible(true);
			ShowHighScores();
			return;
		}
		if (key == 8 && !mCurrentName.empty())
			mCurrentName.pop_back();
		else if (mCurrentName.length() < 10 && key >= 32 && key <= 126)
			mCurrentName += (char)key;
		mEnterNameLabel->SetText("Enter your name: " + mCurrentName + "_");
		return;
	}

	if (mGameState == PLAYING)
	{
		if (!mSpaceship) return;
		if (key == ' ')
			mSpaceship->Shoot();
		if (key == 's' || key == 'S')
		{
			mSpaceship->SetInvulnerable(true);
			mShieldLabel->SetText("** SHIELD ACTIVE **");
			mShieldLabel->SetVisible(true);
			SetTimer(5000, 4);
		}
		if (key == 't' || key == 'T')
		{
			GLVector3f newPos;
			newPos.x = (rand() % 200) - 100;
			newPos.y = (rand() % 200) - 100;
			newPos.z = 0.0f;
			mSpaceship->SetPosition(newPos);
			mSpaceship->SetVelocity(GLVector3f(0.0f, 0.0f, 0.0f));
			mTeleportLabel->SetVisible(true);
			SetTimer(1000, 5);
		}
	}
}

void Asteroids::OnKeyReleased(uchar key, int x, int y) {}

void Asteroids::OnSpecialKeyPressed(int key, int x, int y)
{
	if (mGameState != PLAYING || !mSpaceship) return;

	switch (key)
	{
	case GLUT_KEY_UP: mSpaceship->Thrust(10); break;
	case GLUT_KEY_LEFT: mSpaceship->Rotate(90); break;
	case GLUT_KEY_RIGHT: mSpaceship->Rotate(-90); break;
	}
}

void Asteroids::OnSpecialKeyReleased(int key, int x, int y)
{
	if (mGameState != PLAYING || !mSpaceship) return;

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

		if (rand() % 5 == 0 && mGameState == PLAYING)
		{
			CreateExtraLife();
		}

		if (mAsteroidCount <= 0 && mGameState == PLAYING)
		{
			SetTimer(500, START_NEXT_LEVEL);
		}
	}

	if (object->GetType() == GameObjectType("ExtraLife"))
	{
		mPlayer.AddLife();
		std::ostringstream msg;
		msg << "Lives: " << mPlayer.GetLives();
		mLivesLabel->SetText(msg.str());
	}
}

void Asteroids::OnTimer(int value)
{
	if (value == CREATE_NEW_PLAYER)
	{
		mSpaceship->Reset();
		mGameWorld->AddObject(mSpaceship);
		mSpaceship->SetInvulnerable(true);
		mShieldLabel->SetText("** SPAWN PROTECTION **");
		mShieldLabel->SetVisible(true);
		SetTimer(3000, 4);
	}

	if (value == START_NEXT_LEVEL)
	{
		mLevel++;
		CreateAsteroids(10 + 2 * mLevel);
	}

	if (value == SHOW_GAME_OVER)
	{
	}

	if (value == 4)
	{
		if (mSpaceship) mSpaceship->SetInvulnerable(false);
		mShieldLabel->SetVisible(false);
	}

	if (value == 5)
	{
		mTeleportLabel->SetVisible(false);
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
		static_pointer_cast<GUIComponent>(mGameOverLabel), GLVector2f(0.5f, 0.6f));

	mStartLabel = make_shared<GUILabel>("ASTEROIDS  |  Press H for High Scores  |  Press I for Instructions  |  Press Enter to Start");
	mStartLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mStartLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mStartLabel), GLVector2f(0.5f, 0.65f));

	mDifficultyLabel = make_shared<GUILabel>("Difficulty: EASY (power-ups OFF) | Press D to change | Press Enter to Start");
	mDifficultyLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mDifficultyLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mDifficultyLabel), GLVector2f(0.5f, 0.55f));

	mInstructionsLabel = make_shared<GUILabel>(
		"UP ARROW = Thrust  |  LEFT/RIGHT = Rotate  |  SPACE = Shoot  |  S = Shield  |  T = Teleport  |  ENTER = Start");
	mInstructionsLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstructionsLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstructionsLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mInstructionsLabel), GLVector2f(0.5f, 0.45f));

	mEnterNameLabel = make_shared<GUILabel>("Enter your name: _");
	mEnterNameLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mEnterNameLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mEnterNameLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mEnterNameLabel), GLVector2f(0.5f, 0.45f));

	mHighScoreLabel = make_shared<GUILabel>("HIGH SCORES");
	mHighScoreLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mHighScoreLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mHighScoreLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mHighScoreLabel), GLVector2f(0.5f, 0.5f));

	mShieldLabel = make_shared<GUILabel>("** SHIELD ACTIVE **");
	mShieldLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mShieldLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mShieldLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mShieldLabel), GLVector2f(0.5f, 0.8f));

	mTeleportLabel = make_shared<GUILabel>("** TELEPORTED **");
	mTeleportLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mTeleportLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mTeleportLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mTeleportLabel), GLVector2f(0.5f, 0.7f));
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
		mGameState = ENTER_NAME;
		mGameOverLabel->SetVisible(true);
		mEnterNameLabel->SetText("Enter your name: _");
		mEnterNameLabel->SetVisible(true);
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
	mGameState = PLAYING;
	mLevel = 0;
	mAsteroidCount = 0;
	mPlayer.ResetLives();
	mScoreKeeper.ResetScore();
	mStartLabel->SetVisible(false);
	mDifficultyLabel->SetVisible(false);
	mInstructionsLabel->SetVisible(false);
	mHighScoreLabel->SetVisible(false);
	mShieldLabel->SetVisible(false);
	mTeleportLabel->SetVisible(false);
	mScoreLabel->SetVisible(true);
	mLivesLabel->SetVisible(true);
	mLivesLabel->SetText("Lives: 3");
	mGameWorld->AddObject(CreateSpaceship());
	mSpaceship->SetInvulnerable(true);
	mShieldLabel->SetText("** SPAWN PROTECTION **");
	mShieldLabel->SetVisible(true);
	SetTimer(3000, 4);
	CreateAsteroids(10);
}

void Asteroids::ShowHighScores()
{
	std::ostringstream ss;
	ss << "--- HIGH SCORES ---   ";
	if (mHighScores.empty())
	{
		ss << "No scores yet!";
	}
	else
	{
		for (int i = 0; i < (int)mHighScores.size(); i++)
			ss << i + 1 << ". " << mHighScores[i].name << " - " << mHighScores[i].score << "   ";
	}
	mHighScoreLabel->SetText(ss.str());
	mHighScoreLabel->SetVisible(true);
}

std::string Asteroids::BuildHighScoreString()
{
	std::ostringstream ss;
	for (int i = 0; i < (int)mHighScores.size(); i++)
		ss << i + 1 << ". " << mHighScores[i].name << " - " << mHighScores[i].score << "   ";
	return ss.str();
}

void Asteroids::CreateExtraLife()
{
	Animation* anim = AnimationManager::GetInstance().GetAnimationByName("extralife");
	shared_ptr<Sprite> sprite = make_shared<Sprite>(anim->GetWidth(), anim->GetHeight(), anim);
	sprite->SetLoopAnimation(true);

	shared_ptr<GameObject> extraLife = make_shared<ExtraLife>();
	extraLife->SetBoundingShape(make_shared<BoundingSphere>(extraLife->GetThisPtr(), 5.0f));
	extraLife->SetSprite(sprite);
	extraLife->SetScale(0.2f);
	mGameWorld->AddObject(extraLife);
}