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
// Copyright (C) 2004-2008 by Tom Lechner
//
#ifndef NETS_H
#define NETS_H

#include <lax/interfaces/somedata.h>
#include <lax/interfaces/coordinate.h>
#include <lax/interfaces/linestyle.h>
#include <lax/displayer.h>
#include <lax/dump.h>
#include <lax/lists.h>

//----------------------------------- NetLine -----------------------------------
class NetLine
{
 public:
	int tag; //is an outline, or inner line, for instance, if not using linestyle
	char lineinfo;
	LaxInterfaces::Coordinate *points;
	LaxInterfaces::LineStyle *linestyle;

	NetLine(const char *list=NULL);
	virtual ~NetLine();
	const NetLine &operator=(const NetLine &l);
	virtual int Set(const char *d,LaxInterfaces::LineStyle *ls);
	
	virtual void dumpOut(FILE *f,int indent, int what);
	virtual void dumpInAtts(LaxFiles::Attribute *att, const char *val,int flag);//val=NULL
};

//----------------------------------- NetFaceEdge -----------------------------------
enum FaceTag {
	FACE_None,
	FACE_Actual,
	FACE_Potential,
	FACE_Taken
};
class NetFaceEdge
{
 public:
	int id;
	int tooriginal;
	int toface,tofaceedge;
	int flipflag;
	FaceTag tag; //nolink, actual, potential, taken-elsewhere
	double svalue;
	LaxInterfaces::Coordinate *points;
	//double *basis_adjustment; ***not useful?
	
	NetFaceEdge();
	virtual ~NetFaceEdge();
	const NetFaceEdge &operator=(const NetFaceEdge &line);
};

//----------------------------------- NetFace -----------------------------------
class NetFace
{
 public:
	Laxkit::PtrStack<NetFaceEdge> edges;
	int original;
	char isfront;
	FaceTag tag; //FACE_Actual, FACE_Potential
	double *matrix;

	NetFace();
	virtual ~NetFace();
	const NetFace &operator=(const NetFace &face);
	virtual void clear();
	//virtual int Set(const char *list, const char *link=NULL);
	//virtual int Set(int n,int *list,int *link=NULL,int dellists=0);
	virtual int getOutline(int *n, flatpoint **p, int convert);
	
	virtual void dumpOut(FILE *f,int indent,int what);
	virtual void dumpInAtts(LaxFiles::Attribute *att);
};

//----------------------------------- AbstractNet -----------------------------------
class AbstractNet : 
	virtual public Laxkit::anObject,
	virtual public Laxkit::RefCounted,
	virtual public LaxFiles::DumpUtility
{
 public:
	char *name;
	AbstractNet();
	virtual ~AbstractNet();
	virtual const char *whattype() { return "AbstractNet"; }
	virtual const char *NetName() { return name; }
	virtual int NumFaces()=0;
	virtual NetFace *GetFace(int i) = 0;
	virtual int Modified();
	virtual int Modified(int m);
	virtual const char *Filename();
	virtual int dumpOutNet(FILE *f,int indent,int what) = 0;
};

//----------------------------------- BasicNet -----------------------------------
class BasicNet : public AbstractNet, public Laxkit::PtrStack<NetFace>
{
 public:
	BasicNet(const char *nname=NULL);
	virtual ~BasicNet();
	virtual const char *whattype() { return "BasicNet"; }
	virtual int NumFaces() { return Laxkit::PtrStack<NetFace>::n; }
	virtual NetFace *GetFace(int i);

	virtual int dumpOutNet(FILE *f,int indent,int what);
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *savecontext);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

//----------------------------------- Net -----------------------------------
class Net : public LaxInterfaces::SomeData
{
 protected:
	virtual int deleteFace(int netfacei);
 public:
	char *netname;
	int tabs;
	int info,active;
	LaxInterfaces::SomeData paper;

	AbstractNet *basenet;
	Laxkit::PtrStack<NetLine> lines;
	Laxkit::PtrStack<NetFace> faces;

	Net();
	virtual ~Net();
	virtual void clear();
	virtual Net *duplicate();
	virtual const char *whattype() { return "Net"; }
	virtual const char *whatshape() { return netname; }


	 //net component configuration functions
	virtual void FindBBox();
	virtual void FitToData(Laxkit::DoubleBBox *data,double margin,int setpaper);
	//virtual void Center();
	virtual int validateNet();

	 //informational functions
	virtual int pathOfFace(int i, int *n, flatpoint **p, int convert);
	virtual int pointinface(flatpoint pp);
	virtual int actualLink(int facei, int edgei);
	virtual int numActual();

	 //building functions
	virtual void pushline(NetLine &l,int what,int where=-1,int how=1);
	//virtual int rotateFaceOrientation(int f,int alignxonly=0);


	 //wrap functions
	virtual int Anchor(int basenetfacei);
	virtual int Unwrap(int netfacei,int atedge);
	virtual int TotalUnwrap();
	virtual int PickUp(int netfacei,int cutatedge);
	virtual int Drop(int netfacei);
	virtual int addPotentialsToFace(int facenum);
	virtual int findOriginalFace(int i,int status,int startsearchhere,int *index_ret);
	virtual int clearPotentials(int original);
	virtual int rebuildLines();
	virtual int connectFaces(int f1,int f2,int e);


	 //input and output functions
	virtual int Basenet(AbstractNet *newbasenet);
	virtual AbstractNet *loadBaseNet(const char *filename,char **error_ret);
	virtual int LoadOFF(const char *filename,char **error_ret);
	virtual int LoadFile(const char *file,char **error_ret);
	virtual int SaveSvg(const char *file,char **error_ret);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *savecontext);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
					
};



#endif

