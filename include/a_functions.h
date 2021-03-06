// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004-2007 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Action function prototypes.
///
/// This is a special header file that is meant to be included several times,
/// with different definitions for the macros WEAPON and DACTOR.

// Basic use: prototyping
#ifndef WEAPON
# define WEAPON(x) void A_ ## x (PlayerPawn *, pspdef_t *);
#endif

#ifndef DACTOR
# define DACTOR(x) void A_ ## x (DActor *);
#endif

//=============================================
//  Legacy actions
//=============================================

WEAPON(StartWeaponFS)
WEAPON(StartWeaponACS)
DACTOR(StartFS)
DACTOR(StartACS)
DACTOR(ActiveSound)

//=============================================
//  Doom actions
//=============================================

WEAPON(Light0)
WEAPON(Light1)
WEAPON(Light2)
WEAPON(WeaponReady)
WEAPON(CheckReload)
WEAPON(ReFire)
WEAPON(Lower)
WEAPON(Raise)
WEAPON(Punch)
WEAPON(Saw)
WEAPON(FireMissile)
WEAPON(FireBFG)
WEAPON(FirePlasma)
WEAPON(FirePistol)
WEAPON(FireShotgun)
WEAPON(FireShotgun2)
WEAPON(FireCGun)
WEAPON(GunFlash)
WEAPON(OpenShotgun2)
WEAPON(LoadShotgun2)
WEAPON(CloseShotgun2)
WEAPON(BFGsound)

DACTOR(Chase)
DACTOR(FaceTarget)
DACTOR(Look)
DACTOR(Pain)
DACTOR(Scream)
DACTOR(XScream)
DACTOR(Fall)
DACTOR(Fire)
DACTOR(FireCrackle)
DACTOR(Explode)
DACTOR(VileChase)
DACTOR(VileAttack)
DACTOR(SargAttack)
DACTOR(HeadAttack)
DACTOR(TroopAttack)
DACTOR(SPosAttack)
DACTOR(PosAttack)
DACTOR(BspiAttack)
DACTOR(PainAttack)
DACTOR(BruisAttack)
DACTOR(CPosAttack)
DACTOR(CPosRefire)
DACTOR(CyberAttack)
DACTOR(SkullAttack)
DACTOR(FatAttack1)
DACTOR(FatAttack2)
DACTOR(FatAttack3)
DACTOR(SpidRefire)
DACTOR(Tracer)
DACTOR(SkelMissile)
DACTOR(SkelWhoosh)
DACTOR(SkelFist)
DACTOR(VileStart)
DACTOR(VileTarget)
DACTOR(StartFire)
DACTOR(FatRaise)
DACTOR(PainDie)
DACTOR(PlayerScream)
DACTOR(Hoof)
DACTOR(BossDeath)
DACTOR(BabyMetal)
DACTOR(Metal)
DACTOR(KeenDie)
DACTOR(BrainAwake)
DACTOR(BrainPain)
DACTOR(BrainScream)
DACTOR(BrainExplode)
DACTOR(BrainDie)
DACTOR(BrainSpit)
DACTOR(SpawnFly)
DACTOR(SpawnSound)
DACTOR(BFGSpray)


//=============================================
//  Heretic actions
//=============================================

WEAPON(StaffAttackPL1)
WEAPON(StaffAttackPL2)
WEAPON(BeakRaise)
WEAPON(InitPhoenixPL2)
WEAPON(ShutdownPhoenixPL2)
WEAPON(BeakAttackPL1)
WEAPON(BeakAttackPL2)
WEAPON(GauntletAttack)
WEAPON(FireMacePL1)
WEAPON(FireMacePL2)
WEAPON(FireSkullRodPL1)
WEAPON(FireSkullRodPL2)
WEAPON(FireGoldWandPL1)
WEAPON(FireGoldWandPL2)
WEAPON(FireCrossbowPL1)
WEAPON(FireCrossbowPL2)
WEAPON(FirePhoenixPL1)
WEAPON(FirePhoenixPL2)
WEAPON(FireBlasterPL1)
WEAPON(FireBlasterPL2)
WEAPON(BeakReady)

DACTOR(WizAtk1)
DACTOR(WizAtk2)
DACTOR(WizAtk3)
DACTOR(KnightAttack)
DACTOR(ChicChase)
DACTOR(Sor1Chase)
DACTOR(WhirlwindSeek)
DACTOR(BlueSpark)
DACTOR(DripBlood)
DACTOR(BeastPuff)
DACTOR(SnakeAttack)
DACTOR(SnakeAttack2)
DACTOR(CheckSkullFloor)
DACTOR(ImpMeAttack)
DACTOR(ImpMsAttack)
DACTOR(ImpMsAttack2)
DACTOR(MinotaurAtk1)
DACTOR(MinotaurAtk2)
DACTOR(MinotaurAtk3)
DACTOR(MinotaurDecide)
DACTOR(Sor1Pain)
DACTOR(SorZap)
DACTOR(SorRise)
DACTOR(SorDSph)
DACTOR(SorDExp)
DACTOR(SorDBon)
DACTOR(SorSightSnd)
DACTOR(ImpDeath)
DACTOR(ImpXDeath1)
DACTOR(ImpXDeath2)
DACTOR(ImpExplode)
DACTOR(HScream)
DACTOR(HHeadAttack)
DACTOR(AccTeleGlitter)
DACTOR(RainImpact)
DACTOR(NoBlocking)
DACTOR(ChicLook)
DACTOR(GhostOff)
DACTOR(Srcr1Attack)
DACTOR(Srcr2Attack)
DACTOR(MntrFloorFire)
DACTOR(HBossDeath)
DACTOR(MinotaurCharge)
DACTOR(GenWizard)
DACTOR(Sor2DthInit)
DACTOR(Sor2DthLoop)
DACTOR(Srcr2Decide)
DACTOR(SorcererRise)
DACTOR(MummyAttack)
DACTOR(MummyAttack2)
DACTOR(MummySoul)
DACTOR(MummyFX1Seek)
DACTOR(ClinkAttack)
DACTOR(HeadIceImpact)
DACTOR(BeastAttack)
DACTOR(SpawnTeleGlitter)
DACTOR(SpawnTeleGlitter2)
DACTOR(Feathers)
  //DACTOR(ContMobjSound)
DACTOR(ChicAttack)
DACTOR(ChicPain)
DACTOR(CheckSkullDone)
DACTOR(MaceBallImpact)
DACTOR(MaceBallImpact2)
DACTOR(HeadFireGrow)
DACTOR(SkullRodPL2Seek)
DACTOR(BoltSpark)
DACTOR(FloatPuff)
  //DACTOR(ESound)
DACTOR(FlameSnd)
DACTOR(FlameEnd)
DACTOR(PhoenixPuff)
DACTOR(SkullRodStorm)
DACTOR(HideInCeiling)
DACTOR(AddPlayerRain)
DACTOR(DeathBallImpact)
DACTOR(MacePL1Check)
DACTOR(SpawnRippers)
DACTOR(RestoreSpecialThing1)
DACTOR(RestoreSpecialThing2)
DACTOR(FreeTargMobj)
DACTOR(VolcanoSet)
DACTOR(VolcanoBlast)
DACTOR(VolcBallImpact)
DACTOR(PodPain)
DACTOR(RemovePod)
DACTOR(MakePod)
DACTOR(HideThing)
DACTOR(UnHideThing)
DACTOR(RestoreArtifact)
DACTOR(InitKeyGizmo)


//=============================================
//  Hexen actions
//=============================================

WEAPON(FPunchAttack)
WEAPON(FAxeAttack)
WEAPON(FHammerAttack)
WEAPON(FHammerThrow)
WEAPON(FSwordAttack)
WEAPON(CMaceAttack)
WEAPON(CStaffInitBlink)
WEAPON(CStaffCheckBlink)
WEAPON(CStaffCheck)
WEAPON(CStaffAttack)
WEAPON(CFlameAttack)
WEAPON(CHolyAttack)
WEAPON(CHolyPalette)
WEAPON(MWandAttack)
WEAPON(LightningReady)
WEAPON(MLightningAttack)
WEAPON(MStaffAttack)
WEAPON(MStaffPalette)
WEAPON(SnoutAttack)
WEAPON(FireConePL1)

DACTOR(FreeTargMobj)
DACTOR(FlameCheck)
DACTOR(HideThing)
DACTOR(UnHideThing)
DACTOR(RestoreSpecialThing1)
DACTOR(RestoreSpecialThing2)
DACTOR(RestoreArtifact)
DACTOR(Summon)
DACTOR(ThrustInitUp)
DACTOR(ThrustInitDn)
DACTOR(ThrustRaise)
DACTOR(ThrustBlock)
DACTOR(ThrustImpale)
DACTOR(ThrustLower)
DACTOR(TeloSpawnC)
DACTOR(TeloSpawnB)
DACTOR(TeloSpawnA)
DACTOR(TeloSpawnD)
DACTOR(CheckTeleRing)
DACTOR(FogSpawn)
DACTOR(FogMove)
DACTOR(Quake)
DACTOR(Scream)
DACTOR(Explode)
DACTOR(PoisonBagInit)
DACTOR(PoisonBagDamage)
DACTOR(PoisonBagCheck)
DACTOR(CheckThrowBomb)
DACTOR(NoGravity)
DACTOR(PotteryExplode)
DACTOR(PotteryChooseBit)
DACTOR(PotteryCheck)
DACTOR(CorpseBloodDrip)
DACTOR(CorpseExplode)
DACTOR(LeafSpawn)
DACTOR(LeafThrust)
DACTOR(LeafCheck)
DACTOR(BridgeInit)
DACTOR(BridgeOrbit)
DACTOR(TreeDeath)
DACTOR(PoisonShroom)
DACTOR(Pain)
DACTOR(SoAExplode)
DACTOR(BellReset1)
DACTOR(BellReset2)
DACTOR(NoBlocking)
DACTOR(FSwordFlames)
DACTOR(CStaffMissileSlither)
DACTOR(CFlameRotate)
DACTOR(CFlamePuff)
DACTOR(CFlameMissile)
DACTOR(CHolySeek)
DACTOR(CHolyCheckScream)
DACTOR(CHolyTail)
DACTOR(CHolySpawnPuff)
DACTOR(CHolyAttack2)
DACTOR(LightningZap)
DACTOR(LightningClip)
DACTOR(LightningRemove)
DACTOR(LastZap)
DACTOR(ZapMimic)
DACTOR(MStaffWeave)
DACTOR(MStaffTrack)
DACTOR(ShedShard)
DACTOR(AddPlayerCorpse)
DACTOR(SkullPop)
DACTOR(FreezeDeath)
DACTOR(FreezeDeathChunks)
DACTOR(CheckBurnGone)
DACTOR(CheckSkullFloor)
DACTOR(CheckSkullDone)
DACTOR(SpeedFade)
DACTOR(IceSetTics)
DACTOR(IceCheckHeadDone)
DACTOR(PigPain)
DACTOR(PigLook)
DACTOR(PigChase)
DACTOR(FaceTarget)
DACTOR(PigAttack)
DACTOR(QueueCorpse)
DACTOR(CentaurAttack)
DACTOR(CentaurAttack2)
DACTOR(SetReflective)
DACTOR(CentaurDefend)
DACTOR(UnSetReflective)
DACTOR(CentaurDropStuff)
DACTOR(CheckFloor)
DACTOR(DemonAttack1)
DACTOR(DemonAttack2)
DACTOR(DemonDeath)
DACTOR(Demon2Death)
DACTOR(WraithRaiseInit)
DACTOR(WraithRaise)
DACTOR(WraithInit)
DACTOR(WraithLook)
DACTOR(WraithChase)
DACTOR(WraithFX3)
DACTOR(WraithMelee)
DACTOR(WraithMissile)
DACTOR(WraithFX2)
DACTOR(MinotaurFade1)
DACTOR(MinotaurFade2)
DACTOR(MinotaurLook)
DACTOR(MinotaurChase)
DACTOR(MinotaurRoam)
DACTOR(XMinotaurAtk1)
DACTOR(XMinotaurDecide)
DACTOR(XMinotaurAtk2)
DACTOR(XMinotaurAtk3)
DACTOR(XMinotaurCharge)
DACTOR(SmokePuffExit)
DACTOR(MinotaurFade0)
//DACTOR(MntrFloorFire)
DACTOR(SerpentChase)
DACTOR(SerpentHumpDecide)
DACTOR(SerpentUnHide)
DACTOR(SerpentRaiseHump)
DACTOR(SerpentLowerHump)
DACTOR(SerpentHide)
DACTOR(SerpentBirthScream)
DACTOR(SetShootable)
DACTOR(SerpentCheckForAttack)
DACTOR(UnSetShootable)
DACTOR(SerpentDiveSound)
DACTOR(SerpentWalk)
DACTOR(SerpentChooseAttack)
DACTOR(SerpentMeleeAttack)
DACTOR(SerpentMissileAttack)
DACTOR(SerpentHeadPop)
DACTOR(SerpentSpawnGibs)
DACTOR(SerpentHeadCheck)
DACTOR(FloatGib)
DACTOR(DelayGib)
DACTOR(SinkGib)
DACTOR(BishopDecide)
DACTOR(BishopDoBlur)
DACTOR(BishopSpawnBlur)
DACTOR(BishopChase)
DACTOR(BishopAttack)
DACTOR(BishopAttack2)
DACTOR(BishopPainBlur)
DACTOR(BishopPuff)
DACTOR(SetAltShadow)
DACTOR(BishopMissileWeave)
DACTOR(BishopMissileSeek)
DACTOR(DragonInitFlight)
DACTOR(DragonFlap)
DACTOR(DragonFlight)
DACTOR(DragonAttack)
DACTOR(DragonPain)
DACTOR(DragonCheckCrash)
DACTOR(DragonFX2)
DACTOR(EttinAttack)
DACTOR(DropMace)
DACTOR(FiredRocks)
DACTOR(UnSetInvulnerable)
DACTOR(FiredChase)
DACTOR(FiredAttack)
DACTOR(FiredSplotch)
DACTOR(SmBounce)
DACTOR(IceGuyLook)
DACTOR(IceGuyChase)
DACTOR(IceGuyAttack)
DACTOR(IceGuyDie)
DACTOR(IceGuyMissilePuff)
DACTOR(IceGuyMissileExplode)
DACTOR(ClassBossHealth)
DACTOR(FastChase)
DACTOR(FighterAttack)
DACTOR(ClericAttack)
DACTOR(MageAttack)
DACTOR(SorcSpinBalls)
DACTOR(SpeedBalls)
DACTOR(SpawnFizzle)
DACTOR(SorcBossAttack)
DACTOR(SorcBallOrbit)
DACTOR(SorcBallPop)
DACTOR(BounceCheck)
DACTOR(SorcFX1Seek)
DACTOR(SorcFX2Split)
DACTOR(SorcFX2Orbit)
DACTOR(SorcererBishopEntry)
DACTOR(SorcDeath)
DACTOR(SpawnBishop)
DACTOR(SorcFX4Check)
DACTOR(KoraxStep2)
DACTOR(KoraxChase)
DACTOR(KoraxStep)
DACTOR(KoraxDecide)
DACTOR(KoraxMissile)
DACTOR(KoraxCommand)
DACTOR(KoraxBonePop)
DACTOR(KSpiritRoam)
DACTOR(KBoltRaise)
DACTOR(KBolt)
DACTOR(BatSpawnInit)
DACTOR(BatSpawn)
DACTOR(BatMove)



#undef WEAPON
#undef DACTOR
