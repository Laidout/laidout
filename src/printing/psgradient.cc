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

#include "psgradient.h"

using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 


//! Output postscript for a GradientData. ***imp me!!
/*! 
 */
void psGradient(FILE *f,GradientData *g)
{
	cout <<" *** GradientData ps out not implemented! "<<endl;
	//fprintf(f,
			//"<<"
			//"    /ShadingType  7\n"
			//"    /ColorSpace  /DeviceRGB\n"
			//"    /DataSource  [\n"
			//"      0               %%  edge flag, 0==start a new patch\n"
			//"    ]\n"
			//">> shfill\n",
			//);

}
