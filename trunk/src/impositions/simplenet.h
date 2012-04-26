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
// Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef SIMPLENET_H
#define SIMPLENET_H

#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/displayer.h>

#include "polyptych/src/nets.h"

//----------------------------------- SimpleNetLine -----------------------------------
class SimpleNetLine
{
 public:
	char isclosed, isoutline;
	int np;
	int *points;
	LaxInterfaces::LineStyle *linestyle;
	SimpleNetLine(const char *list=NULL);
	virtual ~SimpleNetLine();
	const SimpleNetLine &operator=(const SimpleNetLine &line);
	virtual int Set(const char *list);
	virtual int Set(int n, int closed);
	
	virtual void dump_out(FILE *f,int indent, int pfirst=0);
	virtual void dump_in_atts(LaxFiles::Attribute *att, const char *val,int flag);//val=NULL
};

//----------------------------------- SimpleNetFace -----------------------------------
class SimpleNetFace
{
 public:
	int np;
	int *points, *facelink;
	double *m;
	int aligno, alignx;
	int faceclass;
	SimpleNetFace();
	virtual ~SimpleNetFace();
	const SimpleNetFace &operator=(const SimpleNetFace &face);
	virtual void clear();
	virtual int Set(const char *list, const char *link=NULL);
	virtual int Set(int n,int *list,int *link=NULL,int dellists=0);
	
	virtual void dump_out(FILE *f,int indent, int pfirst=0);
	virtual void dump_in_atts(LaxFiles::Attribute *att, const char *val,int flag);//val=NULL
};

//----------------------------------- SimpleNet -----------------------------------
class SimpleNet : public AbstractNet, 
				  public Laxkit::PtrStack<NetFace>,
				  public LaxInterfaces::SomeData
{
 public:
	char *filename;
	char *thenettype;
	int np,tabs; 
	flatpoint *points;
	spacepoint *vertices; // optional list of 3-d points
	int nvertices;
	int *pointmap; // which thing (possibly 3-d points) corresponding point maps to
	int nl;
	SimpleNetLine *lines;
	int nf;
	SimpleNetFace *faces;

	SimpleNet();
	virtual ~SimpleNet();
	virtual void clear();
	virtual SimpleNet *duplicate();
	virtual const char *whatshape() { return thenettype; }
	virtual void FindBBox();
	virtual void FitToData(Laxkit::DoubleBBox *data,double margin);
	virtual void ApplyTransform(const double *mm=NULL);
	virtual void Center();
	virtual int pointinface(flatpoint pp,int innetcoord);
	virtual int rotateface(int f,int alignxonly=0);
	virtual void pushline(SimpleNetLine &l,int where=-1);
	virtual void pushface(SimpleNetFace &f);
	virtual void pushpoint(flatpoint pp,int pmap=-1);
	virtual void push3dpoint(double x,double y,double z);
	virtual double *basisOfFace(int which,double *mm=NULL,int total=0);

	 //io functions
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *savecontext);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);

	 //AbstractNet functions
	virtual const char *whattype() { return "SimpleNet"; }
	virtual const char *Filename() { return filename; }
	virtual int Modified();

	virtual int NumFaces() { return nf; }
	virtual NetFace *GetFace(int i,double scaling);

	virtual int dumpOutNet(FILE *f,int indent,int what);
};



#endif

