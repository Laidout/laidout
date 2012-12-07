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
#ifndef NETS_H
#define NETS_H

#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/displayer.h>

//----------------------------------- NetLine -----------------------------------
class NetLine
{
 public:
	char isclosed;
	int np;
	int *points;
	char lsislocal;
	Laxkit::LineStyle *linestyle;
	NetLine(const char *list=NULL);
	virtual ~NetLine();
	const NetLine &operator=(const NetLine &line);
	virtual int Set(const char *list);
	virtual int Set(int n, int closed);
	
	virtual void dump_out(FILE *f,int indent, int pfirst=0);
	virtual void dump_in_atts(LaxFiles::Attribute *att, const char *val);//val=NULL
};

//----------------------------------- NetFace -----------------------------------
class NetFace
{
 public:
	int np;
	int *points, *facelink;
	double *m;
	int aligno, alignx;
	int faceclass;
	NetFace();
	virtual ~NetFace();
	const NetFace &operator=(const NetFace &face);
	virtual void clear();
	virtual int Set(const char *list, const char *link=NULL);
	virtual int Set(int n,int *list,int *link=NULL,int dellists=0);
	
	virtual void dump_out(FILE *f,int indent, int pfirst=0);
	virtual void dump_in_atts(LaxFiles::Attribute *att, const char *val);//val=NULL
};

//----------------------------------- Net -----------------------------------
class Net : public Laxkit::SomeData
{
 public:
	char *thenettype;
	int np,tabs; 
	flatpoint *points;
	int *pointmap; // which thing (possibly 3-d points) corresponding point maps to
	int nl;
	NetLine *lines;
	int nf;
	NetFace *faces;
	Net();
	virtual ~Net();
	virtual void clear();
	virtual Net *duplicate();
	virtual const char *whatshape() { return thenettype; }
	virtual void FindBBox();
	virtual void FitToData(Laxkit::SomeData *data,double margin);
	virtual void ApplyTransform(double *mm=NULL);
	virtual void Center();
	virtual const char *whattype() { return thenettype; }
	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
	virtual int pointinface(flatpoint pp);
	virtual int rotateface(int f,int alignxonly=0);
	virtual void pushline(NetLine &l,int where=-1);
	virtual void pushface(NetFace &f);
	virtual void pushpoint(flatpoint pp,int pmap=-1);
	virtual double *basisOfFace(int which,double *mm=NULL,int total=0);

	//--perhaps for future:
	//virtual void PrintSVG(std::ostream &svg,Laxkit::SomeData *paper,int month=1,int year=2006);
	//virtual void PrintPS(std::ofstream &ps,Laxkit::SomeData *paper);
};



#endif
