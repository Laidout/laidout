#ifndef INTERFACES_H
#define INTERFACES_H

#include <lax/lists.h>
#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/pathinterface.h>

void PushBuiltinPathops();

Laxkit::PtrStack<Laxkit::anInterface> *
GetBuiltinInterfaces(Laxkit::PtrStack<Laxkit::anInterface> *existingpool); //existingpool=NULL

#endif
	
