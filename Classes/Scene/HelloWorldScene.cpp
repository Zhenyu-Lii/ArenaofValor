#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "Component/StateComponent.h"
#include "GameController/HRocker.h"
#include "Actor/Actor.h"
#include "Component/Record.h"
#include "Component/ExpComponent.h"
#include "Hero/HouYi.h"
#include "PathFind/SoldierPath.h"

Scene* HelloWorld::createScene()
{
	return HelloWorld::create();
}

static void problemLoading(const char* filename)
{
	printf("Error while loading: %s\n", filename);
	printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}

bool HelloWorld::init()
{
	if (!Scene::init())
	{
		return false;
	}

	_visibleSize = Director::getInstance()->getVisibleSize();
	_origin = Director::getInstance()->getVisibleOrigin();
	
	//test();

	initMapLayer();

//	loadingAnimation();
	initTower();
	initLabelRecord();
	initHero();
	initHRocker();
	initSkillPanel();

	schedule(schedule_selector(HelloWorld::generateSoldiers), 10.f, -1, 0.f);

	scheduleUpdate();

	return true;
}

void HelloWorld::initMapLayer()
{
	_map = TMXTiledMap::create("map/map2.tmx");
	auto size = _map->getBoundingBox().size;
	_map->setAnchorPoint(Vec2::ZERO);
	log("%f", size.height);
	_map->setPosition(Vec2::ZERO);

	auto wallLayer = _map->getLayer("Wall");
	wallLayer->setZOrder(0);

	_mapInformation = MapInfo(_map);
	_soldierPathPoints = SoldierPath::create("D:/LatestFiles/hello/Data/PathPoints.txt", _mapInformation);
	_actors.pushBack(_soldierPathPoints);

	auto collisionLayer = _map->getLayer("collision");
	collisionLayer->setVisible(false);

	addChild(_map, 0, TAG_MAP);
}

void HelloWorld::initLabelRecord()
{
	_labelRecord = Record::create();
	addChild(_labelRecord);
}

void HelloWorld::initHero()
{
	auto visibleSize = Director::getInstance()->getVisibleSize();

	_myHero = HouYi::create(this, ECamp::BLUE, "HouYi", EAttackMode::REMOTE);
	log("HP: %d", _myHero->getHealthComp()->getMaxState());
	_myHero->setPosition(visibleSize / 2);
	_myHero->setTag(TAG_MYHERO);
	_myHero->setScale(0.5);
	_myHero->setRecordComp(_labelRecord);
	_map->addChild(_myHero);
	_myHero->setZOrder(1);
	_heroes.pushBack(_myHero);
	_actors.pushBack(_myHero);


	auto labelDie = Label::create("D I E", "fonts/HELVETICAEXT-NORMAL.TTF", 100);
	labelDie->setPosition(visibleSize / 2);
	labelDie->setVisible(false);
	labelDie->setTag(TAG_DIE);
	addChild(labelDie);
}

void HelloWorld::initHRocker()
{
	_rocker = HRocker::createHRocker("rocker.png", "rockerBG.png", Vec2(200, 100));
	_rocker->startRocker(true);
	addChild(_rocker);
}

void HelloWorld::initSkillPanel()
{
	String heroName = _myHero->getHeroName();

	auto sprSkill_1 = Sprite::create(StringUtils::format("pictures//hero//%s//%sSkill11.png",heroName.getCString(),heroName.getCString()));
	sprSkill_1->setPosition(Vec2(949, 88));
	sprSkill_1->setTag(TAG_SPRSKILL_1);
	addChild(sprSkill_1);

	auto sprSkill_2 = Sprite::create(StringUtils::format("pictures//hero//%s//%sSkill21.png", heroName.getCString(), heroName.getCString()));
	sprSkill_2->setPosition(Vec2(1027, 222));
	sprSkill_2->setTag(TAG_SPRSKILL_2);
	addChild(sprSkill_2);

	auto sprSkill_3 = Sprite::create(StringUtils::format("pictures//hero//%s//%sSkill31.png", heroName.getCString(), heroName.getCString()));
	sprSkill_3->setPosition(Vec2(1161.5, 298.5));
	sprSkill_3->setTag(TAG_SPRSKILL_3);
	addChild(sprSkill_3);

	auto sprPlus_1 = Sprite::create("pictures//others//SkillLevelUp.png");
	sprPlus_1->setPosition(870.0, 151.0);
	sprPlus_1->setTag(TAG_SPRPLUS_1);
	sprPlus_1->setVisible(false);
	addChild(sprPlus_1);

	auto sprPlus_2 = Sprite::create("pictures//others//SkillLevelUp.png");
	sprPlus_2->setPosition(959.0, 290.0);
	sprPlus_2->setTag(TAG_SPRPLUS_2);
	sprPlus_2->setVisible(false);
	addChild(sprPlus_2);

	auto sprPlus_3 = Sprite::create("pictures//others//SkillLevelUp.png");
	sprPlus_3->setPosition(1095, 367.0);
	sprPlus_3->setTag(TAG_SPRPLUS_3);
	sprPlus_3->setVisible(false);
	addChild(sprPlus_3);
}

void HelloWorld::initTower()
{
	auto tower = Actor::create(this, ECamp::RED);
	tower->setPosition(RED_TOWER_POSITION);
	tower->setScale(0.7);
	_map->addChild(tower);
	_towers.pushBack(tower);
	_actors.pushBack(tower);
}

void HelloWorld::update(float delta)
{
	TowerAttack();

	updateHeroPosition();

	updateBullets();

	updateDamages();

	updateSoldiersState();

	clearObjects();

	updateSkillPanel();
}

void HelloWorld::clearObjects()
{
	auto nowTime = GetCurrentTime() / 1000.f;

	for (auto it = _heroes.begin(); it != _heroes.end(); ++it)
	{
		if ((*it)->getAlreadyDead())
		{
			if ((*it)->getResurgenceTime() <= nowTime)
			{
				(*it)->reborn();

				if ((*it) == _myHero)
				{
					_map->setPosition(Vec2::ZERO);
					getChildByTag(TAG_DIE)->setVisible(false);
				}
			}
			else if ((*it) == _myHero)
			{
				getChildByTag(TAG_DIE)->setVisible(true);
			}
		}
		else
		{
			(*it)->clearBuff();
		}
	}

	for (auto it = _soldiers.begin(); it != _soldiers.end();)
	{

		if ((*it)->getAlreadyDead())
		{
			_map->removeChild(*it);
			(*it)->getHealthComp()->unscheduleAllSelectors();
			it = _soldiers.erase(it);
		}
		else
		{
			(*it)->clearBuff();
			++it;
		}
	}

	for (auto it = _towers.begin(); it != _towers.end();)
	{
		if ((*it)->getAlreadyDead())
		{
			_map->removeChild(*it);
			(*it)->getHealthComp()->unscheduleAllSelectors();
			it = _towers.erase(it);
		}
		else
		{
			(*it)->clearBuff();
			++it;
		}
	}
}

void HelloWorld::TowerAttack()
{
	for (auto it = _towers.begin(); it != _towers.end(); ++it)
	{
		(*it)->attack();
	}
}

void HelloWorld::updateBullets()
{
	auto nowTime = GetCurrentTime() / 1000.f;

	for (auto it = _readyToLaunch.begin(); it != _readyToLaunch.end();)
	{
		if ((*it).first <= nowTime)
		{
			if (!(*it).second->getTarget()->getAlreadyDead() && !(*it).second->getFromActor()->getAlreadyDead())
			{
				(*it).second->setPosition((*it).second->getFromActor()->getPosition());
				_map->addChild((*it).second);
				_bullets.pushBack((*it).second);
			}
			it = _readyToLaunch.erase(it);
		}
		else
		{
			++it;
		}
	}

	for (auto it = _bullets.begin(); it != _bullets.end();)
	{
		if (!(*it)->getTarget()->getAlreadyDead())
		{
			if ((*it)->calculateDistance() < (*it)->getTarget()->getBoundingBox().size.height / 2)
			{
				auto target = (*it)->getTarget();
				target->takeDamage(EDamageType::PHYSICS_DAMAGE, (*it)->getDamage(), (*it)->getFromActor());
				_map->removeChild(*it);
				it = _bullets.erase(it);
			}
			else
			{
				(*it)->calculatePosition();
				++it;
			}
		}
		else
		{
			_map->removeChild(*it);
			it = _bullets.erase(it);
		}
	}

}

void HelloWorld::updateDamages()
{
	for (auto it = _damages.begin(); it != _damages.end();)
	{
		auto nowTime = GetCurrentTime() / 1000.f;

		if (it->_actionTime <= nowTime)
		{
			if (!it->_target->getAlreadyDead())
			{
				it->_target->takeDamage(it->_damageType, it->_damage, it->_instiogator);
			}

			it = _damages.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void HelloWorld::test()
{
	ValueMap data;
	ValueVector p;
	for (int i = 0; i < 4; ++i)
	{
		p.push_back(Value(5.0));
	}
	data["CD"] = p;
	data["MP"] = p;
	FileUtils::getInstance()->writeValueMapToFile(data, "D:\\LatestFiles\\hello\\Data\\data.plist");
}

void HelloWorld::updateSoldiersState()
{
	for (auto& i : _soldiers)
	{
		if (!i->getAlreadyDead())
		{
			i->updateState();
		}
	}
}

void HelloWorld::updateHeroPosition()
{
	if (_rocker->getIsCanMove())
	{
		_myHero->setStandingAngle(_rocker->getAngle());
		auto positionDelta = MyMath::calculatePositionDelta(_myHero->getStandingAngle(),_myHero->getMoveSpeed());
		auto newPosition = _myHero->getPosition() + positionDelta;
		if (_mapInformation.checkCollision(newPosition))
		{
			_myHero->heroMove();
			_map->setPosition(_map->getPosition() - positionDelta);
		}
	}
	else
	{
		_myHero->stopMove();
	}
}

void HelloWorld::updateSkillPanel()
{
	auto nowTime = GetCurrentTime() / 1000.f;
	auto heroName = _myHero->getHeroName();
	auto skillPoints = _myHero->getSkillPoint();

	auto skillTime_1 = _myHero->getCalmTime_1() + _myHero->getLastSkillTime_1();
	auto skillTime_2 = _myHero->getCalmTime_2() + _myHero->getLastSkillTime_2();
	auto skillTime_3 = _myHero->getCalmTime_3() + _myHero->getLastSkillTime_3();

	auto skillLevel_1 = _myHero->getSkillLevel_1();
	auto skillLevel_2 = _myHero->getSkillLevel_2();
	auto skillLevel_3 = _myHero->getSkillLevel_3();

	auto sprSkill_1 = dynamic_cast<Sprite*>(getChildByTag(TAG_SPRSKILL_1));
	auto sprSkill_2 = dynamic_cast<Sprite*>(getChildByTag(TAG_SPRSKILL_2));
	auto sprSkill_3 = dynamic_cast<Sprite*>(getChildByTag(TAG_SPRSKILL_3));
	auto sprPlus_1 = dynamic_cast<Sprite*>(getChildByTag(TAG_SPRPLUS_1));
	auto sprPlus_2 = dynamic_cast<Sprite*>(getChildByTag(TAG_SPRPLUS_2));
	auto sprPlus_3 = dynamic_cast<Sprite*>(getChildByTag(TAG_SPRPLUS_3));

	if (skillLevel_1 == 0 || skillTime_1 >= nowTime)
	{
		sprSkill_1->setTexture(StringUtils::format("pictures//hero//%s//%sSkill11.png", heroName.getCString(), heroName.getCString()));
	}
	else
	{
		sprSkill_1->setTexture(StringUtils::format("pictures//hero//%s//%sSkill1.png", heroName.getCString(), heroName.getCString()));
	}

	if (skillLevel_2 == 0 || skillTime_2 >= nowTime)
	{
		sprSkill_2->setTexture(StringUtils::format("pictures//hero//%s//%sSkill21.png", heroName.getCString(), heroName.getCString()));
	}
	else
	{
		sprSkill_2->setTexture(StringUtils::format("pictures//hero//%s//%sSkill2.png", heroName.getCString(), heroName.getCString()));
	}

	if (skillLevel_3 == 0 || skillTime_3 >= nowTime)
	{
		sprSkill_3->setTexture(StringUtils::format("pictures//hero//%s//%sSkill31.png", heroName.getCString(), heroName.getCString()));
	}
	else
	{
		sprSkill_3->setTexture(StringUtils::format("pictures//hero//%s//%sSkill3.png", heroName.getCString(), heroName.getCString()));
	}

	if (skillPoints)
	{
		if (_myHero->getSkillLevel_1() < 3)
		{
			sprPlus_1->setVisible(true);
		}
		else
		{
			sprPlus_1->setVisible(false);
		}

		if (_myHero->getSkillLevel_2() < 3)
		{
			sprPlus_2->setVisible(true);
		}
		else
		{
			sprPlus_2->setVisible(false);
		}

		if (_myHero->getSkillLevel_3() < 3)
		{
			sprPlus_3->setVisible(true);
		}
		else
		{
			sprPlus_3->setVisible(false);
		}
	}
	else
	{
		sprPlus_1->setVisible(false);
		sprPlus_2->setVisible(false);
		sprPlus_3->setVisible(false);
	}

}

void HelloWorld::generateSoldiers(float delta)
{
	auto visibleSize = Director::getInstance()->getVisibleSize();
	auto origin = Director::getInstance()->getVisibleOrigin();

	auto soldier_1 = Soldier::create(this, EAttackMode::MELEE, ECamp::RED, ERoad::DOWNWAY, _soldierPathPoints);
	soldier_1->setPosition(visibleSize / 4);
	soldier_1->setNextDest(_soldierPathPoints->getNextPoint(soldier_1->getPosition()));
	soldier_1->setScale(1.5);
	_map->addChild(soldier_1);
	_soldiers.pushBack(soldier_1);
	_actors.pushBack(soldier_1);

	auto soldier_2 = Soldier::create(this, EAttackMode::MELEE, ECamp::RED, ERoad::DOWNWAY, _soldierPathPoints);
	soldier_2->setPosition(visibleSize / 4 + Size(0, 30));
	soldier_2->setNextDest(_soldierPathPoints->getNextPoint(soldier_2->getPosition()));
	soldier_2->setScale(1.5);
	_map->addChild(soldier_2);
	_soldiers.pushBack(soldier_2);
	_actors.pushBack(soldier_2);

	auto soldier_3 = Soldier::create(this, EAttackMode::MELEE, ECamp::RED, ERoad::DOWNWAY, _soldierPathPoints);
	soldier_3->setPosition(visibleSize / 4 + Size(0, 60));
	soldier_3->setNextDest(_soldierPathPoints->getNextPoint(soldier_3->getPosition()));
	soldier_3->setScale(1.5);
	_map->addChild(soldier_3);
	_soldiers.pushBack(soldier_3);
	_actors.pushBack(soldier_3);

	auto soldier_4 = Soldier::create(this, EAttackMode::REMOTE, ECamp::RED, ERoad::DOWNWAY, _soldierPathPoints);
	soldier_4->setPosition(visibleSize / 4 + Size(0, 90));
	soldier_4->setNextDest(_soldierPathPoints->getNextPoint(soldier_4->getPosition()));
	soldier_4->setScale(1.5);
	_map->addChild(soldier_4);
	_soldiers.pushBack(soldier_4);
	_actors.pushBack(soldier_4);

	auto skillPosition = visibleSize / 4 + (Size)_map->getPosition();
	log("skillPosition: %f, %f", skillPosition.width, skillPosition.height);
	_myHero->castSkill_2(visibleSize / 4 + (Size)_map->getPosition());
}
