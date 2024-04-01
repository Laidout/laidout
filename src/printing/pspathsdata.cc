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
// Copyright (C) 2004-2007,2011 by Tom Lechner
//
#include "pspathsdata.h"


using namespace Laxkit;
using namespace LaxInterfaces;



namespace Laidout {


static int addpath(FILE *f,Coordinate *path)
{
	Coordinate *p,*p2,*start;
	p=start=path->firstPoint(1);
	if (!p) return 0;

	 //build the path to draw
	flatpoint c1,c2;
	int n=1; //number of points seen

	fprintf(f,"%.10g %.10g ",start->p().x,start->p().y);
	fprintf(f,"moveto\n");
	do { //one loop per vertex point
		p2=p->next; //p points to a vertex
		if (!p2) break;

		n++;

		//p2 now points to first Coordinate after the first vertex
		if (p2->flags&(POINT_TOPREV|POINT_TONEXT)) {
			 //we do have control points
			if (p2->flags&POINT_TOPREV) {
				c1=p2->p();
				p2=p2->next;
			} else c1=p->p();
			if (!p2) break;

			if (p2->flags&POINT_TONEXT) {
				c2=p2->p();
				p2=p2->next;
			} else { //otherwise, should be a vertex
				//p2=p2->next;
				c2=p2->p();
			}

			fprintf(f,"%.10g %.10g %.10g %.10g %.10g %.10g curveto\n",
					c1.x,c1.y,
					c2.x,c2.y,
					p2->p().x,p2->p().y);
		} else {
			 //we do not have control points, so is just a straight line segment
			fprintf(f,"%.10g %.10g lineto\n", p2->p().x,p2->p().y);
		}
		p=p2;
	} while (p && p->next && p!=start);
	if (p==start) fprintf(f,"closepath ");

	return n;
}

//! Output postscript for a PathsData.
void psPathsData(FILE *f,PathsData *pdata)
{
	Coordinate *p,*start;
	flatpoint pp;
	int n;
	LineStyle *lstyle;
	FillStyle *fstyle;

	for (int c=0; c<pdata->paths.n; c++) {
		p=start=pdata->paths.e[c]->path;
		if (!p) continue;

		lstyle=pdata->paths.e[c]->linestyle;
		if (!lstyle) lstyle=pdata->linestyle;//default for all data paths
		int hasstroke=lstyle ? (lstyle->function!=GXnoop) : 0;

		fstyle=pdata->fillstyle;//default for all data paths

		
		if (fstyle && fstyle->hasFill()) {
			n=addpath(f,p);
			if (n) {
				fprintf(f,"%.10g %.10g %.10g setrgbcolor\n",
							fstyle->color.red/65535.,fstyle->color.green/65535.,fstyle->color.blue/65535.);
				fprintf(f,"fill\n");
			}
		}


		if (hasstroke) {
			n=addpath(f,p);

			if (n) {
				if (lstyle->capstyle==CapButt) fprintf(f,"0 setlinecap\n");
				else if (lstyle->capstyle==CapRound) fprintf(f,"1 setlinecap\n");
				else if (lstyle->capstyle==CapProjecting) fprintf(f,"2 setlinecap\n");

				if (lstyle->joinstyle==JoinMiter) fprintf(f,"0 setlinejoin\n");
				else if (lstyle->joinstyle==JoinRound) fprintf(f,"1 setlinejoin\n");
				else if (lstyle->joinstyle==JoinBevel) fprintf(f,"2 setlinejoin\n");

				//setmiterlimit
				//setstrokeadjust

				fprintf(f," %.10g setlinewidth\n",lstyle->width);
				if (lstyle->dotdash==0 || lstyle->dotdash==~0)
					fprintf(f," [] 0 setdash\n");
				else fprintf(f," [%.10g %.10g] 0 setdash\n",lstyle->width,2*lstyle->width);

				fprintf(f,"%.10g %.10g %.10g setrgbcolor\n",
							lstyle->color.red/65535.,lstyle->color.green/65535.,lstyle->color.blue/65535.);
				fprintf(f,"stroke\n");
			}
		}
	}
}


} // namespace Laidout

