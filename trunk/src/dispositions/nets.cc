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
/************ dispositions/nets.cc *************/

#include "nets.h"

#include <lax/strmanip.h>
#include <lax/transformmath.h>
//#include <lax/lists.cc>

#include <lax/attributes.h>
using namespace LaxFiles;

using namespace Laxkit;

#include <iostream>
using namespace std;

int pointisin(flatpoint *points, int n,flatpoint points);
extern void monthday(const char *str,int *month,int *day);


//--------------------------------------- NetLine -------------------------------------------
/*! \class NetLine
 * \brief Class to hold extra lines for use in Net.
 */
//class NetLine
//{
// public:
//	char isclosed;
//	int np;
//	int *points;
//	char lsislocal;
//	LineStyle *linestyle;
//	NetLine(const char *list=NULL);
//	virtual ~NetLine();
//	const NetLine &operator=(const NetLine &line);
//	virtual int Set(const char *list);
//	virtual int Set(int n, int closed);
//	
//	virtual void dump_out(FILE *f,int indent, int pfirst=0);
//	virtual void dump_in_atts(LaxFiles::Attribute *att, const char *val);//val=NULL
//};

NetLine::NetLine(const char *list)
{
	linestyle=NULL;
	lsislocal=0;
	isclosed=0;
	np=0;
	points=NULL;
	if (list) Set(list);
}

NetLine::~NetLine()
{
	delete[] points;
	//if (lsislocal) delete linestyle; else linestyle->dec_count(); ???? 
}

//! Set up points like "1 2 3 ... n", and make closed if closed!=0.
/*! Returns the number of points.
 */
int NetLine::Set(int n, int closed)
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
int NetLine::Set(const char *list)
{
	if (points) delete[] points;
	IntListAttribute(list,&points,&np);
	if (np && points[np-1]==points[0]) {
		np--;
		isclosed=1;
	}
	return np;
}

/*! Copies over all. Warning: does a linestyle=line.linestyle,
 * and does not change lsislocal.
 */
const NetLine &NetLine::operator=(const NetLine &line)
{
	isclosed=line.isclosed;
	np=line.np;
	
	if (linestyle && line.linestyle) *linestyle=*line.linestyle;//***this is maybe problematic
	//lsislocal is not changed
	
	if (points) delete[] points;
	points=new int[np];
	for (int c=0; c<np; c++) points[c]=line.points[c];

	return *this;
}

/*! If pfirst!=0, then immediately output the list of points.
 * Otherwise, do points 3 5 6 6...
 */
void NetLine::dump_out(FILE *f,int indent, int pfirst)//pfirst=0
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (!pfirst) fprintf(f,"%spoints ",spc);
	int c;
	for (c=0; c<np; c++) fprintf(f,"%d ",points[c]);
	fprintf(f,"\n");
	if (isclosed) fprintf(f,"%sclosed\n",spc);
	if (linestyle) {
		fprintf(f,"%slinestyle\n",spc);
		linestyle->dump_out(f,indent+2);
	}
}

/*! If val!=NULL, then the att was something like:
 * <pre>
 *   line 1 2 3
 *     (other stuff)..
 * </pre>
 * In that case, Net would have parsed the "1 2 3", and it would pass
 * that here in val. Otherwise, this function will expect a 
 * "points 1 2 3" sub attribute somewhere in att.
 *
 * If there's a list "1 2 3 1" it will make a point list 1,2,3, and
 * set isclosed=1.
 */
void NetLine::dump_in_atts(LaxFiles::Attribute *att, const char *val)
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
				lsislocal=1;
			}
			linestyle->dump_in_atts(att->attributes.e[c]);
		} else if (!strcmp(name,"closed")) {
			isclosed=BooleanAttribute(value);
		}
	}
}

//--------------------------------------- NetFace -------------------------------------------
/*! \class NetFace
 * \brief Class to hold info about a face in a Net.
 */
/*! \var int *NetFace::points
 * \brief List of indices into Net::points for which points define the net.
 *
 * If positive x is to the right and positive y is up, the points should be
 * such that the inside of the face is on the left. That is, interior points
 * are those that get circled in a counterclockwise direction.
 */
/*! \var int *NetFace::facelink
 * \brief List of indices into Net::faces for which edges connect to which faces.
 */
/*! \var int NetFace::aligno
 * An index into the face's point list.
 * The default is for the face's basis to have the origin at the first listed point,
 * and the x axis lies along the vector going from the first point to the second point.
 * If aligno>=0, then use that point as the origin, and if alignx>=0, use point alignx
 * as where the x axis should point to from aligno. If m is specified, then that is
 * an extra 6 member affine transform matrix to apply to the basis.
 */
/*! \var int NetFace::alignx
 * \brief See aligno.
 */
/*! \var double *NetFace::m
 * \brief See aligno.
 */ 
/*! \var int NetFace::faceclass
 * \brief If >=0, then this face should look the same as others in the same class.
 */
//class NetFace
//{
// public:
//	int np;
//	int *points, *facelink;
//	double *m;
//	int aligno, alignx;
//	int faceclass;
//	NetFace();
//	virtual ~NetFace();
//	const NetFace &operator=(const NetFace &face);
//	virtual void clear();
//	virtual int Set(const char *list, const char *link=NULL);
//	virtual int Set(int n,int *list,int *link=NULL,int dellists=0);
//	
//	virtual void dump_out(FILE *f,int indent, int pfirst=0);
//	virtual void dump_in_atts(LaxFiles::Attribute *att, const char *val);//val=NULL
//};

NetFace::NetFace()
{
	m=NULL;
	aligno=alignx=-1;
	faceclass=-1;
	np=0;
	points=facelink=NULL; 
}
	
NetFace::~NetFace()
{ 
	if (points) delete[] points;
	if (facelink) delete[] facelink;
	if (m) delete[] m; 
}

//! Delete m, points, and facelink, set align stuff to -1.
void NetFace::clear()
{
	if (points) delete[] points; points=NULL;
	if (facelink) delete[] facelink; facelink=NULL;
	if (m) delete[] m; m=NULL;
	np=0;
	aligno=alignx=faceclass=-1;
}

//! Assignment operator, straightforward copy all.
const NetFace &NetFace::operator=(const NetFace &face)
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
	points=new int[np];
	facelink=new int[np];
	for (int c=0; c<np; c++) {
		points[c]=face.points[c];
		if (face.facelink) facelink[c]=face.facelink[c]; else facelink[c]=-1;
	}
	return *this;
}

//! Create points and facelink from list like: "1 2 3".
/*! Returns the number of points.
 *
 * If list and link have differing numbers of elements, link is
 * removed and replaced with a list of -1.
 */
int NetFace::Set(const char *list, const char *link)
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
int NetFace::Set(int n,int *list,int *link,int dellists)//dellists=0
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
 * See Net::dump_out() for what gets put out.
 */
void NetFace::dump_out(FILE *f,int indent, int pfirst)//pfirst=0
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
 * In that case, Net would have parsed the "1 2 3", and it would pass
 * that here in val. Otherwise, this function will expect a 
 * "points 1 2 3" sub attribute somewhere in att.
 */
void NetFace::dump_in_atts(LaxFiles::Attribute *att, const char *val)//val=NULL
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

//--------------------------------------- Net -------------------------------------------
/*! \class Net
 * \brief A type of SomeData that stores polyhedron cut and fold patterns.
 *
 * This is used by NetDisposition. 
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
/*! \var NetLine *Net::lines
 * \brief The lines that make up what the net looks like.
 *
 * Line 0 is assumed to be the outline of the whole net. Having lines specified with these
 * makes it so edge lines are not drawn twice, which is what would happen if the net was
 * drawn simply by outlining the faces.
 *
 * \todo ***actually this might be bad, should perhaps have flag saying that some line is
 * an outline. Tabs get tacked onto outlines, but multiple outlines should be allowed.
 */
/*! \var int Net::tabs
 * <pre>
 *  0  no tabs (default)
 *  1  tabs alternating every other (even)
 *  2  tabs alternating the other every other (odd)
 *  3  tabs on all edges (all or yes)
 * </pre>
 */
//class Net : public Laxkit::SomeData
//{
// public:
//	char *thenettype;
//	int np,tabs;
//	flatpoint *points;
//	int *pointmap; // which thing (possibly 3-d points) corresponding point maps to
//	int nl;
//	NetLine *lines;
//	int nf;
//	NetFace *faces;
//	Net();
//	virtual ~Net();
//	virtual void clear();
//	virtual const char *whatshape() { return thenettype; }
//	virtual int Draw(cairo_t *cairo,Laxkit::Displayer *dp,int month,int year);
//	virtual void DrawMonth(cairo_t *cairo,Laxkit::Displayer *dp,int month,int year,Laxkit::SomeData *monthbox);
//	virtual void FindBBox();
//	virtual void FitToData(Laxkit::SomeData *data,double margin);
//	virtual void ApplyTransform(double *mm=NULL);
//	virtual void Center();
//	virtual const char *whattype() { return thenettype; }
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//	virtual int pointinface(flatpoint pp);
//	virtual int rotateface(int f,int alignxonly=0);
//	virtual void pushline(NetLine &l,int where=-1);
//	virtual void pushface(NetFace &f);
//	virtual void pushpoint(flatpoint pp,int pmap=-1);
//	virtual double *basisOfFace(int which,double *mm=NULL,int total=0);
//
//	//--perhaps for future:
//	//virtual void PrintSVG(std::ostream &svg,Laxkit::SomeData *paper,int month=1,int year=2006);
//	//virtual void PrintPS(std::ofstream &ps,Laxkit::SomeData *paper);
//};

//! Init.
Net::Net()
{
	np=nl=nf=0;
	points=NULL;
	pointmap=NULL;
	lines=NULL;
	faces=NULL;
	tabs=0;
	thenettype=newstr("Net");
}

//! Delete points,lines,faces,pointmap,thenettype.
Net::~Net()
{	clear(); }

void Net::clear()
{
	if (thenettype) { delete[] thenettype; thenettype=NULL; }
	
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
}

//! Return a new copy of this.
Net *Net::duplicate()
{
	Net *net=new Net;
	makestr(net->thenettype,thenettype);
	net->np=np;
	net->tabs=tabs;
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
		net->faces=new NetFace[nf];
		for (int c=0; c<nf; c++) {
			net->faces[c]=faces[c];
		}
	}
	if (nl) {
		net->nl=nl;
		net->lines=new NetLine[nl];
		for (int c=0; c<nl; c++) {
			net->lines[c]=lines[c];
		}
	}
	transform_copy(net->m(),m());
	net->FindBBox();
	return net;
}

/*! perhaps:
 * <pre>
 * name "Square split diagonally"
 * matrix 1 0 0 1 0 0  # optional extra matrix to map this net to a page
 * points \
 *    1 1   to 0  # 0,  the optional 'to number' is map to whatever, goes in pointmap
 *    -1 1  to 1  # 1   it can be used to point to original 3-d points, for instance
 *    -1 -1 to 2  # 2
 *    1 -1  to 3  # 3
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
void Net::dump_out(FILE *f,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	int c;

	fprintf(f,"%sname %s\n",spc,whatshape());
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	
	if (tabs==0) fprintf(f,"%stabs no\n",spc);
	else if (tabs==1) fprintf(f,"%stabs even\n",spc);
	else if (tabs==2) fprintf(f,"%stabs odd\n",spc);
	else if (tabs==3) fprintf(f,"%stabs all\n",spc);
	
	 // dump points
	if (np) {
		fprintf(f,"%spoints \\\n",spc);
		for (c=0; c<np; c++) {
			fprintf(f,"%s  %.10g %.10g ",spc,points[c].x,points[c].y);
			if (pointmap && pointmap[c]>=0) fprintf(f,"to %d ",pointmap[c]);
			fprintf(f,"# %d\n",c);
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
void  Net::dump_in_atts(Attribute *att)
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
			DoubleListAttribute(value,m(),6);
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
			cout <<" *** tabs not implemented in Net!"<<endl;
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
					pm=strtol(t,&e,10);
					if (e==t) break; // broken file
					t=e;
				}
				pushpoint(flatpoint(x,y),pm);
			}
		} else if (!strcmp(name,"outline")) {
			hadoutline=1;
			NetLine netline;
			netline.dump_in_atts(att->attributes.e[c],value);
			pushline(netline,0); // pushes onto position 0
		} else if (!strcmp(name,"line")) {
			NetLine netline;
			netline.dump_in_atts(att->attributes.e[c],value);
			pushline(netline,-1); // pushes onto top
		} else if (!strcmp(name,"face")) {
			NetFace netface;
			netface.dump_in_atts(att->attributes.e[c],value);
			pushface(netface);
		}
	}

	 // if no outline, then assume list of points is the outline.
	if (!hadoutline) {
		NetLine line;
		line.isclosed=1;
		line.points=new int[np];
		line.np=np;
		for (c=0; c<np; c++) line.points[c]=c;	
		pushline(line,0);
	}
	
	//***sanity check on all point references..
	FindBBox();

	cout <<"----------------this was set in Net:-------------"<<endl;
	dump_out(stdout,0);
	cout <<"----------------end Net dump:-------------"<<endl;
}

//! Return a transformation basis to face which. Includes this->m() if total!=0.
/*! If m==NULL, then return a new double[6]. 
 * If that face is not available, then return NULL.
 *
 * Will construct a basis such that the xaxis goes from NetFace::aligno
 * toward NetFace::alignx, but whose length is 1 in net coordinates. The
 * yaxis is just the transpose of the x axis.
 *
 * \todo *** this currently ignore NetFace::matrix!!
 */
double *Net::basisOfFace(int which,double *mm,int total)//mm=NULL, total=0
{
	if (!nf || which<0 || which>=nf) return NULL;
	if (!mm) mm=new double[6];
	transform_identity(mm);

	//*** for debugging	
	cout <<"basisOfFace "<<which<<":\n";
	flatpoint p;
	for (int c=0; c<faces[which].np; c++) {
		p=points[faces[which].points[c]];
		cout <<" p"<<c<<": "<<p.x<<" "<<p.y<<endl;
	}
	
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

	//*** for debugging	
	cout <<"--transformed face "<<which<<":"<<endl;
	transform_invert(s.m(),mm);
	double slen=norm(points[faces[which].points[0]]-points[faces[which].points[1]]);
	p=transform_point(mm,flatpoint(0,0));
	cout <<"  origin:"<<p.x<<" "<<p.y<<endl;
	p=transform_point(mm,flatpoint(slen,0));
	cout <<"  point 1:"<<p.x<<" "<<p.y<<endl;

	
	return mm;
}

//! Rotate face f by moving alignx and/or o by one.
/*! Return 0 for something changed, nonzero for not.
 */
int Net::rotateface(int f,int alignxonly)//alignxonly=0
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
int Net::pointinface(flatpoint pp)
{
	double i[6];
	transform_invert(i,m());
	pp=transform_point(i,pp);
	int c,c2;
	for (c=0; c<nf; c++) {
		flatpoint pnts[faces[c].np];
		for (c2=0; c2<faces[c].np; c2++) pnts[c2]=points[faces[c].points[c2]];
		if (pointisin(pnts,c2,pp)) return c;
	}
	return -1;
}

//! Find the bounding box of points of the net.
void Net::FindBBox()
{
	if (!np) return;
	minx=maxx=points[0].x;
	miny=maxy=points[0].y;
	for (int c=1; c<np; c++) addtobounds(points[c]);
}

//! Center the net around the origin. (changes points)
void Net::Center()
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
void Net::ApplyTransform(double *mm)//mm=NULL
{
	if (mm==NULL) mm=m();
	maxx=maxy=-1;
	minx=miny=0;
	for (int c=0; c<np; c++) {
		points[c]=transform_point(mm,points[c]);
		addtobounds(points[c]);
	}
	transform_identity(m());
}

//! Make *this fit inside bounding box of data (inset by margin).
/*! \todo ***  this clears any rotation that was in the net->m() and it shouldn't
 */
void Net::FitToData(SomeData *data,double margin)
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
void Net::pushpoint(flatpoint pp,int pmap)//pmap=-1
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
void Net::pushface(NetFace &f)
{
	NetFace *nfaces=new NetFace[nf+1];
	for (int c=0; c<nf; c++) nfaces[c]=faces[c]; //cannot do memcpy
	nfaces[nf]=f;
	delete[] faces;
	faces=nfaces;
	nf++;
}

//! Add a line to lines before position where. where<0 implies top of stack.
/*! Makes a copy */
void Net::pushline(NetLine &l,int where)//where=-1
{
	NetLine *nlines=new NetLine[nl+1];
	if (where<0) where=nl;
	for (int c=0; c<where; c++) nlines[c]=lines[c]; //cannot do memcpy
	nlines[where]=l;
	for (int c=where; c<nl; c++) nlines[c+1]=lines[c]; //cannot do memcpy
	delete[] lines;
	lines=nlines;
	nl++;
}


//-------- perhaps for future use:
//1. Helper functions for a dynamic net modifier
//2. SVG out of the net
//
//
////! Output to an svg file.
///*! caller must open and close the stream
// * <pre>
// *   SVG notes
// * Net should optionally be able to read in from svg-style
// * string that has l,L,m,M and closepath(z/Z) commands
// * thus a normal 4 sided rect= "M 100 100 l 200 0 0 200 -200 0 z" 
// * m moveto relative
// * M moveto absolute
// * l lineto rel
// * L lineto abs
// * z/Z closepath
// *  (note that subsequent commands,	in this case 'l' don't have to be specified)
// *  grouping:
// *  \<g id="g2835"  transform="matrix(0.981652,0.000000,0.000000,0.981652,11.18525,18.33537)">
// * 	 \<path ... />
// *  \</g>
// *  or the transform="..." is put within the path: <path ... transform="..." />
// * </pre>
// *
// */
//void Net::PrintSVG(ostream &svg,SomeData *paper,int month,int year) // month=0, year=2006
//{***
//	month--;
//	if (!np) return;
//	//if (!np || !dp) return 0;
//
//	 // prepare images, which holds where the little images should go.
//	images.flush();
//	if (birthdays->attributes.n) {
//		curatt=0;
//		monthday(birthdays->attributes[curatt]->value,&nextattmonth,&nextattday);
//	} else {
//		curatt=-1;
//		nextattmonth=13;
//		nextattday=32;
//	}
//	
//	 // Define the transformation matrix: net to page
//	 // *** it's getting shortened (scaled down) a little into inkscape, what the hell?
//	double M[6]; 
//	flatpoint paperx,papery;
//	paperx=paper->xaxis()/(paper->xaxis()*paper->xaxis());
//	papery=paper->yaxis()/(paper->yaxis()*paper->yaxis());
//	M[0]=xaxis()*paperx;
//	M[1]=xaxis()*papery;
//	M[2]=yaxis()*paperx;
//	M[3]=yaxis()*papery;
//	M[4]=(origin()-paper->origin())*paperx;
//	M[5]=(origin()-paper->origin())*papery;
//	double scaling=1/sqrt(M[0]*M[0]+M[1]*M[1]);
//cout <<"******--- Scaling="<<scaling<<endl;
//
//	char pathheader[400];
//	sprintf(pathheader,"\t<path\n\t\tstyle=\"fill:none;fill-opacity:0.75;fill-rule:evenodd;stroke:#000000;stroke-width:%.6fpt;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:1.0;\"\n\t\t",scaling);
//	const char *pathclose="\n\t/>\n";
//	
//			
//	 // Print out header
//	svg << "<svg"<<endl
//		<< "\twidth=\"612pt\"\n\theight=\"792pt\""<<endl
//		<< "\txmlns:sodipodi=\"http://inkscape.sourceforge.net/DTD/sodipodi-0.dtd\""<<endl
//		<< "\txmlns:xlink=\"http://www.w3.org/1999/xlink\""<<endl
//		<<">"<<endl;
//	
//	 // Write matrix
//	svg <<"\t<g transform=\"scale(1.25)\">"<<endl;
//	svg <<"\t<g transform=\"matrix("<<M[0]<<','<<M[1]<<','<<M[2]<<','<<M[3]<<','<<M[4]<<','<<M[5]<<")\">"<<endl;
//
//
//	
//	 // ---------- draw lines 
//	int c,c2;
//	svg << pathheader << "d=\"M ";
//	
//	flatpoint pp[np+1];
//	for (c=0; c<np; c++) {
//		//pp[c]=dp->realtoscreen(points[c]);***
//		pp[c]=points[c];
//		svg << pp[c].x<<' '<<pp[c].y<<' ';
//		if (c==0) svg << "L ";
//	}
//	svg << "z\""<< pathclose <<endl;
//
//	
//	 // draw extra lines. Note that these are open paths
//	//*** should have linestyle: None, Dotted, Solid
//	svg << "<g>"<<endl; // group the fold lines to make easier to remove later
//	for (c=0; c<nl; c++) {
//		svg << pathheader << "d=\"M ";
//		c2=0;
//		while (lines[c][c2]!=-1) {
//			svg << pp[lines[c][c2]].x<<' '<<pp[lines[c][c2]].y<<' ';
//			if (c2==0) svg << "L ";
//			c2++;
//		}
//		svg << "\"" <<pathclose <<endl;
//	}
//	svg << "</g>"<<endl;
//	
//	 //-------- draw tabs 
//	 //The smallest angle that a tab has to scrunch into is 30 degrees
//	 //The tabs are drawn on each alternate segment in array points
//	svg << "<g>"<<endl; // group the tab lines to make easier to remove later
//	flatpoint p1,p2,p3,v;
//	for (c=0; c<np; c+=2) { // np should always be even
//		p1=pp[c];
//		p2=pp[c+1];
//		v=(p2-p1)/2;
//		v=-transpose(v)*tan(29./180*3.14159265359);
//		p3=(p1+p2)/2+v/2;
//
//		svg << pathheader<<"d=\"M "<<p1.x<<' '<<p1.y<<" L "<<p3.x<<' '<<p3.y<<' '<<p2.x<<' '<<p2.y<< "\"" << pathclose <<endl;
//	}
//	svg <<"\t</g>\n";
//		
//	 // ***----------- draw months 
//	 // m= month info:  [polygontype refpoint1 refpoint2]
//	 // months are in order, starting from month,year passed to Draw
//	SomeData *monthbox=NULL;
//	for (c=0; c<nm; c++) {
//		monthbox=GetMonthBox(c);
//		SVGMonth(svg,1+(month+c)%12,year+(month+c)/12,monthbox);
//	}
//
//	 // draw the images pointing to days.
//	 // draws filled circle at x,y, then line 5*textheight up and right, 
//	 // then image, scaled to 4*textheight
//	if (images.n) {
//		double scale,x2,y2,x,y,w,h,angle;
//		svg <<"\t<g>\n";
//		for (c=0; c<images.n; c++) {
//			//***
//			w=images.e[c]->width;
//			h=images.e[c]->height;
//			h*=images.e[c]->textheight*3/w;
//			w=images.e[c]->textheight*3;
////			w*=scaling;
////			h*=scaling;
//			if (images.e[c]->height) scale=w/h;
//			else scale=1;
//			x=images.e[c]->x;
//			y=images.e[c]->y;
//			x2=images.e[c]->x+images.e[c]->textheight*5;
//			y2=images.e[c]->y+images.e[c]->textheight*5;
//			svg <<"\t<path"<<endl
//				 <<"\t\tstyle=\"opacity:1.0000000;color:#000000;fill:none;fill-opacity:1.0000000;fill-rule:evenodd;stroke:#008200;"
//				 <<"stroke-width:"<<scaling
//				 <<";stroke-linecap:butt;stroke-linejoin:miter;marker:none;marker-start:none;marker-mid:none;stroke-miterlimit:4.0000000;stroke-dasharray:none;stroke-dashoffset:0.0000000;stroke-opacity:1.0000000;visibility:visible;display:inline;overflow:visible\""<<endl
//				 <<"\t\td=\"M "<<x<<','<<y<<" C "
//				 <<x+(x2-x)/3   <<','<< y+(y2-y)/3   << " "
//				 <<x+(x2-x)*2/3 <<','<< y+(y2-y)*2/3 << " "
//				 <<x2<<','<<y2<<"\"\n\t/>"<<endl;
//			svg <<"\t<path"<<endl
//				 <<"\t\tsodipodi:type=\"arc\""<<endl
//				 <<"\t\tstyle=\"fill:#dbfffa;fill-opacity:1.0000000;stroke:#ff0000;stroke-width:0.44999999;stroke-miterlimit:4.0000000;stroke-dasharray:none;stroke-opacity:1.0000000\""<<endl
//				 <<"\t\td=\"M "<<x+images.e[c]->textheight*1.1/2<<','<<y
//				 <<" A "<<images.e[c]->textheight*1.1<<','<<images.e[c]->textheight*1.1<<" 0 1,1 "
//				 <<x+images.e[c]->textheight*1.1/2<<','<<y<<" z\" />"<<endl;
//			angle=180./M_PI*atan2(images.e[c]->m[1],images.e[c]->m[0]);
//			svg <<"\t<image\n"
//				 <<"\t\theight=\""<<h<<"\"\n"
//				 <<"\t\twidth=\""<<w<<"\"\n"
//				 <<"\t\txlink:href=\""<<images.e[c]->imagepath<<"\"\n"
//				 //<<"\t\tx=\""<<x2-w/2<<"\"\n"
//				 //<<"\t\ty=\""<<y2-h/2<<"\"\n"
//				 <<"\t\ttransform=\"translate("<<x2-w/2<<","<<y2-h/2<<") rotate("<<angle<<")\""<<endl
//				 <<"\t/>"<<endl;
////			svg <<"\t<image\n"
////				 <<"\t\theight=\""<<h<<"\"\n"
////				 <<"\t\twidth=\""<<w<<"\"\n"
////				 <<"\t\txlink:href=\""<<images.e[c]->imagepath<<"\"\n"
////				 <<"\t\tx=\""<<x2-w/2<<"\"\n"
////				 <<"\t\ty=\""<<y2-h/2<<"\"\n"
////				 <<"\t\ttransform=\"rotate("<<angle<<")\""<<endl
////				 <<"\t/>"<<endl;
//
//		}
//		svg <<"\t</g>\n";
//	}
//
//	 // Close the net grouping
//	svg <<"\t</g>\n";
//	svg <<"\t</g>\n";
//
//	 // Print out footer
//	svg << "\n</svg>\n";
//}
//
////! Output to a postscript file. ***imp me!
///*! caller must open and close the stream
// */
//void Net::PrintPS(ofstream &ps,SomeData *paper)
//{//***
//	ps <<"0 setgray";
//	cout <<" postscript out *** imp me!"<<endl;
//	return;
//}

