//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/********* dispositions.cc: ****************/

//----------------<< Builtin Disposition Instances >>--------------------
//
//This file's main purpose is to define GetBuiltinDispositionPool.
//
//To compile in your own disposition types, you must:
// 1. Write code, compile and put its object file in ???
// 2. Include its header here,
// 3. push an initial instance onto existingpool in function GetBuiltinDispositionPool
// 
//--- 



#include "laidout.h"
#include "disposition.h"
#include "dispositioninst.h"
#include "dispositions/netdisposition.h"

using namespace Laxkit;

//! Return a new Disposition instance that is type disp.
/*! \ingroup objects
 * Searches laidout->dispositionpool, and returns a duplicate of
 * the disposition or NULL.
 */
Disposition *newDisposition(const char *disp)
{
	if (!disp) return NULL;
	int c;
	for (c=0; c<laidout->dispositionpool.n; c++) {
		if (!strcmp(disp,laidout->dispositionpool.e[c]->Stylename())) {
			return (Disposition *)laidout->dispositionpool.e[c]->duplicate();
		}
	}
	return NULL;
}

//**** new Dispositions might want to install various novel Style/Styledefs,
//	***** or other initializations might have to occur... more thought required here!!
/*! \fn Laxkit::PtrStack<Disposition> *GetBuiltinDispositionPool(Laxkit::PtrStack<Disposition> *existingpool)
 * \ingroup pools
 */
//! Return a stack of defined dispositions.
/*! blah
 *
 */
PtrStack<Disposition> *GetBuiltinDispositionPool(PtrStack<Disposition> *existingpool) //existingpool=NULL
{
	if (!existingpool) { // create new pool if you are not appending to an existing one.
		existingpool=new PtrStack<Disposition>;
	}

	existingpool->push(new Singles(),1);
	existingpool->push(new DoubleSidedSingles(),1);
	existingpool->push(new BookletDisposition(),1);
	existingpool->push(new NetDisposition(),1);
	//existingpool->push(new CompositeDisposition(),1);
	//existingpool->push(new BasicBook(),1);
	//existingpool->push(new AnyOtherSpecificDispositionsYouWantBuiltIn,1);
	//...

	return existingpool;
}
