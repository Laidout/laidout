#ifndef DRAWDATA_H
#define DRAWDATA_H

#include <lax/displayer.h>
#include <lax/interfaces/somedata.h>

void DrawData(Laxkit::Displayer *dp,double *m,Laxkit::SomeData *data,Laxkit::anObject *a1,Laxkit::anObject *a2);
void DrawData(Laxkit::Displayer *dp,Laxkit::SomeData *data,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL);
Laxkit::SomeData *newObject(const char *thetype);
int pointisin(flatpoint *points, int n,flatpoint p);
int boxisin(flatpoint *points, int n,Laxkit::DoubleBBox *bbox);

#endif

