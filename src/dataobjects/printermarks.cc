//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2011,2012,2015,2016 by Tom Lechner
//


#include "printermarks.h"
#include "lpathsdata.h"
#include "datafactory.h"
#include "../language.h"

#include <lax/bezutils.h>
#include <lax/colors.h>


using namespace LaxInterfaces;
using namespace Laxkit;


namespace Laidout {


/*! \file
 * This file defines various common objects people may want.
 */


//! Return a registration mark centered at the origin, and pointsize along the edge.
DrawableObject *RegistrationMark(double pointsize, double linewidthinpoints)
{
	double d=pointsize/72/2; //radius of circle

	LPathsData *paths=dynamic_cast<LPathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));

	//paths->flags|=SOMEDATA_LOCK_CONTENTS;
	ScreenColor color(0,0,0,65535);
	paths->line(d/10,CapButt,JoinRound,&color);

	 //make circle
	flatpoint *pts=bez_circle(NULL,4, 0,0,d/2);
	for (int c=0; c<12; c++) {
		paths->append(pts[c], (c%3==0) ? POINT_TONEXT: (c%3==1?(POINT_VERTEX|POINT_REALLYSMOOTH):POINT_TOPREV) );
	}
	delete[] pts;
	paths->close();

	 //make cross marks
	paths->pushEmpty();
	paths->append(-d,0);
	paths->append( d,0);
	paths->pushEmpty();
	paths->append(0.,-d);
	paths->append(0., d);

	paths->FindBBox();
	paths->Id(_("Registration"));
	return paths;
}

//! Return a gray scale bar, in steps of 10%, each box being pointsize (so 11*pointsize wide, pointsize tall).
/*! Origin is lower left corner, where 100% black is.
 *
 * colorsystem can be grayscale, rgb, or cmyk.
 */
DrawableObject *BWColorBars(double pointsize, int colorsystem)
{
	double s=pointsize/72;

	Group *g = dynamic_cast<Group*>(somedatafactory()->NewObject(LAX_GROUPDATA));
	//g->flags|=SOMEDATA_LOCK_KIDS|SOMEDATA_KEEP_ASPECT;
	g->flags|=SOMEDATA_KEEP_ASPECT;
	//g->obj_flags|=OBJ_IgnoreKids;
	LPathsData *b;

	 //add fills
	ScreenColor color;
	char name[100];
	for (int c=0; c<11; c++) {
		b=dynamic_cast<LPathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
		b->appendRect(c*s,0,s,s);
		if (colorsystem==LAX_COLOR_GRAY) { color.grayf((10-c)/10.); b->fill(&color); }
		else if (colorsystem==LAX_COLOR_RGB) { color.rgbf((10-c)/10.,(10-c)/10.,(10-c)/10.);  b->fill(&color); }
		else if (colorsystem==LAX_COLOR_CMYK) { color.cmykf(0.,0.,0.,(10-c)/10.);  b->fill(&color); }

		sprintf(name, _("Gray %d%%"), c*10);
		b->Id(name);

		color.rgbf(0,0,0);//black outline
		b->line(0,CapButt,JoinMiter,&color);
		b->FindBBox();
		g->push(b);
		b->dec_count();
	}

	 //create outline of whole
	b=dynamic_cast<LPathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
	b->Id(_("Greybar outline"));
	color.rgbf(0,0,0);//black outline
	b->appendRect(0,0,11*s,s);
	//for (int c=1; c<=10; c++) { b->pushEmpty(); b->append(c*s,0); b->append(c*s,s); }
	b->line(s/36,CapButt,JoinMiter,&color);
	b->FindBBox();
	g->push(b);
	b->dec_count();

	g->Id(_("Greybars"));
	g->FindBBox();
	return g;
}

//! Return color bar for palette where each color is in a square of edge length pointsize.
DrawableObject *ColorBars(double pointsize, Palette *palette, int numrows, int numcols)
{
	if (!palette) return NULL;

	 //make numrows, numcols reflect palette
	if (numrows<=0 && numcols<=0) numcols=10;
	if (numrows<=0) numrows=1+(palette->colors.n-1)/numcols;
	if (numcols<=0) numcols=1+(palette->colors.n-1)/numrows;

	double s=pointsize/72;

	Group *g = dynamic_cast<Group*>(somedatafactory()->NewObject(LAX_GROUPDATA));
	LPathsData *b=dynamic_cast<LPathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));

	 //create outline
	b->appendRect(0,0,numcols*s,numrows*s);
	for (int c=1; c<=numcols; c++) { b->append(c*s,0.); b->append(c*s,numrows*s); b->pushEmpty(); }
	for (int r=1; r<=numrows; r++) { b->append(0.,r*s); b->append(numcols*s,r*s); b->pushEmpty(); }
	b->line(s/72);
	g->push(b);
	b->dec_count();

	 //add fills
	ScreenColor color;
	for (int c=0; c<numcols; c++) {
		for (int r=0; r<numrows; r++) {
			b=dynamic_cast<LPathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
			b->appendRect(c*s,r*s,s,s);
			color.rgbf(palette->colors.e[c]->color->values[0],
					   palette->colors.e[c]->color->values[1],
					   palette->colors.e[c]->color->values[2],
					   1.0);
			b->fill(&color);
			b->line(0);
			b->FindBBox();
			g->push(b);
		}
	}

	g->Id(_("Colorbars"));
	g->FindBBox();
	return g;
}

/*! Possible values include:
 * - Spread type  "%t"
 * - Spread number "%c"
 * - total number of spreads  "%n"
 * - Current date  "%D"
 * - Current time  "%T"
 * - file   "%f"
 * - Document name "%d"
 *
 * Default format is "%n, %t %c/%n", ie "Document, Spreadtype 4/10"
 * Default date format is YYYY-MM-DD ("%F" in terms of strftime)
 * Default time format is 24 hour time, with hours and minutes only ("%k%M" of strftime).
 */
/*SomeData *SpreadInfoMark(const char *format, const char *dateformat, const char *timeformat)
{
	if (!format) format="%f, %t %c/%n";
	CaptionData *info=new CaptionData;

	char *docfile=NULL,*docname=NULL,*spreadtype=NULL;
	int spreadi=-1, spreadn=0;


	char *s=newstr(format), *s2;
	char buffer[100];

	s2=replaceallname(s,"%t",spreadtype);
	delete[] s; s=s2;

	sprintf(buffer,"%d",spreadi);
	s2=replaceallname(s,"%c",buffer);
	delete[] s; s=s2;

	sprintf(buffer,"%d",spreadn);
	s2=replaceallname(s,"%n",buffer);
	delete[] s; s=s2;

	if (!dateformat) dateformat="%F"; //  YYYY-MM-DD
	struct tm curtime;
	localtime_r(time(NULL),&curtime);
	strftime(buffer,100, dateformat, &tm);
	s2=replaceallname(s,"%D",buffer);
	delete[] s; s=s2;

	if (!timeformat) timeformat="%k%M"; //buffer Time  13:24, 4:34
	strftime(buffer,100, timeformat, &tm);
	s2=replaceallname(s,"%T",buffer);
	delete[] s; s=s2;

	s2=replaceallname(s,"%f",docfile);
	delete[] s; s=s2;

	s2=replaceallname(s,"%d",docname);
	delete[] s; s=s2;

	info->SetText(s);
	delete[] s;
	delete[] docfile;
	delete[] docname;
	delete[] spreadtype;

	return info;
} */

} //namespace Laidout


