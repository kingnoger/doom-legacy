// $Id$
// Thinker class implementation

#include "g_think.h"
#include "z_zone.h"

Thinker::Thinker()
{
  mp = NULL;
}

Thinker::~Thinker()
{}


int Thinker::Serialize(LArchive & a)
{
  return 0;
}

void *Thinker::operator new (size_t size)
{
  return Z_Malloc(size, PU_LEVSPEC, NULL);    
}

void Thinker::operator delete (void *mem)
{
  Z_Free(mem);
}
