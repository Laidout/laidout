#ifndef PSFILTERS_H
#define PSFILTERS_H

#include <cstdio>

int Ascii85_out(std::FILE *f,unsigned char *in,int len,int puteod,int linewidth);

#endif

