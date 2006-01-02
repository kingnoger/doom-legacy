// Emacs style mode select   -*- C++ -*-
// Parsing driver class for the JDS NTEXTURE lump
// Ville Bergholm 2005

#ifndef ntexture_h
#define ntexture_h

#include <string>
#include "r_data.h"

// Forward declarations
// All Bison parser stuff is inside this namespace
namespace yy
{
  class location;
  class ntexture_parser;
}

union YYSTYPE;


// Conducting the whole scanning and parsing
class ntexture_driver
{
protected:
  char *buffer;
  int   length;

  bool trace_scanning;
  bool trace_parsing;

public:
  // temp variables for building textures
  std::string texname;
  Texture *t;
  LumpTexture dummy;
  bool texeloffsets;
  bool is_sprite;

  ntexture_driver(int lump);
  virtual ~ntexture_driver();
  
  // Handling the scanner.
  void scan_begin();
  void scan_end();

  // Handling the parser.
  int  parse();

  // Error handling.
  void error(const yy::location& l, const std::string& m);
  void error(const std::string& m);
};


// Announce to Flex the prototype we want for lexing function,
//#define YY_DECL  int yylex(YYSTYPE *yylval, yy::location *yylloc, ntexture_driver& driver)
#define YY_DECL  int yylex(YYSTYPE *yylval, yy::location *yylloc)
// and declare it for the parser's sake.
YY_DECL;

#endif
