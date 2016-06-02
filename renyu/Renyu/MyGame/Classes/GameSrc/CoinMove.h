//
//  CoinMove.h
//  MyGame
//
//  Created by wh on 15/6/17.
//
//

#ifndef __MyGame__CoinMove__
#define __MyGame__CoinMove__

#include <stdio.h>
#include "cocos2d.h"
#include "cmd_game.h"
class CoinMove : public cocos2d::Sprite
{
public:
    static CoinMove *create(cocos2d::Vec2 beginpos, cocos2d::Vec2 endpos, float dspan, float dheight, bool bdirection);
    
    void initwith(cocos2d::Vec2 beginpos, cocos2d::Vec2 endpos, float dspan, float dheight, bool bdirection);
    
    void update(float t);
    
private:
    int m_offset;
    float m_span;
    float m_height;
    bool m_direction;
    
    bool m_ismove;
    cocos2d::Vec2 m_endpos;
    cocos2d::Vec2 m_startpos;
};

#endif /* defined(__MyGame__CoinMove__) */
