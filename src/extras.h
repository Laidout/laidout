#ifndef EXTRAS_H
#define EXTRAS_H

#include "laidout.h"
#include <lax/interfaces/imageinterface.h>

int dumpImages(Document *doc, int startpage, const char *pathtoimagedir, int imagesperpage=1, int ddpi=150);
int dumpImages(Document *doc, int startpage, const char **imagefiles, int nimages, int imagesperpage=1, int ddpi=150);
int dumpImages(Document *doc, int startpage, Laxkit::ImageData **images, int nimages, int imagesperpage=1, int ddpi=150);

#endif

