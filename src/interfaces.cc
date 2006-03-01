/**** laidout/src/interfaces.cc *****/


#include "interfaces.h"
#include "laidout.h"
#include <lax/lists.cc>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/bezpathoperator.h>
#include <lax/interfaces/rectinterface.h>


using namespace Laxkit;

//! Push any necessary PathOperator instances onto PathInterface::basepathops
void PushBuiltinPathops()
{
	PathInterface::basepathops.push(new BezpathOperator(NULL,1,NULL),1);
}

//! Get the built in interfaces. NOTE: Must be called after GetBuiltinPathops().
/*! The PathInterface requires that pathoppool be filled already.
 */
PtrStack<anInterface> *GetBuiltinInterfaces(PtrStack<anInterface> *existingpool) //existingpool=NULL
{
	if (!existingpool) { // create new pool if you are not appending to an existing one.
		existingpool=new PtrStack<anInterface>;
	}

	existingpool->push(new ImageInterface(2,NULL),1);
	existingpool->push(new PathInterface(1,NULL,NULL),1); //2nd null is pathop pool
	existingpool->push(new GradientInterface(3,NULL),1);
	existingpool->push(new ColorPatchInterface(4,NULL),1);
	existingpool->push(new RectInterface(5,NULL),1);
	//existingpool->push(new Interface(*****),1);
	//existingpool->push(new Interface(*****),1);
	
	return existingpool;
}
