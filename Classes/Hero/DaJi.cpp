#include "DaJi.h"
#include "Scene/HelloWorldScene.h"

DaJi* DaJi::create(HelloWorld* combatScene, ECamp camp, std::string heroName, EAttackMode attackMode)
{
	DaJi* daJi = new(std::nothrow) DaJi;
	if (daJi && daJi->init(combatScene, camp, heroName, attackMode))
	{
		daJi->autorelease();
		return daJi;
	}
	CC_SAFE_DELETE(daJi);
	return nullptr;
}

bool DaJi::init(HelloWorld* combatScene, ECamp camp, std::string heroName, EAttackMode attackMode)
{
	if (!Hero::init(combatScene, camp, heroName, attackMode))
	{
		return false;
	}
	_magicAttack = _attack;
	return true;
	
}

void DaJi::updateSprSkill_1_Position(float delta)
{
	float effectRadius = 100.f;
	auto sprSkill = dynamic_cast<Sprite*>(_combatScene->getMap()->getChildByTag(TAG_DAJI_SKILL1));
	auto deltaPosition = MyMath::calculatePositionDelta(_shockWaveAngle, SPEED_FLY * 3);
	auto newPosition = sprSkill->getPosition() + deltaPosition;
	sprSkill->setPosition(newPosition);
	float contactLimit = 20.f + sprSkill->getBoundingBox().size.width / 2;

	if (sprSkill->getPosition().distance(_skillPosition) > 600.f)
	{
		unschedule(schedule_selector(DaJi::updateSprSkill_1_Position));
		_skillDamageActors.clear();
		_combatScene->getMap()->removeChild(sprSkill);
		return;
	}
	auto targetPosition = Vec2::ZERO;
	auto damage = 260 + 160 * _skillLevel_1 + 1 *_magicAttack;
//	log("damage: %d", damage);
	auto passiveBuff = Buff::create(EBuffType::NORMAL, 3, 0, 0, -60, 0, 0, 0, 0, 0, 0);
	for (auto it = _combatScene->_soldiers.begin(); it != _combatScene->_soldiers.end(); it++)
	{
		if (_camp != (*it)->getCamp() && (*it)->getPosition().distance(sprSkill->getPosition()) < contactLimit && _skillDamageActors.find(*it) == _skillDamageActors.end())
		{
			_skillDamageActors.insert(*it);
			(*it)->takeBuff(passiveBuff);
			(*it)->takeDamage(EDamageType::MAGIC_DAMAGE, damage, this);
		}
	}
	for (auto it = _combatScene->_heroes.begin(); it != _combatScene->_heroes.end(); it++)
	{
		if (!(*it)->getAlreadyDead() && _camp != (*it)->getCamp() && (*it)->getPosition().distance(sprSkill->getPosition()) < contactLimit && _skillDamageActors.find(*it) == _skillDamageActors.end())
		{
			_skillDamageActors.insert(*it);
			(*it)->takeBuff(passiveBuff);
			(*it)->takeDamage(EDamageType::MAGIC_DAMAGE, damage, this);
		}
	}
}

void DaJi::castSkill_1(Point position)
{
	log("HPRecover : %d", _healthComp->getRecoverRate());
	//技能蓝耗
	_magicComp->changeStateBy(-1 * _magicConsume_1);
	//更新技能释放时间
	auto nowTime = GetCurrentTime() / 1000.f;
	_lastSkillTime_1 = nowTime;
	//初始化技能精灵
	_shockWaveAngle = MyMath::getRad(getPosition(), position);
	_skillPosition = getPosition();
	auto sprSkill_1 = Sprite::create("pictures/hero/DaJi/skill1.png");
	sprSkill_1->setPosition(getPosition());
	sprSkill_1->setRotation(180 - _shockWaveAngle * 180 / M_PI);
	sprSkill_1->setTag(TAG_DAJI_SKILL1);
	_combatScene->getMap()->addChild(sprSkill_1);
	//逐帧更新技能精灵
	schedule(schedule_selector(DaJi::updateSprSkill_1_Position), 1 / 60, -1, 0.f);
}

void DaJi::castSkill_2()
{
	log("HPRecover : %d", _healthComp->getRecoverRate());
	updateAttackTarget();
	if (_attackTarget)
	{
		//技能蓝耗
		_magicComp->changeStateBy(-1 * _magicConsume_2);
		//更新技能释放时间
		auto nowTime = GetCurrentTime() / 1000.f;
		_lastSkillTime_2 = nowTime;
		//初始化技能伤害和效果，buff在HelloWorld::updateBullets中生成
		auto damage = 197.5 + 87.5*_skillLevel_2 + 0.66*_magicAttack;
		//初始化技能精灵
		auto sprSkill_2 = Projectile::create("pictures/hero/DaJi/Normal.png", damage, SPEED_FLY, this, _attackTarget);
		sprSkill_2->setScale(0.7);
		sprSkill_2->setTag(TAG_DAJI_SKILL2);
		sprSkill_2->setPosition(getPosition());
		_combatScene->getMap()->addChild(sprSkill_2);
		//
	}
}

void DaJi::castSkill_3()
{
	log("HPRecover : %d", _healthComp->getRecoverRate());
	updateAttackTarget();
	if (_attackTarget)
	{		
		//技能蓝耗
		_magicComp->changeStateBy(-1 * _magicConsume_3);
		//更新技能释放时间
		auto nowTime = GetCurrentTime() / 1000.f;
		_lastSkillTime_3 = nowTime;
		//添加多个远程普攻飞行物
		for (int i = 0; i < 10; i++)
		{
			auto projecctile = Projectile::create("pictures/hero/DaJi/Normal.png", _magicAttack*0.5, SPEED_FLY, this, _attackTarget);
			projecctile->setScale(0.2);
			_combatScene->_readyToLaunch.insert(nowTime + 0.1*i, projecctile);
		}
	}
}
bool DaJi::attack()
{
	auto nowTime = GetCurrentTime() / 1000.f;
	//当不满足攻击间隔的时候，按攻击键调attack的情况
	if (nowTime - _lastAttackTime < _minAttackInterval)
	{
		return false;
	}
	//先重新生成攻击目标
	updateAttackTarget();
	if (!_attackTarget)
	{
		return false;
	}
	//更新攻击状态和停止动画
	stopAllActions();
	_lastAttackTime = nowTime;
	//根据新的攻击目标计算攻击时人物的角度
	_standingAngle = MyMath::getRad(getPosition(), _attackTarget->getPosition());
	updateDirection();
	//播放攻击动画
	playAttackAnimation();
	auto projecctile = Projectile::create("pictures/hero/DaJi/Normal.png", _magicAttack, SPEED_FLY, this, _attackTarget);
	projecctile->setScale(0.2);
	_combatScene->_readyToLaunch.insert(nowTime + _minAttackInterval / 10 * 2, projecctile);

	return true;
}