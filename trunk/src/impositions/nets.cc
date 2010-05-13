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


#include "nets.h"
#include <lax/interfaces/svgcoord.h>
#include <lax/bezutils.h>
#include <lax/strmanip.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>

#include <lax/lists.cc>

#include "../language.h"

using namespace LaxFiles;
using namespace Laxkit;
using namespace LaxInterfaces;

#include <fstream>

#include <iostream>
using namespace std;
#define DBG 


const char *tagname[5]={
	"FACE_Undefined",
	"FACE_None",
	"FACE_Actual",
	"FACE_Potential",
	"FACE_Taken" };

//--------------------------------------- NetLine -------------------------------------------
/*! \class NetLine
 * \brief Class to hold lines compiled from the faces of a Net.
 */
/*! \var char NetLine::lineinfo
 * \brief Hint about where the line came from.
 *
 * 0 for line is automatically generated from the Net.
 * Other values mean the line is extra, independent from the actual net.
 */
/*! \var int NetLine::tag
 * \brief What type of line this is.
 *
 * This is a shorthand hint that can be used in place of linestyle.
 * 0 is taken to mean the line is an outline.
 * 1 is for inner lines, such as where faces meet.
 * 2 is for tab lines.
 */


NetLine::NetLine(const char *list)
{
	lineinfo=0;
	tag=0;
	linestyle=NULL;
	points=NULL;
	if (list) Set(list,NULL);
}

NetLine::~NetLine()
{
	if (points) delete points;
	if (linestyle) linestyle->dec_count();
}

/*! Copies over all. Warning: does a linestyle=line.linestyle.
 */
const NetLine &NetLine::operator=(const NetLine &l)
{
	if (linestyle && l.linestyle) *linestyle=*l.linestyle;//***this is maybe problematic
	tag=l.tag;
	lineinfo=l.lineinfo;
	
	 //copy points
	if (points) { delete points; points=NULL; }
	if (l.points) {
		Coordinate *pl=l.points,
				   *p=NULL;
		while (pl) {
			if (!p) p=points=new Coordinate(*pl);
			else {
				p->next=new Coordinate(*pl);
				p->next->prev=p;
				p=p->next;
			}
			pl=pl->next;
			if (pl==l.points) {
				p->next=points;
				points->prev=p;
				break;
			}
		}
	}

	return *this;
}

//! Create points from an svg style d attribute like this closed square: "0 0  0 1  1 1  1 0 z".
/*! Returns 0 for success, or nonzero for error.
 */
int NetLine::Set(const char *d,LineStyle *ls)
{
	if (linestyle && ls) linestyle->dec_count();
	if (ls) { ls->inc_count(); linestyle=ls; }
	if (points) delete points;
	points=SvgToCoordinate(d,0,NULL);
	return points?0:1;
}

/*! If pfirst!=0, then immediately output the list of points.
 * Otherwise, do "points 3 5 6 6...".
 *
 * See dumpInAtts() for description of what is dumped out.
 *
 * If what==-1, then dump out description of what gets dumped out.
 *
 * \todo maybe if tag<0 have standard "outline", "innerline", or "tabline"?
 */
void NetLine::dumpOut(FILE *f,int indent, int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%stag  3       #tag number for the line. Might be used as class of line\n",spc);
		fprintf(f,"%spoints  \\   #svg style defined line\n",spc);
		fprintf(f,"%s  M-.5 -.5  L-.5 1.5  L1.5 1.5  L1.5 -.5\n",spc);
		fprintf(f,"%slinestyle    #style of line\n",spc);
		if (linestyle) linestyle->dump_out(f,indent+2,-1,NULL);
		else {
			LineStyle line;
			line.dump_out(f,indent+2,-1,NULL);
		}
		return;
	}
	if (points) {
		fprintf(f,"%spoints ",spc);
		char *svg=CoordinateToSvg(points);
		if (svg) fprintf(f,"%s\n",svg);
	}

	if (tag!=0) fprintf(f,"%stag %d\n",spc,tag); 
	if (linestyle) {
		fprintf(f,"%slinestyle\n",spc);
		linestyle->dump_out(f,indent+2,0,NULL);//note: context is NULL, is this ok?
	}
}

/*! If val!=NULL, then the att was something like:
 * <pre>
 *   line M 1 1 L 0 4
 *     (other stuff)..
 * </pre>
 * In that case, Net would have grabbed the "1 1 0 4", and it would pass
 * that here in val. Otherwise, this function will expect a 
 * "points 1 2 3" subattribute somewhere in att.
 *
 * To summarize the expected input when everything is in subattributes:
 * <pre>
 *   points M 0 0 L 1 0 L 1 1 L 0 1 z  #this is an svg style d path attribute
 *   linestyle
 *      ... #attributes for a LaxInterfaces::LineStyle
 *   tag
 *      3   #optional integer tag
 * </pre>
 */
void NetLine::dumpInAtts(LaxFiles::Attribute *att, const char *val,int flag)
{
	if (!att) return;
	int c;
	if (val) {
		if (points) delete points;
		points=SvgToCoordinate(val,0,NULL);
	}
	char *name,*value;
	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"points")) {
			if (points) delete points;
			points=SvgToCoordinate(value,4,NULL);
			//cout <<"**** rewrite to watch for LOOP_TERMINATOR stuff!!"<<endl;
		} else if (!strcmp(name,"linestyle")) {
			if (!linestyle) {
				linestyle=new LineStyle();
			}
			linestyle->dump_in_atts(att->attributes.e[c],flag,NULL);//note: context is NULL, is this ok?
		} else if (!strcmp(name,"tag")) {
			IntAttribute(value,&tag);
		}
	}
}

//----------------------------------- NetFaceEdge -----------------------------------
/*! \class NetFaceEdge
 * \brief Description for edges in a NetFace.
 */
/*! \var int NetFaceEdge::tooriginal
 * \brief Index number to a face in an AbstractNet.
 */
/*! \var int NetFaceEdge::id
 * \brief Arbitrary number assigned to this edge. Meaning depends on the application.
 */
/*! \var int NetFaceEdge::toface
 * \brief Net face index that this edge connects to.
 *
 * The face connecting to it must actually be connecting to it. If the original face is
 * somewhere else in the net, then toface would be -1, and the edge tag would be FACE_Taken.
 */
/*! \var int NetFaceEdge::tofaceedge
 * \brief Which edge of toface that this edge connects to.
 *
 * Generally, this should be the same edge number as in the the original face 
 * (see NetFaceEdge::tooriginal).
 */
/*! \var int NetFaceEdge::flipflag
 * \brief Whether the face that connects to this edge is normally oriented (0), or flipped over (nonzero).
 *
 * Take the faces such that the vertices are numbered so that the right hand rule says the face's
 * normal sticks up. Say an edge is between points 0 and 1 on face A, and between points 3 and 4 on
 * face B. Then if flipflag==0, then point 0 of A connects to point 4 of B, and point 1 to point 3.
 * Otherwise, 0 connects to point 3, and 1 connects to point 4. Normally, this means that 
 * this edge connects to the opposite side of the specified face.
 */
/*! \var FaceTag NetFaceEdge::tag
 * \brief The status of the connecting face.
 *
 * 0==no link for that edge.\n
 * 1==Does in fact connect to that face.\n
 * 2==Can potentially connect to the face, but should be ignored when actually using the net.\n
 * 3==Would connect to this face, but the face is already laid down elsewhere in the net
 */
/*! \var double NetFaceEdge::svalue
 * \brief For curved edges, the point along the edge to connect to the face.
 *
 * 0 is at the edge beginning, and 1 is edge end.
 */
/*! \var Coordinate *NetFaceEdge::points
 * \brief The path of the edge
 * 
 * For basic polyhedra, this will just be a poly line. For curvy faces, it will be a 
 * cubic bezier path, starting with a vertex point and 2 control points. Following that, there can
 * be any number of vertex-control-control points. So the final point of the list will be
 * a control point, the vertex point following that control point is in the next edge.
 */

NetFaceEdge::NetFaceEdge()
{
	tooriginal=id=toface=tofaceedge=-1;
	flipflag=0;
	tag=FACE_Undefined;
	svalue=0;
	points=NULL;
	//basis_adjustment=NULL;
}

//! Beware that the points list is deleted here.
NetFaceEdge::~NetFaceEdge()
{
	//if (basis_adjustment) delete[] basis_adjustment;
	if (points) delete points; //remember Coordinate delete knows if it is in a loop
}

//! Assignment operator, straightforward copy all.
const NetFaceEdge &NetFaceEdge::operator=(const NetFaceEdge &e)
{
	id        =e.id;
	tooriginal=e.tooriginal;
	toface    =e.toface;
	tofaceedge=e.tofaceedge;
	flipflag  =e.flipflag;
	tag       =e.tag;
	svalue    =e.svalue;

	if (points) { delete points; points=NULL; }
	Coordinate *p=e.points,*p2=NULL;
	while (p) {
		if (!p2) p2=points=new Coordinate(*p);
		else {
			p2->next=new Coordinate(*p);
			p2->next->prev=p2;
			p2=p2->next;
		}
		p=p->next;
	}

	return *this;
}


//--------------------------------------- NetFace -------------------------------------------
/*! \class NetFace
 * \brief Class to hold info about a face in a Net.
 */
/*! \var double *NetFace::matrix
 * \brief Additional offset to place the origin of a face.
 *
 * The default (matrix==NULL or is identity) is for the origin to be at the beginning
 * of the first edge, and the x axis to be parallel to that edge.
 */ 
/*! \var int NetFace::original
 * \brief The index in the AbstractNet corresponding to this face.
 */
/*! \var char NetFace::isfront
 * \brief If nonzero (the default) then this face is the front side. Otherwise, it is the backside.
 *
 * Somewhere else in the net, there may be the reverse side, but that shape will be the
 * mirror image of this face.
 */
/*! \var FaceTag NetFace::tag
 * \brief The status of the connecting face. Similar in spirit to NetFaceEdge::tag.
 *
 * Unlike NetFaceEdge::tag, this tag is usually only:\n
 * FACE_Actual==an actual face.\n
 * FACE_Potential==a potential face
 */
/*! \var int NetFace::tick
 * \brief Used as a traversal flag to create unwrap paths.
 */

NetFace::NetFace()
{
	tick=0;
	tag=FACE_None;
	matrix=NULL;
	isfront=1;
	original=-1;
}
	
NetFace::~NetFace()
{ 
	if (matrix) delete[] matrix;
	//edges.flush();
}

//! Delete matrix, set isfront=1, original=-1, flush edges.
void NetFace::clear()
{
	if (matrix) { delete[] matrix; matrix=NULL; }
	edges.flush();
	original=-1;
	isfront=1;
}

//! Assignment operator, straightforward copy all.
/*! Except tick and tag.
 */
const NetFace &NetFace::operator=(const NetFace &face)
{
	if (matrix) { delete[] matrix; matrix=NULL; }
	edges.flush();

	original=face.original;
	isfront=face.isfront;
	if (face.matrix) {
		matrix=new double[6];
		transform_copy(matrix,face.matrix);
	}

	NetFaceEdge *newe;
	if (face.edges.n) {
		for (int c=0; c<face.edges.n; c++) {
			newe=new NetFaceEdge();
			*newe=*face.edges.e[c];
			edges.push(newe);
		}
	}

	return *this;
}

////! Create points and facelink from list like: "1 2 3".
///*! Returns the number of points.
// *
// * If list and link have differing numbers of elements, link is
// * removed and replaced with a list of -1.
// */
//int NetFace::Set(const char *list, const char *link)
//{***
//	if (list && points) { delete[] points; points=NULL; }
//	if (link && facelink) { delete[] facelink; facelink=NULL; }
//	int n;
//	if (link) IntListAttribute(link,&facelink,&n);
//	else n=np;
//	if (list) IntListAttribute(list,&points,&np);
//	if (n!=np) {
//		if (facelink) { delete[] facelink; facelink=NULL; }
//		facelink=new int[np];
//		for (n=0; n<np; n++) facelink[n]=-1;
//	}
//	return np;
//}
//
////! Set points and facelink from an integer list of length nn.
///*! If dellists!=0, then take possession of the list and link arrays.
// * Otherwise, copy list and link.
// *
// * Returns the number of points.
// */
//int NetFace::Set(int n,int *list,int *link,int dellists)//dellists=0
//{***
//	if (list && points) { delete[] points; points=NULL; }
//	if (link && facelink) { delete[] facelink; facelink=NULL; }
//	
//	 // establish new points
//	if (dellists) points=list;
//	else {
//		points=new int[n];
//		memcpy(points,list,n*sizeof(int));
//	}
//	 // establish new links
//	if (dellists) facelink=link;
//	else {
//		facelink=new int[n];
//		memcpy(facelink,link,n*sizeof(int));
//	}
//	np=n;
//
//	return np;
//}

//! Return the outline of the face.
/*! If convert==0, then the points are in face coordinates. Otherwise, they
 * are in net coordinates, according to NetFace::matrix, if any.
 *
 * Sets p to a new flatpoint[], and n to the number of points in p.
 * If the line has any bezier control points, then p is a list of points
 * in the form control-vertex-control-control-vertex-control-etc, and 2 is returned.
 * Otherwise, the path is a polyline, and 1 is returned.
 *
 * If the path cannot be found, then 0 is returned, and p and n are not changed.
 *
 * \todo right now assumes that for bez segments, 2 controls exist between each
 *   vertex, and vertices exist between a NEXT and PREV coordinate.. should
 *   probably check for validity?
 */
int NetFace::getOutline(int *n, flatpoint **p, int convert)
{
	NumStack<flatpoint> pts;
	char isbez=0;
	Coordinate *cc;
	for (int c=0; c<edges.n; c++) {
		cc=edges.e[c]->points;
		while (cc) {
			if ((!isbez && (cc->flags&POINT_TONEXT)) || (cc->flags&POINT_TOPREV)) {
				isbez=1;
				 // convert all pts to bez segs
				for (int c2=1; c2<pts.n; c2++) {
					pts.push(pts.e[c2-1]+(pts.e[c2]-pts.e[c2-1])/3,c2);        //c1
					pts.push(pts.e[c2-1]+(pts.e[c2+1]-pts.e[c2-1])*2./3,c2+1); //c2
					c2+=2;
				}
			}
			pts.push(cc->fp);
//			***check for list of vertices next to each other is bez line..
//			   if so, add controls between those points....
//			if (isbez) {
//				if ((cc->flags&POINT_VERTEX) && bezpart==0) ***;
//
//				if (cc->flags&POINT_TOPREV) bezpart=1;
//				else if (cc->flags&POINT_TONEXT) bezpart=2;
//				else bezpart=0;
//			}
			cc=cc->next;
		}
	}
	if (isbez) {
		 //move final control point to beginning
		pts.push(pts.e[pts.n-1],0);
		pts.pop(pts.n-1);
	}
	if (convert && matrix) {
		for (int c=0; c<pts.n; c++) {
			pts.e[c]=transform_point(matrix,pts.e[c]);
		}
	}
	*p=pts.extractArray(n);
	if (isbez) return 2;
	return 1;
}

/*! If what==-1, then dump out psuedo-code mockup of format. Basically that means this:
 * <pre>
 *   face
 *     original 3   # the face index in the abstract net corresponding to this face
 *     back         # this face is actual the mirror image of the abstract net face
 *     matrix 1 0 0 1 0 0  # transform that places points in net space
 *     potential           # whether this face is actually used in the net
 *     edge              # this is a typical edge for a laidout net
 *       toface     4.6  # connects to net face index 4, edge index number 6
 *       tooriginal 3    # connects to original face 3
 *       potential       # link is only potential. If absent, then program detects it
 *       svalue   .5     # for bezier edges, the point of contact with adjacent edge, 0 to 1
 *       points \        # point list for this edge. First point must be a vertex point
 *         -.5 -.5       # that is actually on the line. following points can be either
 *         c -.5 1.5     # polyline points or bezier control points
 *         c 1.5 1.5     
 *         v 2 2         # the edge's path can be a bezier line of any length, but
 *         c 1.5 3.6     # remember to leave out the final vertex point. That point is
 *         c 1.8 3.6     # listed in the following edge's point list.
 *     edge             # here's a typical edge for a basic abstract net
 *       tooriginal 2.1  # connects to original face 2 at edge 1
 *       d  M-.5 -.5  L-.5 1.5  L1.5 1.5  L1.5 -.5  #alternate specification of points with svg
 * </pre>
 */
void NetFace::dumpOut(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%soriginal 3   #the face index in the abstract net corresponding to this face\n",spc);
		fprintf(f,"%sback         #this face is actual the mirror image of the abstract net face\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #transform that places points in net space\n",spc);
		fprintf(f,"%spotential           #this face might be, but is not necessarily in the net\n",spc);
		fprintf(f,"%sactual             #this face IS in the net\n",spc);
		fprintf(f,"%sedge              #this is a typical edge for a laidout net\n",spc);
		fprintf(f,"%s  toface     4.6  #connects to net face index 4, edge index number 6\n",spc);
		fprintf(f,"%s  tooriginal 3    #connects to original face 3\n",spc);
		fprintf(f,"%s  potential       #link is to a potential net face\n",spc);
		fprintf(f,"%s  actual          #link is to an actual net face\n",spc);
		fprintf(f,"%s  facetaken       #link is to an actual face that is somehow connected elsewhere\n",spc);
		fprintf(f,"%s  svalue   .5     #for bezier edges, the point of contact with adjacent edge, 0 to 1\n",spc);
		fprintf(f,"%s  points \\        #point list for this edge. First point must be a vertex point\n",spc);
		fprintf(f,"%s    -.5 -.5       # that is actually on the line. following points can be either\n",spc);
		fprintf(f,"%s    c -.5 1.5     # polyline points or bezier control points\n",spc);
		fprintf(f,"%s    c 1.5 1.5     \n",spc);
		fprintf(f,"%s    v 2 2         # the edge's path can be a bezier line of any length, but\n",spc);
		fprintf(f,"%s    c 1.5 3.6     # remember to leave out the final vertex point. That point is\n",spc);
		fprintf(f,"%s    c 1.8 3.6     # listed in the following edge's point list.\n",spc);
		fprintf(f,"%sedge             #here's a typical edge for a basic abstract net\n",spc);
		fprintf(f,"%s  tooriginal 2.1  #connects to original face 2 at edge 1\n",spc);
		fprintf(f,"%s  d  M-.5 -.5  L-.5 1.5  L1.5 1.5  L1.5 -.5  #alternate specification of points with svg\n",spc);
		return;
	}
	
	fprintf(f,"%soriginal %d\n",spc,original);
	if (!isfront) fprintf(f,"%sback\n",spc);
	if (matrix) fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
				spc,matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);

	int c;
	if (tag==FACE_Potential) fprintf(f,"%spotential\n",spc);
	//else fprintf(f,"%stag %d\n",spc,tag);
	
	for (c=0; c<edges.n; c++) {
		fprintf(f,"%sedge\n",spc);
		if (edges.e[c]->toface>=0) {
			fprintf(f,"%s  toface %d",spc,edges.e[c]->toface);
			if (edges.e[c]->tofaceedge>=0) fprintf(f,".%d\n",edges.e[c]->tofaceedge);
			else fprintf(f,"\n");
			if (edges.e[c]->tooriginal>=0)
				fprintf(f,"%s  tooriginal %d\n",spc,edges.e[c]->tooriginal);
		} else if (edges.e[c]->tooriginal>=0) {
			fprintf(f,"%s  tooriginal %d",spc,edges.e[c]->tooriginal);
			if (edges.e[c]->tofaceedge>=0) fprintf(f,".%d\n",edges.e[c]->tofaceedge);
			else fprintf(f,"\n");
		}
		if (edges.e[c]->flipflag) fprintf(f,"%s  flip\n",spc);

		if (edges.e[c]->tag==FACE_Actual) fprintf(f,"%s  actual\n",spc);
		else if (edges.e[c]->tag==FACE_Potential) fprintf(f,"%s  potential\n",spc);
		else if (edges.e[c]->tag==FACE_Taken) fprintf(f,"%s  facetaken\n",spc);
		else if (edges.e[c]->tag==FACE_None) fprintf(f,"%s  noface\n",spc);
		else if (edges.e[c]->tag==FACE_Undefined) fprintf(f,"%s  undefined\n",spc);

		if (edges.e[c]->points) {
			fprintf(f,"%s  points \\\n",spc);
			Coordinate *p=edges.e[c]->points;
			char scratch[500];
			while (p) {
				if (p->flags&POINT_VERTEX) {
					sprintf(scratch,"%s    %.10g %.10g\n",spc,p->fp.x,p->fp.y);
					fprintf(f,"%s",scratch);
				} else {
					sprintf(scratch,"%s    c %.10g %.10g\n",spc,p->fp.x,p->fp.y);
					fprintf(f,"%s",scratch);
				}
				//if (p->flags&POINT_VERTEX) fprintf(f,"%s    %.10g %.10g\n",spc,p->fp.x,p->fp.y);
				//else fprintf(f,"%s    c %.10g %.10g\n",spc,p->fp.x,p->fp.y);
				p=p->next;
			}
		}
	}

	//if (aligno>=0) {
	//	fprintf(f,"%salign %d",spc,aligno);
	//	if (alignx>=0) fprintf(f," %d",alignx);
	//	fprintf(f,"\n");
	//}
}

/*!
 * See dumpOut() for what gets put out.
 */
void NetFace::dumpInAtts(LaxFiles::Attribute *att)
{
	if (!att) return;
	int c;
	char *name,*value,*e;
	NetFaceEdge *edge;
	int error=0;

	isfront=1;
	tag=FACE_Actual;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"matrix")) {
			if (!matrix) matrix=new double[6];
			DoubleListAttribute(value,matrix,6);
		} else if (!strcmp(name,"back")) {
			isfront=!BooleanAttribute(value);
		} else if (!strcmp(name,"front")) {
			isfront=BooleanAttribute(value);
		} else if (!strcmp(name,"actual")) {
			tag=FACE_Actual;
		} else if (!strcmp(name,"potential")) {
			tag=FACE_Potential;
		} else if (!strcmp(name,"original")) {
			IntAttribute(value,&original);
		} else if (!strcmp(name,"edge")) {
			edge=new NetFaceEdge;
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;
				if (!strcmp(name,"tooriginal")) { //could be "3" or "3.5"
					IntAttribute(value,&edge->tooriginal,&e);
					if (e!=value && *e=='.' && isdigit(e[1])) IntAttribute(e+1,&edge->tofaceedge);
				} else if (!strcmp(name,"toface")) {
					IntAttribute(value,&edge->toface,&e);
					if (e!=value && *e=='.' && isdigit(e[1])) IntAttribute(e+1,&edge->tofaceedge);
				} else if (!strcmp(name,"svalue")) {
					DoubleAttribute(value,&edge->svalue,NULL);
				} else if (!strcmp(name,"points")) {
					int which=-1;//last point was: 0=vert, 1=c1, 2=c2
					e=value;
					Coordinate *pts=NULL;
					int f;
					while (e && *e) {
						 //scan in 'v'|'c'|nothing, then 2 reals
						while (isspace(*e)) e++;
						if (!*e) break;
						if (*e=='c') {
							if (which==0) { which=1; f=POINT_TOPREV; }
							else if (which==1) { which=2; f=POINT_TONEXT; }
							else {
								 //too many control points, make this control a vertex
								f=POINT_VERTEX;
								which=0;
							}
							e++;
						} else {
							if (which==1) {
								 //only 1 control point between vertices,
								 //so add second control point, same as last on
								pts->next=new Coordinate(*pts);
								pts->flags=POINT_TONEXT;
							}
							which=0;
							if (*e=='v') e++;
							f=POINT_VERTEX;
						}
						if (!pts) edge->points=pts=new Coordinate();
							else { pts->next=new Coordinate(); pts=pts->next; }
						pts->flags=f;
						char *ee;
						DoubleAttribute(e,&pts->fp.x,&ee); //***assumes f will actually be read in..
						if (ee==e) { error=1; break; }
						DoubleAttribute(ee,&pts->fp.y,&e);
						if (ee==e) { error=1; break; }
					}
				} else if (!strcmp(name,"d")) {
					edge->points=SvgToCoordinate(value,0,NULL);
				} else if (!strcmp(name,"potential")) {
					edge->tag=FACE_Potential;
				} else if (!strcmp(name,"undefined")) {
					edge->tag=FACE_Undefined;
				} else if (!strcmp(name,"facetaken")) {
					edge->tag=FACE_Taken;
				} else if (!strcmp(name,"actual")) {
					edge->tag=FACE_Actual;
				}
			}
			if (edge->toface>=0 && (edge->tag==FACE_Undefined || edge->tag==FACE_None)) edge->tag=FACE_Actual; //***what's this??
			edges.push(edge,1);
//---old:
//		} else if (!strcmp(name,"aligno")) {
//			IntAttribute(value,&aligno);
//		} else if (!strcmp(name,"alignx")) {
//			IntAttribute(value,&alignx);
//		} else if (!strcmp(name,"points")) {
//			if (points) delete[] points;
//			points=NULL;
//			IntListAttribute(val,&points,&np);
		}
	}
}


//--------------------------------------- AbstractNet -------------------------------------------
/*! \class AbstractNet
 * \brief Abstract base class for objects used to generate Net objects.
 *
 * AbstractNet objects hold all the connectivity information used in Net objects.
 * The Net class holds particular arrangements of the faces in an Abstract Net.
 */

/*! \fn NetFace *AbstractNet::GetFace(int i,double scaling)
 * \brief Return a new NetFace corresponding to AbstractNet face with index i.
 *
 * The face returned has edges that have indexes of original faces the edges connect
 * to, but the edge tags are all FACE_Undefined. Upon unwrapping, the Net class is supposed
 * to change those tags to appropriate values.
 *
 * If scaling!=1, then each coordinate has been scaled down by that factor. 
 * Say a point is (10,6), and scaling==.5. Then the point in the returned face will be (5,3).
 */
/*! \fn	const char *AbstractNet::NetName()
 * \brief Return a human readable title of the net, if any.
 *
 * Default returns AbstractNet::name
 */
/*! \fn int AbstractNet::dumpOutNet(FILE *f,int indent,int what)
 * \brief Dump out the net.
 */

AbstractNet::AbstractNet()
{
	name=NULL;
}

AbstractNet::~AbstractNet()
{
	if (name) delete[] name;
}

//! Return whether the net has been modified presumably since the last load or save.
/*! Default is to return 0 for unmodified.
 *
 * Derived classes must figure out how to maintain their actual modified status
 * themselves. There is no default method for that.
 */
int AbstractNet::Modified()
{ 	return 0; }

//! Tag the net as having been modified or not. 
/*! 0 means pretend it hasn't been. nonzero means it has.
 */
int AbstractNet::Modified(int m)
{ 	return 0; }

//! Return the file associated with this net, if any. Default is return NULL.
const char *AbstractNet::Filename()
{	return NULL;  }

//----------------------------------- BasicNet -----------------------------------
/*! \class BasicNet
 * \brief The simplest AbstractNet.
 *
 * Merely a stack of 2-d NetFace objects.
 */

BasicNet::BasicNet(const char *nname)//nname=NULL
{
	makestr(name,nname);
}

BasicNet::~BasicNet()
{ }

//! Return a new NetFace object corresponding to face i.
NetFace *BasicNet::GetFace(int i,double scaling)
{//***
	NetFace *face=new NetFace(*e[i]);
	return face;
}

int BasicNet::dumpOutNet(FILE *f,int indent,int what)
{
	cout <<"***imp BasicNet::dumpOutNet!"<<endl;
	return 0;
}

void BasicNet::dump_out(FILE *f,int indent,int what,Laxkit::anObject *savecontext)
{
	cout <<"***imp BasicNet::dump_out!"<<endl;
}

void BasicNet::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	cout <<"***imp BasicNet::dump_in_atts!"<<endl;
}



//--------------------------------------- Net -------------------------------------------
/*! \class Net
 * \brief Holds a user unwrapped net.
 *
 * A Net consists of an optional AbstractNet, a number of additional lines, and a list of
 * faces that have been laid down.
 *
 * An AbstractNet is the base from which the faces in the
 * net can be derived. The AbstractNet may hold many more faces than are actually represented
 * in a Net. The Net can pick up and put down the faces in the AbstractNet as long as the
 * AbstractNet provides the necessary connectivity information.
 *
 * If there is no AbstractNet, then the net is static, and cannot be wrapped and unwrapped.
 *
 * \todo *** implement tabs
 */ 
/*! \var PtrStack<NetLine> *Net::lines
 * \brief The lines that make up what the net looks like.
 *
 * These lines are what should be seen when the net is rendered. The outlines of the faces 
 * themselves can be accessed through Net::faces.
 *
 * Lines can be automatically generated from the way the faces connect, creating 1 or more outlines,
 * and inner "fold" lines. Lines for tabs on the edges of faces can also be automatically generated.
 * There can also be other user defined lines that have nothing to do with the faces.
 */
/*! \var PtrStack<NetFace> *Net::faces
 * \brief The faces that make up the net.
 *
 * Faces can be actually seen, or only potential faces.
 */
/*! \var int Net::active
 * \brief Not actually used in Net class, another extra info variable for applications.
 *
 * This is just an extra tag you can use for your own purposes, like Net::info.
 */
/*! \var int Net::info
 * \brief Not actually used in Net class, extra info variable for applications.
 *
 * This is just an extra tag you can use for your own purposes, like Net::active.
 */
/*! \var int Net::tabs
 * <pre>
 *  0  no tabs (default)
 *  1  tabs alternating every other (even)
 *  2  tabs alternating the other every other (odd)
 *  3  tabs on all edges (all or yes)
 * </pre>
 */


//! Init.
Net::Net()
{
	_config=0;
	active=1;
	info=0;
	tabs=0;
	netname=newstr("Net");
	basenet=NULL;
}

//! Delete points,lines,faces,pointmap,netname.
Net::~Net()
{	clear(); }

void Net::clear()
{
	if (netname) { delete[] netname; netname=NULL; }
	
	active=0;
	tabs=0;
	if (basenet) basenet->dec_count();
	if (faces.n) faces.flush();
	if (lines.n) lines.flush();
}


//! Return a new copy of this.
/*! Warning: copies reference to basenet, does not make full copy of basenet.
 */
Net *Net::duplicate()
{
	Net *net=new Net;
	makestr(net->netname,netname);
	net->tabs=tabs;
	net->basenet=basenet;
	if (basenet) basenet->inc_count();

	if (lines.n) {
		for (int c=0; c<lines.n; c++) {
			net->lines.push(new NetLine(*lines.e[c]));
		}
	}
	if (faces.n) {
		for (int c=0; c<faces.n; c++) {
			net->faces.push(new NetFace(*faces.e[c]));
		}
	}

	net->m(m());
	net->FindBBox();
	return net;
}

////! Rotate face f coordinate space by moving alignx and/or o by one.
///*! Return 0 for something changed, nonzero for not.
// */
//int Net::rotateFaceOrientation(int f,int alignxonly)//alignxonly=0
//{ ***may need to code this stuff back in. it is useful
//	if (f<0 || f>=nf) return 0;
//	int ao=faces[f].aligno,
//	    ax=faces[f].alignx;
//	if (ao<0) ao=0;
//	if (ax<0) ax=(ao+1)%faces[f].np;
//	int n=0;
//	ax=(ax+1)%faces[f].np;
//	if (!alignxonly) ao=(ao+1)%faces[f].np;
//	if (ax==ao) ax=-1;
//	n=1;
//	if (ax!=faces[f].alignx || ao!=faces[f].aligno) n=0;
//	faces[f].aligno=ao;
//	faces[f].alignx=ax;
//	return n;
//}

//! Return the outline of an individual net face.
/*! If convert==0, then the points are in face coordinates. Otherwise, they
 * are in net coordinates.
 *
 * Sets p to a new flatpoint[], and n to the number of points in p.
 * If the line has any bezier control points, then p is a list of points
 * in the form control-vertex-control-control-vertex-control-etc, and 2 is returned.
 * Otherwise, the path is a polyline, and 1 is returned.
 *
 * If the path cannot be found, then 0 is returned, and p and n are not changed.
 *
 * This does bounds check on i, then relays the request to NetFace::getOutline().
 */
int Net::pathOfFace(int i, int *n, flatpoint **p, int convert)
{
	if (i<0 || i>=faces.n) return 0;

	return faces.e[i]->getOutline(n,p,convert);
}

//! Return the index of the first net face that contains pp, or -1.
int Net::pointinface(flatpoint pp)
{
	double i[6];
	transform_invert(i,m());
	pp=transform_point(i,pp);
	int c,t,n;
	flatpoint *pts=NULL;
	for (c=0; c<faces.n; c++) {
		t=pathOfFace(c,&n,&pts,1);
		if (t==1) { if (point_is_in(pp,pts,n)) t=1000; }
		else if (t==2) { if (point_is_in_bez(pp,pts,n)) t=1000; }

		delete[] pts; pts=NULL;
		if (t==1000) {
			return c;
		}
	}
	return -1;
}

//! Return the number of actual faces in the net.
int Net::numActual()
{
	int n=0;
	for (int c=0; c<faces.n; c++) if (faces.e[c]->tag==FACE_Actual) n++;
	return n;
}

//! Set all the NetFace::tick value to t.
void Net::resetTick(int t)
{
	for (int c=0; c<faces.n; c++) faces.e[c]->tick=t;
}

//! Return information about how a net face is linked.
/*! facei is a net face index, and edgei is an optional edge index within the face.
 *
 * If edgei<0 then return the number of actual faces this face touches in the laid out net,
 * based on the tags of the connected faces.
 * 
 * If edge>=0 then return as follows. If the face connected to the face at that edge is FACE_Actual, then 
 * return the net face index of the connecting face. If the connecting face is not
 * FACE_Actual, then return -1;
 */
int Net::actualLink(int facei, int edgei)
{
	if (facei<0) return -1;
	if (edgei<0) {
		 //find the number of actual faces touching this face
		int n=0;
		for (int c=0; c<faces.e[facei]->edges.n; c++) {
			if (faces.e[facei]->edges.e[c]->toface>=0
					&& faces.e[faces.e[facei]->edges.e[c]->toface]->tag==FACE_Actual) 
				n++;
		}
		return n;
	}
	if (faces.e[facei]->edges.e[edgei]->toface>=0
			&& faces.e[faces.e[facei]->edges.e[edgei]->toface]->tag==FACE_Actual)
		return faces.e[facei]->edges.e[edgei]->toface;
	return -1;
}

//! Find the bounding box of lines of the net.
/*! 
 * \todo right now, this is the naive bbox.. for bez paths, is only grabbing the
 *   bounding box of vertex and control points, not actual bounds of the curve.
 */
void Net::FindBBox()
{
	if (!lines.n) return;
	maxx=maxy=-1;
	minx=maxy=0;
	Coordinate *cc;
	for (int c=0; c<lines.n; c++) {
		cc=lines.e[c]->points;
		if (cc) do {
			addtobounds(cc->fp);
			cc=cc->next;
			if (cc==lines.e[c]->points) cc=NULL;
		} while (cc);
	}
}

////! Center the net around the origin. (changes points)
//void Net::Center()
//{ ***
//	double dx=(maxx+minx)/2,dy=(maxy+miny)/2;
//	for (int c=0; c<np; c++) {
//		points[c]-=flatpoint(dx,dy);
//	}
//	minx-=dx;
//	maxx-=dx;
//	miny-=dy;
//	maxy-=dy;
//}

//! Modify net->m() to fit within data with a margin.
/*! If setpaper!=0, then set the bounds of Net::paper to data.
 */
void Net::FitToData(Laxkit::DoubleBBox *data,double margin,int setpaper)
{
	DoubleBBox box(*data);
	if (margin*2<box.maxx-box.minx) {
		box.minx+=margin;
		box.maxx-=margin;
	}
	if (margin*3<box.maxy-box.miny) {
		box.miny+=margin;
		box.maxy-=margin;
	}
	fitto(NULL,&box,50,50);

	if (setpaper) {
		paper.m_clear();
		paper.origin(flatpoint(data->minx,data->miny));
		paper.minx=paper.maxx=0;
		paper.maxx=data->maxx-data->minx;
		paper.maxy=data->maxy-data->miny;
	}
}

//! Add a line to lines before position where. where<0 implies top of stack.
/*! Makes a copy. The NetLine::tag is what. If how is 0, then the line 
 * is marked as an automatic line, and will be destroyed whenever picking up or
 * putting down faces. Otherwise, it is a user-defined line, and will persist.
 */
void Net::pushline(NetLine &l,int what,int where,int how)//where=-1, how=1
{
	NetLine *newline=new NetLine();
	*newline=l;
	newline->lineinfo=(how?1:0);
	newline->tag=what;
	lines.push(newline,1,where);
}

//----------------Net load and save functions:

/*! perhaps:
 * <pre>
 * name "Square split diagonally"
 * matrix 1 0 0 1 0 0  # optional extra matrix to map this net to a page
 * line M 1 1 L 0 4    # optional extra user created lines
 *
 *  # Tabs defaults to 'no'.
 *  # and you can also specify 'left' or 'right'.(<-- this bit not imp yet, maybe never)
 * # *** tabs are not currently implemented
 * tabs no  # one of 'no','yes','all','default','even', or 'odd',
 * \#tab 1 4 left  # draw tab from point 1 to 4, on ccw side from 4 using 1 as origin
 *  
 *  # For each face, default is align x axis to be (face point 1)-(face point 0).
 *  # you can specify which other points the x axis should correspond to.
 *  # In that case, the 'align' points are indices into the points list,
 *  # NOT indices into the face's point list.
 *  # Otherwise, you can specify an arbitrary matrix that transforms
 *  # from whatever is the default face basis to the desired one
 * face 0 1 2
 *    align 1 2
 *    class 0  # optional tag. All faces of this class number are assumed
 *             # to use the same outline and matrix(?)
 * face 1 2 3
 *    matrix 1 0 0 1 .2 .3
 * </pre>
 *
 * \todo *** implement abstract net references and file references
 */
void Net::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sname Bent Square #this is just any name you give the net\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  # transform to map the net to a paper\n",spc);
		fprintf(f,"%sbasenet             #the base abstract net\n",spc);
		fprintf(f,"%sinfo  333           #general integer info about the net\n",spc);
		fprintf(f,"%sactive              #present if the net is supposed to be somehow active.\n",spc);
		fprintf(f,"%s                    # the exacte meaning of active is application dependent.\n",spc);
		fprintf(f,"%stabs no             #(***TODO) whether to put tabs on face edges\n",spc);
		
		NetFace face;
		fprintf(f,"%sface               #there can be any number of laid out faces\n",spc);
		face.dumpOut(f,indent+2,-1);

		NetLine line;
		fprintf(f,"%sline               #there can be any number of extra lines\n",spc);
		line.dumpOut(f,indent+2,-1);
		return;
	}
	if (active) fprintf(f,"%sactive\n",spc);
	fprintf(f,"%sinfo %d\n",spc,info);
	if (basenet) {
		if (basenet->NetName() && !strcmp("BasicNet",basenet->NetName())) {
			fprintf(f,"%sbasenet BasicNet\n",spc);
			basenet->dumpOutNet(f,indent+2,0);
//		} else if (***basenet is from unmodified file) {
//			fprintf(f,"%sbasenet file://%s\n",spc,basenet->Filename);
//		} else if (***we are having references to basenet) {
//			fprintf(f,"%sbasenet ref:/%s\n",spc,basenet->NetName());
		}
	}
	if (lines.n) {
		for (int c=0; c<lines.n; c++) {
			fprintf(f,"%sline\n",spc);
			lines.e[c]->dumpOut(f,indent+2,0);
		}
	}
	if (faces.n) {
		for (int c=0; c<faces.n; c++) {
			//DBG cerr <<"dump out face "<<c<<endl;
			fprintf(f,"%sface #%d\n",spc,c);
			faces.e[c]->dumpOut(f,indent+2,0);
		}
	}
	if (f!=stdout && f!=stderr) {
		//fclose(f);
		//exit(1);
	}
}

//! Set up net from a Laxkit::Attribute.
/*! If there is no 'outline' attribute, then it is assumed that
 * the list of points in order is the outline.
 * 
 * \todo *** MUST implement the sanity check..
 */
void  Net::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	int c;
	clear();
	const char *baseref=NULL;
	for (c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"name")) {
			makestr(netname,value);
		} else if (!strcmp(name,"info")) {
			IntAttribute(value,&info);
		} else if (!strcmp(name,"active")) {
			active=BooleanAttribute(value);
		} else if (!strcmp(name,"matrix")) {
			double mm[6];
			if (DoubleListAttribute(value,mm,6)==6) m(mm);
		} else if (!strcmp(name,"tabs")) {
			if (!value) tabs=1;
			else {
				if (!strcmp(value,"no") || !strcmp(value,"default")) tabs=0;
				else if (!strcmp(value,"even")) tabs=1;
				else if (!strcmp(value,"odd")) tabs=2;
				else if (!strcmp(value,"yes") || !strcmp(value,"all")) tabs=3;
				else tabs=0; // catch all for bad value
			}
		} else if (!strcmp(name,"basenet")) {
			if (!value) continue;
			if (!strcmp(value,"BasicNet")) {
				basenet=new BasicNet();
				for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
					name=att->attributes.e[c]->attributes.e[c2]->name;
					value=att->attributes.e[c]->attributes.e[c2]->value;
					if (!strcmp(value,"face")) {
						NetFace *face=new NetFace;
						face->dumpInAtts(att->attributes.e[c]->attributes.e[c2]);
					} else if (!strcmp(value,"name")) {
						makestr(basenet->name,value);
					}
				}
			} else if (!strncmp(value,"ref:",4)) {
				baseref=value+4;
			} else if (!strncmp(value,"file:",4)) {
				basenet=loadBaseNet(value+5,NULL);//***ignores error string return
			}
		} else if (!strcmp(name,"line")) {
			NetLine netline;
			netline.dumpInAtts(att->attributes.e[c],value,flag);
			pushline(netline,1,-1,0); // pushes onto top
		} else if (!strcmp(name,"face")) {
			NetFace *netface=new NetFace;
			netface->dumpInAtts(att->attributes.e[c]);
			faces.push(netface,1);
		}
	}

	//***sanity check on all point references..
	FindBBox();

	DBG cerr <<"----------------this was set in Net:-------------"<<endl;
	DBG dump_out(stderr,0,0,NULL);
	DBG cerr <<"----------------end Net dump:-------------"<<endl;
}

//! Clear the net, and install newbasenet to basenet.
/*! This inc's count of newbasenet.
 */
int Net::Basenet(AbstractNet *newbasenet)
{
	clear();
	newbasenet->inc_count();
	basenet=newbasenet;
	return 0;
}

//! From a file reference, load the base abstract net.
/*! This means figuring out what sort of file the base net is. Default Net
 * classes can only understand BasicNet files in indented format.
 */
AbstractNet *Net::loadBaseNet(const char *filename,char **error_ret)
{
	BasicNet *net=new BasicNet;
	Attribute att;
	att.dump_in(filename,NULL);
	net->dump_in_atts(&att,0,NULL);
	return net;
}

//! Replace the existing net with a net generated from the given OFF file.
/*! This is assumed to be a 2 dimensional net. For 3 dimensional points in an OFF,
 * only the x and y components are used. Return 0 for success, or nonzero for error.
 *
 * \todo *** imp me...
 */
int Net::LoadOFF(const char *filename,char **error_ret)
{ //***
	return 1;
//	if (file_exists(filename,1,NULL)!=S_IFREG) {
//		if (error_ret) *error_ret=_("Cannot read that file.");
//		return 1;
//	}
//	FILE *f=fopen(filename,"r");
//	if (!f) {
//		if (error_ret) *error_ret=_("Cannot read that file.");
//		return 1;
//	}
//	setlocale(LC_ALL,"C");
//	if (error_ret) *error_ret=NULL;
//
//	int e=0, 
//		numv=0,
//		numf=0,
//		c;
//	NumStack<spacepoint> pts;
//	PtrStack<int> fcs(2);
//	char *line=NULL;
//	size_t n=0;
//	while (1) {
//		c=getline(&line,&n,f);
//		if (c<=0 || strcmp(line,"OFF")) { e=1; break; }
//
//		c=getline(&line,&n,f);
//		if (c<=0) { e=1; break; }
//
//		int *i=new int[3];
//		double p[4];
//		c=IntListAttribute(line,i,3,NULL);
//		if (c!=3) { e=1; break; }
//		numv=i[0];
//		numf=i[1];
//		for (int v=0; v<numv; v++) {
//			c=getline(&line,&n,f);
//			if (c<=0) { e=1; break; }
//			
//			c=DoubleListAttribute(line,p,3,NULL);
//			if (c!=3) { e=1; break; }
//
//			pts.push(spacepoint(p));
//		}
//
//		delete[] i; i=NULL;
//		int *ff;
//		for (int fc=0; fc<numf; fc++) {
//			c=getline(&line,&n,f);
//			if (c<=0) { e=1; break; }
//			
//			c=IntListAttribute(line,&i,NULL,NULL);
//			if (!i || c!=i[0]+1) { e=1; break; }
//
//			ff=new int[i[0]];
//			memcpy(ff,i,i[0]*sizeof(int));
//			fcs.push(ff);
//			delete[] i; i=NULL;
//		}
//	}
//	if (line) free(line);
//	fclose(f);
//	setlocale(LC_ALL,"");
//	if (e && error_ret) *error_ret=_("Bad OFF file.");
//
//	cout <<"***must implement convert pts and fcs to some kind of net"<<endl;
//
//	return e;
}

//! Try to load a net from the given file.
/*! Currently, the file format must be the native Net format. See dump_out() for details.
 *
 * Return 0 for success.
 *
 * \todo *** check that file can be either an OFF/COFF file, the native Net format, or cff2 format.
 * \todo implement cff2 format
 * \todo *** does not check whether was actually readable format
 */
int Net::LoadFile(const char *file,char **error_ret)
{
	char *error=NULL;
	int e=LoadOFF(file,&error);
	if (!e) return 0; //was a readable OFF file

	FILE *f=fopen(file,"r");
	if (!f) {
		if (error_ret) appendline(*error_ret,_("Cannot read that file."));
		return 1;
	}

	cerr << "WARNING: assuming indented format for net file "<<file<<endl;//***check!
	dump_in(f,0,NULL,NULL);

	fclose(f);

	return 0; //***what if file was not readable format?
}

//! Write out the lines of the net to SVG.
int Net::SaveSvg(const char *file,char **error_ret)
{
	if (!lines.n) return 1;

	ofstream svg(file);
	if (!svg.is_open()) return 1;

	 // Define the transformation matrix: net to paper.
	 // For simplicity, we assume that the bounds in paper hold the size of the paper,
	 // so Letter is maxx=8.5, maxy=11, and minx=miny=0.
	 //
	 // bbox shall have comparable bounds. When paper is invalid, then bbox has the same
	 // size as the net, but the origin is shifted so that point (0,0) is at a corner
	 // of the net's bounding box.
	 //
	 // The net transforms into parent space, and the paper also transforms into parent space.
	 // So the transform from net to paper space is net.m()*paper.m()^-1
	double M[6]; 
	SomeData bbox;
	if (paper.validbounds()) {
		bbox.maxx=paper.maxx;
		bbox.minx=paper.minx;
		bbox.maxy=paper.maxy;
		bbox.miny=paper.miny;
	} else {
		bbox.maxx=maxx-minx;
		bbox.minx=0;
		bbox.maxy=maxy-miny;
		bbox.miny=0;
		bbox.m(m()); //map same as the net
		bbox.origin(flatpoint(minx,miny));      //...except for the origin
	}

	double t[6];
	transform_invert(t,bbox.m());
	transform_mult(M,m(),t);

	 //figure out decent line width scaling factor. *** this is a hack!! must use linestyle stuff.
	 //In SVG, 1in = 90px = 72pt
	double linewidth=.01; //inches, where 1 inch == 1 paper unit
	double scaling=1/sqrt(M[0]*M[0]+M[1]*M[1]); //supposed to scale to within the M group
	DBG cerr <<"******--- Scaling="<<scaling<<endl;

	 //define some repeating header stuff
	char pathheader[400];
	sprintf(pathheader,"\t<path\n\t\tstyle=\"fill:none;fill-opacity:0.75;fill-rule:evenodd;stroke:#000000;stroke-width:%.6fpt;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:1.0;\"\n\t\t",scaling*linewidth);
	const char *pathclose="\n\t/>\n";
	
			
	 // Print out header
	svg << "<svg"<<endl
		<< "\twidth=\""<<bbox.maxx<<"in\"\n\theight=\""<<bbox.maxy<<"in\""<<endl
		<< "\txmlns:sodipodi=\"http://inkscape.sourceforge.net/DTD/sodipodi-0.dtd\""<<endl
		<< "\txmlns:xlink=\"http://www.w3.org/1999/xlink\""<<endl
		<<">"<<endl;
	
	 // Write matrix
	svg <<"\t<g transform=\"scale(90)\">"<<endl;
	//svg <<"\t<g >"<<endl;
	//svg <<"\t<g transform=\"matrix(1, 0, 0, 1, "<<M[4]<<','<<M[5]<<")\">"<<endl;
	svg <<"\t<g transform=\"matrix("<<M[0]<<','<<M[1]<<','<<M[2]<<','<<M[3]<<','<<M[4]<<','<<M[5]<<")\">"<<endl;


	
	 // ---------- draw lines 
	int c;
	NetLine *line;

	for (c=0; c<lines.n; c++) {
		line=lines.e[c];
		if (line->points) {
			svg << pathheader << "d=\"";
			char *d=CoordinateToSvg(line->points);
			if (d) {
				svg <<d<<"\""<< pathclose <<endl;
				delete[] d;
			}
		}
	}

	

	 // Close the net grouping
	svg <<"\t</g>\n";
	svg <<"\t</g>\n";

	 // Print out footer
	svg << "\n</svg>\n";

	return 0;
}


//----------------Net wrapping functions

//! Drop a face down, starting a new grouping of faces.
/*! basenetfacei is a face index in basenet. If -1 is passed in instead, then 
 * anchor the next basenet face that is not represented.
 *
 * Return 0 for face dropped. Return 1 for face is already dropped. 
 * Return 2 for no more faces to anchor! Other number
 * for nothing dropped.
 *
 * \todo the placement of new groups is a little flaky... should probably have ability to find
 *   bbox of each group of faces, and auto spread out so no overlap..
 */
int Net::Anchor(int basenetfacei)
{
	if (!basenet || basenetfacei>=basenet->NumFaces()) return 1;

	 //search for basenetfacei already in net
	if (faces.n && basenetfacei>=0) {
		int c=findOriginalFace(basenetfacei,1,-1,NULL);
		if (c>=0) return 1; //face already existed
	}
	 
	 //if there are faces and we want to anchor the next available, the find the next available...
	if (faces.n && basenetfacei<0) {
		for (basenetfacei=0; basenetfacei<basenet->NumFaces(); basenetfacei++) {
			if (findOriginalFace(basenetfacei,1,-1,NULL)==0) break;
		}
		if (basenetfacei==basenet->NumFaces()) return 2; //no more faces!
	}
	if (basenetfacei<0) basenetfacei=0;

	 //ok, so now we drop basenetfacei
	NetFace *newface=basenet->GetFace(basenetfacei,1);
	newface->tag=FACE_Actual;
	if (!newface->matrix) newface->matrix=new double[6];

	 //make matrix place it just outside current bbox
	transform_set(newface->matrix,1,0,0,1,(maxx>=minx?maxx:0),0);
		
	faces.push(newface,1);

	 //for each edge of the newly dropped face, add potential/already taken tags
	addPotentialsToFace(faces.n-1);

	 //remove any potential links to the newly dropped face in all the other faces
	clearPotentials(newface->original);

	if (!(_config&1)) rebuildLines();

	return 0;
}

//! Add the potential faces to net face number facenum.
/*! Returns the number of faces added.
 *
 * If an edge is tagged FACE_Actual, but there is no edge->toface then, the edge is actually
 * potential, and the potential face is added.
 */
int Net::addPotentialsToFace(int facenum)
{
	int n=0;
	for (int c=0; c<faces.e[facenum]->edges.n; c++) {
		 //skip edges already connected to something
		if (faces.e[facenum]->edges.e[c]->toface>=0) continue;
		if (faces.e[facenum]->edges.e[c]->tooriginal<0) continue; //do not connect if nothing at edge!

		 //add potentials to bare edges when that original face is not
		 //already down somewhere
		if (findOriginalFace(faces.e[facenum]->edges.e[c]->tooriginal,1,-1,NULL)==1) {
			 //face is already in the net, so mark edge as taken elsewhere
			faces.e[facenum]->edges.e[c]->tag=FACE_Taken;
		} else {
			 //to-face is not laid down yet, so connect a potential face to the new face.
			 //transform so pface is touching newface along proper edge
			NetFace *pface=basenet->GetFace(faces.e[facenum]->edges.e[c]->tooriginal,1);
			faces.e[facenum]->edges.e[c]->tag=FACE_Potential;
			pface->tag=FACE_Potential;
			faces.push(pface,1);
			connectFaces(facenum,faces.n-1,c);
			n++;
		}
	}
	return n;
}

//! Clear any potential face anywhere in the net that has the given original number.
/*! This is called only when a face has been laid down, typicall with Anchor() or Unwrap().
 * Usually the face will have been potential in several places, so the irrelevent potentials
 * need to be updated to indicate that the face is taken elsewhere.
 *
 * After calling this function, rebuildLines() should be called.
 *
 * Return 0 for nothing changed, 1 for successful change, or 2 for original was somehow invalid.
 */
int Net::clearPotentials(int original)
{
	int changed=0;
	for (int c=0; c<faces.n; c++) {
		if (!(faces.e[c]->original==original && faces.e[c]->tag==FACE_Potential)) continue;

		 //need to remove this whole face.
		 //so for each edge, make the connecting face be tagged taken, with no toface link
		for (int c2=0; c2<faces.e[c]->edges.n; c2++) {
			if (faces.e[c]->edges.e[c2]->toface>=0) {
				 //blank out connecting actual face's link to this potential face
				faces.e[faces.e[c]->edges.e[c2]->toface]
					->edges[faces.e[c]->edges.e[c2]->tofaceedge]->toface=-1;
				 //make connecting actual face's edge link say "taken"
				faces.e[faces.e[c]->edges.e[c2]->toface]
					->edges[faces.e[c]->edges.e[c2]->tofaceedge]->tag=FACE_Taken;
			}
		}
		faces.remove(c);

		 //now need to decrement by one links to faces >= c
		for (int c2=0; c2<faces.n; c2++) {
			for (int c3=0; c3<faces.e[c2]->edges.n; c3++) {
				DBG if (faces.e[c2]->edges.e[c3]->toface==c) cerr <<"***WARNING! Still had ref to deleted potential!"<<endl;

				if (faces.e[c2]->edges.e[c3]->toface>c) //hopefully no more are == c
					faces.e[c2]->edges.e[c3]->toface--;
			}
		}
		
		changed=1;
		c--; //we want counter to stay at same number after being incremented...
	}
	return changed;
}

//! Connect net face f1 to net face f2 along edge ee of f1.
/*! If ee<0 then autodetect the edge.
 *
 * This will change face f2's matrix so that it lies along the proper edge of 
 * face f1. f1 will not be transformed at all.
 *
 * Return 0 for success, nonzero for error, and nothing changed.
 *
 * \todo *** implement bezier edge connecting via svalue.
 * \todo *** right now assumes face normals all stick up, ignores flipflag
 */
int	Net::connectFaces(int f1,int f2,int ee)
{
	if (f1<0 || f1>=faces.n) return 1;
	if (f2<0 || f2>=faces.n) return 2;
	if (ee>=faces.e[f1]->edges.n) return 3;

	NetFace *from=faces.e[f1],
			*to=faces.e[f2];

	 //if necessary, autodetect the correct edge to place the face
	if (ee<0) {
		for (int c=0; c<from->edges.n; c++) {
			if (from->edges.e[c]->tooriginal==to->original) {
				ee=c;
				break; 
			}
		}
	}

	int e2=from->edges.e[ee]->tofaceedge; //the edge number in f2
	flatpoint p1 =from->edges.e[ee]->points->fp,
			  p2 =from->edges.e[(ee+1)%from->edges.n]->points->fp,
			  pt2=  to->edges.e[e2]->points->fp,
			  pt1=  to->edges.e[(e2+1)%to->edges.n]->points->fp;

	from->edges.e[ee]->tag   =to->tag;
	to  ->edges.e[e2]->tag   =from->tag;
	from->edges.e[ee]->toface=f2;
	to  ->edges.e[e2]->toface=f1;

	if (!from->matrix) from->matrix=transform_identity(NULL);
	if (!  to->matrix)   to->matrix=transform_identity(NULL);

	 //connect pt1 -> p1,  pt2 -> p2
	p1 =transform_point(from->matrix,p1);
	p2 =transform_point(from->matrix,p2);
	pt1=transform_point(  to->matrix,pt1);
	pt2=transform_point(  to->matrix,pt2);
	
	 // pt1*tm --> T --> p1*fm
	 // pt2*tm --> T --> p2*fm
	 //
	 //   [ a b 0 ]
	 // T=[ c d 0 ] --> [a,b,c,d,e,f]
	 //   [ e f 1 ]
	
	 //transform
	double a,b,c,d,e,f; //c=-b, a=d
	double q1x,q1y,q2x,q2y, p1x,p1y,p2x,p2y, dd;
	q1x=p1.x;
	q1y=p1.y;
	q2x=p2.x;
	q2y=p2.y;
	p1x=pt1.x;
	p1y=pt1.y;
	p2x=pt2.x;
	p2y=pt2.y;
	
	dd=(p1x*p1x-p1x*p2x-p1y*p2y+p2x*p2x+p1y*p1y-p1y*p2y-p1x*p2x+p2y*p2y);
	a=(q1x*(p1x-p2x)+q1y*(p1y-p2y)+q2x*(p2x-p1x)+q2y*(p2y-p1y))/dd;
	b=(q1x*(p2y-p1y)+q1y*(p1x-p2x)+q2x*(p1y-p2y)+q2y*(p2x-p1x))/dd;
	c=-b;
	d=a;
	e=(q1x*(p2x*p2x-p1y*p2y-p1x*p2x+p2y*p2y)+q1y*(p1x*p2y-p1y*p2x)+q2x*(p1x*p1x-p1x*p2x+p1y*p1y-p1y*p2y)+q2y*(p1y*p2x-p1x*p2y))/dd;
	f=(q1x*(p1y*p2x-p1x*p2y)+q1y*(p2x*p2x-p1x*p2x-p1y*p2y+p2y*p2y)+q2x*(p1x*p2y-p1y*p2x)+q2y*(p1x*p1x-p1y*p2y+p1y*p1y-p1x*p2x))/dd;

	double m[6];
	m[0]=a;
	m[1]=b;
	m[2]=c;
	m[3]=d;
	m[4]=e;
	m[5]=f;

	double mm[6];
	transform_mult(mm,to->matrix,m);
	transform_copy(to->matrix,mm);

	return 0;
}

//! For each edge of each face, make sure edge tag for potential/already-taken is set properly.
/*! This will add net faces as necessary.
 *
 * This is called after a face is dropped down.
 *
 * \todo this should be more efficient
 */
int Net::validateNet()
{
	return 0;
//	NumStack<int> taken;
//	int c;
//	for (c=0; c<faces.n; c++) 
//		if (faces.e[c]->original>=0) taken.pushnodup(faces.e[c]->original);
//	for (c=0; c<faces.n; c++) {
//		if (faces.e[c]->tag==FACE_Potential && taken.findindex(faces.e[c]->original>=0)) {
//			***remove this potential face:
//				1. make any edge in any face that potentially points to this net face be AlreadyTaken
//				2. pop from stack
//				3. decrement index references >= this face's previous position
//		}
//		for (int c2=0; c2<faces.e[c]->edges.n; c2++) {
//			***
//		}
//	}
}

//! Reconstruct the stack of lines as necessary.
/*! Lines that are tagged as automatically created will be deleted and replaced with new lines.
 * This function is usually called after laying down or picking up faces.
 *
 *
 * \todo *** FIXME! is lazy lines right now, poly line around every face...
 */
int Net::rebuildLines()
{
	cerr <<" ***need to do real implementation of Net::rebuildLines()..."<<endl;
	 // remove auto lines from lines stack...
	for (int c=lines.n-1; c>=0; c--) {
		if (lines.e[c]->lineinfo==0) lines.remove(c);
	}
	
	Coordinate *p,*nl,*nlp;
	NetLine *l;
	for (int c=0; c<faces.n; c++) {
		if (faces.e[c]->tag!=FACE_Actual) continue;

		nl=NULL;
		for (int c2=0; c2<faces.e[c]->edges.n; c2++) {
			p=faces.e[c]->edges.e[c2]->points;
			while (p) {
				if (!nl) { nlp=nl=new Coordinate(); }
				else { nlp->next=new Coordinate(); nlp->next->prev=nlp; nlp=nlp->next; }

				if (faces.e[c]->matrix) nlp->fp=transform_point(faces.e[c]->matrix,p->fp);
				else nlp->fp=p->fp;

				p=p->next; 
			}
		}
		nlp->next=nl;
		nl->prev=nlp;
		l=new NetLine();
		l->points=nl;
		l->lineinfo=0; //means autogenerated
		lines.push(l,1);
	}
	return 0;
}

//! Find the net face index for an original face index of i.
/*! 
 * status==1 means return 1 for actual face found, else 0.
 * status==2 means return 2 for potential face found, else 0.
 * status==(any other number) means return 0 for face not found, 1 for actual face found,
 * and 2 for potential face found.
 *
 * If startsearchhere>=0, then start lookind at net faces from net face index startsearchhere.
 *
 */
int Net::findOriginalFace(int i,int status,int startsearchhere, int *index_ret)
{
	int c;
	for (c=(startsearchhere>=0?startsearchhere:0); c<faces.n; c++) {
		if (faces.e[c]->original==i) {
			if (status!=2 && faces.e[c]->tag==FACE_Actual) { if (index_ret) *index_ret=c; return 1; }
			else if (status!=1 && faces.e[c]->tag==FACE_Potential) { if (index_ret) *index_ret=c; return 2; }
		}
	}
	if (index_ret) *index_ret=-1;
	return 0;
}

//! Drop down the face connected to net index netfacei, edge number atedge.
/*! If netfacei==-1, and ategde<0 then completly unwrap the whole of basenet.
 *  If netfacei==-1, and ategde>=0 then drop original face with index atedge if
 *  it has not already been dropped. If the face is already in the net, then nothing
 *  is done and 0 is returned.
 *
 * If netfacei>=0 and atedge==-1, the drop down all faces connected to netfacei.
 *
 * If netfacei>=0 and atedge>=0, then drop down only the face connected to edge number 
 * atedge of net nace number netfacei.
 *
 * Return 0 for success. Nonzero for error.
 *
 * \todo *** for dropping fresh faces, should do a bounding box check so no overlap
 *   with existing faces
 */
int Net::Unwrap(int netfacei,int atedge)
{
	if (!basenet) return 1;
	DBG cerr <<"Net::Unwrap netfacei:"<<netfacei<<"  atedge:"<<atedge<<endl;
	if (netfacei<0) {
		 //unwrap all or drop a new seed face

		if (atedge<0) { TotalUnwrap(); return 0; }
		if (atedge>=basenet->NumFaces()) return 2;
		if (findOriginalFace(atedge,1,0,NULL)==1) return 0;
		 
		 //drop down original face atedge, starting new group
		 //***this does not properly adjust potentials...
		NetFace *face=basenet->GetFace(atedge,1);
		face->tag=FACE_Actual;
		faces.push(face,1);
		addPotentialsToFace(faces.n-1);

		//DBG cerr <<"------------------unwrap first--"<<endl;
		//DBG dump_out(stderr,0,0,NULL);
		//DBG cerr <<"--------------------------------"<<endl;

		DBG cerr <<"...done unwrap 1"<<endl;
		return 0;
	}
	if (netfacei>=faces.n) return 3;


	NetFace *f1=faces.e[netfacei],
			*f2=NULL;
	int s, e; //the first and last edges to unwrap 
	if (atedge>=0) s=e=atedge; else { s=0; e=f1->edges.n-1; }

	if (s<0 || s>=f1->edges.n || e<0 || e>=f1->edges.n) return 3;

	int changed=0;
	for (int c=s; c<e+1; c++) { //for each edge, drop a face that is potential
		atedge=c;
		if (f1->edges.e[c]->tag!=FACE_Potential) {
			DBG cerr <<"skipping edge "<<c<<", tag=="<<tagname[f1->edges.e[c]->tag]<<endl;
			continue;
		}

		 // drop down that face
		f2=faces.e[f1->edges.e[c]->toface];
		f2->tag=FACE_Actual;
		f1->edges.e[c]->tag=FACE_Actual;
		clearPotentials(f2->original);
		addPotentialsToFace(faces.findindex(f2));
		changed=1;
	}

	if (changed && !(_config&1)) rebuildLines();

	//--------------------
	DBG if (!f2) {
	DBG 	DBG cerr <<"WARNING!! missing a dropped face!!!"<<endl;
	DBG } else {
	DBG   for (int c=0; c<f2->edges.n; c++) {
	DBG 	cerr <<"Net::Unwrap a face, edge "<<c<<": tag="<<tagname[f2->edges.e[c]->tag]
	DBG 		<<"  toface:"<<f2->edges.e[c]->toface
	DBG 		<<"  tooriginal:"<<f2->edges.e[c]->tooriginal<<endl;
	DBG   }
	DBG }
	//--------------------

	DBG cerr <<"...done unwrap 2"<<endl;
	return 0;
}

//! For any edges with potential faces, unwrap there, until all is unwrapped.
int Net::TotalUnwrap()
{
	if (!faces.n) Unwrap(-1,0);
	if (!faces.n) return 1;
	do {
		for (int c=0; c<faces.n; c++) {
			if (faces.e[c]->tag!=FACE_Actual) continue;
			for (int c2=0; c2<faces.e[c]->edges.n; c2++) {
				if (faces.e[c]->edges.e[c2]->tag==FACE_Potential) Unwrap(c,c2);
			}

		}
		break;
	} while (1);
	FindBBox();
	return 0;
}

//! Convert a potential net face with index netfacei to an actual one.
/*! Return 0 for success, or nonzero for error and nothing changed.
 *
 * If the face is already tagged with FACE_Actual, then nothing is done,
 * and 2 is returned.
 */
int Net::Drop(int netfacei)
{
	if (netfacei<0 || netfacei>=faces.n) return 1;
	if (faces.e[netfacei]->tag==FACE_Actual) return 2;

	NetFace *netf=faces.e[netfacei];

	 //change the face's tag to actual
	netf->tag=FACE_Actual;

	 //make any edge pointing to this face be actual
	for (int c=0; c<netf->edges.n; c++) {
		if (netf->edges.e[c]->toface>=0) 
			faces.e[netf->edges.e[c]->toface]->edges.e[netf->edges.e[c]->tofaceedge]->tag=FACE_Actual;
	}

	 //reconfigure rest of net to be consistent
	clearPotentials(netf->original);
	addPotentialsToFace(faces.findindex(netf));

	if (!(_config&1)) rebuildLines();
	return 0;
}

//! Pick up net face number netfacei.
/*! The easy case is when netfacei connects to only one other actual face. In that case, netfacei
 * becomes a potential face, rather than an actual one, and cutatedge is ignored.
 *
 * If netfacei connects to more than one other face, then edge number cutatedge of the face netfacei
 * is used as a hint about which group of faces to keep. All faces connected to netfacei EXCEPT the 
 * one on the other side of edge cutatedge are pulled up. If faces happen to make a loop including 
 * the faces on both sides of cutadedge, then usually only netfacei is picked up, potentially creating
 * several disconnected groups of faces. (****note that this is not implemented!!)
 *
 * Return 0 for success, nonzero for error.
 *
 * \todo fully implement this!!
 */
int Net::PickUp(int netfacei,int cutatedge)
{
	if (netfacei<0 || netfacei>=faces.n) return 1;
	int total=0,n=0,c;
	NetFace *netf=faces.e[netfacei];

	 //figure out how many faces are set up touching this one
	for (c=0; c<netf->edges.n; c++) {
		if (netf->edges.e[c]->toface>=0) {
			total++;
			if (netf->edges.e[c]->tag==FACE_Actual) n++;
		}
	}
	DBG cerr <<"face "<<netfacei<<" connected to "<<n<<" actual faces, to "<<total<<" total faces"<<endl;
	if (n>1) return 2;

	if (n==0) {
		 //netfacei is standing alone, with possibly several potentials connected to it
		//***remove all those!
		//deleteFace(netfacei);
		return 1;
	}

	 //face is connected to 1 actual face, and possible many potentials
	 //***for now assume any potentials connected to it only connect with this one

	 //set this face to potential, whatever it was before, and delete any potentials
	 //connected to it
	netf->tag=FACE_Potential;
	for (int c=0; c<netf->edges.n; c++) {
		if (netf->edges.e[c]->toface>=0) {
			 //for actual faces, only change its edge tag
			if (netf->edges.e[c]->tag==FACE_Actual) { 
				faces.e[netf->edges[c]->toface]->edges.e[netf->edges[c]->tofaceedge]->tag=FACE_Potential;
				continue;
			}
			if (netf->edges.e[c]->tag==FACE_Potential)
				deleteFace(netf->edges.e[c]->toface);		
		}
	}
	 
	 //for each face that points to netf->original, add a potential if necessary
	 //the face edges could have had FACE_Taken
	for (int c=0; c<faces.n; c++) {
		if (faces.e[c]->tag!=FACE_Actual) continue;
		for (int c2=0; c2<faces.e[c]->edges.n; c2++) {
			if (faces.e[c]->edges.e[c2]->tooriginal==netf->original
					&& faces.e[c]->edges.e[c2]->toface<0) {
				
				NetFace *pface=basenet->GetFace(netf->original,1);
				faces.e[c]->edges.e[c2]->tag=FACE_Potential;
				pface->tag=FACE_Potential;
				faces.push(pface,1);
				connectFaces(c,faces.n-1,c2);
			}
		}
	}

	return 0;
}

//! Delete the face with net index netfacei.
/*! This simply removes the face, and updates all the indices in the edge info
 * of all the faces as necessary. It does NOT update potentials.
 *
 * Return 0 for face deleted, nonzero for error, and face not deleted.
 */
int Net::deleteFace(int netfacei)
{
	if (netfacei<0 || netfacei>=faces.n) return 1;
	DBG cerr <<"-----deleteFace("<<netfacei<<")"<<endl;

	faces.remove(netfacei);

	 //now need to decrement by one links to faces >= netfacei
	for (int c2=0; c2<faces.n; c2++) {
		for (int c3=0; c3<faces.e[c2]->edges.n; c3++) {
			if (faces.e[c2]->edges.e[c3]->toface>netfacei) { //hopefully no more are == c
				faces.e[c2]->edges.e[c3]->toface--;
			} else if (faces.e[c2]->edges.e[c3]->toface==netfacei) {
				//DBG cerr <<"**** warning! face("<<c2<<")->edge("<<c3<<") == face being deleted!!"<<endl;
				faces.e[c2]->edges.e[c3]->toface=-1;
			}
		}
	}
	DBG cerr <<"-----deleteFace done"<<endl;
	return 0;
}

