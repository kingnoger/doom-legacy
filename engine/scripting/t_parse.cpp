// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id$
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2003 Doom Legacy Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Log$
// Revision 1.4  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.3  2004/01/02 14:25:02  smite-meister
// cleanup
//
// Revision 1.2  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.1.1.1  2002/11/16 14:18:19  hurdler
// Initial C++ version of Doom Legacy
//
//
//--------------------------------------------------------------------------
//
// Parsing.
//
// Takes lines of code, or groups of lines and runs them.
// The main core of FraggleScript
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include <stdarg.h>
#include <string.h>

#include "doomdef.h"
#include "doomtype.h"
#include "command.h"
#include "sounds.h"
#include "z_zone.h"

#include "t_parse.h"
#include "t_prepro.h"
#include "t_spec.h"
#include "t_oper.h"
#include "t_vari.h"
#include "t_func.h"

void parse_script();
void parse_data(char *data, char *end);
svalue_t evaluate_expression(int start, int stop);

Actor *trigger_obj;            // object which triggered script

char *tokens[T_MAXTOKENS];
tokentype_t tokentype[T_MAXTOKENS];
int num_tokens = 0;
int script_debug = false;

script_t *current_script;       // the current script
Map *current_map; // current_script->mp, just for convenience

svalue_t nullvar = { svt_int,  {0} };      // null var for empty return
int killscript;         // when set to true, stop the script quickly
section_t *prev_section;       // the section from the previous statement

/************ Divide into tokens **************/

char *linestart;        // start of line
char *rover;            // current point reached in script

        // inline for speed
#define isnum(c) ( ((c)>='0' && (c)<='9') || (c)=='.' )
        // isop: is an 'operator' character, eg '=', '%'
#define isop(c)   !( ( (c)<='Z' && (c)>='A') || ( (c)<='z' && (c)>='a') || \
                     ( (c)<='9' && (c)>='0') || ( (c)=='_') )

        // for simplicity:
#define tt (tokentype[num_tokens-1])
#define tok (tokens[num_tokens-1])

section_t *current_section; // the section (if any) found in parsing the line
int bracetype;              // bracket_open or bracket_close
static void add_char(char c);

// next_token: end this token, go onto the next

static void next_token()
{
  if(tok[0] || tt == to_string)
    {
      num_tokens++;
      tokens[num_tokens-1] = tokens[num_tokens-2]
        + strlen(tokens[num_tokens-2]) + 1;
      tok[0] = 0;
    }
  
  // get to the next token, ignoring spaces, newlines,
  // useless chars, comments etc
  
  while(1)
    {
      // empty whitespace
      if(*rover && (*rover==' ' || *rover<32))
        {
          while((*rover==' ' || *rover<32) && *rover) rover++;
        }
      // end-of-script?
      if(!*rover)
        {
          if(tokens[0][0])
            {
              CONS_Printf("%s %i %i\n", tokens[0],
                       rover-current_script->data, current_script->len);
              // line contains text, but no semicolon: an error
              script_error("missing ';'\n");
            }
          // empty line, end of command-list
          return;
        }
      // 11/8 comments moved to new preprocessor
      
      break;  // otherwise
    }

  if(num_tokens>1 && *rover == '(' && tokentype[num_tokens-2] == to_name)
    tokentype[num_tokens-2] = to_function;
  
  if(*rover == '{' || *rover == '}')
    {
      if(*rover == '{')
        {
          bracetype = bracket_open;
          current_section = find_section_start(rover);
        }
      else            // closing brace
        {
          bracetype = bracket_close;
          current_section = find_section_end(rover);
        }
      if(!current_section)
        {
          script_error("section not found!\n");
          return;
        }
    }
  else if(*rover == ':')  // label
    {
      // ignore the label : reset
      num_tokens = 1;
      tokens[0][0] = 0; tt = to_name;
      rover++;        // ignore
    }
  else if(*rover == '\"')
    {
      tt = to_string;
      if(tokentype[num_tokens-2] == to_string)
        num_tokens--;   // join strings
      rover++;
    }
  else
    {
      tt = isop(*rover) ? to_oper :
        isnum(*rover) ? to_number : to_name;
    }
}

// return an escape sequence (prefixed by a '\')
// do not use all C escape sequences

static char escape_sequence(char c)
{
  if(c == 'n') return '\n';
  if(c == '\\') return '\\';
  if(c == '"') return '"';
  if(c == '?') return '?';
  if(c == 'a') return '\a';         // alert beep
  if(c == 't') return '\t';         //tab
//  if(c == 'z') return *FC_TRANS;    // translucent toggle
  
  // font colours
  //Hurdler: fix for Legacy text color
  if(c >= '0' && c <= '9') return /*128 +*/ (c-'0');
  
  return c;
}

// add_char: add one character to the current token

static void add_char(char c)
{
  char *out = tok + strlen(tok);
  
  *out++ = c;
  *out = 0;
}

// get_tokens.
// Take a string, break it into tokens.

// individual tokens are stored inside the tokens[] array
// tokentype is also used to hold the type for each token:

//   to_name: a piece of text which starts with an alphabet letter.
//         probably a variable name. Some are converted into
//         function types later on in find_brackets
//   to_number: a number. like '12' or '1337'
//   to_oper: an operator such as '&&' or '+'. All FraggleScript
//             operators are either one character, or two character
//             (if 2 character, 2 of the same char or ending in '=')
//   to_string: a text string that was enclosed in quote "" marks in
//           the original text
//   to_unset: shouldn't ever end up being set really.
//   to_function: a function name (found in second stage parsing)

void get_tokens(char *s)
{
  rover = s;
  num_tokens = 1;
  tokens[0][0] = 0; tt = to_name;
  
  current_section = NULL;   // default to no section found
  
  next_token();
  linestart = rover;      // save the start
  
  if(*rover)
    while(1)
      {
        if(killscript) return;
        if(current_section)
          {
            // a { or } section brace has been found
            break;        // stop parsing now
          }
        else if(tt != to_string)
          {
            if(*rover == ';') break;     // check for end of command ';'
          }
        
        switch(tt)
          {
          case to_unset:
          case to_string:
            while(*rover != '\"')     // dedicated loop for speed
              {
                if(*rover == '\\')       // escape sequences
                  {
                    rover++;
                    add_char(escape_sequence(*rover));
                  }
                else
                  add_char(*rover);
                rover++;
              }
            rover++;
            next_token();       // end of this token
            continue;
            
          case to_oper:
            // all 2-character operators either end in '=' or
            // are 2 of the same character
            // do not allow 2-characters for brackets '(' ')'
            // which are still being considered as operators
            
            // operators are only 2-char max, do not need
            // a seperate loop
            
            if((*tok && *rover != '=' && *rover!=*tok) ||
               *tok == '(' || *tok == ')')
              {
                // end of operator
                next_token();
                continue;
              }
            add_char(*rover);
            break;
            
          case to_number:

            // add while number chars are read

            while(isnum(*rover))       // dedicated loop
              add_char(*rover++);
            next_token();
            continue;

          case to_name:

            // add the chars

            while(!isop(*rover))        // dedicated loop
              add_char(*rover++);
            next_token();
            continue;
            
          default: break; // shut up compiler

          }
        rover++;
      }
  
  // check for empty last token

  if(!tok[0])
    {
      num_tokens = num_tokens - 1;
    }
  
  rover++;
}


void print_tokens()     // DEBUG
{
  int i;
  for(i=0; i<num_tokens; i++)
    {
      CONS_Printf("\n'%s' \t\t --", tokens[i]);
      switch(tokentype[i])
        {
        case to_string: CONS_Printf("string");        break;
        case to_oper: CONS_Printf("operator");    break;
        case to_name: CONS_Printf("name");            break;
        case to_number: CONS_Printf("number");        break;
        case to_unset : CONS_Printf("duh");           break;
        case to_function: CONS_Printf("function name"); break;
        }
    }
  CONS_Printf("\n");
  if(current_section)
    CONS_Printf("current section: offset %i\n",
             (int)(current_section->start-current_script->data) );
}


// run_script
//
// the function called by t_script.c

void run_script(script_t *script)
{
  // set current script
  current_script = script;
  
  // start at the beginning of the script
  rover = current_script->data;

  current_script->lastiftrue = false;
  
  parse_script(); // run it
}

void continue_script(script_t *script, char *continue_point)
{
  current_script = script;
  
  // continue from place specified
  rover = continue_point;
  
  parse_script(); // run 
}

void parse_script()
{
  // check for valid rover
  if(rover < current_script->data || 
     rover > current_script->data+current_script->len)
    {
      script_error("parse_script: trying to continue from point"
                   "outside script!\n");
      return;
    }
  
  trigger_obj = current_script->trigger;  // set trigger
  
  parse_data(current_script->data,
             current_script->data+current_script->len);
  
  // dont clear global vars!
  if(current_script->scriptnum != -1)
    clear_variables(current_script);        // free variables

  current_script->lastiftrue = false;
}

void parse_data(char *data, char *end)
{
  char *token_alloc;      // allocated memory for tokens
  
  killscript = false;     // dont kill the script straight away
  
  // allocate space for the tokens
  token_alloc = (char *)Z_Malloc(current_script->len + T_MAXTOKENS, PU_STATIC, 0);
  
  prev_section = NULL;  // clear it
  
  while(*rover)   // go through the script executing each statement
    {
      // past end of script?
      if(rover > end)
        break;
      
      // reset the tokens before getting the next line
      tokens[0] = token_alloc;
      
      prev_section = current_section; // store from prev. statement
      
      // get the line and tokens
      get_tokens(rover);
      
      if(killscript) break;
      
      if(!num_tokens)
        {
          if(current_section)       // no tokens but a brace
            {
              // possible } at end of loop:
              // refer to spec.c
              spec_brace();
            }
          
          continue;  // continue to next statement
        }
      
      if(script_debug) print_tokens();   // debug
      run_statement();         // run the statement
    }
  Z_Free(token_alloc);
}

void run_statement()
{
  // decide what to do with it
  
  // NB this stuff is a bit hardcoded:
  //    it could be nicer really but i'm
  //    aiming for speed
  
  // if() and while() will be mistaken for functions
  // during token processing
  if(tokentype[0] == to_function)
    {
      if(!strcmp(tokens[0], "if"))
        {
          current_script->lastiftrue = spec_if() ? true: false;
          return;
        }
      else if(!strcmp(tokens[0], "elseif"))
        {
          if(!prev_section || (prev_section->type != st_if && prev_section->type != st_elseif))
          {
            script_error("elseif without if!\n");
            return;
          }
          current_script->lastiftrue = spec_elseif(current_script->lastiftrue) ? true : false;
          return;
        }
      else if(!strcmp(tokens[0], "else"))
        {
          if(!prev_section || (prev_section->type != st_if && prev_section->type != st_elseif))
          {
            script_error("else without if!\n");
            return;
          }
          spec_else(current_script->lastiftrue);
          current_script->lastiftrue = true;
          return;
        }
      else if(!strcmp(tokens[0], "while"))
        {
          spec_while();
          return;
        }
      else if(!strcmp(tokens[0], "for"))
        {
          spec_for();
          return;
        }
    }
  else if(tokentype[0] == to_name)
    {
      // NB: goto is a function so is not here

      // if a variable declaration, return now
      if(spec_variable()) return;
    }

  // just a plain expression
  evaluate_expression(0, num_tokens-1);
}

/***************** Evaluating Expressions ************************/

        // find a token, ignoring things in brackets        
int find_operator(int start, int stop, char *value)
{
  int i;
  int bracketlevel = 0;
  
  for(i=start; i<=stop; i++)
    {
      // only interested in operators
      if(tokentype[i] != to_oper) continue;
      
      // use bracketlevel to check the number of brackets
      // which we are inside
      bracketlevel += tokens[i][0]=='(' ? 1 :
        tokens[i][0]==')' ? -1 : 0;
      
      // only check when we are not in brackets
      if(!bracketlevel && !strcmp(value, tokens[i]))
        return i;
    }
  
  return -1;
}

        // go through tokens the same as find_operator, but backwards
int find_operator_backwards(int start, int stop, char *value)
{
  int i;
  int bracketlevel = 0;
  
  for(i=stop; i>=start; i--)      // check backwards
    {
      // operators only

      if(tokentype[i] != to_oper) continue;
      
      // use bracketlevel to check the number of brackets
      // which we are inside
      
      bracketlevel += tokens[i][0]=='(' ? -1 :
        tokens[i][0]==')' ? 1 : 0;
      
      // only check when we are not in brackets
      // if we find what we want, return it

      if(!bracketlevel && !strcmp(value, tokens[i]))
        return i;
    }
  
  return -1;
}

// simple_evaluate is used once evalute_expression gets to the level
// where it is evaluating just one token

// converts number tokens into svalue_ts and returns
// the same with string tokens
// name tokens are considered to be variables and
// attempts are made to find the value of that variable
// command tokens are executed (does not return a svalue_t)

extern svalue_t nullvar;

static svalue_t simple_evaluate(int n)
{
  svalue_t returnvar;
  svariable_t *var;
  
  switch(tokentype[n])
    {
    case to_string:
      returnvar.type = svt_string;
      returnvar.value.s = tokens[n];
      return returnvar;

    case to_number:
      if(strchr(tokens[n], '.'))
        {
          returnvar.type = svt_fixed;
          returnvar.value.f = fixed_t(atof(tokens[n]) * FRACUNIT);
        }
      else
        {
          returnvar.type = svt_int;
          returnvar.value.i = atoi(tokens[n]);
        }
      return returnvar;

    case to_name:
      var = find_variable(tokens[n]);
      if(!var)
        {
          script_error("unknown variable '%s'\n", tokens[n]);
          return nullvar;
        }
      else
        return getvariablevalue(var);

    default: return nullvar;
    }
}

// pointless_brackets checks to see if there are brackets surrounding
// an expression. eg. "(2+4)" is the same as just "2+4"
//
// because of the recursive nature of evaluate_expression, this function is
// neccesary as evaluating expressions such as "2*(2+4)" will inevitably
// lead to evaluating "(2+4)"

static void pointless_brackets(int *start, int *stop)
{
  int bracket_level, i;
  
  // check that the start and end are brackets
  
  while(tokens[*start][0] == '(' && tokens[*stop][0] == ')')
    {
      
      bracket_level = 0;
      
      // confirm there are pointless brackets..
      // if they are, bracket_level will only get to 0
      // at the last token
      // check up to <*stop rather than <=*stop to ignore
      // the last token
      
      for(i = *start; i<*stop; i++)
        {
          if(tokentype[i] != to_oper) continue; // ops only
          bracket_level += (tokens[i][0] == '(');
          bracket_level -= (tokens[i][0] == ')');
          if(bracket_level == 0) return; // stop if braces stop before end
        }
      
      // move both brackets in
      
      *start = *start + 1;
      *stop = *stop - 1;
    }
}

// evaluate_expresion is the basic function used to evaluate
// a FraggleScript expression.
// start and stop denote the tokens which are to be evaluated.
//
// works by recursion: it finds operators in the expression
// (checking for each in turn), then splits the expression into
// 2 parts, left and right of the operator found.
// The handler function for that particular operator is then
// called, which in turn calls evaluate_expression again to
// evaluate each side. When it reaches the level of being asked
// to evaluate just 1 token, it calls simple_evaluate

svalue_t evaluate_expression(int start, int stop)
{
  int i, n;

  if(killscript) return nullvar;  // killing the script
  
  // possible pointless brackets
  if(tokentype[start] == to_oper && tokentype[stop] == to_oper)
    pointless_brackets(&start, &stop);
  
  if(start == stop)       // only 1 thing to evaluate
    {
      return simple_evaluate(start);
    }
  
  // go through each operator in order of precedence
  
  for(i=0; i<num_operators; i++)
    {
      // check backwards for the token. it has to be
      // done backwards for left-to-right reading: eg so
      // 5-3-2 is (5-3)-2 not 5-(3-2)

      if( -1 != (n = (operators[i].direction==forward ?
                find_operator_backwards : find_operator)
                 (start, stop, operators[i].str)) )
        {
          // CONS_Printf("operator %s, %i-%i-%i\n", operators[count].str, start, n, stop);

          // call the operator function and evaluate this chunk of tokens

          return operators[i].handler(start, n, stop);
        }
    }
  
  if(tokentype[start] == to_function)
    return evaluate_function(start, stop);
  
  // error ?
  {        
    char tempstr[1024]="";
    
    for(i=start; i<=stop; i++)
      sprintf(tempstr,"%s %s", tempstr, tokens[i]);

    script_error("couldnt evaluate expression: %s\n",tempstr);
    return nullvar;
  }
  
}

void script_error(char *s, ...)
{
  va_list args;
  char tempstr[2048];
  
  va_start(args, s);
  
  if(killscript) return;  //already killing script
  
  if(current_script->scriptnum == -1)
    CONS_Printf("global");
  else
    CONS_Printf("%i", current_script->scriptnum);
  
  // find the line number
  
  if(rover >= current_script->data &&
     rover <= current_script->data+current_script->len)
    {
      int linenum = 1;
      char *temp;
      for(temp = current_script->data; temp<linestart; temp++)
        if(*temp == '\n') linenum++;    // count EOLs
      CONS_Printf(", %i", linenum);
    }
  
  // print the error
  vsprintf(tempstr, s, args);
  CONS_Printf(": %s", tempstr);
  
  // make a noise
  S_StartAmbSound(sfx_pldeth);
  
  killscript = true;
}

//
// sf: string value of an svalue_t
//

const char *stringvalue(svalue_t v)
{
  static char buffer[256];

  switch(v.type)
   {
      case svt_string:
        return v.value.s;

      case svt_actor:
        return "map object";

      case svt_fixed:
        {
          double val = ((double)v.value.f / FRACUNIT);
          sprintf(buffer, "%g", val);
          return buffer;
        }

      case svt_int:
      default:
        sprintf(buffer, "%li", v.value.i);
        return buffer;
    }
}
