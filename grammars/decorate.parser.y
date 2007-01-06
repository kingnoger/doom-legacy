// Emacs style mode select   -*- C++ -*-
//---------------------------------------------------------------------
//
// $Id: $
//
// Copyright (C) 2006-2007 by DooM Legacy Team.
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
//---------------------------------------------------------------------

/// \file
/// \brief Lemon parser for DECORATE lumps.


%name DECORATE_Parse
%extra_argument {decorate_driver *d}

%parse_accept {
  fprintf(stderr, "Parsing complete!\n");
}

%parse_failure {
  fprintf(stderr,"Giving up.  Parser is hopelessly lost...\n");
}

%stack_overflow {
  fprintf(stderr,"Giving up.  Parser stack overflow\n");
}

%token_type {yy_t}
%type str {const char *}
%type int {int}
%type num {float}

%include {
#include "g_decorate.h"
#include "parser_driver.h"
#include "sounds.h"
}

//============================================================================

start ::= definitions.

// sequence of 0-N actor definitions
definitions ::= . // empty
definitions ::= definitions actor. // left recursion

actor ::= actor_init construction doomednum L_BRACE actor_properties R_BRACE.
  {
    d->t = NULL; // done updating it
  }
actor ::= SEMICOLON. // HACK


// initialize temp variables
actor_init ::= ACTOR. { d->t = NULL; }


// after this we must have a valid ActorInfo pointer
construction ::= str(A).
  {
    d->t = aid.Find(A); // see if it already exists
    if (!d->t)
    {
      d->t = new ActorInfo(A);
      aid.Insert(d->t);
    }
  }
construction ::= str(A) COLON str(P). // inheritance
  {
    d->t = aid.Find(A); // see if it already exists
    if (!d->t)
    {
      ActorInfo *parent = aid.Find(P);
      if (!parent)
	{
	  CONS_Printf("DECORATE error: parent class '%s' is unknown.\n", P);
	  d->t = new ActorInfo(A);
	}
      else
	{
	  // copy of parent
	  d->t = new ActorInfo(*parent);
	  d->t->SetName(A);
	}

      aid.Insert(d->t);
    }
    else
    {
      CONS_Printf("DECORATE error: class '%s' already exists, and thus cannot inherit.\n", A);
    }
  }


doomednum ::= . // empty
doomednum ::= int(A).
  {
    d->t->doomednum = A;
    aid.InsertDoomEd(d->t, true);
  }


// list of 0-N properties
actor_properties ::= . // empty
actor_properties ::= actor_properties actor_property.


actor_property ::= OBITUARY str(A). { d->t->obituary = A; }
actor_property ::= MODEL str(A).    { d->t->modelname = A; }

actor_property ::= HEALTH int(A).       { d->t->spawnhealth = A; }
actor_property ::= REACTIONTIME int(A). { d->t->reactiontime = A; }
actor_property ::= PAINCHANCE int(A).   { d->t->painchance = A; }
actor_property ::= SPEED num(A).        { d->t->speed = A; }
actor_property ::= DAMAGE int(A).       { d->t->damage = A; }

actor_property ::= RADIUS num(A). { d->t->radius = A; }
actor_property ::= HEIGHT num(A). { d->t->height = A; }
actor_property ::= MASS num(A).   { d->t->mass = A; }

actor_property ::= SEESOUND str(A).    { d->t->seesound = S_GetSoundID(A); }
actor_property ::= ATTACKSOUND str(A). { d->t->attacksound = S_GetSoundID(A); }
actor_property ::= PAINSOUND str(A).   { d->t->painsound = S_GetSoundID(A); }
actor_property ::= DEATHSOUND str(A).  { d->t->deathsound = S_GetSoundID(A); }
actor_property ::= ACTIVESOUND str(A). { d->t->activesound = S_GetSoundID(A); }

// flags
actor_property ::= PLUS  str(A).
actor_property ::= MINUS str(A).

// state definition structure
actor_property ::= STATES L_BRACE state_defs R_BRACE.

state_defs ::= . // empty
state_defs ::= state_defs state_def.

state_def ::= str(L) COLON. // label
state_def ::= str(S) str(F) int(T) str(A). // sequence of states


// string
str(A) ::= STR(B). { A = B.stype; }

// int
int(A) ::= INT(B). { A = B.itype; }

// any real number
num(A) ::= FLOAT(B). { A = B.ftype; }
num(A) ::= INT(B).   { A = B.itype; } // make it a float
