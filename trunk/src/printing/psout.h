#ifndef PSOUT_H
#define PSOUT_H

#include "../document.h"
#include <cstdio>

void psdumpobj(FILE *f,Laxkit::SomeData *obj);
int psout(FILE *f,Document *doc);
int psout(Document *doc,const char *file=NULL);

#endif


