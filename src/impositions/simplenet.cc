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
// Copyright (C) 2004-2013 by Tom Lechner
//


#include <lax/strmanip.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>

#include "../language.h"
#include "simplenet.h"

#include <iostream>
using namespace std;
#define DBG 
using namespace Laxkit;
using namespace LaxInterfaces;
using namespace Polyptych;


namespace Laidout {


//--------------------------------------- SimpleNetLine -------------------------------------------
/*! \class SimpleNetLine
 * \brief Class to hold extra lines for use in SimpleNet.
 */


SimpleNetLine::SimpleNetLine(const char *list)
{
	linestyle=NULL;
	isclosed=0;
	np=0;
	points=NULL;
	if (list) Set(list);
}

SimpleNetLine::~SimpleNetLine()
{
	if (points) delete[] points;
	if (linestyle) linestyle->dec_count();
}

//! Set up points like "0 1 2 3 ... n", and make closed if closed!=0.
/*! Returns the number of points.
 */
int SimpleNetLine::Set(int n, int closed)
{
	if (points) delete[] points;
	points=new int[n];
	np=n;
	for (int c=0; c<n; c++) points[c]=c;
	isclosed=closed;
	return n;
}

//! Create points from list like: "1 2 3", or "1 2 3 1" for a closed path.
/*! Returns the number of points.
 */
int SimpleNetLine::Set(const char *list)
{
	if (points) delete[] points;
	IntListAttribute(list,&points,&np);
	if (np && points[np-1]==points[0]) {
		np--;
		isclosed=1;
	}
	return np;
}

/*! Copies over all. Warning: does a linestyle=line.linestyle.
 */
const SimpleNetLine &SimpleNetLine::operator=(const SimpleNetLine &line)
{
	isclosed=line.isclosed;
	np=line.np;
	
	if (linestyle && line.linestyle) *linestyle=*line.linestyle;//***this is maybe problematic
	
	if (points) delete[] points;
	points=new int[np];
	for (int c=0; c<np; c++) points[c]=line.points[c];

	return *this;
}

/*! If pfirst!=0, then immediately output the list of points.
 * Otherwise, do points 3 5 6 6...
 */
void SimpleNetLine::dump_out(FILE *f,int indent, int pfirst)//pfirst=0
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (!pfirst) fprintf(f,"%spoints ",spc);
	int c;
	for (c=0; c<np; c++) fprintf(f,"%d ",points[c]);
	fprintf(f,"\n");
	if (isclosed) fprintf(f,"%sclosed\n",spc);
	if (linestyle) {
		fprintf(f,"%slinestyle\n",spc);
		linestyle->dump_out(f,indent+2,0,NULL);
	}
}

/*! If val!=NULL, then the att was something like:
 * <pre>
 *   line 1 2 3
 *     (other stuff)..
 * </pre>
 * In that case, SimpleNet would have parsed the "1 2 3", and it would pass
 * that here in val. Otherwise, this function will expect a 
 * "points 1 2 3" sub attribute somewhere in att.
 *
 * If there's a list "1 2 3 1" it will make a point list 1,2,3, and
 * set isclosed=1.
 */
void SimpleNetLine::dump_in_atts(Laxkit::Attribute *att, const char *val,int flag)
{
	if (!att) return;
	int c;
	if (val) {
		if (points) delete[] points;
		points=NULL;
		c=IntListAttribute(val,&points,&np);
	}
	char *name,*value;
	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"points")) {
			if (points) delete[] points;
			points=NULL;
			IntListAttribute(val,&points,&np);
			if (np && points[np-1]==points[0]) { np--; isclosed=1; }
		} else if (!strcmp(name,"linestyle")) {
			if (!linestyle) {
				linestyle=new LineStyle();
			}
			linestyle->dump_in_atts(att->attributes.e[c],flag,NULL);
		} else if (!strcmp(name,"closed")) {
			isclosed=BooleanAttribute(value);
		}
	}
}

//--------------------------------------- SimpleNetFace -------------------------------------------
/*! \class SimpleNetFace
 * \brief Class to hold info about a face in a SimpleNet.
 */
/*! \var int *SimpleNetFace::points
 * \brief List of indices into SimpleNet::points for which points define the net.
 *
 * If positive x is to the right and positive y is up, the points should be
 * such that the inside of the face is on the left. That is, interior points
 * are those that get circled in a counterclockwise direction.
 */
/*! \var int *SimpleNetFace::facelink
 * \brief List of indices into SimpleNet::faces for which edges connect to which faces.
 *
 * That is, connected by virtue of SimpleNet::pointmap, not by which faces touch in the
 * flattened net.
 */
/*! \var int SimpleNetFace::aligno
 * An index into the face's point list.
 * The default is for the face's basis to have the origin at the first listed point,
 * and the x axis lies along the vector going from the first point to the second point.
 * If aligno>=0, then use that point as the origin, and if alignx>=0, use point alignx
 * as where the x axis should point to from aligno. If m is specified, then that is
 * an extra 6 member affine transform matrix to apply to the basis.
 */
/*! \var int SimpleNetFace::alignx
 * \brief See aligno.
 */
/*! \var double *SimpleNetFace::m
 * \brief See aligno.
 */ 
/*! \var int SimpleNetFace::faceclass
 * \brief If >=0, then this face should look the same as others in the same class.
 */


SimpleNetFace::SimpleNetFace()
{
	m=NULL;
	aligno=alignx=-1;
	faceclass=-1;
	np=0;
	points=facelink=NULL; 
}
	
SimpleNetFace::~SimpleNetFace()
{ 
	if (points) delete[] points;
	if (facelink) delete[] facelink;
	if (m) delete[] m; 
}

//! Delete m, points, and facelink, set align stuff to -1.
void SimpleNetFace::clear()
{
	if (points) { delete[] points; points=NULL; }
	if (facelink) { delete[] facelink; facelink=NULL; }
	if (m) { delete[] m; m=NULL; }
	np=0;
	aligno=alignx=faceclass=-1;
}

//! Assignment operator, straightforward copy all.
const SimpleNetFace &SimpleNetFace::operator=(const SimpleNetFace &face)
{
	if (points) delete[] points;
	if (facelink) delete[] facelink;
	if (m) { delete[] m; m=NULL; }

	np=face.np;
	aligno=face.aligno;
	alignx=face.alignx;
	faceclass=face.faceclass;
	if (face.m) {
		m=new double[6];
		transform_copy(m,face.m);
	}
	if (np) {
		points=new int[np];
		facelink=new int[np];
		for (int c=0; c<np; c++) {
			points[c]=face.points[c];
			if (face.facelink) facelink[c]=face.facelink[c]; else facelink[c]=-1;
		}
	} else {
		points=NULL;
		facelink=NULL;
	}
	return *this;
}

//! Create points and facelink from list like: "1 2 3".
/*! Returns the number of points.
 *
 * If list and link have differing numbers of elements, link is
 * removed and replaced with a list of -1.
 */
int SimpleNetFace::Set(const char *list, const char *link)
{
	if (list && points) { delete[] points; points=NULL; }
	if (link && facelink) { delete[] facelink; facelink=NULL; }
	int n;
	if (link) IntListAttribute(link,&facelink,&n);
	else n=np;
	if (list) IntListAttribute(list,&points,&np);
	if (n!=np) {
		if (facelink) { delete[] facelink; facelink=NULL; }
		facelink=new int[np];
		for (n=0; n<np; n++) facelink[n]=-1;
	}
	return np;
}

//! Set points and facelink from an integer list of length nn.
/*! If dellists!=0, then take possession of the list and link arrays.
 * Otherwise, copy list and link.
 *
 * Returns the number of points.
 */
int SimpleNetFace::Set(int n,int *list,int *link,int dellists)//dellists=0
{
	if (list && points) { delete[] points; points=NULL; }
	if (link && facelink) { delete[] facelink; facelink=NULL; }
	
	 // establish new points
	if (dellists) points=list;
	else {
		points=new int[n];
		memcpy(points,list,n*sizeof(int));
	}
	 // establish new links
	if (dellists) facelink=link;
	else {
		facelink=new int[n];
		memcpy(facelink,link,n*sizeof(int));
	}
	np=n;

	return np;
}

/*! If pfirst!=0, then immediately output the list of points.
 * Otherwise, do points 3 5 6 6...
 *
 * See SimpleNet::dump_out() for what gets put out.
 */
void SimpleNetFace::dump_out(FILE *f,int indent, int pfirst)//pfirst=0
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (!pfirst) fprintf(f,"%spoints ",spc);
	int c;
	for (c=0; c<np; c++) fprintf(f,"%d ",points[c]);
	fprintf(f,"\n");
	if (facelink) {
		for (c=0; c<np; c++) if (facelink[c]!=-1) break;
		if (c!=np) {
			fprintf(f,"%sfacelink",spc);
			for (c=0; c<np; c++) fprintf(f," %d",facelink[c]);
			fprintf(f,"\n");
		}
	}
	if (faceclass>=0) fprintf(f,"%sfaceclass %d\n",spc,faceclass);
	if (aligno>=0) {
		fprintf(f,"%salign %d",spc,aligno);
		if (alignx>=0) fprintf(f," %d",alignx);
		fprintf(f,"\n");
	}
	if (m) fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
				spc,m[0],m[1],m[2],m[3],m[4],m[5]);
}

/*! If val!=NULL, then the att was something like:
 * <pre>
 *   face 1 2 3
 *     (other stuff)..
 * </pre>
 * In that case, SimpleNet would have parsed the "1 2 3", and it would pass
 * that here in val. Otherwise, this function will expect a 
 * "points 1 2 3" sub attribute somewhere in att.
 */
void SimpleNetFace::dump_in_atts(Laxkit::Attribute *att, const char *val,int flag)//val=NULL
{
	if (!att) return;
	int c,n=0;
	if (val) {
		if (points) delete[] points;
		points=NULL;
		c=IntListAttribute(val,&points,&np);
	}
	char *name,*value;
	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"matrix")) {
			DoubleListAttribute(value,m,6);
		} else if (!strcmp(name,"facelink")) {
			if (facelink) { delete[] facelink; facelink=NULL; }
			IntListAttribute(value,&facelink,&n);
			if (n!=np) {
				for (int c2=n; c2<np; c2++) facelink[c2]=-1;
			}
		} else if (!strcmp(name,"faceclass")) {
			IntAttribute(value,&faceclass);
		} else if (!strcmp(name,"aligno")) {
			IntAttribute(value,&aligno);
		} else if (!strcmp(name,"alignx")) {
			IntAttribute(value,&alignx);
		} else if (!strcmp(name,"points")) {
			if (points) delete[] points;
			points=NULL;
			IntListAttribute(val,&points,&np);
		}
	}
}

//--------------------------------------- SimpleNet -------------------------------------------
/*! \class SimpleNet
 * \brief A type of SomeData that stores polyhedron cut and fold patterns.
 *
 * This is used by NetImposition. 
 * 
 * Lines will be drawn, using only those coordinates from points.
 * Tabs are drawn on alternating outline point, or as specified.
 * 
 * net->m() transforms a net->point to the space that contains the net.
 * net->basisOfFace() would transform a point within that face to the net
 * coordinate system. basisOfFace()*m() would transform a face point to
 * the space that contains net.
 * 
 * \todo *** implement tabs
 */ 
/*! \var SimpleNetLine *SimpleNet::lines
 * \brief The lines that make up what the net looks like.
 *
 * Line 0 is assumed to be the outline of the whole net. Having lines specified with these
 * makes it so edge lines are not drawn twice, which is what would happen if the net was
 * drawn simply by outlining the faces.
 *
 * \todo ***actually this might be bad, should perhaps have flag saying that some line is
 * an outline. Tabs get tacked onto outlines, but multiple outlines should be allowed.
 */
/*! \var int SimpleNet::tabs
 * <pre>
 *  0  no tabs (default)
 *  1  tabs alternating every other (even)
 *  2  tabs alternating the other every other (odd)
 *  3  tabs on all edges (all or yes)
 * </pre>
 */
/*! \var spacepoint *SimpleNet::vertices
 * \brief Optional list of 3-d points. See also the pointmap list.
 */
/*! \var int SimpleNet::nvertices
 * \brief The number of elements in the vertices array.
 */
/*! \var int *SimpleNet::pointmap;
 * \brief Which thing (possibly 3-d points) corresponding (2-d) point maps to.
 *
 * If there is a vertices array, then the values in pointmap are assumed to be
 * indices into the vertices array. Those vertices can be used to map a 
 * polyhedron net into a spherical texture map and vice versa.
 */


//! Init.
SimpleNet::SimpleNet()
{
	filename=NULL;
	np=nl=nf=nvertices=0;
	points=NULL;
	pointmap=NULL;
	vertices=NULL;
	lines=NULL;
	faces=NULL;
	tabs=0;
	thenettype=newstr("SimpleNet");
}

//! Delete points,lines,faces,pointmap,thenettype.
SimpleNet::~SimpleNet()
{	clear(); }

void SimpleNet::clear()
{
	if (thenettype) { delete[] thenettype; thenettype=NULL; }
	if (filename) { delete[] filename; filename=NULL; }
	
	if (points) {
		delete[] points;
		delete[] pointmap;
		points=NULL;
		pointmap=NULL;
		np=0;
	}
	if (lines) {
		delete[] lines;
		lines=NULL;
		nl=0;
	}
	if (faces) {
		delete[] faces;
		faces=NULL;
		nf=0;
	}
	if (vertices) {
		delete[] vertices;
		vertices=NULL;
		nvertices=0;
	}
}

//! Return a new copy of this.
SimpleNet *SimpleNet::duplicate()
{
	SimpleNet *net=new SimpleNet;
	makestr(net->thenettype,thenettype);
	net->np=np;
	net->tabs=tabs;
	if (vertices) {
		net->vertices = new spacepoint[nvertices];
		net->nvertices = nvertices;
		//memcpy(net->vertices,vertices,nvertices * sizeof(spacepoint));
		for (int c=0; c<nvertices; c++) net->vertices[c] = vertices[c];
	}
	if (np) {
		net->pointmap=new int[np];
		net->points=new flatpoint[np];
		for (int c=0; c<np; c++) {
			if (pointmap) net->pointmap[c]=pointmap[c];
			else net->pointmap[c]=-1;
			net->points[c]=points[c];
		}
	}
	if (nf) {
		net->nf=nf;
		net->faces=new SimpleNetFace[nf];
		for (int c=0; c<nf; c++) {
			net->faces[c]=faces[c];
		}
	}
	if (nl) {
		net->nl=nl;
		net->lines=new SimpleNetLine[nl];
		for (int c=0; c<nl; c++) {
			net->lines[c]=lines[c];
		}
	}
	net->m(m());
	net->FindBBox();
	return net;
}

/*! perhaps:
 * <pre>
 * name "Square split diagonally"
 * matrix 1 0 0 1 0 0  # optional extra matrix to map this net to a page
 * points \
 *    1 1   to 3  # 0,  the optional 'to number' is map to whatever, goes in pointmap
 *    -1 1  to 2  # 1   it can be used to point to original 3-d points, for instance.
 *    -1 -1 to 0  # 2   pointmap is also used to build facelink in SimpleNetFace
 *    1 -1  to 1  # 3
 * vertices \     \#list of 3d points
 *    0  0  0   #0
 *    1  0  0   #1
 *    0  1  0   #2
 *    .5 .5 .7  #3
 *    
 *  # All the lines to draw when laying out on the page. Numbers are
 *  # indices into the point list above.
 *  # The line marked with 'outline' internally becomes lines[0].
 *  # If no outline attribute is given, then the point list, in the order
 *  # as given is assumed to be the outline.
 * outline 0 1 2 3
 *    linestyle
 *       color 255 25 25 255
 * line 1 2 3
 *    facelink 0 1 2 # edges link to other faces
 *    linestyle
 *       color 100 100 50 255
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
 */
void SimpleNet::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sname Bent Square #this is just any name you give the net\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  # transform to map the net to a paper\n",spc);
		fprintf(f,"%stabs no             #(***TODO) whether to put tabs on face edges\n",spc);
		fprintf(f,"%spoints \\ \n",spc);
		fprintf(f,"%s  1 1   to 0  # 0, the optional 'to number' is map to some common index, usually\n",spc);
		fprintf(f,"%s  -1 1  to 1  # 1   the 3-d vertex index of the vertices list below. These let the\n",spc);
		fprintf(f,"%s  -1 -1 to 2  # 2   interactive net unwrapper actually work\n",spc);
		fprintf(f,"%s  1 -1  to 3  # 3\n",spc);
		fprintf(f,"%svertices \\       #optional list of 3d points\n",spc);
		fprintf(f,"%s  0  0  0   #0\n",spc);
		fprintf(f,"%s  1  0  0   #1\n",spc);
		fprintf(f,"%s  0  1  0   #2\n",spc);
		fprintf(f,"%s  .5 .5 .7  #3\n",spc);
		fprintf(f,"%sface 0 1 3       #defines a face by list of indices into the points list.\n",spc);
		fprintf(f,"%sface 2 3 1       # The origin is by default at the first point, and the\n",spc);
		fprintf(f,"%s                 # x axis extends to the next point.\n",spc);
		fprintf(f,"%sline 0 3         #defines a line made from the points list that gets drawn onscreen\n",spc);
		fprintf(f,"%soutline 0 1 2 3  #defines a (closed) outline, onto which tabs can be placed\n",spc);
		
		return;
	}
	int c;
	fprintf(f,"%sname %s\n",spc,whatshape());
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,m((int)0),m(1),m(2),m(3),m(4),m(5));
	
	if (tabs==0) fprintf(f,"%stabs no\n",spc);
	else if (tabs==1) fprintf(f,"%stabs even\n",spc);
	else if (tabs==2) fprintf(f,"%stabs odd\n",spc);
	else if (tabs==3) fprintf(f,"%stabs all\n",spc);
	
	 // dump points
	if (np) {
		fprintf(f,"%spoints \\\n",spc);
		for (c=0; c<np; c++) {
			fprintf(f,"%s  %-13.10g %-13.10g ",spc,points[c].x,points[c].y);
			if (pointmap && pointmap[c]>=0) fprintf(f,"to %d ",pointmap[c]);
			fprintf(f,"# %d\n",c);
		}
		fprintf(f,"\n");
	}
	
	 // dump 3-d points
	if (nvertices) {
		fprintf(f,"%svertices \\\n",spc);
		for (c=0; c<nvertices; c++) {
			fprintf(f,"%s  %-13.10g %-13.10g %-13.10g # %d\n",spc,vertices[c].x,vertices[c].y,vertices[c].z,c);
		}
		fprintf(f,"\n");
	}
	
	 // dump lines
	if (nl) {
		for (c=0; c<nl; c++) {
			if (c==0) fprintf(f,"%soutline ",spc);
			else fprintf(f,"%sline ",spc);
			lines[c].dump_out(f,indent+2,1);
		}
	}
	
	 // dump faces
	if (nf) {
		for (c=0; c<nf; c++) {
			fprintf(f,"%sface ",spc);
			faces[c].dump_out(f,indent+2,1);
		}
	}
}

//! Set up net from a Laxkit::Attribute.
/*! If there is no 'outline' attribute, then it is assumed that
 * the list of points in order is the outline.
 * 
 * \todo *** MUST implement the sanity check..
 */
void SimpleNet::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;
	char *name,*value,*t,*e,*newname=NULL;
	double x,y;
	int pm,hadoutline=0;
	int c;
	for (c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"name")) {
			makestr(newname,value);
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
		} else if (!strcmp(name,"tab")) {
			cout <<" *** tabs not implemented in SimpleNet!"<<endl;
		} else if (!strcmp(name,"points")) {
			 // parse arbitrarily long list of 
			 //   1.223432 3.2342 to 2
			 //   ...
			t=value;
			while (t && *t) {
				pm=-1;
				x=strtod(t,&e); 
				if (e==t) break;
				t=e;
				y=strtod(t,&e); 
				if (e==t) break;
				t=e;
				while (isspace(*t) && *t!='\n') t++;
				if (t[0]=='t' && t[1]=='o' && isspace(t[2])) {
					pm=strtol(t+2,&e,10);
					if (e==t) break; // broken file
					t=e;
				}
				pushpoint(flatpoint(x,y),pm);
			}
		} else if (!strcmp(name,"vertices")) {
			 // parse arbitrarily long list of 3-d points
			 //   1.2 1.5 -1.8 \n ...
			if (nvertices) {
				delete[] vertices;
				vertices=NULL;
				nvertices=0;
			}
			int n;
			double p3[3];
			t=e=value;
			while (t && *t) {
				n=DoubleListAttribute(t,p3,3,&e);
				if (e==t) break;
				if (n!=3) { t=e; continue; }
				while (*e && *e!='\n') e++;
				t=*e?e+1:NULL;
				push3dpoint(p3[0],p3[1],p3[2]);
			}
		} else if (!strcmp(name,"outline")) {
			hadoutline=1;
			SimpleNetLine netline;
			netline.dump_in_atts(att->attributes.e[c],value,flag);
			pushline(netline,0); // pushes onto position 0
		} else if (!strcmp(name,"line")) {
			SimpleNetLine netline;
			netline.dump_in_atts(att->attributes.e[c],value,flag);
			pushline(netline,-1); // pushes onto top
		} else if (!strcmp(name,"face")) {
			SimpleNetFace netface;
			netface.dump_in_atts(att->attributes.e[c],value,flag);
			pushface(netface);
		}
	}

	 // if no outline, then assume list of points is the outline.
	if (!hadoutline) {
		SimpleNetLine line;
		line.isclosed=1;
		line.points=new int[np];
		line.np=np;
		for (c=0; c<np; c++) line.points[c]=c;	
		pushline(line,0);
	}
	
	//***sanity check on all point references..
	FindBBox();

	//DBG cerr <<"----------------this was set in SimpleNet:-------------"<<endl;
	//DBG dump_out(stderr,0,0);
	//DBG cerr <<"----------------end SimpleNet dump:-------------"<<endl;
}

//! Add a 3-d point to vertices at the end.
void SimpleNet::push3dpoint(double x,double y,double z)
{
	spacepoint *npts=new spacepoint[nvertices];
	if (vertices) {
		memcpy((void *)npts,(const void *)vertices, np*sizeof(spacepoint));
		delete[] vertices;
	}
	npts[nvertices].x=x;
	npts[nvertices].y=y;
	npts[nvertices].z=z;
	vertices=npts;
	nvertices++;
}

//! Return a transformation basis to face which.
/*! If total!=0, then includes this->m(). Thus the transform brings face points
 * to the containing coordinates of the net. If total==0, the the returned transform
 * only maps face points to net units.
 * 
 * If m==NULL, then return a new double[6]. 
 * If that face is not available, then return NULL.
 *
 * Will construct a basis such that the xaxis goes from SimpleNetFace::aligno
 * toward SimpleNetFace::alignx, but whose length is 1 in net coordinates. The
 * yaxis is just the transpose of the x axis.
 *
 * \todo *** this currently ignore SimpleNetFace::matrix!!
 */
double *SimpleNet::basisOfFace(int which,double *mm,int total)//mm=NULL, total=0
{
	if (!nf || which<0 || which>=nf) return NULL;
	if (!mm) mm=new double[6];
	transform_identity(mm);

	//DBG //*** for debugging	
	//DBG  cerr <<"basisOfFace "<<which<<":\n";
	flatpoint p;
	//DBG for (int c=0; c<faces[which].np; c++) {
	//DBG 	p=points[faces[which].points[c]];
	//DBG 	cerr <<" p"<<c<<": "<<p.x<<" "<<p.y<<endl;
	//DBG }
	
	int o=faces[which].aligno,x=faces[which].alignx;
	if (o<0) o=0;
	if (x<0) x=(o+1)%faces[which].np;
	flatpoint origin=points[faces[which].points[o]],
			  xtip=points[faces[which].points[x]];
	SomeData s;
	s.origin(origin);
	p=xtip-origin;
	p=p/sqrt(p*p); //normalize p
	s.xaxis(p);
	s.yaxis(transpose(p)); // s.m() is (face coords) -> (paper)
	if (total) transform_mult(mm,s.m(),m());
		else transform_copy(mm,s.m());

	//DBG //*** for debugging	
	//DBG cerr <<"--transformed face "<<which<<":"<<endl;
	//DBG transform_invert(s.m(),mm);
	//DBG double slen=norm(points[faces[which].points[0]]-points[faces[which].points[1]]);
	//DBG p=transform_point(mm,flatpoint(0,0));
	//DBG cerr <<"  origin:"<<p.x<<" "<<p.y<<endl;
	//DBG p=transform_point(mm,flatpoint(slen,0));
	//DBG cerr <<"  point 1:"<<p.x<<" "<<p.y<<endl;

	
	return mm;
}

//! Rotate face f by moving alignx and/or o by one.
/*! Return 0 for something changed, nonzero for not.
 */
int SimpleNet::rotateface(int f,int alignxonly)//alignxonly=0
{
	if (f<0 || f>=nf) return 0;
	int ao=faces[f].aligno,
	    ax=faces[f].alignx;
	if (ao<0) ao=0;
	if (ax<0) ax=(ao+1)%faces[f].np;
	int n=0;
	ax=(ax+1)%faces[f].np;
	if (!alignxonly) ao=(ao+1)%faces[f].np;
	if (ax==ao) ax=-1;
	n=1;
	if (ax!=faces[f].alignx || ao!=faces[f].aligno) n=0;
	faces[f].aligno=ao;
	faces[f].alignx=ax;
	return n;
}

//! Return the index of the first face that contains points, or -1.
int SimpleNet::pointinface(flatpoint pp,int innetcoords)
{
	if (!innetcoords) {
		double i[6];
		transform_invert(i,m());
		pp=transform_point(i,pp);
	}
	int c,c2;
	for (c=0; c<nf; c++) {
		flatpoint pnts[faces[c].np];
		for (c2=0; c2<faces[c].np; c2++) pnts[c2]=points[faces[c].points[c2]];
		if (point_is_in(pp,pnts,c2)) return c;
	}
	return -1;
}

//! Find the bounding box of points of the net.
void SimpleNet::FindBBox()
{
	if (!np) return;
	minx=maxx=points[0].x;
	miny=maxy=points[0].y;
	for (int c=1; c<np; c++) addtobounds(points[c]);
}

//! Center the net around the origin. (changes points)
void SimpleNet::Center()
{
	double dx=(maxx+minx)/2,dy=(maxy+miny)/2;
	for (int c=0; c<np; c++) {
		points[c]-=flatpoint(dx,dy);
	}
	minx-=dx;
	maxx-=dx;
	miny-=dy;
	maxy-=dy;
}

//! Apply a transform to the points, changing them.
/*! If m==NULL, then use this->m().
 */
void SimpleNet::ApplyTransform(const double *mm)//mm=NULL
{
	if (mm==NULL) mm=m();
	maxx=maxy=-1;
	minx=miny=0;
	for (int c=0; c<np; c++) {
		points[c]=transform_point(mm,points[c]);
		addtobounds(points[c]);
	}
	setIdentity();
}

//! Make *this fit inside bounding box of data (inset by margin).
/*! \todo ***  this clears any rotation that was in the net->m() and it shouldn't
 */
void SimpleNet::FitToData(Laxkit::DoubleBBox *data,double margin)
{
	if (!data || !np) return;
	double wW=(data->maxx-data->minx-2*margin)/(maxx-minx);
	double hH=(data->maxy-data->miny-2*margin)/(maxy-miny);
	if (hH<wW) wW=hH; // pick the smaller of the two size ratios

	flatpoint mid=flatpoint((maxx+minx)/2,(maxy+miny)/2),
			midp=flatpoint((data->maxx+data->minx)/2,(data->maxy+data->miny)/2);
	xaxis(flatpoint(wW,0));
	yaxis(flatpoint(0,wW));
	mid=transform_point(m(),mid);
	origin(origin()+midp-mid);
	
//	xaxis(wW*data->xaxis());
//	yaxis(wW*data->yaxis());
//	origin(data->origin()+midp.x*data->xaxis()+midp.y*data->yaxis()-mid.x*xaxis()-mid.y*yaxis());
}

//! Add point pp to top of the list of points.
void SimpleNet::pushpoint(flatpoint pp,int pmap)//pmap=-1
{
	flatpoint *npts=new flatpoint[np+1];
	int *newmap=new int[np+1];
	if (points) memcpy((void *)npts,(const void *)points, np*sizeof(flatpoint));
	if (pointmap) memcpy((void *)newmap,(const void *)pointmap, np*sizeof(int));
	else if (np) for (int c=0; c<np; c++) newmap[c]=-1;
	npts[np]=pp;
	newmap[np]=pmap;
	delete[] points;
	delete[] pointmap;
	points=npts;
	pointmap=newmap;
	np++;
}

//! Add a face to top of faces.
/*! Makes a copy */
void SimpleNet::pushface(SimpleNetFace &f)
{
	SimpleNetFace *nfaces=new SimpleNetFace[nf+1];
	for (int c=0; c<nf; c++) nfaces[c]=faces[c]; //cannot do memcpy
	nfaces[nf]=f;
	delete[] faces;
	faces=nfaces;
	nf++;
}

//! Add a line to lines before position where. where<0 implies top of stack.
/*! Makes a copy */
void SimpleNet::pushline(SimpleNetLine &l,int where)//where=-1
{
	SimpleNetLine *nlines=new SimpleNetLine[nl+1];
	if (where<0) where=nl;
	for (int c=0; c<where; c++) nlines[c]=lines[c]; //cannot do memcpy,need deep copy
	nlines[where]=l;
	for (int c=where; c<nl; c++) nlines[c+1]=lines[c]; //cannot do memcpy
	delete[] lines;
	lines=nlines;
	nl++;
}

//---------------------------- AbstractNet Functions

NetFace *SimpleNet::GetFace(int i,double scaling)
{//***
	cout <<"*************  BAD BAD MUST IMPLEMENT SimpleNet::GetFace()!!!"<<endl;
	return NULL;

//	-------------------------
//	if (i<0 || i>=faces.n) return NULL;
//
//	NetFaceEdge *e;
//	NetFace *f=new NetFace();
//	f->original=i;
//
//	Pgon pgon=FaceToPgon(i,0); //generate flattened face
//	for (int c=0; c<faces.e[i]->pn; c++) {
//		e=new NetFaceEdge();
//		e->id=c;
//		e->tooriginal=faces.e[i]->f[c]; //edge connects to which face
//		e->toface=-1;
//
//		 //find edge of the face this edge connects to
//		if (e->tooriginal>=0) {
//			for (int c2=0; c2<faces.e[e->tooriginal]->pn; c2++) {
//				if (faces.e[e->tooriginal]->f[c2]==i) {
//					e->tofaceedge=c2;
//					break;
//				}
//			}
//		}
//		e->points=new LaxInterfaces::Coordinate(pgon.p[c].x*scaling, pgon.p[c].y*scaling);
//		f->edges.push(e,1);
//	}
//	return f;
}

int SimpleNet::dumpOutNet(FILE *f,int indent,int what)
{
	dump_out(f,indent,what,NULL);
	return 0;
}


} // namespace Laidout

