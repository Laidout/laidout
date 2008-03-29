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


//------------------------------ Edge -------------------------------------

/*! \class Edge
 * \brief Holds two integers that are indices into a vertex list.
 *
 * Also holds places for 2 faces meeting at that edge.
 */


//------------------------------ Face -------------------------------------

/*! \class Face
 * \brief Holds point list (indices to v array), and number labels for the edges.
 */
/*! \var int *Face::f
 * \brief Face labels, on this edge, connects to another face at index of Polyhedron::f.
 */
/*! \var int *Face::v
 * \brief Vertex labels, index of point in Polyhedron::p???. ***diff from p how?
 */
/*! \var int *Face::p
 * \brief Vertex indices, index of point in Polyhedron::p.
 */


//! Create empty face.
Face::Face()
{
	planeid=setid=-1; pn=0; v=NULL; f=NULL; p=NULL;
}

//! Create a Face with the ps array as the point indices.
/*! ps is copied.
 */
Face::Face(int numof,int *ps) 
{
	p=NULL; f=NULL; v=NULL;
	planeid=setid=-1;
	if (numof<3) { pn=0; return; }
	pn=numof;
	v=new int[pn];
	f=new int[pn];
	p=new int[pn];
	for (int c=0; c<pn; c++) { p[c]=ps[c]; p[c]=-1; p[c]=-1; }
}

//! Shortcut to create polygons with up to 5 vertices.
/*! Must have at least 3. After that, stops at the first -1.
 * 
 * \todo *** could have this with the ... variadic func thing
 */
Face::Face(int p1,int p2,int p3, int p4, int p5) /* p4=-1, p5=-1 */
{
	planeid=setid=-1;
	if (p4==-1) pn=3;
	else if (p5==-1) pn=4;
	else pn=5;

	p=new int[pn];
	f=new int[pn];
	v=new int[pn];
	p[0]=p1; p[1]=p2; p[2]=p3;
	if (pn>3) p[3]=p4;
	if (pn>4) p[4]=p5;
	for (int c=0; c<pn; c++) { f[c]=-1; v[c]=-1; }
}

//! Create a new face from a string like "3 4 5" or "3,5,6". Delimiter is ignored.
/*! \todo is delimiter ignored???
 */
Face::Face(const char *pointlist,const char *linklist)
{
	planeid=setid=-1; pn=0; v=NULL; f=NULL; p=NULL;

	IntListAttribute(pointlist,&p,&pn,NULL);
	if (!pn) return;

	if (!f) f=new int[pn];
	v=new int[pn];
	for (int c=0; c<pn; c++) { f[c]=-1; v[c]=-1; }

	int n;
	if (linklist) IntListAttribute(linklist,&f,&n,NULL);
	//if (pn!=n) there is a problem!!
}

//! Create copy of fce.
Face::Face(const Face &fce)
{
	v=NULL;
	f=NULL;
	p=NULL;
	*this=fce;
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
	color=0; pn=0; id=-1;
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
	color=0;
	id=-1;
	pn=f.pn;
	p=new flatpoint[pn];
	dihedral=NULL;
	elabel=new int[pn];
	flabel=new int[pn];
	vlabel=new int[pn];
	for (int c=0; c<pn; c++) {
		flabel[c]=f.f[c];
		elabel[c]=f.v[c]; //*** ?
		vlabel[c]=f.p[c];
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
	color=0;
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
	color=0; pn=0; id=-1;
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
	delete[] e;
	name=new char[strlen(nset.name)+1];
	strcpy(name,nset.name);
	ne=nset.ne;
	on=nset.on;
	color=nset.color;
	if (ne) {
		e=new int[ne];
		for (int c=0; c<ne; c++) e[c]=nset.e[c];
	} else e=NULL;
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
	name=NULL;
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

//! Equal operator for Polyhedron.
Polyhedron &Polyhedron::operator=(const Polyhedron &nphed)
{
	clear();
	name=new char[strlen(nphed.name)+1];
	strcpy(name,nphed.name);
	int c,n;
	if (nphed.vertices.n) {
		spacepoint *t;
		t=new spacepoint[n];
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
	delete[] name;  name=NULL;
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
	DBG cout << "Validating..";
	for (c=0; c<edges.n; c++)
		if (    edges.e[c]->p1<0 || edges.e[c]->p1>=vertices.n ||
				edges.e[c]->p2<0 || edges.e[c]->p2>=vertices.n) {
			DBG cout << "\nBad edge "<<c<<"."; 
			return 0; 
		}
	for (c=0; c<faces.n; c++) {
		if (faces.e[c]->pn==0) {
			DBG cout << "\nEmpty face "<<c<<"."; 
			return 0; 
		}
		for (c2=0; c2<faces.e[c]->pn; c2++)
			if (faces.e[c]->p[c2]<0 || faces.e[c]->p[c2]>=vertices.n)	{
				DBG cout << "\nBad face "<<c<<' '<<c2<<".\n";
				return 0; 
			}
	}
	DBG cout <<"..done validating\n";
	return 1;
}

//! Automatically connect faces in Face::f.
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
  		  	if (faces.e[c3]->p[c4]==a1 && faces.e[c3]->p[(c4+1)%faces.e[c]->pn]==a2
				 || faces.e[c3]->p[c4]==a2 && faces.e[c3]->p[(c4+1)%faces.e[c]->pn]==a1) {
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

//! From the sets list, tag the faces with the proper set id.
/*! \todo if a face has some previous set id, and the face is no longer
 *    in any set, then the previous set id is kept, and it shouldn't be..
 */
void Polyhedron::applysets()
{
	if (sets.n==0) return;
	int a,c2;
	for (int c=0; c<sets.n; c++) 
		for (c2=0; c2<sets.e[c]->ne; c2++) {
			if (a=sets.e[c]->e[c2],a>=faces.n) continue;
			faces.e[a]->setid=c;
		}		
}

////! Return lists of color and whether the face is on.
//void Polyhedron::faceson(unsigned long *&colors,BYTE *&pon)
//{***
//	pon=new BYTE[npl];
//	colors=new unsigned long[npl];
//	int a,b,c;
//	for (c=0; c<npl; c++) { pon[c]=1; colors[c]=255<<16; }
//	if (sn==0) return;
//	for (c=0; c<fn; c++) {
//		if (a=f[c].setid,a<0 || a>=sn) continue;
//		if (b=f[c].planeid,b<0 || b>=npl) continue;
//		pon[b]=sets[a].on;
//		colors[b]=sets[a].color;
//	}
//}

//! From the faces list, construct the planes list.
int Polyhedron::makeplanes()
{
	if (planes.n>0) return 0;
	if (faces.n==0) return 0;
	plane pl;
	int c2;
	for (int c=0; c<faces.n; c++) {
		pl=planeofface(c);
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
 */
int Polyhedron::makeedges()
{
	DBG cout <<"making edges..."<<endl;
	if (faces.n==0 || vertices.n==0) return 0;
	
	int c,c2,c3,emax=0,p1,p2;
	if (edges.n) edges.flush();

	for (c=0; c<faces.n; c++) emax+=faces.e[c]->pn;
	if (emax<=0) return 0;
	
	DBG cout <<"makeedges emax:"<<emax;
	
	for (c=0; c<faces.n; c++) {
		for (c2=0; c2<faces.e[c]->pn; c2++) {
			p1=faces.e[c]->p[c2];
			p2=faces.e[c]->p[(c2+1)%faces.e[c]->pn];
			for (c3=0; c3<edges.n; c3++) { //check edge against list
				if ((edges.e[c3]->p1==p1 && edges.e[c3]->p2==p2) ||
					(edges.e[c3]->p1==p2 && edges.e[c3]->p2==p1)) break;
			}
			if (c3==edges.n) { // edge not found
				edges.push(new Edge(p1,p2,c,-1));
				faces.e[c]->f[c2]=-1;
			} else {
				edges.e[c3]->f2=c;
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
	DBG cout <<"...total edges="<<edges.n<<'\n';
	DBG for (c=0; c<faces.n; c++) {
	DBG 	cout <<"face "<<c<<": ";
	DBG 	for (c2=0; c2<faces.e[c]->pn; c2++) cout <<faces.e[c]->f[c2]<<" ";
	DBG 	cout <<endl;
	DBG }
	return 1;
}

//! Return the average of the points in face with index fce.
spacepoint Polyhedron::center(int fce)
{
	spacepoint pnt;
	for (int c=0; c<faces.e[fce]->pn; c++) pnt=pnt+vertices[faces.e[fce]->p[c]];
	return(pnt/faces.e[fce]->pn);
}

//! Return the vertex that is pt in list held by face with index fce (vertices[f[fce].vertices[pt]).
spacevector Polyhedron::vofface(int fce, int pt)
{
	if (fce<0 || fce>=faces.n) return spacevector(0,0,0);
	if (pt<0 || pt>=faces.e[fce]->pn) return spacevector();
	if (faces.e[fce]->p[pt]>=0) return vertices[faces.e[fce]->p[pt]];
	return spacevector();
	
//	 // vertex index is not listed in face, so compute..***??? whats this??
//	int p2=f[fce].vertices[pt], p3=f[fce].vertices[pt-1<0 ? f[fce].pn-1 : pt-1];
//	
//	if (p2==-1 || p3==-1) return spacevector();
//	return spacevector(plane(planes[fce].vertices,planes[fce].z)*
//			  plane(planes[p2].vertices, planes[p2].z)*
//			  plane(planes[p3].vertices, planes[p3].z));
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
	if (faces.e[n]->setid==-1) npgon.color=((255)*256+255)*256+255;
	   else npgon.color=sets.e[faces.e[n]->setid]->color;
	npgon.dihedral=new double[npgon.pn];
	basis bas;
	if (useplanes && faces.e[n]->planeid>=0) bas=planes.e[faces.e[n]->planeid];
	else {
		if (faces.e[n]->p[0]==-1) { npgon.clear(); return npgon; }
		plane pl=planeofface(n,0);
		bas=basis(pl.p, pl.p+pl.n, pl.p+vertices.e[faces.e[n]->p[1]]-vertices[faces.e[n]->p[0]]);
	}
	for (int c=0; c<npgon.pn; c++) {	
		npgon.p[c]=flatten(vofface(n,c),bas);
		if (faces.e[n]->p[c]!=-1)
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
basis Polyhedron::BasisOfFace(int n)
{
	if (n<0 || n>=faces.n) return basis();
	plane pl=planeofface(n,0);
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
plane Polyhedron::planeofface(int fce,char centr)
{
	if (fce<0 || fce>=faces.n) return plane();
	if (faces.e[fce]->pn==0 || faces.e[fce]->p[0]==-1) return plane();
	plane pl1;
	pl1=plane(vertices[faces.e[fce]->p[0]],vertices[faces.e[fce]->p[1]],vertices[faces.e[fce]->p[2]]);
	if (centr) pl1.p=center(fce);
	return(pl1);
}

//! Return the angle between faces with index a and b. dec!=0 means make angle in degrees, not radians.
double Polyhedron::angle(int a, int b,int dec) /* uses planes, not faces */
{
	if (a<0 || a>=faces.n || b<0 || b>=faces.n) return 0;
	plane pl1,pl2;
	pl1=planeofface(a);
	pl2=planeofface(b);
	return(::angle(pl1.n,(-1)*pl2.n,dec));
}


//---------------------------------- dump functions:

//! Save a polyhedron in indented data format.
/*! If this file stands alone, then it should be made to begin "#Polyp".
 *  This function does not write that out. If a file is created with "#Polyp", then
 *  it can be recognized by dumpInFile() as an indented polyhedron file.
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
 *    vertices 0 0 0
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

			if (faces.e[c]->planeid>=0) fprintf(ff,"%s  planeid %d\n",spc,faces.e[c]->planeid);
			if (faces.e[c]->setid>=0)   fprintf(ff,"%s  setid %d\n",  spc,faces[c]->setid);
		}
	}
	if (sets.n) { 
		fprintf(ff,"%s #%d sets\n",spc,sets.n);
		for (c=0; c<sets.n; c++) {
			fprintf(ff,"%sset %s\n",spc,sets.e[c]->name);
			fprintf(ff,"%s  %s\n",spc,sets.e[c]->on?"on":"off");
			fprintf(ff,"%s  color %d %d %d %d\n",spc,
						(int)sets.e[c]->color&0xff,
						int((sets.e[c]->color&0xff00)>>8),
						int((sets.e[c]->color&0xff0000)>>16),
						int((sets.e[c]->color&0xff000000)>>12));
			if (sets.e[c]->ne) {
				fprintf(ff,"%s  faces ",spc);
				for (int c2=0; c2<sets.e[c]->ne; c2++) fprintf(ff,"%d ",sets.e[c]->e[c2]);
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
	//unsigned long color;
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
					IntListAttribute(value,&set->e,&set->ne);
				} else if (!strcmp(nme,"color")) {
					int i[4];
					n=IntListAttribute(value,i,4);
					if (n<4) i[3]=255;
					if (n<3) i[2]=0;
					if (n<2) i[1]=0;
					if (n<1) i[0]=255;
					set->color=i[3]<<24 | i[0]<<16 | i[1]<<8 | i[2];
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
		DBG cout << "ERROR";
		clear();
		return; 
	}
	
	makeedges();
	applysets();
	DBG cout << "...Done Polyhedron::dump_in_atts() "<<endl;
	
	if (!validate()) { 
		clear(); 
		DBG cout << "Validation failed\n";
		return; 
	}
	DBG cout << "Validation succeeded\n"; 
}

//! Saves a VRML 2.0 model of the polyhedron to filename.
/*! Outputs a wireframe model.
 */
int Polyhedron::dumpOutVrml(const char *filename) // does edges
{
	if (edges.n==0) return 0;
	if (!filename) return 0;
	DBG cout << "\nSaving vrml:" << filename <<'.';

	FILE *ff=fopen(filename,"r");
	if (!ff) {
		DBG cout << "Bad filename.";
		return 1;
	}
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

	fclose(ff);
	
	DBG cout << "..done vrmlsave\n";
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

		nfv=strtod(line,&endptr);
		if (nfv<=0) return 6;

		i=new int[nfv];
		c=IntListAttribute(endptr,i,nfv,&endptr);
		if ((int)c!=nfv) { delete[] i; return 7; }

		nface=new Face(nfv,i);
		delete[] i;
		faces.push(nface,1);
	}

	validate();
	return 0;
}


//! Try to read in the file.
/*! The file can be an OFF file (see dumpInOFF()), or a so-called Polyp file, which is the
 * native indented file format for this class.
 */
int Polyhedron::dumpInFile(const char *file, char **error_ret)
{
	FILE *f=fopen(file,"r");
	if (!f) return 1;

	char first100[101];
	int c=fread(first100,1,100,f);
	first100[c]='\0';
	rewind(f);
	c=-1;

	 //check for the various OFF starts
	int p=0,foundoff=0;
	while (p<100 && isalpha(first100[p])) p++;
	if (p) p--;
	if (p>=2 && p<10) {
		if (first100[p-2]=='O' && first100[p-1]=='F' && first100[p]=='F') {
			//***this could be more thorough...
			foundoff=1;
		}
	}
	if (foundoff) {
		clear();
		c=dumpInOFF(f,error_ret);
	} else if (!strncmp("#Polyp",first100,6) && isspace(first100[6])) {
		Attribute att;
		att.dump_in(f,NULL);
		dump_in_atts(&att,0,NULL);
		c=0;
	} else c=1; 

	fclose(f);
	return c;
}

//--------------AbstractNet methods:

int Polyhedron::dumpOutNet(FILE *f,int indent,int what)
{
	dump_out(f,indent,what,NULL);
	return 0;
}

int Polyhedron::NumFaces() { return faces.n; }

/*! \todo if face normals are not defined properly, this may not work as expected.
 */
NetFace *Polyhedron::GetFace(int i)
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
		e->points=new LaxInterfaces::Coordinate(pgon.p[c].x,pgon.p[c].y);
		f->edges.push(e,1);
	}
	return f;
}


