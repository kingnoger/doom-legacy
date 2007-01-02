%{// Emacs style mode select   -*- C++ -*-
// GNU Flex scanner for various plaintext lumps.
// Ville Bergholm 2005-2007

// prologue
#include <string>
#include "parser_driver.h" // token data struct

// include correct token types
#include "decorate.parser.h"

#define YY_DECL  int yylex(yy_t& yylval)

static int line = 1, col = 0;
static std::string temp;
%}


D    [0-9]
L    [a-zA-Z_]
H    [a-fA-F0-9]
E    [Ee][+-]?{D}+
WHITESP [ \t\v\r\f]


%option noyywrap nounput batch outfile="decorate.lexer.c"
%x comment
%x str
%%

%{ // C++ -style comments
%}
"//"     { BEGIN(comment); }
<comment>{
  [^\n]*
  \n     { col = 0; line++;  BEGIN(INITIAL); }
}

%{
  // C string literal parsing
  // no octal escape sequences, no backslash-newline continuation :)
%}
\"      { temp.clear(); BEGIN(str); }
<str>{
  \"      { // closing quote
            BEGIN(INITIAL);
	    // TODO col update
            yylval.stype = temp.c_str(); return STR; }

  \n       { /* error, unterminated string */ }
  \\[0-9]+ { /* error, unknown escape seq */ }

  \\n   { temp += '\n'; }
  \\t   { temp += '\t'; }
  \\r   { temp += '\r'; }
  \\b   { temp += '\b'; }
  \\f   { temp += '\f'; }

  \\.   { temp += yytext[1]; }

  [^\\\n\"]+  { temp += yytext; }
}

%{ // DECORATE keywords
%}
actor        { return ACTOR; }
mass         { return MASS; }
obituary     { return OBITUARY; }
model        { return MODEL; }


%{
// octals? can be confusing...
//0{D}+	     { yylval.itype = strtol(yytext, NULL, 8);  return INT; }
%}

0[xX]{H}+  { col += strlen(yytext);  yylval.itype = strtol(yytext, NULL, 16); return INT; }
{D}+	   { col += strlen(yytext);  yylval.itype = strtol(yytext, NULL, 10); return INT; }

{D}+{E}          |
{D}*"."{D}+{E}?	 |
{D}+"."{D}*{E}?	 { col += strlen(yytext);  yylval.ftype = strtod(yytext, NULL); return FLOAT; }

%{ // unquoted string TODO should accept more chars
%}
{L}({L}|{D})*  { col += strlen(yytext);  temp = yytext; yylval.stype = temp.c_str(); return STR; }

%{ // delimiters
%}
";" { col++; return SEMICOLON; }
":" { col++; return COLON; }
"{" { col++; return L_BRACE; }
"}" { col++; return R_BRACE; }


{WHITESP}  { col += strlen(yytext); /* skip whitespace */ }
\n         { col = 0; line++; }

%%


void DECORATE_lexer_reset()
{
  line = 1;
  col  = 1;
}


void DECORATE_error(char *s)
{
  fprintf(stderr, "Flex error: %s at line: %d col: %d\n", s, line, col);
}
