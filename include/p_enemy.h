// $Id$
// Doom and Heretic monsters
//

#ifndef p_enemy_h
#define p_enemy_h 1

#include "info.h" // mobjtype_t

#define HITDICE(a) ((1+(P_Random() & 7))*(a))

#define USERANGE        (64*FRACUNIT)
#define MELEERANGE      (64*FRACUNIT)
#define MISSILERANGE    (32*64*FRACUNIT)

#define FLOATSPEED      (FRACUNIT*4)

class Actor;
class PlayerPawn;

void   P_NoiseAlert(Actor *target, Actor *emitter);
void   A_FaceTarget(Actor *actor);
void   A_Chase(Actor *actor);
void   A_Fall(Actor *actor);
void   A_Look(Actor *actor);
void   A_Pain(Actor *actor);
void   P_BulletSlope(PlayerPawn *p);
//Actor *P_SpawnMissile(Actor *source, Actor *dest, mobjtype_t type);


#endif
