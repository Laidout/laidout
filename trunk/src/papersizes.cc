//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
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
/*************** papersizes.cc ********************/

//TODO
//******* make this a Style, and define StyleDef
//
//*** various screen sizes: 800x600 72dpi, 1024x768 100dpi, 1280x1024, 1600x1200
//*** put in color attribute, 
//*** separate the list.cc/list.h

//#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;

#include "papersizes.h"
#include <lax/strmanip.h>

using namespace LaxFiles;
using namespace Laxkit;

//       PAPERSIZE    X inches   Y inches   X cm      Y cm
//       -----------------------------------------------------
const char *BuiltinPaperSizes[33*3]=
	{
		"Letter","8.5","11",
		"Legal","8.5","14",
		"Tabloid","11","17",
		"Ledger","17","11",
		"Index","3","5",
		"A4","8.26389","11.6944",
		"A3","11.6944","16.5278",
		"A2","16.5278","23.3889",
		"A1","23.3889","33.0556",
		"A0","33.0556","46.7778",
		"A5","5.84722","8.26389",
		"A6","4.125","5.84722",
		"A7","2.91667","4.125",
		"A8","2.05556","2.91667",
		"A9","1.45833","2.05556",
		"A10","1.02778","1.45833",
		"B0","39.3889","55.6667",
		"B1","27.8333","39.3889",
		"B2","19.6944","27.8333",
		"B3","13.9167","19.6944",
		"B4","9.84722","13.9167",
		"B5","6.95833","9.84722",
		"ArchA","9","12",
		"ArchB","12","18",
		"ArchC","18","24",
		"ArchD","24","36",
		"ArchE","36","48",
		"Flsa","8.5","13",
		"Flse","8.5","13",
		"Halfletter","5.5","8.5",
		"Note","7.5","10",
		"Custom","8.5","11",
		"Whatever","8.5","11"
	};
//---------------------------------- PaperStyle --------------------------------

/*! \class PaperStyle
 * \brief A simple class to hold the dimensions and orientation of a piece of paper.
 *
 * A number of standard sizes are built in. See GetBuiltinPaperSizes() for a list of them.
 * The only thing in flags is (flags&1) if it is landscape, or !(flags&1) if portrait.
 * Width/height will stay the same, but w() and h() will return the swapped values
 * for landscape.
 *
 * The extra peculiar paper type is "Whatever", which is used when you don't care about paper,
 * and don't want the paper outline drawn. Really this makes Laidout perform in full scratchboard mode.
 */
/*! \var unsigned int PaperStyle::flags
 * \brief flags&&1==landscape, !&&1=portrait
 */
/*! \var double PaperStyle::width
 * \brief The portrait style width of the paper.
 */
/*! \var double PaperStyle::height
 * \brief The portrait style height of the paper.
 */
/*! \fn double PaperStyle::w()
 * \brief If landscape (flags&&1), then return height, else return width.
 */
/*! \fn double PaperStyle::h()
 * \brief If landscape (flags&&1), then return width, else return height.
 */


/*! Dump out like the following. Note that the width and height are for the portrait
 * style. For the adjusted heights based on whether it is landscape, w() and h() return
 * width and height swapped.
 * <pre>
 *   name Letter
 *   width 8.5
 *   height 11
 *   dpi 360
 *   landscape
 * </pre>
 */
void PaperStyle::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (name) fprintf(f,"%sname %s\n",spc,name);
	fprintf(f,"%swidth %.10g\n",spc,width); 
	fprintf(f,"%sheight %.10g\n",spc,height);
	fprintf(f,"%sdpi %d\n",spc,dpi);
	fprintf(f,"%s%s\n",spc,(flags&1?"landscape":"portrait"));
}

//! Basically reverse of dump_out.
void PaperStyle::dump_in_atts(LaxFiles::Attribute *att,int flag)
{
	if (!att) return;
	char *aname,*value;
	for (int c=0; c<att->attributes.n; c++) {
		aname= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(aname,"name")) {
			if (value) makestr(name,value);
		} else if (!strcmp(aname,"width")) {
			DoubleAttribute(value,&width);
		} else if (!strcmp(aname,"height")) {
			DoubleAttribute(value,&height);
		} else if (!strcmp(aname,"dpi")) {
			IntAttribute(value,&dpi);
		} else if (!strcmp(aname,"landscape")) {
			flags|=1;//*** make this a define?
		} else if (!strcmp(aname,"portrait")) {
			flags&=~1;
		}
	}
}

//! Copy over name, width, height, dpi.
Style *PaperStyle::duplicate(Style *s)//s==NULL
{
	if (s==NULL) return (Style *)new PaperStyle(name,width,height,flags,dpi);
	if (!dynamic_cast<PaperStyle *>(s)) return NULL;
	PaperStyle *ps=dynamic_cast<PaperStyle *>(s);
	if (!ps) return NULL;
	makestr(ps->name,name);
	ps->width=width;
	ps->height=height;
	return s;
}

//! Simple constructor, sets name, w, h, f, dpi.
PaperStyle::PaperStyle(const char *nname,double w,double h,unsigned int f,int ndpi)
{
	if (nname) {
		name=new char[strlen(nname)+1];
		strcpy(name,nname);
	} else name=NULL;
	width=w;
	height=h;
	dpi=ndpi;
	flags=f;
}

//-------------------------------- GetBuiltinPaperSizes ------------------

//! Get a stack of PaperStyles with all the builtin paper sizes.
/*! \ingroup pools
 * If papers is NULL, then a new stack is created, filled and returned, otherwise,
 * the buitlins are pushed onto the given stack.
 * 
 * Currently, the builtin sizes (in inches X by Y) are:
 * <pre>
 *		"Letter","8.5","11",
 *		"Legal","8.5","14",
 *		"Tabloid","11","17",
 *		"Ledger","17","11",
 *		"Index","3","5",
 *		"A4","8.26389","11.6944",
 *		"A3","11.6944","16.5278",
 *		"A2","16.5278","23.3889",
 *		"A1","23.3889","33.0556",
 *		"A0","33.0556","46.7778",
 *		"A5","5.84722","8.26389",
 *		"A6","4.125","5.84722",
 *		"A7","2.91667","4.125",
 *		"A8","2.05556","2.91667",
 *		"A9","1.45833","2.05556",
 *		"A10","1.02778","1.45833",
 *		"B0","39.3889","55.6667",
 *		"B1","27.8333","39.3889",
 *		"B2","19.6944","27.8333",
 *		"B3","13.9167","19.6944",
 *		"B4","9.84722","13.9167",
 *		"B5","6.95833","9.84722",
 *		"ArchA","9","12",
 *		"ArchB","12","18",
 *		"ArchC","18","24",
 *		"ArchD","24","36",
 *		"ArchE","36","48",
 *		"Flsa","8.5","13",
 *		"Flse","8.5","13",
 *		"Halfletter","5.5","8.5",
 *		"Note","7.5","10",
 *		"Custom","-","-",
 *		"Whatever","-","-"
 * </pre>
 *
 * *** add NTSC, HDTV, 800x600, 1024x768, etc
 */
PtrStack<PaperStyle> *GetBuiltinPaperSizes(PtrStack<PaperStyle> *papers)
{
	if (papers==NULL) papers=new PtrStack<PaperStyle>;
	double x,y; 
	for (int c=0; c<33*3; c+=3) {
		 // x,y were in inches
		x=atof(BuiltinPaperSizes[c+1]);
		y=atof(BuiltinPaperSizes[c+2]);
		papers->push(new PaperStyle(BuiltinPaperSizes[c],x,y,0,360));
	}
	return papers;
}
	


