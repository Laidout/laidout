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
//	NetFace() { linestyle=NULL; lsislocal=0; isclosed=0; np=0; points=NULL; }
//	virtual ~NetFace() { if (m) delete[] m; if (lsislocal) delete linestyle; else linestyle->dec_count(); ???? }
//	const NetLine &operator=(const NetLine &line);
//};

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
//	const NetLine &operator=(const NetLine &line);
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
 * <pre>
 *   SVG notes
 * Net should optionally be able to read in from svg-style
 * string that has l,L,m,M and closepath(z/Z) commands
 * thus a normal 4 sided rect= "M 100 100 l 200 0 0 200 -200 0 z" 
 * m moveto relative
 * M moveto absolute
 * l lineto rel
 * L lineto abs
 * z/Z closepath
 *  (note that subsequent commands,	in this case 'l' don't have to be specified)
 *  grouping:
 *  \<g id="g2835"  transform="matrix(0.981652,0.000000,0.000000,0.981652,11.18525,18.33537)">
 * 	 \<path ... />
 *  \</g>
 *  or the transform="..." is put within the path: <path ... transform="..." />
 * </pre>
 */ 
/*! \var NetLine *Net::lines
 * \brief The lines that make up what the net looks like.
 *
 * Line 0 is assumed to be the outline of the whole net. Having lines specified with these
 * makes it so edge lines are not drawn twice, which is what would happen if the net was
 * drawn simply by outlining the faces.
 *
 * ***actually this might be bad, should perhaps have flag saying that some line is
 * an outline. Tabs get tacked onto outlines, but multiple outlines should be allowed.
 */
//class Net : public SomeData
//{
// public:
//	char *thenettype;
//	int np; 
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
//	virtual void PrintPS(std::ofstream &ps,Laxkit::SomeData *paper);
//	virtual void PrintSVG(std::ostream &svg,Laxkit::SomeData *paper,int month=1,int year=2006);
//	virtual void FindBBox();
//	virtual void FitToData(Laxkit::SomeData *data,double margin);
//	virtual void Center();
//	virtual SomeData *GetMonthBox(int which);
//	virtual void SVGMonth(ostream &svg,int month,int year,SomeData *monthbox);
//	virtual const char *whattype() { return thenettype; }
//	virtual void dump_out(FILE *f,int indent);
//	virtual void  dump_in(FILE *f,int indent);
//	virtual void pushface(int *f,int n);
//	virtual void pushpoint(flatpoint pp);
//	virtual int pointinface(flatpoint pp);
//	virtual void rotateface(int f,int endonly=0);
//};

//! Init np,nl,nm,points,lines,mo.
Net::Net()
{
	np=nl=0;
	points=NULL;
	pointmap=NULL;
	lines=mo=NULL;
	nf=0;
	faces=NULL;
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
		for (int c=0; c<nl; c++) delete[] lines[c];
		delete[] lines;
		lines=NULL;
		nl=0;
	}
	if (faces) {
		for (int c=0; c<nf; c++) delete[] faces[c];
		delete[] faces;
		faces=NULL;
		nf=0;
	}
}

//------ no longer used:
////! Copy the -1 terminated list to a new'd array that gets put in dest.
//void copylist(int *&dest,int *src)
//{
//	int c=0;
//	while (src[c]!=-1) c++;
//	dest=new int[c+1];
//	c=0;
//	while (src[c]!=-1) {
//		dest[c]=src[c];
//		c++;
//	}
//}

//! Return a new copy of this.
Net *Net::duplicate()
{
	Net *net=new Net;
	makestr(net->thenettype,thenettype);
	net->np=np;
	if (np) {
		net->pointmap=new int[np];
		net->points=new flatpoint[np];
		for (int c=0; c<np; c++) {
			net->pointmap[c]=pointmap[c];
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
 * line 1 2
 *    linestyle
 *       color 100 100 50 255
 *
 *  # Tabs defaults to 'no'. It can one of 'no','yes','all','default','even', or 'odd',
 *  # and you can also specify 'left' or 'right'.
 * tabs no
 * tab 1 4 left  # draw tab from point 1 to 4, on ccw side from 4 using 1 as origin
 *  
 *  # For each face, default is align x axis to be (face point 1)-(face point 0).
 *  # you can specify which other points the x axis should correspond to.
 *  # In that case, the 'align' points are indices into the points list,
 *  # NOT indices into the face's point list.
 *  # Otherwise, you can specify an arbitrary matrix that transforms
 *  # from the default face basis to the desired one
 * face 0 1 2
 *    align 1 2
 *    class 0  # optional tag. All faces of this class number are assumed
 *             # to use the same outline and matrix(?)
 * face 1 2 3
 *    matrix 1 0 0 1 .2 .3
 * </pre>
 */
void Net::dump_out(FILE *f,int indent)
{***
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	int c,c2;

	fprintf(f,"%sname %s\n",spc,whatshape());
	fprintf(f,"%ssomedata %s\n",spc,whatshape());
	SomeData::dump_out(f,indent+2);
	
	 // dump points
	if (np) {
		fprintf(f,"%spoints \\\n",spc);
		for (c=0; c<np; c++) {
			fprintf(f,"%s  %.10g %.10g ",spc,points[c].x,points[c].y);
			if (pointmap[c]>=0) fprintf(f,"to %d ",pointmap[c]);
			fprintf(f,"# %d\n",c);
		}
		fprintf(f,"\n");
	}
	
	 // dump extra lines
	if (nl) {
		for (c=0; c<nl; c++) {
			fprintf(f,"%sline ",spc);
			for (c2=0; lines[c][c2]!=-1; c2++) {
				fprintf(f,"%d ",lines[c][c2]);
			}
			fprintf(f,"\n");
		}
		fprintf(f,"\n");
	}
	
	 // dump faces
	if (nf) {
		for (c=0; c<nf; c++) {
			fprintf(f,"%sface ",spc);
			for (c2=0; faces[c][c2]!=-1; c2++) {
				fprintf(f,"%d ",faces[c][c2]);
			}
			fprintf(f,"\n");
		}
		fprintf(f,"\n");
	}
}

//! Read in a net from an already opened file.
/*! \todo *** must implement the sanity check..
 */
void  Net::dump_in_atts(Attribute *att)
{***
	Attribute att;
	att.dump_in(f,indent);
	cout <<"-----loaded in file, preparing to parse:"<<endl;
	att.dump_out(stdout,0);
	cout <<"----------end att dump, now parsing.."<<endl;
	
	char *name,*value,*t,*e,*newname=NULL;
	NumStack<int> intstack;
	PtrStack<int> Lines(2);
	PtrStack<int> Months(2);
	PtrStack<int> Faces(2);
	NumStack<flatpoint> Points;
	double x,y;
	int i,*list;
	for (int c=0; c<att.attributes.n; c++) {
		name=att.attributes.e[c]->name;
		value=att.attributes.e[c]->value;
		if (!strcmp(name,"name")) {
			makestr(newname,value);
			continue;
		} else if (!strcmp(name,"points")) {
			t=value;
			while (t && *t) {
				x=strtof(t,&e); 
				if (e==t) break;
				t=e;
				y=strtof(t,&e); 
				if (e==t) break;
				t=e;
				Points.push(flatpoint(x,y));
			}
		} else if (!strcmp(name,"line")) {
			t=value;
			intstack.flush();
			while (t && *t) {
				i=strtol(t,&e,10);
				if (e==t) break;
				intstack.push(i);
				t=e;
			}
			if (intstack.n>1) {
				intstack.push(-1);
				Lines.push(intstack.extractArray());
			}
		} else if (!strcmp(name,"month")) {
			t=value;
			intstack.flush();
			while (t && *t) {
				i=strtol(t,&e,10);
				if (e==t) break;
				intstack.push(i);
				t=e;
			}
			if (intstack.n==4) {
				intstack.push(-1);
				Months.push(intstack.extractArray());
			}
		} else if (!strcmp(name,"face")) {
			t=value;
			intstack.flush();
			while (t && *t) {
				i=strtol(t,&e,10);
				if (e==t) break;
				intstack.push(i);
				t=e;
			}
			if (intstack.n>2) {
				intstack.push(-1);
				Faces.push(intstack.extractArray());
			}
		} else if (!strcmp(name,"somedata")) {
			int c2,c3=0;
			for (c2=0; c2<att.attributes.e[c]->attributes.n; c2++) { 
				if (!strcmp(att.attributes.e[c]->attributes.e[c2]->name,"matrix")) {
					t=att.attributes.e[c]->attributes.e[c2]->value;
					while (t && *t) {
						if (c3==6) break;
						m(c3++,strtof(t,&e)); 
						if (e==t) break;
						t=e;
					}
				}
			}
		}
	}
	//***sanity check on all point references..
	//if (***ok) {
	if (1) {
		clear();
		makestr(thenettype,newname);
		points=Points.extractArray(&np);
		lines=Lines.extractArrays(NULL,&nl);
		faces=Faces.extractArrays(NULL,&nf);
	}
	cout <<"----------------this was set in Net:-------------"<<endl;
	dump_out(stdout,0);
	cout <<"----------------enddump:-------------"<<endl;
}

//! Rotate face f by moving alignx and/or o by one.
void Net::rotateface(int f,int endonly)//endonly=0
{***

	if (f<0 || f>=nf) return;
	int c;
	for (c=0; c<nm; c++) if (f==mo[c][3]) break;
	if (c==nm) return;
	int s=mo[c][1],e=mo[c][2],ns=-1,ne=-1;
	int n=0;
	for (n=0; faces[f][n]!=-1; n++) if (faces[f][n]==s) { ns=n; }
	for (ne=0; ne<n; ne++) if (faces[f][ne]==e) { break; }
	if (ns==-1 || ne==-1) return;
	if (!endonly) mo[c][1]=faces[f][(ns+1)%n];
	mo[c][2]=faces[f][(ne+1)%n];
	if (mo[c][2]==mo[c][1]) mo[c][2]=faces[f][(ne+2)%n];
}

//! Return the index of the first face that contains points, or -1.
int Net::pointinface(flatpoint pp)
{***
	NumStack<flatpoint> pnts;
	double i[6];
	transform_invert(i,m());
	pp=transform_point(i,pp);
	int c,c2;
	for (c=0; c<nf; c++) {
		for (c2=0; faces[c][c2]!=-1; c2++) pnts.push(points[faces[c][c2]]);
		//if (c2) pnts.push(pnts.e[0]); <--pushing first not necessary
		if (pointisin(pnts.e,pnts.n,pp)) return c;
		pnts.flush();
	}
	return -1;
}

void Net::pushpoint(flatpoint pp)
{***
	NumStack<flatpoint> stack;
	stack.insertArray(points,np);
	stack.push(pp);
	points=stack.extractArray(&np);
}

//! Add a face. Also add a month for that face
void Net::pushface(int *f,int n)
{***
	NumStack<int> intstack;
	PtrStack<int> ptrstack(2);
	intstack.insertArray(f,n);
	intstack.push(-1);
	f=intstack.extractArray(&n);
	
	ptrstack.insertArrays(faces,NULL,nf);
	ptrstack.push(f);
	faces=ptrstack.extractArrays(NULL,&nf);

	int *newmonth=new int[4];
	newmonth[0]=0;
	newmonth[1]=f[0];
	newmonth[2]=f[1];
	newmonth[3]=nf-1;

	ptrstack.insertArrays(mo,NULL,nm);
	ptrstack.push(newmonth);
	mo=ptrstack.extractArrays(NULL,&nm); 
}

//! Output to an svg file.
/*! caller must open and close the stream
 */
void Net::PrintSVG(ostream &svg,SomeData *paper,int month,int year) // month=0, year=2006
{***
	month--;
	if (!np) return;
	//if (!np || !dp) return 0;

	 // prepare images, which holds where the little images should go.
	images.flush();
	if (birthdays->attributes.n) {
		curatt=0;
		monthday(birthdays->attributes[curatt]->value,&nextattmonth,&nextattday);
	} else {
		curatt=-1;
		nextattmonth=13;
		nextattday=32;
	}
	
	 // Define the transformation matrix: net to page
	 // *** it's getting shortened (scaled down) a little into inkscape, what the hell?
	double M[6]; 
	flatpoint paperx,papery;
	paperx=paper->xaxis()/(paper->xaxis()*paper->xaxis());
	papery=paper->yaxis()/(paper->yaxis()*paper->yaxis());
	M[0]=xaxis()*paperx;
	M[1]=xaxis()*papery;
	M[2]=yaxis()*paperx;
	M[3]=yaxis()*papery;
	M[4]=(origin()-paper->origin())*paperx;
	M[5]=(origin()-paper->origin())*papery;
	double scaling=1/sqrt(M[0]*M[0]+M[1]*M[1]);
cout <<"******--- Scaling="<<scaling<<endl;

	char pathheader[400];
	sprintf(pathheader,"\t<path\n\t\tstyle=\"fill:none;fill-opacity:0.75;fill-rule:evenodd;stroke:#000000;stroke-width:%.6fpt;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:1.0;\"\n\t\t",scaling);
	const char *pathclose="\n\t/>\n";
	
			
	 // Print out header
	svg << "<svg"<<endl
		<< "\twidth=\"612pt\"\n\theight=\"792pt\""<<endl
		<< "\txmlns:sodipodi=\"http://inkscape.sourceforge.net/DTD/sodipodi-0.dtd\""<<endl
		<< "\txmlns:xlink=\"http://www.w3.org/1999/xlink\""<<endl
		<<">"<<endl;
	
	 // Write matrix
	svg <<"\t<g transform=\"scale(1.25)\">"<<endl;
	svg <<"\t<g transform=\"matrix("<<M[0]<<','<<M[1]<<','<<M[2]<<','<<M[3]<<','<<M[4]<<','<<M[5]<<")\">"<<endl;


	
	 // ---------- draw lines 
	int c,c2;
	svg << pathheader << "d=\"M ";
	
	flatpoint pp[np+1];
	for (c=0; c<np; c++) {
		//pp[c]=dp->realtoscreen(points[c]);***
		pp[c]=points[c];
		svg << pp[c].x<<' '<<pp[c].y<<' ';
		if (c==0) svg << "L ";
	}
	svg << "z\""<< pathclose <<endl;

	
	 // draw extra lines. Note that these are open paths
	//*** should have linestyle: None, Dotted, Solid
	svg << "<g>"<<endl; // group the fold lines to make easier to remove later
	for (c=0; c<nl; c++) {
		svg << pathheader << "d=\"M ";
		c2=0;
		while (lines[c][c2]!=-1) {
			svg << pp[lines[c][c2]].x<<' '<<pp[lines[c][c2]].y<<' ';
			if (c2==0) svg << "L ";
			c2++;
		}
		svg << "\"" <<pathclose <<endl;
	}
	svg << "</g>"<<endl;
	
	 //-------- draw tabs 
	 //The smallest angle that a tab has to scrunch into is 30 degrees
	 //The tabs are drawn on each alternate segment in array points
	svg << "<g>"<<endl; // group the tab lines to make easier to remove later
	flatpoint p1,p2,p3,v;
	for (c=0; c<np; c+=2) { // np should always be even
		p1=pp[c];
		p2=pp[c+1];
		v=(p2-p1)/2;
		v=-transpose(v)*tan(29./180*3.14159265359);
		p3=(p1+p2)/2+v/2;

		svg << pathheader<<"d=\"M "<<p1.x<<' '<<p1.y<<" L "<<p3.x<<' '<<p3.y<<' '<<p2.x<<' '<<p2.y<< "\"" << pathclose <<endl;
	}
	svg <<"\t</g>\n";
		
	 // ***----------- draw months 
	 // m= month info:  [polygontype refpoint1 refpoint2]
	 // months are in order, starting from month,year passed to Draw
	SomeData *monthbox=NULL;
	for (c=0; c<nm; c++) {
		monthbox=GetMonthBox(c);
		SVGMonth(svg,1+(month+c)%12,year+(month+c)/12,monthbox);
	}

	 // draw the images pointing to days.
	 // draws filled circle at x,y, then line 5*textheight up and right, 
	 // then image, scaled to 4*textheight
	if (images.n) {
		double scale,x2,y2,x,y,w,h,angle;
		svg <<"\t<g>\n";
		for (c=0; c<images.n; c++) {
			//***
			w=images.e[c]->width;
			h=images.e[c]->height;
			h*=images.e[c]->textheight*3/w;
			w=images.e[c]->textheight*3;
//			w*=scaling;
//			h*=scaling;
			if (images.e[c]->height) scale=w/h;
			else scale=1;
			x=images.e[c]->x;
			y=images.e[c]->y;
			x2=images.e[c]->x+images.e[c]->textheight*5;
			y2=images.e[c]->y+images.e[c]->textheight*5;
			svg <<"\t<path"<<endl
				 <<"\t\tstyle=\"opacity:1.0000000;color:#000000;fill:none;fill-opacity:1.0000000;fill-rule:evenodd;stroke:#008200;"
				 <<"stroke-width:"<<scaling
				 <<";stroke-linecap:butt;stroke-linejoin:miter;marker:none;marker-start:none;marker-mid:none;stroke-miterlimit:4.0000000;stroke-dasharray:none;stroke-dashoffset:0.0000000;stroke-opacity:1.0000000;visibility:visible;display:inline;overflow:visible\""<<endl
				 <<"\t\td=\"M "<<x<<','<<y<<" C "
				 <<x+(x2-x)/3   <<','<< y+(y2-y)/3   << " "
				 <<x+(x2-x)*2/3 <<','<< y+(y2-y)*2/3 << " "
				 <<x2<<','<<y2<<"\"\n\t/>"<<endl;
			svg <<"\t<path"<<endl
				 <<"\t\tsodipodi:type=\"arc\""<<endl
				 <<"\t\tstyle=\"fill:#dbfffa;fill-opacity:1.0000000;stroke:#ff0000;stroke-width:0.44999999;stroke-miterlimit:4.0000000;stroke-dasharray:none;stroke-opacity:1.0000000\""<<endl
				 <<"\t\td=\"M "<<x+images.e[c]->textheight*1.1/2<<','<<y
				 <<" A "<<images.e[c]->textheight*1.1<<','<<images.e[c]->textheight*1.1<<" 0 1,1 "
				 <<x+images.e[c]->textheight*1.1/2<<','<<y<<" z\" />"<<endl;
			angle=180./M_PI*atan2(images.e[c]->m[1],images.e[c]->m[0]);
			svg <<"\t<image\n"
				 <<"\t\theight=\""<<h<<"\"\n"
				 <<"\t\twidth=\""<<w<<"\"\n"
				 <<"\t\txlink:href=\""<<images.e[c]->imagepath<<"\"\n"
				 //<<"\t\tx=\""<<x2-w/2<<"\"\n"
				 //<<"\t\ty=\""<<y2-h/2<<"\"\n"
				 <<"\t\ttransform=\"translate("<<x2-w/2<<","<<y2-h/2<<") rotate("<<angle<<")\""<<endl
				 <<"\t/>"<<endl;
//			svg <<"\t<image\n"
//				 <<"\t\theight=\""<<h<<"\"\n"
//				 <<"\t\twidth=\""<<w<<"\"\n"
//				 <<"\t\txlink:href=\""<<images.e[c]->imagepath<<"\"\n"
//				 <<"\t\tx=\""<<x2-w/2<<"\"\n"
//				 <<"\t\ty=\""<<y2-h/2<<"\"\n"
//				 <<"\t\ttransform=\"rotate("<<angle<<")\""<<endl
//				 <<"\t/>"<<endl;

		}
		svg <<"\t</g>\n";
	}

	 // Close the net grouping
	svg <<"\t</g>\n";
	svg <<"\t</g>\n";

	 // Print out footer
	svg << "\n</svg>\n";
}

//! Output to a postscript file. ***imp me!
/*! caller must open and close the stream
 */
void Net::PrintPS(ofstream &ps,SomeData *paper)
{//***
	ps <<"0 setgray";
	cout <<" postscript out *** imp me!"<<endl;
	return;
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
	xaxis(wW*data->xaxis());
	yaxis(wW*data->yaxis());
	origin(data->origin()+midp.x*data->xaxis()+midp.y*data->yaxis()-mid.x*xaxis()-mid.y*yaxis());
}



















