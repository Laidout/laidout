//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2004-2007 by Tom Lechner
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
	LaxInterfaces::LineStyle *linestyle;
	NetLine(const char *list=NULL);
	virtual ~NetLine();
	const NetLine &operator=(const NetLine &line);
	virtual int Set(const char *list);
	virtual int Set(int n, int closed);
	
	virtual void dump_out(FILE *f,int indent, int pfirst=0);
	virtual void dump_in_atts(LaxFiles::Attribute *att, const char *val,int flag);//val=NULL
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
	virtual void dump_in_atts(LaxFiles::Attribute *att, const char *val,int flag);//val=NULL
};

//----------------------------------- Net -----------------------------------
class Net : public LaxInterfaces::SomeData
{
 public:
	char *thenettype;
	int np,tabs; 
	flatpoint *points;
	spacepoint *vertices; // optional list of 3-d points
	int nvertices;
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
	virtual void FitToData(Laxkit::DoubleBBox *data,double margin);
	virtual void ApplyTransform(double *mm=NULL);
	virtual void Center();
	virtual const char *whattype() { return thenettype; }
	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag);
	virtual int pointinface(flatpoint pp);
	virtual int rotateface(int f,int alignxonly=0);
	virtual void pushline(NetLine &l,int where=-1);
	virtual void pushface(NetFace &f);
	virtual void pushpoint(flatpoint pp,int pmap=-1);
	virtual void push3dpoint(double x,double y,double z);
	virtual double *basisOfFace(int which,double *mm=NULL,int total=0);

	virtual int LoadOFF(const char *filename,char **error_ret);
};



#endif

