// Emacs style mode select   -*- C++ -*-
//---------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2005-2007 by DooM Legacy Team.
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
/// \brief Lemon parser for JDS NTEXTURE lumps.


%name NTEXTURE_Parse
%extra_argument {ntexture_driver& d}

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
%token_destructor { if (yymajor == STR) Z_Free(const_cast<char*>($$.stype)); }

%type str {const char *}
%type int {int}
%type num {float}

%include {
#include "parser_driver.h"
#include "decorate.parser.h"
}

//============================================================================

start ::= definitions.

// sequence of 0-N texture definitions
definitions ::= . // empty
definitions ::= definitions texture. // left recursion


// single texture declaration
texture ::= tex_type tex_name tex_construction tex_properties R_BRACE.
  {
    //-------------
    // texture definition finished, insert it into cache
    if (d.t != &d.dummy)
    {
      CONS_Printf(" Built texture '%s'.\n", d.t->GetName());
      d.t->Initialize();
      if (d.is_sprite)
        tc.InsertSprite(d.t);
      else
        tc.InsertTexture(d.t);
    }
  }
texture ::= COLON. // temp HACK


tex_type ::= TEXTURE. { d.is_sprite = false; }
tex_type ::= SPRITE.  { d.is_sprite = true;  }


tex_name ::= str(A).
  {
    d.texname = A;
    d.t = NULL;
    d.texeloffsets = false;
  }


tex_construction ::= L_BRACE tex_initialization.
/*
tex_construction ::= COLON str(P) L_BRACE.  // inheritance, no initialization part allowed
  {
    Texture *parent = tc.GetPtr(P, TEX_wall);
    if (parent == tc.default_item)
    {
      CONS_Printf(" Parent Texture '%s' not found.\n", P);
    }
    d.t = new XXX_Texture(*parent); // copy construction
    d.t->SetName(d.texname.c_str());
  }
*/

// one or the other, not both
tex_initialization ::= DATA str(A) SEMICOLON.
  {
    d.t = tc.Load(A);
    if (!d.t)
    {
      CONS_Printf(" Texture lump '%s' not found.\n", A);
      d.t = &d.dummy;
    }
    d.t->SetName(d.texname.c_str());
  }
// | COMPOSE INT INT SEMICOLON { d.t = new ComposedTexture(d.texname, $2, $3); } 


// list of 0-N properties
tex_properties ::= . // empty
tex_properties ::= tex_properties tex_property. // left recursion


// now we know that t points to a valid texture object, let's just fill in the properties
tex_property ::= WORLDSIZE num(A) num(B) SEMICOLON.
  {
    d.t->xscale = d.t->width/A;
    d.t->yscale = d.t->height/B;
  }
tex_property ::= SCALE num(A) num(B) SEMICOLON.  { d.t->yscale = 1.0/A; d.t->xscale = d.t->yscale/B; } 
tex_property ::= SCALE num(A) SEMICOLON.         { d.t->xscale = d.t->yscale = 1.0/A; }
tex_property ::= TEXELOFFSETS int(A) SEMICOLON.  { d.texeloffsets = A; }
tex_property ::= OFFSET num(A) num(B) SEMICOLON.
    {
      if (d.texeloffsets)
	{
	  d.t->leftoffs = (A / d.t->xscale).floor();
	  d.t->topoffs = (B / d.t->yscale).floor();
	}
      else
	{
	  d.t->leftoffs = A;
	  d.t->topoffs = B;
	}
    }


// string
str(A) ::= STR(B). { A = B.stype; }

// int
int(A) ::= INT(B). { A = B.itype; }

// any real number
num(A) ::= FLOAT(B). { A = B.ftype; }
num(A) ::= INT(B).   { A = B.itype; } // make it a float
