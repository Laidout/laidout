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
// Copyright 2008 Tom Lechner
//
#ifndef POLY_H
#define POLY_H

#include <lax/anobject.h>
#include <lax/refcounted.h>
#include <lax/dump.h>
#include <lax/lists.h>
#include <lax/vectors.h>
#include <lax/attributes.h>

#include "nets.h"

#include <cstdlib>
#include <cstdio>

//-------------------------------- Edge --------------------------------------
class Edge
{
 public:
	int p1,p2;
	int f1,f2; //
	Edge(void) { p1=p2=0; f1=f2=-1; }
	Edge(int x, int y,int f=-1,int g=-1) {p1=x; p2=y; f1=f; f2=g; }
	Edge(flatpoint &f) { p1=(int) f.x; p2=(int) f.y;  f1=f2=-1; }
	Edge(const Edge &e) { p1=e.p1; p2=e.p2; f1=e.f1; f2=e.f2; }
};

//-------------------------------- Face --------------------------------------
class Face
{
 public:
	int pn; /* number of edges/points */
	int *p; /* list of vertices */
	int *f,*v;  /* Face, vert labels (not edges yet) */
	int planeid,setid;
	Face();
	Face(int numof,int *ps);
	Face(int p1,int p2,int p3, int p4=-1, int p5=-1);
	Face(const char *pointlist,const char *linklist);
	~Face() { delete[] p; delete[] f; delete[] v; }
	Face(const Face &);
	Face &operator=(const Face &);
};

//-------------------------------- Pgon --------------------------------------
class Pgon
{
 public:
	unsigned long color;
	int pn,id;
	flatpoint *p;
	int *vlabel,*elabel,*flabel;
	double *dihedral;
	Pgon();
	Pgon(const Face &f);
	Pgon(int n,double r=1,int w=1); /* make regular polygon, radius r */
	Pgon(const Pgon &np);
	Pgon &operator=(const Pgon &np);
	~Pgon();
	void setup(int c,int newid);
	void clear();
	int findextent(double &xl,double &xr,double &yt,double &yb);
	flatpoint center();
};

//-------------------------------- Settype --------------------------------------
class Settype
{
 public:
	char *name;
	unsigned char on;
	unsigned long color;
	int ne,*e;
	Settype() { name=NULL; ne=0; e=NULL; }
	~Settype() { delete[] name; delete[] e; }
	Settype(const Settype &nset) { name=NULL; e=NULL; *this=nset; }
	Settype &operator=(const Settype &);
	void newname(const char *n);
};

//-------------------------------- Polyhedron --------------------------------------
class Polyhedron : 
	virtual public Laxkit::anObject,
	virtual public Laxkit::RefCounted,
	virtual public LaxFiles::DumpUtility,
	virtual public AbstractNet
{
 public:
	char *name, *filename;

	Laxkit::NumStack<spacepoint> vertices;
	Laxkit::PtrStack<Edge>       edges;
	Laxkit::PtrStack<Face>       faces;
	Laxkit::NumStack<basis>      planes;
	Laxkit::PtrStack<Settype>    sets;

	Polyhedron();
	Polyhedron(const Polyhedron &);
	Polyhedron &operator=(const Polyhedron &);
	virtual ~Polyhedron();

	virtual void clear();
	int validate();
	void connectFaces();
	spacepoint center(int);
	double pdistance(int a, int b);
	double segdistance(int fce,int fr,int p1,int p2);
	int makeplanes();
	int makeedges();
//	void planeson(unsigned long *&,unsigned char *&);
	void applysets();
	spacevector vofface(int fce, int pt);
	Pgon FaceToPgon(int n,char useplanes);
	basis BasisOfFace(int n);
	plane planeof(int pln);
	plane planeofface(int fce,char centr=1);
	double angle(int a, int b,int dec=0); /* uses planes, not faces */

	 //building functions
	virtual void AddPoint(spacepoint p);
	virtual void AddPoint(double x,double y,double z);
	virtual int  AddFace(const char *str);

	virtual void dump_out(FILE *ff,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *context);

	virtual int dumpInFile(const char *file, char **error_ret);
	virtual int dumpInOFF(FILE *f,char **error_ret);
	virtual int dumpOutVrml(const char *filename);

	virtual NetFace *GetFace(int i);
	virtual int NumFaces();
	virtual const char *Filename();
	virtual int dumpOutNet(FILE *f,int indent,int what);
};

#endif

