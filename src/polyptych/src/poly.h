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
// Copyright 2008-2012 Tom Lechner
//
#ifndef POLY_H
#define POLY_H

#include <lax/anobject.h>
#include <lax/dump.h>
#include <lax/lists.h>
#include <lax/vectors.h>
#include <lax/attributes.h>
#include <lax/screencolor.h>

#include "nets.h"

#include <cstdlib>
#include <cstdio>


namespace Polyptych {


//-------------------------------- Edge --------------------------------------

class Edge
{
 public:
	int p1,p2; //index in Polyhedron::vertices
	int f1,f2; //index in Polyhedron::faces
	int info = 0;
	bool is_seam = false; // for unwrapping
	bool is_cut = false;

	Edge(void) { p1=p2=0; f1=f2=-1; }
	Edge(int x, int y,int f=-1,int g=-1) {p1=x; p2=y; f1=f; f2=g; }
	Edge(Laxkit::flatpoint &f) { p1=(int) f.x; p2=(int) f.y;  f1=f2=-1; }
	Edge(const Edge &e) { p1=e.p1; p2=e.p2; f1=e.f1; f2=e.f2; }
};


//-------------------------------- Face --------------------------------------

/*! \class ExtraFace
 * \brief class to hold extra cache data for a Polyhedron's faces.
 */
class ExtraFace
{
 public:
	int numsides;
	Laxkit::spacepoint *points3d; //actual 3-d positions of vertices in space
	Laxkit::spacepoint center;
	Laxkit::flatpoint *points2d; //2-d points in relation to axis
	int *connectionedge; //edge index in another face this one connects to
	int *connectionstate; //whether face is actually connected to face in connections
	Laxkit::Basis axis;
	int facemode; //is face still in normal position, or maybe stage of animation to other position
	double a;    //can be used for a temporary angle face is tilted at
	double *dihedral;
	Laxkit::anObject *extra;
	clock_t timestamp;

	ExtraFace();
	virtual ~ExtraFace();
};

class Face
{
 public:
	int pn; // number of edges/points
	int *p; // list of vertices
	int *f; // face connections
	int *v; // vert labels (not edges yet)
	double *dihedral;
	int planeid;
	int setid;
	int facegroupid;
	char *label;
	ExtraFace *cache;

	Face();
	Face(int numof,int *ps);
	Face(int p1,int p2,int p3, int p4=-1, int p5=-1);
	Face(const char *pointlist,const char *linklist);
	virtual ~Face();
	Face(const Face &);
	Face &operator=(const Face &);
};


//-------------------------------- Pgon --------------------------------------

class Pgon
{
 public:
	Laxkit::ScreenColor color;
	int pn,id;
	Laxkit::flatpoint *p;
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
	Laxkit::flatpoint center();
};


//-------------------------------- Settype --------------------------------------

class Settype
{
 public:
	char *name;
	unsigned char on;
	int info;
	Laxkit::ScreenColor color;
	Laxkit::NumStack<int> faces;
	Settype() { name=NULL; on=1; info=0; }
	Settype(const Settype &nset) { name=NULL; on=1; info=0; *this=nset; }
	Settype &operator=(const Settype &);
	~Settype() { delete[] name; }
	void newname(const char *n);
};


//-------------------------------- Polyhedron --------------------------------------

class Polyhedron : 
	virtual public Laxkit::anObject,
	virtual public Laxkit::DumpUtility,
	virtual public AbstractNet
{
 public:
	char *name, *filename;

	Laxkit::NumStack<Laxkit::spacepoint> vertices;
	Laxkit::NumStack<Laxkit::Basis>      planes;
	Laxkit::PtrStack<Edge>    edges;
	Laxkit::PtrStack<Face>    faces;
	Laxkit::PtrStack<Settype> sets;

	Laxkit::Attribute meta;

	Polyhedron();
	Polyhedron(const Polyhedron &);
	Polyhedron &operator=(const Polyhedron &);
	virtual ~Polyhedron();
	virtual const char *whattype() { return "Polyhedron"; }

	// low level management functions
	virtual void clear();
	int validate();
	void connectFaces();
	int makeplanes();
	int makeedges();
	void applysets();
	virtual int AddToSet(int face, int set, const char *newsetname);

	// informational functions
	Laxkit::spacepoint CenterOfFace(int,int cache=0);
	Laxkit::spacevector VertexOfFace(int fce, int pt, int cache);
	Pgon FaceToPgon(int n,char useplanes);
	Laxkit::Basis basisOfFace(int n);
	Laxkit::Plane planeof(int pln);
	Laxkit::Plane planeOfFace(int fce,char centr=1);
	double pdistance(int a, int b);
	double segdistance(int fce,int fr,int p1,int p2);
	double angle(int a, int b,int dec=0); //uses planes, not faces


	// building functions
	virtual int AddPoint(Laxkit::spacepoint p);
	virtual int AddPoint(double x,double y,double z);
	virtual int AddFace(const char *str);
	virtual int AddFace(int n, ...);
	virtual void BuildExtra(); //create face cache
	virtual int FindEdge(int v1, int v2, int *dir_ret = nullptr);
	virtual bool SetEdgeInfo(int v1, int v2, int info,  bool add_if_not_exists = true);
	virtual bool SetEdgeSeam(int v1, int v2, bool seam, bool add_if_not_exists = true);
	virtual bool SetEdgeCut (int v1, int v2, bool cut,  bool add_if_not_exists = true);
	virtual ExtraFace *newExtraFace();
	virtual void collapseVertices(double zero, int vstart=-1, int vend=-1);
	virtual void MergeFaces(int face1, int edge);
	virtual int FindUniqueFaceId();

	virtual void dump_out(FILE *ff,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);

	virtual const char *InFileFormats();
	virtual int dumpInFile(const char *file, char **error_ret);
	virtual int dumpInOFF(FILE *f,char **error_ret);
	virtual int dumpInObj(FILE *f,char **error_ret);
	virtual int dumpInFold(const char *filename, char **error_ret);

	virtual const char *OutFileFormats();
	virtual int dumpOutFile(const char *file, const char *format,char **error_ret);
	virtual int dumpOutOFF(FILE *f,char **error_ret);
	virtual int dumpOutObj(FILE *f,char **error_ret);
	virtual int dumpOutVrml(FILE *f,char **error_ret);
	virtual int dumpOutGlb(FILE *f,char **error_ret);
	virtual int dumpOutGltfJson(FILE *f,char **error_ret, long binlength, const char *binfile);
	virtual int dumpOutFold(FILE *f,char **error_ret);

	// Abstract net functions
	virtual NetFace *GetFace(int i,double scaling);
	virtual int NumFaces();
	virtual const char *Filename();
	virtual const char *NetName() { return name; }
	virtual int dumpOutNet(FILE *f,int indent,int what);
};


} //namespace Polyptych

#endif

