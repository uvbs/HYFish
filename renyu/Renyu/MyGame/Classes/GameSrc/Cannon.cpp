//
//  Cannon.cpp
//  MyGame
//
//  Created by wh on 15/6/1.
//
//

#include "Cannon.h"
#include "GameDataMgr.h"
#include "HallDataMgr.h"
#include "NetworkMgr.h"
#include "SimpleAudioEngine.h"
USING_NS_CC;
using namespace ui;

#define Tag_Award  10
#define Tag_Lignt  20
#define Tag_Type   30

#define AutoShoot  "autoshoot"
#define OtherShoot "othershoot"
#define TypeTime   "typetime"

Cannon * Cannon::create()
{
    auto pcannon = new Cannon;
    if (pcannon->init()) {
        pcannon->autorelease();
        return pcannon;
    }
    CC_SAFE_DELETE(pcannon);
    return nullptr;
}

bool Cannon::init()
{
    if (!Layout::init()) {
        return false;
    }
    m_fort = nullptr;
    m_nickname = nullptr;
    m_score = nullptr;
    m_multiple = nullptr;
    m_cannonpoint = Vec2::ZERO;
    m_isshoot = false;
    m_Type = Normal_Cannon;
    m_index = 0;
    m_autoshoot = false;
    m_canshoot = true;
    m_typeTime = 0.f;
    m_targetpoint = Vec2::ZERO;
    return true;
}

bool Cannon::initwithUser(UserData *puser)
{
    this->setContentSize(cocos2d::Size(238,106));
    this->setAnchorPoint(Vec2(0.5f, 0.5f));
    auto cannonbg = Sprite::create("game_res/im_cannon_bg.png");
    cannonbg->setPosition(Vec2(cannonbg->getContentSize().width/2, this->getContentSize().height/2));
    this->addChild(cannonbg);
    
    m_fort = Sprite::createWithSpriteFrameName("fort_0.png");
    m_fort->setPosition(Vec2(44, 65));
    m_fort->setAnchorPoint(Vec2(0.5f, 0.44f));
    this->addChild(m_fort,1);

    auto nicknamebg = Sprite::create("game_res/im_info_bg.png");
    nicknamebg->setPosition(Vec2(166, this->getContentSize().height-45));
    this->addChild(nicknamebg);
    
    m_userID = puser->m_date.dwUserID;
    m_nickname = Label::createWithSystemFont(puser->m_nickname, FontNormal, 18);
    m_nickname->setPosition(Vec2(86, nicknamebg->getContentSize().height/2));
    m_nickname->setAlignment(TextHAlignment::CENTER);
    nicknamebg->addChild(m_nickname);
    
    m_pos = puser->m_date.wChairID;
    if (GameDataMgr::getInstance()->m_reversal) {
        m_pos = 5-m_pos;
    }
    if (m_pos < 3 ) {
        cannonbg->setRotation(180);
        cannonbg->setFlippedX(true);
        m_fort->setPositionY(this->getContentSize().height-65);
        m_fort->setRotation(180);
    }
    if (m_pos > 2) {
        nicknamebg->setPositionY(45);
    }
    
    if (m_userID != HallDataMgr::getInstance()->m_dwUserID)
    {
        auto scorestr = __String::createWithFormat("%lld", puser->m_date.lScore)->getCString();
        m_score = Label::createWithSystemFont(scorestr, FontNormal, 18);
        m_score->setDimensions(140, 22);
        m_score->setHorizontalAlignment(TextHAlignment::CENTER);
        m_score->setPosition(Vec2(86, 12));
        nicknamebg->addChild(m_score);
        if (m_pos<3) {
            m_nickname->setPositionY(nicknamebg->getContentSize().height-12);
        }
        if (m_pos > 2) {
            m_nickname->setPositionY(12);
            m_score->setPositionY(nicknamebg->getContentSize().height-12);
        }
        
        auto multiplebg = Sprite::create("game_res/im_multiple_bg.png");
        multiplebg->setPosition(Vec2(166, this->getContentSize().height-76));
        if (m_pos > 2) {
            multiplebg->setPositionY(76);
        }
        this->addChild(multiplebg);
        
        m_multiple = Label::createWithSystemFont("100倍", FontNormal, 15);
        m_multiple->setColor(Color3B::YELLOW);
        m_multiple->setPosition(multiplebg->getContentSize()/2);
        multiplebg->addChild(m_multiple);
    }
    Labellengthaddpoint(m_nickname, 140);
    
    return true;
}

void Cannon::setScore(SCORE score)
{
    if (m_userID == HallDataMgr::getInstance()->m_dwUserID) {
        Director::getInstance()->getEventDispatcher()->dispatchCustomEvent(whEvent_User_Data_Change);
    }
    if (!m_score) {
        return;
    }
    auto scorestr = __String::createWithFormat("%lld", score)->getCString();
    m_score->setString(scorestr);
}

void Cannon::setMultiple(DWORD multiple)
{
    if (m_multiple) {
        m_multiple->setString(__String::createWithFormat("%d倍",multiple)->getCString());
    }
}

void Cannon::shoot(cocos2d::Vec2 &vec2, bool isbegin)
{
    if (!m_canshoot) {
        m_isshoot = isbegin;
        return;
    }
    if (m_cannonpoint.isZero()) {
        m_cannonpoint = this->convertToWorldSpace(m_fort->getPosition());
    }
    float angle = GameDataMgr::getInstance()->getAngleByTwoPoint(vec2, m_cannonpoint);
    m_targetpoint = vec2;
    log("%f",angle);
    if (angle<90.f) {
        if (!GameDataMgr::getInstance()->m_autolock) {
            m_fort->setRotation(angle);
        }
    }
    if (m_Type == Laser_Shooting) {
        return;
    }
    if (m_Type == Laser_Cannon) {
        
        this->shootLaster();
        return;
    }
    
    if (m_autoshoot || GameDataMgr::getInstance()->m_autolock) {
        return;
    }
    if (m_isshoot == false && isbegin == true) {
        m_isshoot = true;
        auto ptime = GameDataMgr::getInstance()->m_sceneData.nBulletCoolingTime[BULLET_EIGHT]/1000.f;
        if (m_Type == Special_Cannon) {
            ptime /= 2;
        }
        this->selfupdateshoot(0);
        this->schedule([=](float t){this->selfupdateshoot(t);}, ptime, AutoShoot);
    }
    
    if (isbegin == false ) {
        m_isshoot = false;
        this->unschedule(AutoShoot);
    }
}

void Cannon::shoot(CMD_S_Fire *fireinfo)
{
    auto pfire = new CMD_S_Fire;
    memset(pfire, 0, sizeof(CMD_S_Fire));
    memcpy(pfire, fireinfo, sizeof(CMD_S_Fire));
    m_firelist.push_back(pfire);
    setMultiple(fireinfo->nBulletScore);
    
    auto ptime = GameDataMgr::getInstance()->m_sceneData.nBulletCoolingTime[BULLET_EIGHT]/1200.f;
    if (m_Type == Special_Cannon) {
        ptime /= 2;
    }
    this->schedule(CC_CALLBACK_1(Cannon::updateshoot, this), ptime, OtherShoot);
}

void Cannon::shootLaster()
{
    CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("sound_res/PROP_ARMOUR_PIERCING.wav");
    m_Type = Laser_Shooting;
    auto panim = Animate::create(AnimationCache::getInstance()->getAnimation(FortLightAnim));
    m_fort->runAction(panim);
    this->removeChildByTag(Tag_Lignt);
    
    auto playout = Layout::create();
    playout->setAnchorPoint(Vec2(0.5f, 0.5f));
    playout->setContentSize(cocos2d::Size(10,10));
    auto fangle = m_fort->getRotation();
    fangle = CC_DEGREES_TO_RADIANS(90-fangle);
    auto movedir = Vec2::forAngle(fangle);
    movedir.scale(50);
    playout->setPosition(m_cannonpoint+movedir);
    this->getParent()->addChild(playout);
    
    auto plight = Sprite::createWithSpriteFrameName("light.png");
    plight->setPosition(Vec2(playout->getContentSize().width/2, 0));
    plight->setScale(0.5f, 1.f);
    plight->runAction(Sequence::create(ScaleTo::create(1.f, 1.f, 1.f),ScaleTo::create(1.f, 0.5f, 1.f) ,CallFunc::create([=]{plight->removeFromParent();}), NULL));
    playout->addChild(plight);
    
    for (int index=0; index<4; ++index) {
        auto pfortlight = Sprite::createWithSpriteFrameName(__String::createWithFormat("fortlight_%d.png", index)->getCString());
        pfortlight->setPosition(Vec2(playout->getContentSize().width/2, pfortlight->getContentSize().height*0.6f + index*pfortlight->getContentSize().height*1.2f - 5*index));
        pfortlight->setScale(0.1f,1.2f);
        pfortlight->runAction(Sequence::create(ScaleTo::create(0.5f, 1.f, 1.2f), ScaleTo::create(2.f, 0.f, 1.2f), NULL));
        playout->addChild(pfortlight);
    }
    playout->setRotation(m_fort->getRotation());
    
    playout->runAction(Sequence::createWithTwoActions(DelayTime::create(2.3f), CallFunc::create([=]{
        playout->removeFromParent();
        m_Type = Normal_Cannon;
        m_fort->setSpriteFrame("fort_0.png");
        if (m_autoshoot || m_isshoot) {
            auto ptime = GameDataMgr::getInstance()->m_sceneData.nBulletCoolingTime[BULLET_EIGHT]/1000.f;
            if (m_Type == Special_Cannon) {
                ptime /= 2;
            }
            this->selfupdateshoot(0);
            this->schedule([=](float t){this->selfupdateshoot(t);}, ptime, AutoShoot);
        }
        else if(m_firelist.size())
        {
            auto ptime = GameDataMgr::getInstance()->m_sceneData.nBulletCoolingTime[BULLET_EIGHT]/1200.f;
            if (m_Type == Special_Cannon) {
                ptime /= 2;
            }
            this->schedule(CC_CALLBACK_1(Cannon::updateshoot, this), ptime, OtherShoot);
        }
    })));
    
    if (m_userID == HallDataMgr::getInstance()->m_dwUserID) {
        CMD_C_Laser prequst;
        memset(&prequst, 0, sizeof(prequst));
        
        auto bpoint = GameDataMgr::getInstance()->convertCoordinateSystem(m_cannonpoint, 0, GameDataMgr::getInstance()->m_reversal);
        auto epoint = GameDataMgr::getInstance()->convertCoordinateSystem(playout->getPosition(), 0, GameDataMgr::getInstance()->m_reversal);
        prequst.ptBeginPos = CShortPoint(bpoint.x, bpoint.y);
        prequst.ptEndPos = CShortPoint(epoint.x, epoint.y);
        prequst.unLossTime = (DWORD)(getsystemtomillisecond() - GameDataMgr::getInstance()->getEnterTime());
        NetworkMgr::getInstance()->sendData(MDM_GF_GAME, SUB_C_LASER, &prequst, sizeof(prequst));
    }
}

void Cannon::updateshoot(float t)
{
    auto pinfo = m_firelist.at(0);
    auto point = Vec2(pinfo->ptPos.x, pinfo->ptPos.y);
    point = GameDataMgr::getInstance()->convertCoordinateSystem(point, 1, GameDataMgr::getInstance()->m_reversal);
    
    if (m_cannonpoint.isZero()) {
        m_cannonpoint = this->convertToWorldSpace(m_fort->getPosition());
    }
    
    float angle = GameDataMgr::getInstance()->getAngleByTwoPoint(point, m_cannonpoint);
    m_fort->setRotation(angle);
    
    
    this->productbullet(false, pinfo->nTrackFishIndex, GETCOLOR_LONG(pinfo->crFishNetColoer));
    auto puser = HallDataMgr::getInstance()->m_UserList.at(m_userID);
    if (puser) {
        puser->m_date.lScore -= pinfo->nBulletScore;
        if (puser->m_date.lScore < 0) {
            puser->m_date.lScore = 0;
        }
        this->setScore(puser->m_date.lScore);
    }
    
    for (auto iter=m_firelist.begin(); iter!=m_firelist.end(); ++iter) {
        if (*iter == pinfo) {
            m_firelist.erase(iter);
            break;
        }
    }
    
    CC_SAFE_DELETE(pinfo);
    if (!m_firelist.size()) {
        m_firelist.clear();
        this->unschedule(OtherShoot);
        m_isshoot = false;
    }
}

void Cannon::selfupdateshoot(float t)
{
    if (!m_canshoot || m_Type == Laser_Cannon ) {
        return;
    }
    auto puser = HallDataMgr::getInstance()->m_UserList.at(m_userID);
    
    auto pscore = puser->m_date.lScore - GameDataMgr::getInstance()->m_sceneData.nMultipleValue[GameDataMgr::getInstance()->m_sceneData.nMultipleIndex];
    if (pscore<0) {
        this->unschedule(AutoShoot);
        m_autoshoot = false;
        HallDataMgr::getInstance()->AddpopLayer("系统提示", "金币不足", Type_Ensure);
        if (GameDataMgr::getInstance()->m_autolock ) {
            GameDataMgr::getInstance()->m_autolock = false;
        }
        if (GameDataMgr::getInstance()->m_autoshoot) {
            GameDataMgr::getInstance()->m_autoshoot = false;
        }
        Director::getInstance()->getEventDispatcher()->dispatchCustomEvent(whNd_Auto_Change);
        return;
    }
    if(m_autoshoot && GameDataMgr::getInstance()->m_autolock)
    {
        if(GameDataMgr::getInstance()->m_fishIndex == INT_MAX)
        {
            return;
        }
        auto pfish = GameDataMgr::getInstance()->m_fishList.at(GameDataMgr::getInstance()->m_fishIndex);
        if (!pfish) {
            return;
        }
        auto fishpoint = pfish->getPosition();
        if (GameDataMgr::getInstance()->m_reversal) {
            fishpoint = Vec2(WinSize.width, WinSize.height) - fishpoint;
        }
        auto fangle = GameDataMgr::getInstance()->getAngleByTwoPoint(fishpoint, m_cannonpoint);
        m_fort->setRotation(fangle);
        this->productbullet(true, GameDataMgr::getInstance()->m_fishIndex, GameDataMgr::getInstance()->getNetColor());
    }
    else
    {
        this->productbullet(true,INT_MAX, GameDataMgr::getInstance()->getNetColor());
    }
    puser->m_date.lScore = pscore;
    GameDataMgr::getInstance()->m_sceneData.lBulletConsume[0] += GameDataMgr::getInstance()->m_sceneData.nMultipleValue[GameDataMgr::getInstance()->m_sceneData.nMultipleIndex];
    this->setScore(puser->m_date.lScore);
}

void Cannon::typeTimeUpdate(float t)
{
    m_typeTime -= t;
    int time = (int)m_typeTime;
    auto pnode = this->getChildByTag(Tag_Type);
    if (pnode) {
        auto ptxt = (Label *)pnode->getChildByTag(Tag_Type);
        ptxt->setString(__String::createWithFormat("%d",time)->getCString());
    }
    
    if (m_typeTime <= 0) {
        this->unschedule(TypeTime);
        this->setCannonType(Normal_Cannon, 0);
    }
}

void Cannon::productbullet(bool isself, DWORD fishindex, cocos2d::Color3B color)
{
    m_index++;
    m_index = m_index%UINTMAX_MAX;
    auto paction = Animate::create(AnimationCache::getInstance()->getAnimation(FortAnim));
    m_fort->runAction(paction);
    auto fangle = m_fort->getRotation();
    
    auto pbullet = Bullet::create(fangle);
    auto pbullet1 = Bullet::create(fangle);
    
    fangle = CC_DEGREES_TO_RADIANS(90-fangle);
    auto movedir = Vec2::forAngle(fangle);
    movedir.scale(25);
    auto offsetvec2 = Vec2(20*sinf(fangle), 20*cosf(fangle));
    
    pbullet->setType(m_Type);
    pbullet->setIndex(m_index);
    pbullet->setPos(m_pos);
    pbullet->setBbullet(pbullet1);
    pbullet->setIsSelf(isself);
    pbullet->setFishIndex(fishindex);
    pbullet->setNetColor(color);
    pbullet->setPosition(m_cannonpoint+movedir+Vec2(offsetvec2.x/2, -offsetvec2.y/2));
    this->getParent()->addChild(pbullet);
    //GameDataMgr::getInstance()->m_bulletList.at(m_pos).pushBack(pbullet);

    pbullet1->setType(m_Type);
    pbullet1->setIndex(m_index);
    pbullet1->setPos(m_pos);
    pbullet1->setBbullet(pbullet);
    pbullet1->setIsSelf(isself);
    pbullet1->setFishIndex(fishindex);
    pbullet1->setNetColor(color);
    pbullet1->setPosition(m_cannonpoint+movedir+Vec2(-offsetvec2.x/2, offsetvec2.y/2));
    this->getParent()->addChild(pbullet1);
    //GameDataMgr::getInstance()->m_bulletList.at(m_pos).pushBack(pbullet1);
    
    if (isself) {
        CMD_C_Fire request;
        memset(&request, 0, sizeof(request));
        
        request.nBulletKey = m_index;
        request.crFishNetColoer = GLCOLOR_RGB1(Color3B::RED);
        request.nTrackFishIndex = pbullet->getFishIndex();
        request.nPropIndex = PROP_COUNT_MAX;
        auto targetPoint = m_cannonpoint+movedir*25;
        targetPoint = GameDataMgr::getInstance()->convertCoordinateSystem(targetPoint, 0, GameDataMgr::getInstance()->m_reversal);
        //log("targetpoint x_%f, y_%f",targetPoint.x,targetPoint.y);
        request.ptPos.x = targetPoint.x;
        request.ptPos.y = targetPoint.y;
        NetworkMgr::getInstance()->sendData(MDM_GF_GAME, SUB_C_FIRE, &request, sizeof(request));
        
//        if (!m_autoshoot) {
//            pbullet->setTargetPoint(m_targetpoint);
//            pbullet1->setTargetPoint(m_targetpoint);
//        }
        CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("sound_res/SHELL_8.wav");
    }
}

void Cannon::setAutoShoot(bool b)
{
    m_autoshoot = b;
    if (m_cannonpoint.isZero()) {
        m_cannonpoint = this->convertToWorldSpace(m_fort->getPosition());
    }
    if (m_Type >= Laser_Cannon) {
        return;
    }
    if (m_autoshoot) {
        auto ptime = GameDataMgr::getInstance()->m_sceneData.nBulletCoolingTime[BULLET_EIGHT]/1000.f;
        if (m_Type == Special_Cannon) {
            ptime /= 2;
        }
        this->schedule([=](float t){this->selfupdateshoot(t);}, ptime, AutoShoot);
    }
    else
    {
        this->unschedule(AutoShoot);
    }
}

void Cannon::setCannonType(CannonType type, int time)
{
    if (m_Type == Special_Cannon && type != Special_Cannon) {
        if (m_autoshoot || m_isshoot) {
            this->unschedule(AutoShoot);
            auto ptime = GameDataMgr::getInstance()->m_sceneData.nBulletCoolingTime[BULLET_EIGHT]/1000.f;
            this->schedule([=](float t){this->selfupdateshoot(t);}, ptime, AutoShoot);
        }
        
        if (m_firelist.size()) {
            this->unschedule(OtherShoot);
            auto ptime = GameDataMgr::getInstance()->m_sceneData.nBulletCoolingTime[BULLET_EIGHT]/1200.f;
            this->schedule(CC_CALLBACK_1(Cannon::updateshoot, this), ptime, OtherShoot);
        }
    }
    switch (type) {
        case Special_Cannon:
        {
            m_Type = Special_Cannon;
            if (m_autoshoot || m_isshoot) {
                this->unschedule(AutoShoot);
                auto ptime = GameDataMgr::getInstance()->m_sceneData.nBulletCoolingTime[BULLET_EIGHT]/2000.f;
                this->schedule([=](float t){this->selfupdateshoot(t);}, ptime, AutoShoot);
            }
            
            if (m_firelist.size()) {
                this->unschedule(OtherShoot);
                auto ptime = GameDataMgr::getInstance()->m_sceneData.nBulletCoolingTime[BULLET_EIGHT]/2400.f;
                this->schedule(CC_CALLBACK_1(Cannon::updateshoot, this), ptime, OtherShoot);
            }
            
            m_typeTime  = time;
            this->schedule(CC_CALLBACK_1(Cannon::typeTimeUpdate, this), 1.f, TypeTime);
            this->removeChildByTag(Tag_Type);
            
            auto ptype = Sprite::create("game_res/im_icon_0.png");
            ptype->setTag(Tag_Type);
            ptype->setPosition(Vec2(-16, 40));
            this->addChild(ptype);
            if (m_pos < 3) {
                ptype->setPositionY(this->getContentSize().height-30);
            }
            
            auto ptime = Label::createWithSystemFont(__String::createWithFormat("%d",time)->getCString(), FontNormal, 18);
            ptime->setColor(Color3B::YELLOW);
            ptime->setPosition(Vec2(ptype->getContentSize().width/2, 12));
            ptime->setTag(Tag_Type);
            ptype->addChild(ptime);
        }
            break;
        case Laser_Cannon:
        {
            if (m_Type == Laser_Cannon) {
                if (m_userID == HallDataMgr::getInstance()->m_dwUserID) {
                    m_typeTime = time;
                }
                return;
            }
            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("sound_res/SMALL_BEGIN.wav");
            m_Type = Laser_Cannon;
            this->unschedule(AutoShoot);
            m_fort->stopAllActions();
            m_fort->setSpriteFrame("fort_light_0.png");
            auto plight = Sprite::createWithSpriteFrameName("light_0.png");
            if (m_cannonpoint.isZero()) {
                m_cannonpoint = this->convertToWorldSpace(m_fort->getPosition());
            }
            plight->setPosition(m_fort->getPosition());
            plight->setTag(Tag_Lignt);
            BlendFunc blendFunc = {GL_SRC_ALPHA,GL_ONE};
            plight->setBlendFunc(blendFunc);
            this->addChild(plight);
            
            auto panimate = Animate::create(AnimationCache::getInstance()->getAnimation(LightAnim));
            auto paciton = RepeatForever::create(Sequence::createWithTwoActions(panimate, panimate->reverse()));
            plight->runAction(paciton);
            
            if (m_userID == HallDataMgr::getInstance()->m_dwUserID) {
                this->runAction(Sequence::createWithTwoActions(DelayTime::create(time), CallFunc::create([=]{
                    if (m_Type == Laser_Cannon) {
                        this->shootLaster();
                    }
                })));
            }
            
        }
            break;
        case Bignet_Cannon:
        {
            m_Type = Bignet_Cannon;
            m_typeTime = time;
            this->schedule(CC_CALLBACK_1(Cannon::typeTimeUpdate, this), 1.f, TypeTime);
            this->removeChildByTag(Tag_Type);
            
            auto ptype = Sprite::create("game_res/im_icon_1.png");
            ptype->setTag(Tag_Type);
            ptype->setPosition(Vec2(-16, 40));
            this->addChild(ptype);
            if (m_pos < 3) {
                ptype->setPositionY(this->getContentSize().height-30);
            }
            
            auto ptime = Label::createWithSystemFont(__String::createWithFormat("%d",time)->getCString(), FontNormal, 18);
            ptime->setColor(Color3B::YELLOW);
            ptime->setPosition(Vec2(ptype->getContentSize().width/2, 12));
            ptime->setTag(Tag_Type);
            ptype->addChild(ptime);
        }
            break;
        case Normal_Cannon:
        {
            m_Type = Normal_Cannon;
            this->removeChildByTag(Tag_Type);
        }
            
        default:
            break;
    }
}


void Cannon::fastDeal()
{
    m_canshoot = false;
    this->runAction(Sequence::createWithTwoActions(DelayTime::create(5.f), CallFunc::create([=]{m_canshoot = true;})));
}

void Cannon::showSupply(CMD_S_Supply *psupply)
{
    auto pbox = Sprite::createWithSpriteFrameName("fish_19_die_0.png");
    if (m_pos<3) {
        pbox->setPosition(Vec2(-40, -30));
    }
    else
    {
        pbox->setPosition(Vec2(-40, this->getContentSize().height-30));
    }
    
    if (m_pos==1 || m_pos==3) {
        pbox->setPositionX(170);
    }
    this->addChild(pbox,1);
    int supplytype = psupply->nSupplyType;
    
    auto paction = Animate::create(AnimationCache::getInstance()->getAnimation("fish_19_die"));
    pbox->runAction(Sequence::createWithTwoActions(paction, CallFunc::create([=]{
        if (supplytype != EST_Null) {
            auto pgold = Sprite::create("game_res/im_box_gold.png");
            pgold->setPosition(pbox->getContentSize()/2);
            pbox->addChild(pgold);
            
            auto typestr = __String::createWithFormat("game_res/im_supply_%d.png", supplytype)->getCString();
            Sprite *ptitle = Sprite::create(typestr);
            if (ptitle) {
                ptitle->setPosition(Vec2(pbox->getContentSize().width/2, 150));
                pbox->addChild(ptitle);
            }
        }
    })));
    
    pbox->runAction(Sequence::createWithTwoActions(DelayTime::create(4.f), CallFunc::create([=]{pbox->removeFromParent();})));
    
    if (supplytype == EST_Laser && m_userID==HallDataMgr::getInstance()->m_dwUserID) {
        this->setCannonType(Laser_Cannon, (int)psupply->lSupplyCount);
        CMD_C_BeginLaser prequest;
        memset(&prequest, 0, sizeof(prequest));
        prequest.ptPos.x = rand()%200 + 200.f;
        prequest.ptPos.y = rand()%200 + 200.f;
        NetworkMgr::getInstance()->sendData(MDM_GF_GAME, SUB_C_BEGIN_LASER, &prequest, sizeof(prequest));
    }
    else if (supplytype == EST_Speed)
    {
        this->setCannonType(Special_Cannon, (int)psupply->lSupplyCount);
    }
    else if (supplytype == EST_Strong)
    {
        this->setCannonType(Bignet_Cannon, (int)psupply->lSupplyCount);
    }
}

void Cannon::showAwardTip(CMD_S_AwardTip *paward)
{
    this->removeChildByTag(2);
    auto ptip = Sprite::create("game_res/im_award_0.png");
    ptip->setTag(2);
    if (m_pos<3) {
        ptip->setPosition(Vec2(44, -50));
    }
    else
    {
        ptip->setPosition(Vec2(44, this->getContentSize().height+90));
    }
    this->addChild(ptip,1);
    ptip->setScale(0.1f);
    ptip->runAction(Sequence::create(ScaleTo::create(0.3f, 1.f), DelayTime::create(4.f), ScaleTo::create(0.3f, 0.1f), CallFunc::create([=]{ptip->removeFromParent();}), NULL));
    
    auto plight = Sprite::create("game_res/im_award_1.png");
    plight->setPosition(ptip->getContentSize()/2);
    plight->runAction(RepeatForever::create(RotateBy::create(1.f, 30)));
    ptip->addChild(plight);
    
    auto pbox = Sprite::create("game_res/im_award_2.png");
    pbox->setPosition(Vec2(ptip->getContentSize().width/2, ptip->getContentSize().height/2-40));
    ptip->addChild(pbox);
    
    auto pscore = Label::createWithCharMap("game_res/num_award.png", 21, 21, '0');
    pscore->setString(__String::createWithFormat("%lld", paward->lFishScore)->getCString());
    pscore->setPosition(Vec2(pbox->getContentSize().width/2, 40));
    pbox->addChild(pscore);
    
    auto pget = Sprite::create("game_res/im_award_3.png");
    pget->setPosition(Vec2(pscore->getPositionX()-pscore->getContentSize().width/2-pget->getContentSize().width/2, 40));
    pbox->addChild(pget);
    
    auto pmei = Sprite::create("game_res/im_award_4.png");
    pmei->setPosition(Vec2(pscore->getPositionX()+pscore->getContentSize().width/2+pmei->getContentSize().width/2, 40));
    pbox->addChild(pmei);
}
