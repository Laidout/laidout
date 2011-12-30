//
// $Id$
//	
// Laidout, for laying out
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//

#include "pseps.h"

//! Prepare and include an EPS in the postscript file f.
/*! \todo *** must strip out the preview if any while inserting to f
 */
void psEps(FILE *f,EpsData *epsdata)
{
	if (!epsdata) return;
	
	FILE *in=fopen(epsdata->filename,"r");
	if (!in) return;
	
	fprintf(f,"\n");
			  
	
	fprintf(f,"BeginEPS\n"); //<- a function always written out in psout()
	fprintf(f,"\n"); 
	
	//***further transform: undo change to inches?
	
	fprintf(f,"%%BeginDocument\n");
		
	char buf[1024];
	size_t c;

	//*** must strip out preview, EPS spec says beware mac PICT resource and 
	//    windows metafile previews (pg 20, v3)
	while (!feof(in) && !ferror(in)) {
		c=fread(buf,1,1024,in);
		if (c>0) fwrite(buf,1,c,f);
	}

	fprintf(f,"%%EndDocument\n");
	fprintf(f,"EndEPS\n"); //<- a function always written out in psout()
}
