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
// Copyright 2008-2012 Tom Lechner
//

#include <fstream>
#include <iostream>
#include <cstring>
#include <cctype>

#include <lax/attributes.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include "poly.h"
#include <lax/lists.cc>
//#include <lax/attributes.h>

#define DBG 

using namespace std;
using namespace LaxFiles;
using namespace Laxkit;


namespace Polyptych {

//------------------------------ Edge -------------------------------------

/*! \class Edge
 * \brief Holds two integers that are indices into a vertex list.
 *
 * Also holds places for 2 faces meeting at that edge.
 */


//-------------------------------- Face --------------------------------------
/*! \class ExtraFace
 * \brief class to hold extra cache data for a Polyhedron's Face objects.
 *
 * You may add arbitrary data in ExtraFace::extra. This will be dec_count()'d
 * in ~ExtraFace().
 */

ExtraFace::ExtraFace()
{
	numsides=0;
	points3d=NULL;
	points2d=NULL;
	dihedral=NULL;
	connectionedge=connectionstate=NULL;
	facemode=-1;
	extra=NULL;
	timestamp=0;
}

ExtraFace::~ExtraFace()
{
	if (points3d) delete[] points3d;
	if (points2d) delete[] points2d;
	if (dihedral) delete[] dihedral;
	if (connectionedge) delete[] connectionedge;
	if (connectionstate) delete[] connectionstate;
	if (extra) extra->dec_count();
}
//------------------------------ Face -------------------------------------

/*! \class Face
 * \brief Holds point list (indices to v array), and number labels for the edges.
 */
/*! \var int *Face::f
 * \brief Face labels, on this edge, connects to another face at index of Polyhedron::faces.
 */
/*! \var int *Face::v
 * \brief Vertex labels, index of point in Polyhedron::p???. ***diff from p how?
 */
/*! \var int *Face::p
 * \brief Vertex indices, index of point in Polyhedron::p.
 */


DBG void dumpface(Face *face,int facenum)
DBG {
DBG	cerr <<"face "<<facenum<<", pn="<<face->pn<<":  ";
DBG	for (int c=0; c<face->pn; c++) cerr <<face->p[c]<<" ";
DBG	cerr <<endl;
DBG }

//! Create empty face.
Face::Face()
{
	cache=NULL;
	planeid=setid=-1; pn=0; v=NULL; f=NULL; p=NULL;
	facegroupid=-1;
	dihedral=NULL;
}

//! Create a Face with the ps array as the point indices.
/*! ps is copied.
 */
Face::Face(int numof,int *ps) 
{
	cache=NULL;
	p=NULL; f=NULL; v=NULL;
	planeid=setid=-1;
	facegroupid=-1;
	if (numof<3) { pn=0; return; }
	pn=numof;
	v=new int[pn];
	f=new int[pn];
	p=new int[pn];
	dihedral=new double[pn];
	for (int c=0; c<pn; c++) { p[c]=ps[c]; f[c]=-1; v[c]=-1; dihedral[c]=0; }
}

//! Shortcut to create polygons with up to 5 vertices.
/*! Must have at least 3. After that, stops at the first -1.
 * 
 * \todo *** could have this with the ... variadic func thing
 */
Face::Face(int p1,int p2,int p3, int p4, int p5) /* p4=-1, p5=-1 */
{
	cache=NULL;
	planeid=setid=-1;
	facegroupid=-1;
	if (p4==-1) pn=3;
	else if (p5==-1) pn=4;
	else pn=5;

	p=new int[pn];
	f=new int[pn];
	v=new int[pn];
	dihedral=new double[pn];
	p[0]=p1; p[1]=p2; p[2]=p3;
	if (pn>3) p[3]=p4;
	if (pn>4) p[4]=p5;
	for (int c=0; c<pn; c++) { f[c]=-1; v[c]=-1; dihedral[c]=0; }
}

//! Create a new face from a string like "3 4 5" or "3,5,6". Delimiter is ignored.
/*! \todo is delimiter ignored???
 */
Face::Face(const char *pointlist,const char *linklist)
{
	cache=NULL;
	planeid=setid=-1; pn=0; v=NULL; f=NULL; p=NULL;
	facegroupid=-1;
	dihedral=NULL;

	IntListAttribute(pointlist,&p,&pn,NULL);
	if (!pn) return;

	if (!f) f=new int[pn];
	v=new int[pn];
	dihedral=new double[pn];
	for (int c=0; c<pn; c++) { f[c]=-1; v[c]=-1; dihedral[c]=0; }

	int n;
	if (linklist) IntListAttribute(linklist,&f,&n,NULL);
	//if (pn!=n) there is a problem!!
}

//! Create copy of fce.
Face::Face(const Face &fce)
{
	dihedral=NULL;
	cache=NULL;
	v=NULL;
	f=NULL;
	p=NULL;
	*this=fce;
}

Face::~Face()
{ 
	if (cache) delete cache;
	delete[] p;
	delete[] f;
	delete[] v; 
	if (dihedral) delete[] dihedral;
}

//! Face equals operator.
Face &Face::operator=(const Face &fce)
{
	delete[] v; delete[] f; delete[] p;
	if (fce.pn) {
		pn=fce.pn;
		v=new int[pn];
		f=new int[pn];
		p=new int[pn];
		for (int c=0; c<pn; c++) {
			v[c]=fce.v[c];
			f[c]=fce.f[c];
			p[c]=fce.p[c];
		}

	} else {pn=0; p=NULL; f=NULL; p=NULL; }
	planeid=fce.planeid;
	setid=fce.setid;
	facegroupid=fce.facegroupid;
	return *this;
}



//------------------------------ Pgon -------------------------------------

/*! \class Pgon
 * \brief Class to hold 2-dimensional polygons with various labels.
 */
/*! \var int *Pgon::vlabel
 * \brief Vertex label
 */
/*! \var int *Pgon::elabel
 * \brief Vertex label
 */
/*! \var int *Pgon::flabel
 * \brief Vertex label
 */
/*! \var double *Pgon::dihedral
 * \brief Dihedral angles to adjacent faces.
 */


//! Create blank Pgon.
Pgon::Pgon()
{
	id=-1;
	pn=0;
	p=NULL;
	vlabel=elabel=flabel=NULL;
	dihedral=NULL;
}

//! Clear the Pgon, then allocate all the arrays for c points.
void Pgon::setup(int c,int newid)
{
	if (pn || c<3) return;
	delete[] p;
	delete[] elabel;
	delete[] flabel;
	delete[] vlabel;
	delete[] dihedral;
	id=newid;
	pn=c;
	p=new flatpoint[pn];
	dihedral=NULL;
	elabel=new int[pn];
	flabel=new int[pn];
	vlabel=new int[pn];
	for (c=0; c<pn; c++) {
		flabel[c]=-1;
		elabel[c]=-1;
		vlabel[c]=-1;
	}
}

//! Create as much of a copy of f as possible.
/*! Copies over all the edge/face/vertex labels, 
 * but the actual points remain undefined.
 */
Pgon::Pgon(const Face &f)
{
	id=-1;
	pn=f.pn;
	p=new flatpoint[pn];
	dihedral=NULL;
	elabel=new int[pn];
	flabel=new int[pn];
	vlabel=new int[pn];
	for (int c=0; c<pn; c++) {
		flabel[c]=(f.f?f.f[c]:-1);
		elabel[c]=(f.v?f.v[c]:-1);
		vlabel[c]=(f.p?f.p[c]:-1);
	}
}

//! Create a regular polygon with n points inside circle of radius r.
/*! If w==1, then the first point is on the positive x axis. If w==-1 
 * then the first point is on the negative x axis. If w is any other
 * value, then 1 is used for w.
 */
Pgon::Pgon(int n,double r,int w) // r=radius=1,w 1 on+x,-1 on-x
{
	if (w!=-1) w=1;
	id=-1;
	vlabel=elabel=flabel=NULL;
	dihedral=NULL;
	if (n<3) { pn=0; p=NULL; return; }
	pn=n;
	p=new flatpoint[pn];
	for (int c=0; c<pn; c++)
		p[c]=flatpoint(w*r*cos(c*2*M_PI/n),r*sin(c*2*M_PI/n));
}

//! Create copy of np.
Pgon::Pgon(const Pgon &np)
{
	p=NULL;
	vlabel=elabel=flabel=NULL;
	dihedral=NULL;
	*this=np;
}

Pgon::~Pgon()
{
	delete[] p;
	delete[] vlabel;
	delete[] elabel;
	delete[] flabel;
	delete[] dihedral;
}

//! Flush all the data of the Pgon.
void Pgon::clear()
{
	delete[] p; 	 p=NULL;
	delete[] vlabel;	 vlabel=NULL;
	delete[] elabel;	 elabel=NULL;
	delete[] flabel;	 flabel=NULL;
	delete[] dihedral; dihedral=NULL;
	pn=0; id=-1;
}

//! Pgon equals operator.
Pgon &Pgon::operator=(const Pgon &np)
{
	color=np.color;
	id=np.id;
	pn=np.pn;
	delete[] p;
	delete[] vlabel;
	delete[] elabel;
	delete[] flabel;
	delete[] dihedral;
	int c;
	if (np.p) {
		p=new flatpoint[pn];
		for (c=0; c<pn; c++) p[c]=np.p[c];
	} else p=NULL;
	if (np.vlabel) {
		vlabel=new int[pn];
		for (c=0; c<pn; c++) vlabel[c]=np.vlabel[c];
	} else vlabel=NULL;
	if (np.elabel) {
		elabel=new int[pn];
		for (c=0; c<pn; c++) elabel[c]=np.elabel[c];
	} else elabel=NULL;
	if (np.flabel) {
		flabel=new int[pn];
		for (c=0; c<pn; c++) flabel[c]=np.flabel[c];
	} else flabel=NULL;
	if (np.dihedral) {
		dihedral=new double[pn];
		for (c=0; c<pn; c++) dihedral[c]=np.dihedral[c];
	} else dihedral=NULL;
	return *this;
}

//! Return the average of the points of the Pgon.
flatpoint Pgon::center()
{
	if (!pn) return flatpoint();
	flatpoint cen;
	for (int c=0; c<pn; c++) cen+=p[c];
	cen/=pn;
	return cen;
}

//! Find the bounding box of the Pgon.
int Pgon::findextent(double &xl,double &xr,double &yt,double &yb)
{
	if (!p) return 0;
	xl=xr=p[0].x; yt=yb=p[0].y;
	for (int c=1; c<pn; c++)
	{
		if (p[c].x<xl) xl=p[c].x;
		if (p[c].x>xr) xr=p[c].x;
		if (p[c].y<yb) yb=p[c].y;
		if (p[c].y>yt) yt=p[c].y;
	}
	return 1;
}


//------------------------------ Settype -------------------------------------

/*! \class Settype
 * \brief Class to hold some grouping that all have similar attributes like color, or visibility.
 */


//! Rename the set.
void Settype::newname(const char *n)
{
	delete[] name;
	name=new char[strlen(n)+1];
	strcpy(name,n);
} 

Settype &Settype::operator=(const Settype &nset)
{
	delete[] name;
	name=new char[strlen(nset.name)+1];
	strcpy(name,nset.name);
	faces=nset.faces;
	on=nset.on;
	color=nset.color;
	return *this;
}

//------------------------------ Polyhedron -------------------------------------

/*! \class Polyhedron
 * \brief Polyhedron class.
 *
 * A polyhedron here consists of a list of vertices, edges, faces, planes, and sets.
 * There are only as many planes as necessary to hold all the faces. The edge and face
 * lists hold indices into the vertex array, which is an array of spacevectors.
 */


//! Create a totally blank polyhedron.
Polyhedron::Polyhedron()
{
	name=filename=NULL;
}

//! Destructor, just calls clear().
Polyhedron::~Polyhedron()
{
	clear();
}

//! Copy constructor for Polyhedron.
Polyhedron::Polyhedron(const Polyhedron &nphed)
{
	*this=nphed;
}

//! Return a new blank ExtraFace object to attach to a face, usually via BuildExtra().
ExtraFace *Polyhedron::newExtraFace()
{
	return new ExtraFace;
}

//! Create extra data pertaining to each polyhedron face.
/*! Will delete any face cache data that already exists.
 *
 * This will assume that makeedges() has already been called, or
 * the edges all already exist.
 *
 * Will set facemode=0, and connectionstate[*]=1. The basis is same as basisOfFace(),
 * which has the basis origin at the first face point, xaxis points toward the second point,
 * z points away from shape, y=z cross x.
 */
void Polyhedron::BuildExtra()
{
	if (faces.n==0) return;
	ExtraFace *cache;
	Face *face;
	int n;
	for (int c=0; c<faces.n; c++) {
		face=faces.e[c];
		n=face->pn;
		if (face->cache) delete face->cache;
		cache=face->cache=newExtraFace();
		cache->numsides=face->pn;
		cache->points3d=new spacevector[n];
		cache->points2d=new flatvector[n];
		cache->connectionedge=new int[n];
		cache->connectionstate=new int[n];
		cache->facemode=0;
		cache->a=0;
		cache->axis=basisOfFace(c);
		for (int c2=0; c2<n; c2++) {
			cache->points3d[c2]=vertices[face->p[c2]];
			cache->center+=cache->points3d[c2];
			cache->points2d[c2]=flatten(vertices[face->p[c2]],cache->axis);

			cache->connectionstate[c2]=1;
			cache->connectionedge[c2]=-1;//***should be the actual value...
		}
		if (n) cache->center/=n;
	}
}

//! Equal operator for Polyhedron.
Polyhedron &Polyhedron::operator=(const Polyhedron &nphed)
{
	clear();
	name=new char[strlen(nphed.name)+1];
	strcpy(name,nphed.name);
	int c,n;
	if (nphed.vertices.n) {
		n=nphed.vertices.n;
		spacepoint *t;
		t=new spacepoint[nphed.vertices.n];
		memcpy(t,nphed.vertices.e,sizeof(spacepoint));
		vertices.insertArray(t,n);
	}
	if (nphed.edges.n) {
		for (c=0; c<nphed.edges.n; c++) edges.push(new Edge(*nphed.edges.e[c]));
	}
	if (nphed.faces.n) {
		for (c=0; c<nphed.faces.n; c++) faces.push(new Face(*nphed.faces.e[c]));
	} 
	if (nphed.planes.n) {
		for (c=0; c<nphed.planes.n; c++) planes.push(basis(nphed.planes.e[c]));
	} 
	if (nphed.sets.n) {
		for (c=0; c<nphed.sets.n; c++) sets.push(new Settype(*nphed.sets.e[c]));
	} 
	return *this;
}

//! Wipe out all the data in the polyhedron.
void Polyhedron::clear()
{
	if (name) delete[] name;  name=NULL;
	if (filename) delete[] filename;  filename=NULL;

	faces.flush();
	edges.flush();
	vertices.flush();
	planes.flush();
	sets.flush();
}

//! Return the distance between points with indices a and b. 
double Polyhedron::pdistance(int a, int b)
{ 
	return ::distance(vertices[a],vertices[b]); 
}

//! Validate all the indices between the various lists.
/*!
 * \todo  make the return value meaningful.
 * \todo MUST validate setid versus what a set thinks it has, also planeid.
 *   this is a must for after reading in a potentially corrupted file.
 */
int Polyhedron::validate()
{
	int c,c2;
	DBG cerr << "Validating..";
	for (c=0; c<edges.n; c++)
		if (    edges.e[c]->p1<0 || edges.e[c]->p1>=vertices.n ||
				edges.e[c]->p2<0 || edges.e[c]->p2>=vertices.n) {
			DBG cerr << "\nBad edge "<<c<<"."; 
			return 0; 
		}
	for (c=0; c<faces.n; c++) {
		if (faces.e[c]->pn==0) {
			DBG cerr << "\nEmpty face "<<c<<"."; 
			return 0; 
		}
		for (c2=0; c2<faces.e[c]->pn; c2++)
			if (faces.e[c]->p[c2]<0 || faces.e[c]->p[c2]>=vertices.n)	{
				DBG cerr << "\nBad face "<<c<<' '<<c2<<".\n";
				return 0; 
			}
	}
	DBG cerr <<"..done validating\n";
	return 1;
}

//! Automatically connect faces in Face::f.
/*! First clears any face connection info in the faces already there, then connects them properly.
 *
 * You would call this after doing a lot of AddPoint() and AddFace().
 */
void Polyhedron::connectFaces()
{
	 //first clear all face links
	for (int c=0; c<faces.n; c++) {
		if (!faces.e[c]->f) faces.e[c]->f=new int[faces.e[c]->pn];
		for (int c2=0; c2<faces.e[c]->pn; c2++) {
			faces.e[c]->f[c2]=-1;
		}
	}

	 //now find connections
	int c,c2,c3,c4;
	int a1,a2;
	for (c=0; c<faces.n; c++) {
	  for (c2=0; c2<faces.e[c]->pn; c2++) {
	  	if (faces.e[c]->f[c2]!=-1) continue;

	  	a1=faces.e[c]->p[c2];
	  	a2=faces.e[c]->p[(c2+1)%faces.e[c]->pn];

	  	for (c3=0; c3<faces.n; c3++) {
  		  for (c4=0; c4<faces.e[c]->pn; c4++) {
  		  	if ((faces.e[c3]->p[c4]==a1 && faces.e[c3]->p[(c4+1)%faces.e[c]->pn]==a2)
				 || (faces.e[c3]->p[c4]==a2 && faces.e[c3]->p[(c4+1)%faces.e[c]->pn]==a1)) {
  		  		faces.e[c]->f[c2]=c3;
				faces.e[c3]->f[c4]=c;
				c3=faces.n;
				break;
  		  	}
  		  }
	  	}
	  }
	}
}

//! Add a face to a set.
/*! If set>=0 and set<sets.n, then add to that set. Otherwise, create a new
 * set with the given name. If not creating a new set, newsetname is ignored.
 */
int Polyhedron::AddToSet(int face, int set, const char *newsetname)
{
	if (set>=0 && set<sets.n) {
		sets.e[set]->faces.pushnodup(face);
		return 0;
	}

	Settype *newset=new Settype();
	newset->newname(newsetname);
	newset->faces.push(face);

	return 0;
}

//! From the sets list, tag the faces with the proper set id.
void Polyhedron::applysets()
{
	if (sets.n==0) return;
	int a,c2;
	for (int c=0; c<faces.n; c++) faces.e[c]->setid=-1;
	for (int c=0; c<sets.n; c++) {
		for (c2=0; c2<sets.e[c]->faces.n; c2++) {
			a=sets.e[c]->faces.e[c2];
			if (a>=faces.n) continue;
			faces.e[a]->setid=c;
		}
	}
}

//! From the faces list, construct the planes list.
int Polyhedron::makeplanes()
{
	if (planes.n>0) return 0;
	if (faces.n==0) return 0;
	plane pl;
	int c2;
	for (int c=0; c<faces.n; c++) {
		pl=planeOfFace(c);
		for (c2=0; c2<planes.n; c2++) {
			if (distance(planes.e[c2].p,pl)==0 && isnotvector(pl.n/planes.e[c2].z)) {
				 // plane exists already
				faces.e[c]->planeid=c2;
				break;
			}
		}
		if (c2==planes.n) {
			faces.e[c]->planeid=planes.n;
			planes.push(basis(pl.p, pl.p+pl.n, pl.p+vertices[faces.e[c]->p[1]]-vertices[faces.e[c]->p[0]]));
		}
	}
	return planes.n;
}

//! From the faces list, construct the edges list.
/*! Call this when only verts and faces defined, and you want a list of 
 * edges. This also updates the face labels of the face list (Face::flabel).
 *
 * Any edges already there are flushed.
 */
int Polyhedron::makeedges()
{
	DBG cerr <<"making edges..."<<endl;
	if (faces.n==0 || vertices.n==0) return 0;
	
	int c,c2,c3,emax=0,p1,p2;
	if (edges.n) edges.flush();

	for (c=0; c<faces.n; c++) emax+=faces.e[c]->pn;
	if (emax<=0) return 0;
	
	DBG cerr <<"makeedges emax:"<<emax;
	
	 //for each edge of each face, see if it matches any edge in any other face.
	for (c=0; c<faces.n; c++) {              //for each face in polyhedron
		for (c2=0; c2<faces.e[c]->pn; c2++) { //for each edge in face
			p1=faces.e[c]->p[c2];  //point 1 of a face's edge
			p2=faces.e[c]->p[(c2+1)%faces.e[c]->pn]; //point 2 of a face's edge
			for (c3=0; c3<edges.n; c3++) { //check current edge against known edges
				if ((edges.e[c3]->p1==p1 && edges.e[c3]->p2==p2) ||
					(edges.e[c3]->p1==p2 && edges.e[c3]->p2==p1)) break;
			}
			if (c3==edges.n) { // edge not found in this->edges
				edges.push(new Edge(p1,p2,c,-1));
				faces.e[c]->f[c2]=-1; //-1 because we are not sure what face it connects to yet
			} else {
				 //edge already exists, which means that the edge references 1 face, since
				 //edges was flushed above, and edges will be encountered a total of exactly 2 times.
				 //so connect face c with face referenced in edge c3:
				 // change face->f
				edges.e[c3]->f2=c; //f1 had reference to other face already
				faces.e[c]->f[c2]=edges.e[c3]->f1; //set f label for vertices=c2
				for (int c4=0; c4<faces.e[edges.e[c3]->f1]->pn; c4++) {
					if (faces.e[edges.e[c3]->f1]->p[c4]==p1 && 
							faces.e[edges.e[c3]->f1]->p[(c4+1)%faces.e[edges.e[c3]->f1]->pn]==p2) {
						faces.e[edges.e[c3]->f1]->f[c4]=edges.e[c3]->f2;
						break; 
					}
					if (faces.e[edges.e[c3]->f1]->p[c4]==p1 &&
							faces.e[edges.e[c3]->f1]->p[c4>0?c4-1:faces.e[edges.e[c3]->f1]->pn-1]==p2) {
						faces.e[edges.e[c3]->f1]->f[c4>0?c4-1:faces.e[edges.e[c3]->f1]->pn-1]=edges.e[c3]->f2; 
						break; 
					}
				}
			}
		}
	}

	DBG cerr <<"...total edges="<<edges.n<<'\n';
	DBG for (c=0; c<faces.n; c++) {
	DBG 	cerr <<"face "<<c<<": ";
	DBG 	for (c2=0; c2<faces.e[c]->pn; c2++) cerr <<faces.e[c]->f[c2]<<" ";
	DBG 	cerr <<endl;
	DBG }

	return 1;
}

//! Make any vertices within a certain distance be the same vertex.
/*! For approximation purposes, you can say zero=1e-10, for instance, to 
 * collapse all points that distance or less. Pass in 0 if the point has to be exactly the same.
 *
 * This will update all face references to the points.
 * You might want to run makeedges() again afterwards, as currently edges is flushed.
 *
 * \todo should update edges too, right now just flushes edges
 * \todo **** must collapse edges when points are too near!! such as for gore tips...
 */
void Polyhedron::collapseVertices(double zero, int vstart, int vend)
{
	 //remove existing edges
	edges.flush();

	zero*=zero;
	double d;
	spacevector v;
	if (vstart<0) vstart=0;
	else if (vstart>=vertices.n) vstart=vertices.n-1;
	if (vend<0 || vend>=vertices.n) vend=vertices.n-1;

	 //collapse vertices
	for (int c=vstart; c<=vend; c++) {
		for (int c2=c+1; c2<=vend; c2++) {
			v=vertices.e[c2]-vertices.e[c];
			d=v*v;
			if (d<zero || (d>0 && vertices.e[c2]==vertices.e[c])) {
				 //found one to collapse
				for (int c3=0; c3<faces.n; c3++) {
					for (int c4=0; c4<faces.e[c3]->pn; c4++) {
						if (faces.e[c3]->p[c4]==c2) {
							faces.e[c3]->p[c4]=c;
						} else if (faces.e[c3]->p[c4]>c2) {
							faces.e[c3]->p[c4]--;
						}
					}
				}
				vertices.pop(c2);
				vend--;
				c2--;
			}
		}
	}

	 //check faces for null edges and remove.
	 //If a null face results, then remove
	int compto;
	for (int c3=0; c3<faces.n; c3++) {
		for (int c4=0; c4<faces.e[c3]->pn; c4++) {
			compto=c4-1;
			if (compto<0) compto=faces.e[c3]->pn-1;

			if (faces.e[c3]->p[compto]==faces.e[c3]->p[c4]) {
				 //null edge found...

				 //only move memory if you really have to
				if (c4>0 && c4<faces.e[c3]->pn-1) {
					memmove(faces.e[c3]->p+(c4  ),  //to
							faces.e[c3]->p+(c4+1),  //from
							(faces.e[c3]->pn-1-c4)*sizeof(int));//count
					if (faces.e[c3]->f) memmove(faces.e[c3]->f+c4, faces.e[c3]->f+c4+1, (faces.e[c3]->pn-1-c4)*sizeof(int));
					if (faces.e[c3]->v) memmove(faces.e[c3]->v+c4, faces.e[c3]->v+c4+1, (faces.e[c3]->pn-1-c4)*sizeof(int));
				}
				faces.e[c3]->pn--;
				c4--;
			}
		}
	}
}

//! Return the average of the points in face with index fce.
/*! If cache!=0, then return the center of the cached face (face->cache->points3d),
 * not the center of the actual face. If there is no cache, then the actual face
 * center is returned.
 */
spacepoint Polyhedron::CenterOfFace(int fce,int cache)//cache==0
{
	spacepoint pnt;
	for (int c=0; c<faces.e[fce]->pn; c++) {
		if (cache && faces.e[fce]->cache && faces.e[fce]->cache->points3d) 
			pnt+=faces.e[fce]->cache->points3d[c];
		else pnt=pnt+vertices[faces.e[fce]->p[c]];
	}
	return (pnt/faces.e[fce]->pn);
}

//! Return the vertex that is pt in list held by face with index fce (vertices[f[fce].p[pt]]).
/*! If cache!=0 then use the cached points3d points instead.
 */
spacevector Polyhedron::VertexOfFace(int fce, int pt, int cache)
{
	if (fce<0 || fce>=faces.n) return spacevector(0,0,0);
	if (pt<0 || pt>=faces.e[fce]->pn) return spacevector();
	if (faces.e[fce]->p[pt]>=0) {
		if (cache && faces.e[fce]->cache && faces.e[fce]->cache->points3d) 
			return faces.e[fce]->cache->points3d[pt];
		return vertices[faces.e[fce]->p[pt]];
	}
	return spacevector();
}

//! Return a Pgon corresponding to face with index n.
/*! If useplanes==0, then the pgon has origin at f.vertices[0], and x is toward f.vertices[1]. 
 * Scaling is same as for original space.
 * 
 * If useplanes!=0, then try to use planes[f[n].planeid] as the basis to
 * flatten the face with. This is useful when there are several faces on
 * the same plane and you want pgons defined for them all on the same grid.
 * Of course, the origin and rotation of these pgons do not correspond to
 * f.vertices[0].
 */
Pgon Polyhedron::FaceToPgon(int n, char useplanes)
{
	Pgon npgon(*faces.e[n]);
	if (faces.e[n]->setid==-1) npgon.color.rgbf(1.,1.,1.,1.);
	   else npgon.color=sets.e[faces.e[n]->setid]->color;
	npgon.dihedral=new double[npgon.pn];
	basis bas;
	if (useplanes && faces.e[n]->planeid>=0) bas=planes.e[faces.e[n]->planeid];
	else {
		if (faces.e[n]->p[0]==-1) { npgon.clear(); return npgon; }
		plane pl=planeOfFace(n,0);
		bas=basis(pl.p, pl.p+pl.n, pl.p+vertices.e[faces.e[n]->p[1]]-vertices[faces.e[n]->p[0]]);
	}
	for (int c=0; c<npgon.pn; c++) {	
		npgon.p[c]=flatten(VertexOfFace(n,c,0),bas);
		if (faces.e[n]->f[c]!=-1)
			npgon.dihedral[c]=angle(faces.e[n]->planeid,faces.e[faces.e[n]->f[c]]->planeid,1);
		else npgon.dihedral[c]=-1;
	}
	npgon.id=faces.e[n]->planeid;
	return npgon;
}

//! Return the basis of face n.
/*! The basis has origin at f.vertices[0], x is toward f.vertices[1], and y is z (cross-product) x.
 * Scaling is same as for original space.
 *
 * If n is out of range, then return identity basis.
 */
basis Polyhedron::basisOfFace(int n)
{
	if (n<0 || n>=faces.n) return basis();
	plane pl=planeOfFace(n,0);
	return basis(pl.p, pl.p+pl.n, pl.p+vertices[faces.e[n]->p[1]]-vertices[faces.e[n]->p[0]]);
}

//! Return the length of the segment between intersection of planes fce,fr,p1 and fce,fr,p2.
/*! Please note that error checking is not done.
 */
double Polyhedron::segdistance(int fce,int fr,int p1,int p2)
{
	if (fce <0 || fr<0 || p1<0 || p2<0 || fce>=planes.n || fr>=planes.n ||
 		p1>=planes.n || p2>=planes.n) return 0;
	return ::distance(planeof(fce)*planeof(fr)*planeof(p1),
			   planeof(fce)*planeof(fr)*planeof(p2));
}

//! Return a copy of the plane with index pli.
plane Polyhedron::planeof(int pli)
{
	if (pli<0 || pli>=planes.n) return plane();
	return plane(planes[pli].p,planes[pli].z);
}

//! Return a plane that holds face with index fce. 
/*! Plain point is the center if centr!=0, else the point is the face's first point.
 */
plane Polyhedron::planeOfFace(int fce,char centr)
{
	if (fce<0 || fce>=faces.n) return plane();
	if (faces.e[fce]->pn==0 || faces.e[fce]->p[0]==-1) return plane();
	plane pl1;
	pl1=plane(vertices[faces.e[fce]->p[0]],vertices[faces.e[fce]->p[1]],vertices[faces.e[fce]->p[2]]);
	if (centr) pl1.p=CenterOfFace(fce,0);
	return pl1;
}

//! Return the angle between faces with index a and b. dec!=0 means make angle in degrees, not radians.
double Polyhedron::angle(int a, int b,int dec) /* uses planes, not faces */
{
	if (a<0 || a>=faces.n || b<0 || b>=faces.n) return 0;
	plane pl1,pl2;
	pl1=planeOfFace(a);
	pl2=planeOfFace(b);
	return(::angle(pl1.n,(-1)*pl2.n,dec));
}

//! Add a vertex to the polyhedron. Returns the index of the new point.
int Polyhedron::AddPoint(spacepoint p)
{
	return vertices.push(p);
}

//! Add a vertex to the polyhedron. Returns the index of the new point.
int Polyhedron::AddPoint(double x,double y,double z)
{
	return vertices.push(spacepoint(x,y,z));
}

//! Pass in something like "2 3 6" to make a face out of vertices with indices 2,3,6.
/*! \todo *** need to validate the new face?
 */
int Polyhedron::AddFace(const char *str)
{
	Face *f=new Face();
	IntListAttribute(str,&f->p,&f->pn);
	f->v=new int[f->pn];
	f->f=new int[f->pn];
	for (int c=0; c<f->pn; c++) { f->v[c]=f->f[c]=-1; }
	faces.push(f,1);
	return 0;
}

//---------------------------------- dump functions:

//! Save a polyhedron in indented data format.
/*! If this file stands alone, then it should be made to begin "#Polyp".
 *  This function does not write that out. If a file is created with "#Polyp", then
 *  it can be recognized by dumpInFile() as an indented polyhedron file.
 *
 * If what==-1, then output a pseudocode mockup of the file format.
 *
 * <pre>
 *  name Cube
 *  vertices \
 *    1 1 1
 *    -1 1 1
 *    -1 -1 1
 *    1 -1 1
 *    -1 1 -1
 *    1 1 -1
 *    1 -1 -1
 *   -1 -1 -1
 *  edge 0 1
 *  face 0 1 2 3
 *  face 1 0 5 4
 *  face 2 1 4 7
 *  face 0 3 6 5
 *  face 3 2 7 6
 *  face 4 5 6 7
 *  set SetName
 *    faces 0 1
 *    on
 *    color 255 0 0
 *  set Another Set
 *    faces 2 3
 *    off
 *  plane 
 *    p 0 0 0
 *    x 1 0 0
 *    y 0 1 0
 *    z 0 0 1
 * </pre>
 */
void Polyhedron::dump_out(FILE *ff,int indent,int what,Laxkit::anObject *context)
{
	DBG cerr << "\ndump_out Polyhedron... "<<endl;
	if (!ff) return;

	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(ff,"%sname \"Some name\"  #whatever name you give the polyhedron\n",spc);
		fprintf(ff,"%svertices \\  #a list of 3-d vertices of the polyhedron\n",spc);
		fprintf(ff,"%s   0 0 0    #the 0th vertex\n",spc);
		fprintf(ff,"%s   1 0 0    #the 1st vertex\n",spc);
		fprintf(ff,"%s   0 1 0    #etc\n",spc);
		fprintf(ff,"%s   0 0 1\n",spc);
		fprintf(ff,"%sedge 0 1    #an edge connected those vertices. optional, generated automatically\n",spc);
		fprintf(ff,"%sface 0 1 2  #a face, defined by connected vertices 0, 1, and 2\n",spc);
		fprintf(ff,"%sset \"Some set name\"  #extra information for grouping faces\n",spc);
		fprintf(ff,"%s  faces 0 1  #which faces are in the set (number is order they appear in file)\n",spc);
		fprintf(ff,"%s  on        #or off. whether faces in this set should be displayed or not\n",spc);
		fprintf(ff,"%s  color 255 0 0 #color of these faces\n",spc);
		fprintf(ff,"%splane       #optionally define particular planes that things happen on\n",spc);
		fprintf(ff,"%s            #planes might contain any number of faces\n",spc);
		fprintf(ff,"%s  p 0 0 0   #a 3-d point that is in the plane\n",spc);
		fprintf(ff,"%s  x 0 0 0   #the x direction in the plane\n",spc);
		fprintf(ff,"%s  y 0 0 0   #the y direction in the plane\n",spc);
		fprintf(ff,"%s  z 0 0 0   #the normal direction away from the plane\n",spc);
		return;
	}

	if (name) fprintf(ff,"%sname %s\n",spc,name);
	
	int c;
	if (vertices.n) {
		fprintf(ff,"%s #%d vertices\n%svertices \\\n",spc,vertices.n,spc);
		for (c=0; c<vertices.n; c++)
			fprintf(ff,"%s  %.15g %.15g %.15g  # %d\n",spc,vertices[c].x,vertices[c].y,vertices[c].z,c);
	}
	if (edges.n) { 	
		fprintf(ff,"%s #%d edges\n",spc,edges.n);
		for (c=0; c<edges.n; c++) {
			fprintf(ff,"%sedge %d %d #%d\n",spc,edges.e[c]->p1,edges.e[c]->p2,c);
		}
	}
	if (faces.n) { 
		fprintf(ff,"%s #%d faces\n",spc,faces.n);
		for (c=0; c<faces.n; c++) {
			fprintf(ff,"%sface ",spc);
			for (int c2=0; c2<faces.e[c]->pn; c2++) fprintf(ff,"%d ",faces.e[c]->p[c2]);
			fprintf(ff,"# %d\n",c);

			 //write out face link if necessary, ignored when reading in again
			if (faces.e[c]->f) {
				int c2;
				for (c2=0; c2<faces.e[c]->pn; c2++) {
					if (faces.e[c]->f[c2]!=-1) break;

				}
				if (c2!=faces.e[c]->pn) {
					fprintf(ff,"%s  facelink ",spc);
					for (int c2=0; c2<faces.e[c]->pn; c2++) fprintf(ff,"%d ",faces.e[c]->f[c2]);
					fprintf(ff,"\n");
				}
			}

			if (faces.e[c]->planeid>=0)     fprintf(ff,"%s  planeid %d\n",     spc,faces.e[c]->planeid);
			if (faces.e[c]->setid>=0)       fprintf(ff,"%s  setid %d\n",       spc,faces[c]->setid);
			if (faces.e[c]->facegroupid>=0) fprintf(ff,"%s  facegroupid %d\n", spc,faces[c]->facegroupid);
		}
	}
	if (sets.n) { 
		fprintf(ff,"%s #%d sets\n",spc,sets.n);
		for (c=0; c<sets.n; c++) {
			fprintf(ff,"%sset %s\n",spc,sets.e[c]->name);
			fprintf(ff,"%s  %s\n",spc,sets.e[c]->on?"on":"off");
			fprintf(ff,"%s  color %d %d %d %d\n",spc,
						(int)sets.e[c]->color.red,
						(int)sets.e[c]->color.green,
						(int)sets.e[c]->color.blue,
						(int)sets.e[c]->color.alpha);
			if (sets.e[c]->faces.n) {
				fprintf(ff,"%s  faces ",spc);
				for (int c2=0; c2<sets.e[c]->faces.n; c2++) fprintf(ff,"%d ",sets.e[c]->faces.e[c2]);
				fprintf(ff,"\n");
			}
		}
	}
	if (planes.n) { 
		fprintf(ff,"%s #%d planes\n",spc,planes.n);
		for (c=0; c<planes.n; c++) {
			fprintf(ff,"%splane #%d\n",spc,c);
			fprintf(ff,"%s  p %.15g %.15g %.15g\n",spc,planes[c].p.x,planes[c].p.y,planes[c].p.z);
			fprintf(ff,"%s  x %.15g %.15g %.15g\n",spc,planes[c].x.x,planes[c].x.y,planes[c].x.z);
			fprintf(ff,"%s  y %.15g %.15g %.15g\n",spc,planes[c].y.x,planes[c].y.y,planes[c].y.z);
			fprintf(ff,"%s  z %.15g %.15g %.15g\n",spc,planes[c].z.x,planes[c].z.y,planes[c].z.z);
		}
	}
	
	DBG cerr <<"...done saving\n";
}

//! Read in a polyhedron from an Attribute.
void Polyhedron::dump_in_atts(Attribute *att,int what,Laxkit::anObject *context)
{
	clear();

	//Sets:
	//char *name;
	//unsigned char on;
	//ScreenColor color;
	//int ne,*e;
	
	char *nme,*value, *ee,*tt;
	int error=0;
	int n;
	for (int c=0; c<att->attributes.n; c++) {
		nme=  att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(nme,"name")) {
			makestr(name,value);

		} else if (!strcmp(nme,"vertices")) {
			 // parse arbitrarily long list of 3-d points
			 //   1.2 1.5 -1.8 \n ...
			int n;
			double p3[3];
			tt=ee=value;
			while (tt && *tt) {
				n=DoubleListAttribute(tt,p3,3,&ee);
				if (ee==tt) break;
				if (n!=3) { tt=ee; continue; }
				while (*ee && *ee!='\n') ee++;
				tt=*ee?ee+1:NULL;
				vertices.push(spacepoint(p3[0],p3[1],p3[2]));
			}

		} else if (!strcmp(nme,"edge")) {
			int i[2];
			n=IntListAttribute(value,i,2);
			if (n==2) edges.push(new Edge(i[0],i[1]));

		} else if (!strcmp(nme,"face")) {
			Face *newface=new Face();
			IntListAttribute(value,&newface->p,&newface->pn);
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				nme=  att->attributes.e[c2]->name;
				value=att->attributes.e[c2]->value;
				if (!strcmp(nme,"planeid")) {
					IntAttribute(value,&newface->planeid);
				} else if (!strcmp(nme,"facegroupid")) {
					IntAttribute(value,&newface->facegroupid);
				} else if (!strcmp(nme,"setid")) {
					IntAttribute(value,&newface->setid);
				} else if (!strcmp(nme,"facelink")) {
					IntListAttribute(value,&newface->f,&n);
					//if (n != newface->pn) ***is error!!
				}
			}
			if (!newface->f && newface->p) {
				newface->f=new int[newface->pn];
				for (int c2=0; c2<newface->pn; c2++) newface->f[c2]=-1;
			}
			if (!newface->v && newface->p) {
				newface->v=new int[newface->pn];
				for (int c2=0; c2<newface->pn; c2++) newface->v[c2]=-1;
			}
			faces.push(newface);

		} else if (!strcmp(nme,"set")) {
			Settype *set=new Settype;
			makestr(set->name,value);
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				nme=  att->attributes.e[c]->name;
				value=att->attributes.e[c]->value;
				if (!strcmp(nme,"on")) {
					set->on=1;
				} else if (!strcmp(nme,"off")) {
					set->on=0;
				} else if (!strcmp(nme,"faces")) {
					int *ii,nn;
					IntListAttribute(value,&ii,&nn);
					set->faces.insertArray(ii,nn);
				} else if (!strcmp(nme,"color")) {
					int i[4];
					n=IntListAttribute(value,i,4);
					if (n<4) i[3]=255;
					if (n<3) i[2]=0;
					if (n<2) i[1]=0;
					if (n<1) i[0]=255;
					set->color.rgb(i[0],i[1],i[2],i[3]);
				}
			}
			sets.push(set);

		} else if (!strcmp(nme,"plane")) {
			basis bas;
			double p3[3];
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				nme=  att->attributes.e[c]->name;
				value=att->attributes.e[c]->value;
				if (!strcmp(nme,"p")) {
					n=DoubleListAttribute(tt,p3,3,&ee);
					if (n==3) bas.p=spacevector(p3);
				} else if (!strcmp(nme,"x")) {
					n=DoubleListAttribute(tt,p3,3,&ee);
					if (n==3) bas.x=spacevector(p3);
				} else if (!strcmp(nme,"y")) {
					n=DoubleListAttribute(tt,p3,3,&ee);
					if (n==3) bas.y=spacevector(p3);
				} else if (!strcmp(nme,"z")) {
					n=DoubleListAttribute(tt,p3,3,&ee);
					if (n==3) bas.z=spacevector(p3);
				}
				planes.push(bas);
			}
		}
	}

	if (error) { 
		DBG cerr << "ERROR";
		clear();
		return; 
	}
	
	makeedges();
	applysets();
	DBG cerr << "...Done Polyhedron::dump_in_atts() "<<endl;
	
	if (!validate()) { 
		clear(); 
		DBG cerr << "Validation failed\n";
		return; 
	}
	DBG cerr << "Validation succeeded\n"; 
}

//! Saves a VRML 2.0 model of the polyhedron to filename.
/*! Outputs a wireframe model, using only data in edges.
 *
 * Return 0 for success, or nonzero for error.
 */
int Polyhedron::dumpOutVrml(FILE *ff,char **error_ret) // does edges
{
	if (edges.n==0) {
		if (error_ret) appendline(*error_ret,"Need edges to output to vrml");
		return 1;
	}
	DBG cerr << "\nSaving vrml:" << filename <<'.';

	fprintf(ff,"#VRML V2.0 utf8\n\n");

	/* with points a,b */
	spacevector a,b,f,d;
	int rot=1,t1,t2;
	double height;
	for (int c=0; c<edges.n; c++) {
		t1=edges.e[c]->p1;
		t2=edges.e[c]->p2;
		a=vertices[t1];
		b=vertices[t2];
		f=(a+b)/2;
		d=b-a;
		d=d/norm(d);
		d.y+=1;
		height=norm(vertices[t1]-vertices[t2]);
		if (isnotvector(d)) rot=0;

		fprintf(ff, "# %f,%f,%f to %f,%f,%f\n",
					a.x,a.y,a.z, b.x,b.y,b.z);
		fprintf(ff, "Transform {\n");
		fprintf(ff, "   translation %f %f %f\n",
					f.x,f.y,f.z);
		if (rot) fprintf(ff, "   rotation %f %f %f 3.1415926535897\n",
						d.x,d.y,d.z);
		   //else rot=1;
		fprintf(ff, "   children\n");
		fprintf(ff, "	Shape {\n");
		fprintf(ff, "	   appearance Appearance {\n");
		fprintf(ff, "		material Material { diffuseColor .8 .8 1 }\n");
		fprintf(ff, "	   }\n");
		fprintf(ff, "	   geometry Cylinder { height %f radius .1 }\n",height);
		fprintf(ff, "	}\n");
		fprintf(ff, "}\n\n");   
	}

	DBG cerr << "..done vrmlsave\n";
	return 0;
}

//! Output an OFF file. Return 0 for things output, else nonzero.
int Polyhedron::dumpOutOFF(FILE *f,char **error_ret)
{
	if (!faces.n || !vertices.n) return 1;

	fprintf(f,"OFF\n");
	if (name) fprintf(f,"# %s\n",name);
	fprintf(f,"%d %d 0\n",vertices.n,faces.n);

	for (int c=0; c<vertices.n; c++) {
		fprintf(f,"%.15g %.15g %.15g\n",vertices.e[c].x,vertices.e[c].y,vertices.e[c].z);
	}
	for (int c=0; c<faces.n; c++) {
		fprintf(f,"%d ",faces.e[c]->pn);
		for (int c2=0; c2<faces.e[c]->pn; c2++) {
			fprintf(f,"%d ",faces.e[c]->p[c2]);
		}
		fprintf(f,"\n");
	}
	return 0;
}

//! Output an Obj file. Return 0 for things output, else nonzero.
/*! See dumpInObj for format
 */
int Polyhedron::dumpOutObj(FILE *f,char **error_ret)
{
	if (!faces.n || !vertices.n) return 1;

	fprintf(f,"# Polyptych outputting Obj\n");
	if (name) fprintf(f,"o %s\n",name);

	for (int c=0; c<vertices.n; c++) {
		fprintf(f,"v %.15g %.15g %.15g\n",vertices.e[c].x,vertices.e[c].y,vertices.e[c].z);
	}
	for (int c=0; c<faces.n; c++) {
		fprintf(f,"f ");
		for (int c2=0; c2<faces.e[c]->pn; c2++) {
			fprintf(f,"%d ",faces.e[c]->p[c2]+1);
		}
		fprintf(f,"\n");
	}
	return 0;
}

/*! Return 0 for success, nonzero for error.
 *
 * The OFF file format is originally from geomview.org and is roughly thus:
 * <pre>
 *    [ST][C][N][4][n]OFF    # Header keyword
 *    [Ndim]        # Space dimension of vertices, present only if nOFF
 *    NVertices  NFaces  NEdges   # NEdges not used or checked
 *    
 *    x[0]  y[0]  z[0]    # Vertices, possibly with normals,
 *                        # colors, and/or texture coordinates, in that order,
 *                        # if the prefixes N, C, ST
 *                        # are present.
 *                        # If 4OFF, each vertex has 4 components,
 *                        # including a final homogeneous component.
 *                        # If nOFF, each vertex has Ndim components.
 *                        # If 4nOFF, each vertex has Ndim+1 components.
 *    ...
 *    x[NVertices-1]  y[NVertices-1]  z[NVertices-1]
 *    
 *                    # Faces
 *                    # Nv = # vertices on this face
 *                    # v[0] ... v[Nv-1]: vertex indices
 *                    #        in range 0..NVertices-1
 *    Nv  v[0] v[1] ... v[Nv-1]  colorspec
 *    ...
 *                    # colorspec continues past v[Nv-1]
 *                    # to end-of-line; may be 0 to 4 numbers
 *                    # nothing: default
 *                    # integer: colormap index
 *                    # 3 or 4 integers: RGB[A] values 0..255
 *                    # 3 or 4 floats: RGB[A] values 0..1
 * </pre>
 */
int Polyhedron::dumpInOFF(FILE *f,char **error_ret)
{
	if (!f) return 1;

	char *line=NULL;
	size_t n=0,c;
	int p;
	char multidim=0;
	int nv,nf,ne;


	 //parse header line
	c=getline_indent_nonblank(&line,&n,f,0,"#"); //skips blank and comment lines
	p=0;
	do {
		if (line[p]=='S' && line[p+1]=='T') { 
			 //vertices have texture coords
			p+=2;
		} else if (line[p]=='C') {
			 //vertices have colors
			p++;
		} else if (line[p]=='N') {
			 //vertices have normals
			p++;
		} else if (line[p]=='4') {
			 //vertices have 4 components. final is homogeneous component
			p++;
		} else if (line[p]=='n') {
			 //vertices are higher than 3 dimensional
			multidim=1;
			p++;
		} else break;
	} while (1);

	if (multidim) {
		c=getline_indent_nonblank(&line,&n,f,0,"#"); //skip higher dimensional stuff
	}

	 //get number of vertices, faces, and edges
	c=getline_indent_nonblank(&line,&n,f,0,"#");
	if (c<=0) return 1;
	c=sscanf(line,"%d %d %d",&nv,&nf,&ne);
	if (c!=3) return 2;

	 //read in the vertices
	double x,y,z;
	for (int c2=0; c2<nv; c2++) {
		c=getline_indent_nonblank(&line,&n,f,0,"#");
		if (c<=0) return 3;

		c=sscanf(line,"%lf %lf %lf",&x,&y,&z);
		if (c<=0) return 4;

		vertices.push(spacepoint(x,y,z));
	}

	 //read in the faces
	int nfv,*i;
	char *endptr;
	Face *nface=NULL;
	for (int c2=0; c2<nf; c2++) {
		c=getline_indent_nonblank(&line,&n,f,0,"#");
		if (c<=0) return 5;

		nfv=strtol(line,&endptr,10);
		if (nfv<=0) return 6;

		i=new int[nfv];
		c=IntListAttribute(endptr,i,nfv,&endptr);
		if ((int)c!=nfv) { delete[] i; return 7; }

		nface=new Face(nfv,i);
		delete[] i;
		faces.push(nface,1);
	}

	validate();
	makeedges();
	return 0;
}

//! Read in an Obj file.
/*! An obj file has lists of vertices, texture coordinates, and normals.
 * Faces reference those vertex and normal lists.
 *
 * The file format has many features, but the things Polyhedron cares about are:
 * <pre>
 *   # This is a comment, a line with pound symbol at the beginning.
 *   # List of Vertices, with (x,y,z) coordinates.
 *   # 1st is vertex 1, not 0...
 *   v 0.123 0.234 0.345
 *   v ...
 *   ...
 *  
 *   # Face Definitions, 1st line is simple vertices.
 *   # second is vertex/texture points, next is vertex/texture/normals.
 *   # We ignore all but the vertex points.
 *   f 1 2 3
 *   f 3/1 4/2 5/3
 *   f 6/4/1 3/5/3 7/6/5
 *   f ...
 * </pre>
 * 
 * More on the format here: http://en.wikipedia.org/wiki/Wavefront_.obj_file
 *
 * Texture vertices start with "vt", normals "vn", curving surface points "vp". We only care about 
 * simple "v" points, and the "f" face definitions.
 *
 * Return 0 for success, nonzero for error.
 */
int Polyhedron::dumpInObj(FILE *f,char **error_ret)
{
	char *line=NULL, *ptr,*e;
	size_t n=0;
	int c;
	int firstvertex=vertices.n;
	int firstface=faces.n;
	double d[3];
	int error=0;


	while (!feof(f)) {
		c=getline(&line, &n, f);
		if (!c) continue;

		ptr=line;
		while (*ptr && isspace(*ptr)) ptr++;

		if (*ptr=='#') continue; //skip comments

		if (*ptr=='v' && ptr[1]==' ') {
			 //found vertex
			ptr++;
			e=NULL;
			c=DoubleListAttribute(ptr,d,3, &e);
			if (c!=3) { error=1; break; } // *** Error! Broken vertex definition

			vertices.push(spacepoint(d));

		} else if (*ptr=='o' && ptr[1]==' ') {
			makestr(name,ptr);

		} else if (*ptr=='f' && ptr[1]==' ') {
			 //found face
			ptr++;
			while (*ptr && isspace(*ptr)) ptr++;
			e=ptr;

			do {
				if (!isdigit(*e)) { error=3; break; }
				while (isdigit(*e)) e++; //skip over first number
				if (*e=='/') {
					*e=' '; e++;
					while (isdigit(*e)) { *e=' '; e++; }//blank out parts we don't need
					if (*e=='/') {
						*e=' '; e++;
						while (isdigit(*e)) { *e=' '; e++; }//blank out parts we don't need
					}
				}

				while (*e && isspace(*e)) e++;
			} while (*e);
			if (error) break;

			Face *face=new Face(ptr,NULL); //face disects as a point list
			if (face->pn<3) { error=2; delete face; break; }

			 //remap to actual vertex list
			for (int c=0; c<face->pn; c++) face->p[c]+=firstvertex-1;

			faces.push(face);
		}
	}
	if (line) free(line);

	if (error) {
		 // delete all added faces and vertices
		while (vertices.n!=firstvertex) vertices.pop();
		while (faces.n!=firstface) faces.remove();
	}

	return error;
}

int Polyhedron::dumpOutFile(const char *outfile, const char *outformat,char **error_ret)
{
	if (isblank(outfile)) return 1;

	int status=0;
	FILE *f=fopen(outfile,"w");
	if (!strcasecmp(outformat,"idat")) {
		fprintf(f,"#Polyp\n");
		status=0;
		dump_out(f,2,0,NULL);
	} else if (!strcasecmp(outformat,"off")) status=dumpOutOFF(f,error_ret);
	else if (!strcasecmp(outformat,"obj"))   status=dumpOutObj(f,error_ret);
	else if (!strcasecmp(outformat,"vrml"))  status=dumpOutVrml(f,error_ret);
	else status=2;
	fclose(f);

	return status;
}

//! Return a space delimited list of formats than can be read in with dumpInFile().
const char *Polyhedron::InFileFormats()
{ return "idat off obj"; }

//! Return a space delimited list of formats than can be output in with dumpOutFile().
/*! Each can be used as the file format parameter in dumpOutFile().
 */
const char *Polyhedron::OutFileFormats()
{ return "idat off obj vrml"; }

//! Try to read in the file.
/*! The file can be an OFF file (see dumpInOFF()), or obj (dumpInObj(), or so-called Polyp file, which is the
 * native indented file format for this class.
 *
 * Return 0 for success, nonzero for error.
 */
int Polyhedron::dumpInFile(const char *file, char **error_ret)
{
	FILE *f=fopen(file,"r");
	if (!f) return 1;

	int filefound=0;
	char first1000[1001];
	int c=fread(first1000,1,1000,f);
	first1000[c]='\0';
	rewind(f);
	c=-1;

	 //first check for default format
	if (!strncmp("#Polyp",first1000,6) && isspace(first1000[6])) {
		Attribute att;
		att.dump_in(f,0,NULL);
		dump_in_atts(&att,0,NULL);
		filefound=1;
		c=0;
	}

	 //check if is OFF file
	if (!filefound) {
		 //check for the various OFF starts
		int p=0,foundoff=0;
		while (p<100 && isalpha(first1000[p])) p++;
		if (p) p--;
		if (p>=2 && p<10) {
			if (first1000[p-2]=='O' && first1000[p-1]=='F' && first1000[p]=='F') {
				//***this could be more thorough...
				foundoff=1;
			}
		}

		if (foundoff) {
			clear();
			c=dumpInOFF(f,error_ret);
			filefound=2;
		}
	}

	 //check if is Obj file
	if (!filefound) {
		if (strstr(first1000,"\nv ") || strstr(first1000,"\r\nv ")
		 	   || strstr(first1000,"\nf ") || strstr(first1000,"\r\nf ")) {
			clear();
			c=dumpInObj(f,error_ret);
			filefound=3;
		}
	}

	fclose(f);
	if (filefound) makestr(filename,file);
	return c | !filefound;
}

//--------------AbstractNet methods:

//! Just return filename.
const char *Polyhedron::Filename()
{
	return filename;
}

//! Currently, just pass along to dump_out().
int Polyhedron::dumpOutNet(FILE *f,int indent,int what)
{
	dump_out(f,indent,what,NULL);
	return 0;
}

int Polyhedron::NumFaces() { return faces.n; }

//! Returns a new NetFace object for face i.
/*! \todo if face normals are not defined properly, this may not work as expected.
 */
NetFace *Polyhedron::GetFace(int i,double scaling)
{
	if (i<0 || i>=faces.n) return NULL;

	NetFaceEdge *e;
	NetFace *f=new NetFace();
	f->original=i;

	Pgon pgon=FaceToPgon(i,0); //generate flattened face
	for (int c=0; c<faces.e[i]->pn; c++) {
		e=new NetFaceEdge();
		e->id=c;
		e->tooriginal=faces.e[i]->f[c]; //edge connects to which face
		e->toface=-1;

		 //find edge of the face this edge connects to
		if (e->tooriginal>=0) {
			for (int c2=0; c2<faces.e[e->tooriginal]->pn; c2++) {
				if (faces.e[e->tooriginal]->f[c2]==i) {
					e->tofaceedge=c2;
					break;
				}
			}
		}
		e->points=new LaxInterfaces::Coordinate(pgon.p[c].x*scaling, pgon.p[c].y*scaling);
		f->edges.push(e,1);
	}
	return f;
}


} //namespace Polyptych

