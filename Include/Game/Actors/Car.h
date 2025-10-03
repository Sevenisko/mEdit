#pragma once

#include <Game/Actor.h>

class Car : public Actor {
  public:
    Car() { m_Type = ACTOR_CAR; }
};