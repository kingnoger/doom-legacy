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
class DActor;
class PlayerPawn;

void   P_NoiseAlert(Actor *target, Actor *emitter);
void   P_BulletSlope(PlayerPawn *p);

void   A_FaceTarget(DActor *actor);
void   A_Chase(DActor *actor);
void   A_Fall(DActor *actor);
void   A_Look(DActor *actor);
void   A_Pain(DActor *actor);

#endif
