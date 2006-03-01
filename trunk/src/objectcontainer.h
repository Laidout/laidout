#ifndef OBJECTCONTAINER_H
#define OBJECTCONTAINER_H


#include <lax/anobject.h>
#include "styles.h"

//------------------------------ ObjectContainer ----------------------------------

class ObjectContainer : virtual public Laxkit::anObject
{
 public:
	virtual int contains(Laxkit::anObject *d,FieldPlace &place);
	virtual Laxkit::anObject *getanObject(FieldPlace &place,int offset=0);
	virtual int nextObject(FieldPlace &place, FieldPlace &first, int curlevel, Laxkit::anObject **d=NULL);
	virtual int n()=0;
	virtual Laxkit::anObject *object_e(int i)=0;
};








#endif

