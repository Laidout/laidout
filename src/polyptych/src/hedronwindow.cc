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
// Copyright (C) 2011-2012 by Tom Lechner
//



#include "hedronwindow.h"
#include "../language.h"

#include <Imlib2.h>
#include <lax/strmanip.h>
#include <lax/freedesktop.h>
#include <lax/transformmath.h>
#include <lax/laxutils.h>
#include <lax/fileutils.h>
#include <lax/filedialog.h>

#include <lax/lists.cc>
#include <lax/refptrstack.cc>


#include <iostream>
using namespace std;
using namespace Laxkit;
using namespace LaxFiles;

#define DBG


//! Namespace for various polyhedron 3-d stuff.
namespace Polyptych {


//----------gl setup----------------------

// *** these really need to be integrated somehow into the laxkit
static int  attributeListDouble[]  =  {
		GLX_RGBA,		
		GLX_DOUBLEBUFFER,		
		GLX_RED_SIZE,	1,	
		GLX_GREEN_SIZE,	1,
		GLX_BLUE_SIZE,  1,
		GLX_DEPTH_SIZE, 1,
		None 
	};

GLXContext glcontext=0;
XVisualInfo *glvisual=NULL;


//---------------------------- misc ----------------------
enum PaperPart {
	PAPER_None=0,
	PAPER_Left,
	PAPER_Right,
	PAPER_Top,
	PAPER_Bottom,
	PAPER_Inside
};


//--------------------------------- HedronWindow modes and other things -------------------------------

//hovering indicators
enum HedronWindowHoverPart {
	OGROUP_None=0,

	 //overlays:
	OGROUP_TouchHelpers,
	OGROUP_Papers,
	OGROUP_ImageStack,

	 //3-d things:
	OGROUP_Face,
	OGROUP_Potential,
	OGROUP_Paper,
	 
	OGROUP_MAX
};

enum Mode {
	MODE_Net,
	MODE_Solid,
	MODE_FaceSet,
	MODE_NetArrange,
	MODE_Stellate,
	MODE_Build_Sphere,
	MODE_Help,
	MODE_Last
};


//! Return a string corresponding to the given Polyptych::Mode.
const char *modename(int mode)
{
	if (mode==MODE_Net) return "net mode";
	if (mode==MODE_NetArrange) return "layout mode";
	if (mode==MODE_Solid) return "solid mode";
	if (mode==MODE_Stellate) return "stellate mode";
	if (mode==MODE_Build_Sphere) return "build mode";
	if (mode==MODE_FaceSet) return "face set mode";
	if (mode==MODE_Help) return "help mode";
	return " ";
}


//--------------------------- FaceData -------------------------------

/*! \class FaceData
 * \brief Stack of FaceDataObj for use in ExtraFace::extra.
 */

/*! \class FaceDataObj
 * \brief Base class for face textures.
 */

FaceDataObj::FaceDataObj(FaceDataType datatype)
{
	type=datatype;
}

//--------------------------- ColorFace -------------------------------
/*! \class ColorFace
 */

ColorFace::ColorFace(double r,double g, double b,double a)
  : FaceDataObj(FACE_Color), red(r), green(g), blue(b), alpha(a)
{}


//--------------------------- ImageFace -------------------------------
/*! \class ImageFace
 */

ImageFace::ImageFace(const char *file)
  : FaceDataObj(FACE_Image)
{}


//--------------------------- PanoramaFace -------------------------------
/*! \class PanoramaFace
 */

PanoramaFace::PanoramaFace(PanoramaInfo *pano)
  : FaceDataObj(FACE_Panorama)
{
	panorama=pano;
	if (pano) pano->inc_count();
}



//--------------------------- HedronWindow -------------------------------

//--- Net, Solid, Faceset mode state:
//   rendermode
//   fovy
//   draw_edges (of polyhedron)
//   unwrap_extent
//   draw_texture
//   draw_overlays
//   draw_seams (cuts and/or connections)
//   draw_info  *** need imp ui to change
//   draw_axes  *** need imp ui to change
//   draw_seams
//   cylinderscale
//   rotation_axis
//   rotation_speed
//   animation_fps
//
//   unwrapangle
//   currentnet
//   currentface
//   currentpotential
//   currentfacestatus
//   current camera
//
//---Layout mode state:
//    current net(s)
//
//--- Mouseable activities
//   toggle mode
//
//    --move shape--
//   roll shape
//   roll through viewer axis
//
//    --move camera--
//   zoom in/out
//   pan camera
//
//    --texture--
//   shift texture
//   rotate texture
//
//    --unwrap related
//   shift unwrap angle
//   unwrap
//   reseed
//    

/*! class HedronWindow
 * \brief Main viewer window for a hedron.
 *
 * poly->face->cache->facemode==-net.object_id if face is seed face.
 * facemode==0 if face still on hedron.
 * facemode==net.object_id if face is a non-seed face of that net.
 */
/*! \var Laxkit::RefPtrStack<Net> nets
 * \brief Stack of nets attached to hedron faces.
 *
 * Each net->info is the index of the original face that acts as the seed
 * for that net.
 */
/*! \var Thing *HedronWindow::hedron
 * \brief Holds the gl information associated with poly.
 *
 * Reminder that the gl info needs to wait until the proper current gl window is in place,
 * so no gl information should be initialized in init(), and boy did it take a damn long
 * time to figure that out.
 */


/*! If newpoly, then inc it's count. It does NOT create a copy from it current, so watch out.
 */
HedronWindow::HedronWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		Polyhedron *newpoly)
 	: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,NULL,0,0), rendermode(0),
	  default_paper("Letter",8.5,11,"in")
{
	poly=newpoly;
	if (poly) {
		poly->inc_count();
		poly->BuildExtra();
	}
	hedron=NULL;

	draw_papers=1;
	draw_texture=1;
	draw_axes=1;
	draw_info=1;
	draw_overlays=1;
	draw_edges=1;
	draw_seams=3;
	free_rotate = false;

	mouseover_overlay=-1;
	mouseover_index=-1;
	mouseover_group=OGROUP_None;
	mouseover_paper=-1;
	grab_overlay=-1; //mouse down on this overlay
	active_action=ACTION_None;
	touchmode=0;

	oldmode=mode=MODE_Net;
	cylinderscale=edgeScaleFromBox();

	fontsize=20;
	pad=20; 
	helpoffset=10000;

	polyptychfile         =NULL;
	polyhedronfile        =NULL;
	spherefile            =NULL;
	spheremap_data        =NULL;
	spheremap_data_rotated=NULL;
	spheremap_width       =0;
	spheremap_height      =0;
	spheretexture=flattexture=0; //gl texture ids

	currentmessage=lastmessage=NULL;

	consolefontfile=NULL;
	makestr(consolefontfile,"/usr/share/fonts/truetype/freefont/FreeSans.ttf"); // ***
	consolefont=NULL;
	movestep=.5;
	autorepeat=1;
	curobj=0;
	firsttime=1;
	currentface=-1;
	currentpotential=-1;
	currentnet=NULL;
	currentpaper=-1;

	unwrapangle=1.0;

	fovy=50*M_PI/180;
	current_camera=-1;
	instereo=0;
	cureye=0;
	
	sc=NULL;
}

HedronWindow::~HedronWindow()
{
	if (currentnet) currentnet->dec_count();
	if (poly) poly->dec_count();
	if (spheremap_data) delete[] spheremap_data;
	if (spheremap_data_rotated) delete[] spheremap_data_rotated;

	if (spherefile) delete[] spherefile;
	if (polyhedronfile) delete[] polyhedronfile;
	if (polyptychfile) delete[] polyptychfile;
	if (consolefontfile) delete[] consolefontfile;

	if (sc) sc->dec_count();
}

//! Set the font size, if newfontsize>0.
/*! If fontfile!=NULL, then load a new font based on that file.
 * 
 * Returns the old font size, whether or not newfontsize>0.
 */
double HedronWindow::FontAndSize(const char *fontfile, double newfontsize)
{
	double f=fontsize;
	if (newfontsize>0) fontsize=newfontsize;
	if (fontfile) makestr(consolefontfile,fontfile);
	return f;
}

//! Update the current message displayed in the window to str.
void HedronWindow::newMessage(const char *str)
{
	messagetick=0;
	makestr(currentmessage,str);
	needtodraw=1;
}

//! Return a reasonable edge width to use based on the size of the polyhedron bounding box.
double HedronWindow::edgeScaleFromBox()
{
	if (!poly || poly->vertices.n==0) { return .02; }

	spacepoint p1,p2; //p1==min point, p2==max point
	p1=p2=poly->vertices.e[0];
	for (int c=1; c<poly->vertices.n; c++) {
		if (poly->vertices.e[c].x<p1.x) p1.x=poly->vertices.e[c].x;
		else if (poly->vertices.e[c].x>p2.x) p2.x=poly->vertices.e[c].x;

		if (poly->vertices.e[c].y<p1.y) p1.y=poly->vertices.e[c].y;
		else if (poly->vertices.e[c].y>p2.y) p2.y=poly->vertices.e[c].y;

		if (poly->vertices.e[c].z<p1.z) p1.z=poly->vertices.e[c].z;
		else if (poly->vertices.e[c].z>p2.z) p2.z=poly->vertices.e[c].z;
	}
	
	return .005*norm(p1-p2);
}

//! Save a Polyptych file
/*! Return 0 for success or nonzero for error.
 */
int HedronWindow::Save(const char *saveto)
{
	FILE *f=fopen(saveto,"w");
	if (!f) {
		cerr<<"WARNING: could not open "<<saveto<<" for saving!"<<endl;
		return 1;
	}
	
	if (polyhedronfile && *polyhedronfile) fprintf(f,"polyhedronfile %s\n",polyhedronfile);
	if (spherefile && *spherefile) fprintf(f,"spherefile %s\n",spherefile);

	
	fprintf(f,"basis\n"
			  "  p %.10g,%.10g,%.10g\n"
			  "  x %.10g,%.10g,%.10g\n"
			  "  y %.10g,%.10g,%.10g\n"
			  "  z %.10g,%.10g,%.10g\n",
			   extra_basis.p.x, extra_basis.p.y, extra_basis.p.z,
			   extra_basis.x.x, extra_basis.x.y, extra_basis.x.z,
			   extra_basis.y.x, extra_basis.y.y, extra_basis.y.z,
			   extra_basis.z.x, extra_basis.z.y, extra_basis.z.z
		   );
	if (nets.n) {
		for (int c=0; c<nets.n; c++) {
			if (nets.e[c]->numActual()==1) continue; //don't bother with single face nets
			fprintf(f,"net\n");
			nets.e[c]->dump_out(f,2,0,NULL);
		}
	}


	fclose(f);
	touch_recently_used_xbel(saveto, "application/x-polyptych-doc", NULL,NULL, "Polyptych", true,true, NULL);
	return 0;
}


//----------------------- polyhedron gl mapping

//! Break down a triangle into many, assigning spherical texture points along the way.
/*! This is akin to geodesic-izing, with n segments along each original triangle edge,
 *
 * Use p1,p2,p3 as the actual place of the triangle in space, and p1o,p2o,p3o
 * for where to put the triangle for texturizing.
 */
void HedronWindow::triangulate(spacepoint p1 ,spacepoint p2 ,spacepoint p3,
							 spacepoint p1o,spacepoint p2o,spacepoint p3o,
							 int n)
{
	spacepoint row [2][n+1],
			   rowO[2][n+1],
			   left,right,
			   lefto,righto,
			   normal;
	normal=(p3-p1)/(p2-p1);
	normal/=norm(normal);

	int a=0, b=1, c, m=n;
	for (c=0; c<n; c++) {
		 //construct a point grid, say n==4:
		 //40
		 //30 31
		 //20 21 22
		 //10 11 12 13
		 //00 01 02 03 04
		if (c==0) {
			left= p1;
			right=p2;
			lefto=p1o;
			righto=p2o;

			for (int c2=0; c2<=m; c2++) {
				row[a][c2] =left +(right -left )*(double)c2/m;
				rowO[a][c2]=lefto+(righto-lefto)*(double)c2/m;				
			}
		}
		left= p1+(p3-p1)*(double)(c+1)/n;
		right=p2+(p3-p2)*(double)(c+1)/n;
		lefto= p1o+(p3o-p1o)*(double)(c+1)/n;
		righto=p2o+(p3o-p2o)*(double)(c+1)/n;

		m--;
		for (int c2=0; c2<=m; c2++) {
			row [b][c2]=left +(right -left )*(double)c2/m;
			rowO[b][c2]=lefto+(righto-lefto)*(double)c2/m;
		}

		 //output gl coordinates for each point in the grid
		glBegin(GL_TRIANGLE_STRIP);
		  for (int c2=0; c2<=m; c2++) {
			//normal=row[a][c2]/norm(row[a][c2]);
			addGLSphereTexpt(rowO[a][c2].x, rowO[a][c2].y, rowO[a][c2].z, &extra_basis);
			glNormal3f(normal.x, normal.y, normal.z);
			glVertex3f(row[a][c2].x, row[a][c2].y, row[a][c2].z);
			
			//normal=row[b][c2]/norm(row[b][c2]);
			addGLSphereTexpt(rowO[b][c2].x, rowO[b][c2].y, rowO[b][c2].z, &extra_basis);
			glNormal3f(normal.x, normal.y, normal.z);
			glVertex3f(row[b][c2].x, row[b][c2].y, row[b][c2].z);
	 	  }
		  addGLSphereTexpt(rowO[a][m+1].x, rowO[a][m+1].y, rowO[a][m+1].z, &extra_basis);
		  glNormal3f(row[a][m+1].x, row[a][m+1].y, row[a][m+1].z);
		  glVertex3f(row[a][m+1].x, row[a][m+1].y, row[a][m+1].z);
		glEnd();

		if (a==0) { a=1; b=0; } else { a=0; b=1; }
	}
}

//! Update the gl hedron (referenced by thing) with the current texture.
/*! This uses the coordinates of poly, but thing can be anything.
 *
 * \todo There is surely a more intelligent way to drag around textures. There is serious
 *       lag for high face counts
 */
void HedronWindow::mapPolyhedronTexture(Thing *thing)
{
	//DBG cerr <<"mapping hedron texture..."<<endl;
	spacepoint center,centero,normal,point;
	spacepoint p1,p2, p1o,p2o;
	int c,c2;

	glNewList(thing->id,GL_COMPILE);
	for (c=0; c<poly->faces.n; c++) {
		//DBG if (c!=poly->faces.n-3) continue;
		if (poly->faces.e[c]->pn) {
			center =poly->CenterOfFace(c,1);
			centero=poly->CenterOfFace(c,0);
			//DBG cerr <<"--center: "<<center.x<<","<<center.y<<","<<center.z<<endl;

			//-----------------------------
			for (c2=0; c2<=poly->faces.e[c]->pn; c2++) {
				p1 =poly->VertexOfFace(c, (c2+1)%poly->faces.e[c]->pn, 1);
				p1o=poly->VertexOfFace(c, (c2+1)%poly->faces.e[c]->pn, 0);
				p2 =poly->VertexOfFace(c, c2%poly->faces.e[c]->pn, 1);
				p2o=poly->VertexOfFace(c, c2%poly->faces.e[c]->pn, 0);

				//DBG cerr <<"--p1: "<<p1.x<<","<<p1.y<<","<<p1.z<<endl;
				//DBG cerr <<"--p2: "<<p2.x<<","<<p2.y<<","<<p2.z<<endl;
				//DBG cerr <<"--p1o: "<<p1o.x<<","<<p1o.y<<","<<p1o.z<<endl;
				//DBG cerr <<"--p2o: "<<p2o.x<<","<<p2o.y<<","<<p2o.z<<endl;

				triangulate(center ,p1, p2,
							centero,p1o,p2o,
							4);
			}
			//-----------------------------
//			glBegin(GL_TRIANGLE_FAN);
//
//			normal=center/norm(center);
//			addGLSphereTexpt(center.x,center.y,center.z);
//			glNormal3f(normal.x,normal.y,normal.z);
//			glVertex3f(center.x,center.y,center.z);
//			for (c2=0; c2<=poly->faces.e[c]->pn; c2++) {
//				if (c2==poly->faces.e[c]->pn) point=poly->vertices.e[poly->faces.e[c]->p[0]];
//				else point=poly->vertices.e[poly->faces.e[c]->p[c2]];
//				DBG cerr <<"point: "<<point.x<<","<<point.y<<","<<point.z<<endl;
//				normal=point/norm(point);
//		  		addGLSphereTexpt(point.x,point.y,point.z);
//		  		glNormal3f(normal.x,normal.y,normal.z);
//		  		glVertex3f(point.x,point.y,point.z);
//			}
//
//			glEnd();
			//-----------------------------
		}
	}

	//------------------------
//	GLUtesselator* polygon=gluNewTess();
//	gluTessCallback
//	if (polygon==NULL) return; // not enough memory
//
//	int r,g,b;
//	for (c=0; c<poly->faces.n; c++) {
//		if (poly->faces.e[c]->pn) {
//
//			*** to keep using vectors, would have to make an array here with all the points in *GLdouble format
//			gluTessBeginPolygon(polygon);
//			gluTessBeginContour(polygon);
//				for(c2=0; c2<poly->faces.e[c]->pn; c2++) {
//					 gluTessVertex(polygon,poly->vertices.e[poly->faces.e[c]->p[c2]],poly->vertices.e[poly->faces.e[c]->p[c2]]);
//						 *** this uses double[3] for coordinates, not vector
//							 ***poly->f[c].p[c2].x,poly->f[c].p[c2].y,poly->f[c].p[c2].y);
//				}
//			gluTessEndContour(polygon);
//			gluTessEndPolygon(polygon);
//
//		}
//	}
//	gluDeleteTess(polygon);
	//------------------------
	
	glEndList();
}

//! Create a new Thing with a new gl call list id.
/*! This is independent of any texture, or polyhedron coordinates.
 */
Thing *HedronWindow::makeGLPolyhedron()
{
	if (!poly) return NULL;
	
	GLuint hedronlist=glGenLists(1);
	Thing *thing=new Thing(hedronlist);

	thing->SetScale(5.,5.,5.);
	thing->SetColor(1., 1., 1., 1.0);
	thing->SetPos(0,0,0);

	//mapPolyhedronTexture(thing);

	//DBG cerr <<"New hedron call list id = "<<hedronlist<<endl;
	//DBG dumpMatrix4(thing->m,"hedron initial matrix");
	return thing;
}


//---------------------- data for a camera shape ----------------

//! Define positions and orientations for 4 basic cameras.
/*! This sets up 3 cameras, one per axis pointing at the origin,
 * and one along the line between (0,0,0) and (1,1,1).
 * 
 * The basic shape is a pyramid with base at z=-1, apex at origin, and a little arrow pointing in +y direction
 */
void HedronWindow::makecameras(void)
{
	GLuint ACAMERA=glGenLists(1);

	glNewList(ACAMERA,GL_COMPILE);
		glColor3f(1,1,1);
		glBegin(GL_LINE_STRIP);
			glVertex3f(1,-1,-1);
			glVertex3f(1,1,-1);
			glVertex3f(-1,1,-1);
			glVertex3f(-1,-1,-1);
			glVertex3f(1,-1,-1);
			glVertex3f(0,0,0);
			glVertex3f(1,1,-1);
			glVertex3f(.3,1.1,-1);
			glVertex3f(0,1.5,-1);
			glVertex3f(-.3,1.1,-1);
			glVertex3f(-1,1,-1);
			glVertex3f(0,0,0);
			glVertex3f(-1,-1,-1);
		glEnd();
	glEndList();
	camera_shape.id=ACAMERA;
	camera_shape.SetScale(3.,3.,7.);

	current_camera=0;
	EyeType *eye=new EyeType();
	eye->Set(spacepoint(40,0,0),spacepoint(0,0,0),spacepoint(40,0,1));
	eye->makestereo();
	cameras.push(eye);

	eye=new EyeType();
	eye->Set(spacepoint(0,40,0),spacepoint(0,0,0),spacepoint(0,40,1));
	eye->makestereo();
	cameras.push(eye);

	eye=new EyeType();
	eye->Set(spacepoint(0,0,40),spacepoint(0,0,0),spacepoint(1,0,40));
	eye->makestereo();
	cameras.push(eye);

	eye=new EyeType();
	eye->Set(spacepoint(40,40,40),spacepoint(0,0,0),spacepoint(40,40,41));
	eye->makestereo();
	cameras.push(eye);
}


/*--------------------- Lighting ------------------------*/

void HedronWindow::setlighting(void)
{
	lightmaterial.mask=MAT_DIFFUSE|MAT_SPECULAR|MAT_EMISSION;
	lightmaterial.face=GL_FRONT_AND_BACK;
	lightmaterial.diffuse[0]=0;
	lightmaterial.diffuse[1]=0;
	lightmaterial.diffuse[2]=0;
	lightmaterial.diffuse[3]=1;
	lightmaterial.specular[0]=0;
	lightmaterial.specular[1]=0;
	lightmaterial.specular[2]=0;
	lightmaterial.specular[3]=1;
	lightmaterial.emission[0]=.8;
	lightmaterial.emission[1]=.8;
	lightmaterial.emission[2]=.8;
	lightmaterial.emission[3]=1;
		

	Light *light=new Light;

	 // Create LIGHT0
	light->Position(20.,-20.,20.,1.0);
	light->Ambient( 0.6, 0.6, 0.6, 1.0 );
	//light->Ambient( 1., 1, 1, 1.0 );
	light->Diffuse( 1., 1., 1., 1.0 );
	light->Specular(  1.0, 1.0, 1.0, .5 );
	//light->Specular(  0, 0, 1.0, 1.0 );
	glLightfv (GL_LIGHT0, GL_AMBIENT, light->ambient);
	glLightfv (GL_LIGHT0, GL_DIFFUSE, light->diffuse);
	glLightfv (GL_LIGHT0, GL_SPECULAR, light->specular);
	glLightfv (GL_LIGHT0, GL_POSITION, light->position);
	glLightf (GL_LIGHT0, GL_LINEAR_ATTENUATION, .1);
	glEnable (GL_LIGHT0);
	lights.push(light);
	
	 // Create LIGHT1
	light=new Light;
	light->Position(20,20,30,1);
	light->Ambient( 0.6, 0.6, 0.6, 1.0 );
	//light->Ambient( 1., 1, 1, 1.0 );
	light->Diffuse( 1., 1., 1., 1.0 );
	//light->Specular(  1.0, 1.0, 1.0, 1.0 );
	light->Specular(  1.0, 1.0, 1.0, .5 );
	glLightfv (GL_LIGHT1, GL_AMBIENT, light->ambient);
	glLightfv (GL_LIGHT1, GL_DIFFUSE, light->diffuse);
	glLightfv (GL_LIGHT1, GL_SPECULAR, light->specular);
	glLightfv (GL_LIGHT1, GL_POSITION, light->position);
	glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION, .1);
	glEnable (GL_LIGHT1);
	lights.push(light);
	
	 // Additional lighting setup calls
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

	glEnable (GL_LIGHTING);
	//glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE); <-- default
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);//smooth is default
	//glEnable(GL_AUTO_NORMAL);
	//glEnable(GL_NORMALIZE);
	//glCullFace (GL_BACK);
	//glEnable (GL_CULL_FACE);
		
}

//! Make a Polyhedron cube, with sides length 2, centered at origin.
Polyhedron *HedronWindow::defineCube()
{
	Polyhedron *newpoly=new Polyhedron;
	makestr(newpoly->name,"Cube");

	newpoly->vertices.push(spacepoint(1,1,1));
	newpoly->vertices.push(spacepoint(-1,1,1));
	newpoly->vertices.push(spacepoint(-1,-1,1));
	newpoly->vertices.push(spacepoint(1,-1,1));
	newpoly->vertices.push(spacepoint(-1,1,-1));
	newpoly->vertices.push(spacepoint(1,1,-1));
	newpoly->vertices.push(spacepoint(1,-1,-1));
	newpoly->vertices.push(spacepoint(-1,-1,-1));

	newpoly->faces.push(new Face("0 1 2 3",""));
	newpoly->faces.push(new Face("1 0 5 4",""));
	newpoly->faces.push(new Face("2 1 4 7",""));
	newpoly->faces.push(new Face("0 3 6 5",""));
	newpoly->faces.push(new Face("3 2 7 6",""));
	newpoly->faces.push(new Face("4 5 6 7",""));
	newpoly->connectFaces();
	newpoly->makeedges();
	//DBG newpoly->dump_out(stdout, 0, 0, NULL);
	newpoly->BuildExtra();

	return newpoly;
}

int HedronWindow::init()
{
	//---------------Polyhedron coordinate and texture final setup

	 // *** these two are static variables that really should be elsewhere
	if (!glvisual)  glvisual = glXChooseVisual(app->dpy, DefaultScreen(app->dpy), attributeListDouble);
	if (!glcontext) glcontext= glXCreateContext(app->dpy, glvisual, 0, GL_TRUE);

	if (!poly) {
		poly=defineCube();
		cylinderscale=edgeScaleFromBox();
		nets.flush();
		if (currentnet) currentnet->dec_count();
		currentnet=NULL;
		makestr(polyhedronfile,NULL);
		currentface=currentpotential=-1;
		needtodraw=1;
	}

	if (!spheremap_data) {
		UseGenericImageData();
	}


	 //-----------------Base window initialization
	anXWindow::init();


	return 0;
}

//! Enable GL_TEXTURE_2D, define spheretexture id (if definetid), and load in spheremapdata.
void HedronWindow::installSpheremapTexture(int definetid)
{
	//called from a MapNotify,
	//also after installImage()


	 //--------------------texture initialization
	glEnable(GL_TEXTURE_2D);

	 //generate a gl texture id (it is put in spheretexture)
	if (definetid) glGenTextures(1,&spheretexture); //can generate several texture ids
	//floor_texture=spheretexture;
	//glGenTextures(1,&flattexture);

	glBindTexture(GL_TEXTURE_2D, spheretexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	 //init with mipmapping:
//	 // when texture area is small, bilinear filter the closest mipmap
//	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
//	                 GL_LINEAR_MIPMAP_NEAREST );
//	 // when texture area is large, bilinear filter the original
//	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
//
//	 // the texture wraps over at the edges (repeat)
//	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
//	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	 //alternate:
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


	 //Note cacheing textures has to be done after some other gl initializations,
	 // or else they won't stick. Took forever to debug exactly what, and I forgot
	 // to write down exactly what initializations! Grr!
	glTexImage2D(GL_TEXTURE_2D,   //target
				 0,               //mipmap level
				 GL_RGB,          //number of color components per pixel
				 spheremap_width, //width
				 spheremap_height,//height
				 0,               //border with, must be 0 or 1
				 GL_BGR,          //order of data
				 GL_UNSIGNED_BYTE,//type of data
				 spheremap_data); //pixel data
	//gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, infoheader.data);


	 //check if texture is resident
	DBG GLint yes;
	DBG glGetTexParameteriv(spheretexture,GL_TEXTURE_RESIDENT,&yes);
	DBG if (yes) cerr <<"----------------texture is resident"<<endl;
	DBG else cerr <<"----------------texture is NOT resident"<<endl;
}

void HedronWindow::installOverlays()
{
	overlays.push(new Overlay("Roll",ACTION_Roll));
	overlays.push(new Overlay("Unwrap",ACTION_Unwrap));
	overlays.push(new Overlay("Reseed",ACTION_Reseed));
	overlays.push(new Overlay("Shift Texture", ACTION_Shift_Texture));
	overlays.push(new Overlay("Rotate Texture", ACTION_Rotate_Texture));
	overlays.push(new Overlay("Zoom", ACTION_Zoom));

	//ACTION_Rotate,
	//ACTION_Pan,
	//ACTION_Toggle_Mode,
	//ACTION_Unwrap_Angle,
	
	 //---place overlays
	int maxw=0, w, h;
	FTBBox bbox;
	FTPoint p1,p2;

	 //determine bounds
	for (int c=0; c<overlays.n; c++) {
		if (isblank(overlays.e[c]->Text())) continue;

		bbox=consolefont->BBox(overlays.e[c]->Text(),-1);
		p1=bbox.Upper();
		p2=bbox.Lower();
		w=p1.X()-p2.X() + pad;
		h=p1.Y()-p2.Y() + pad;

		if (w>maxw) maxw=w;
		overlays.e[c]->setbounds(0,w, 0,h);
	}
	placeOverlays();


	remapPaperOverlays();
}

void HedronWindow::remapPaperOverlays()
{
	static char str[300];

	paperoverlays.flush();
	//paperoverlays.push(new Overlay("Papers",ACTION_Paper,OVERLAY_Just_Display,-1));
	paperoverlays.push(new Overlay("Papers",ACTION_Paper,OVERLAY_Button,-1));
	
//	if (currentpaper>=0) {
//		sprintf(str,"%s, %gx%g %s", papers.e[currentpaper]->name,
//									papers.e[currentpaper]->width,
//									papers.e[currentpaper]->height,
//									papers.e[currentpaper]->units);
//		paperoverlays.push(new Overlay(str,ACTION_None,OVERLAY_Just_Display,-1));
//	}
	for (int c=0; c<papers.n; c++) {
		sprintf(str,"%d. %s, %gx%g %s", c+1,
									papers.e[c]->name,
									papers.e[c]->width,
									papers.e[c]->height,
									papers.e[c]->units);
		paperoverlays.push(new Overlay(str,ACTION_Paper,OVERLAY_Button,c));
	}
	paperoverlays.push(new Overlay("+",ACTION_AddPaper,OVERLAY_Button,-1));


	int y=fontsize;
	double w,h;
	for (int c=0; c<paperoverlays.n; c++) {
		w=getExtent(paperoverlays.e[c]->Text(),-1, NULL,&h);
		//w+=pad;
		//h+=pad;
		paperoverlays.e[c]->setbounds(win_w-w,win_w, y,y+h);
		y+=h;
	}
}

//! Determine bounding box of the string with consolefont.
/*! Return value is the width, the same as what width variable gets set to.
 */
double HedronWindow::getExtent(const char *str,int len, double *width,double *height)
{
	double w=0,h=0;
	FTBBox bbox;
	FTPoint p1,p2;

	if (!isblank(str) && consolefont) {

		bbox=consolefont->BBox(str,len);
		p1=bbox.Upper();
		p2=bbox.Lower();
		w=p1.X()-p2.X() + pad;
		h=p1.Y()-p2.Y() + pad;
	}
	if (width) *width=w;
	if (height) *height=h;
	return w;
}

//! Assuming bounds already set, line up overlays centered on left of screen.
void HedronWindow::placeOverlays()
{
	 //Touch helper overlays:
	int totalheight=0;
	for (int c=0; c<overlays.n; c++) {
		totalheight+=overlays.e[c]->maxy-overlays.e[c]->miny+1;
	}
	 //place the overlays centered on left
	int y=win_h/2 - totalheight/2;
	int h;
	for (int c=0; c<overlays.n; c++) {
		h=overlays.e[c]->maxy-overlays.e[c]->miny;
		overlays.e[c]->miny=y;
		overlays.e[c]->maxy=y+h;

		y+=h+1;
	}

	 //Paper overlays:
	remapPaperOverlays();
}

int HedronWindow::MoveResize(int x,int y,int w,int h)
{
	int c=anXWindow::MoveResize(x,y,w,h);
	reshape(0,0, win_w,win_h);
	return c;
}

int HedronWindow::Resize(int w,int h)
{
	int c=anXWindow::Resize(w,h);
	reshape(0,0, win_w,win_h);
	return c;
}

//! Setup camera based on new window dimensions, and placeOverlays().
/*! This is called when the window size is changed.
 */
void HedronWindow::reshape (int x, int y, int w, int h)
{
	helpoffset=10000; //a special value to force remap based on text extents
	placeOverlays();

	glViewport (x, y, (GLsizei) w, (GLsizei) h);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective(fovy/M_PI*180, //field of view vertically in degrees (fovy)
				   ((double)w)/h, //ratio width to height
				   1.0,           //distance to near clipping plane
				   300.0);        //distance to far clipping plane
	
	//x field of view == 2*atan(tan(fovy/2)*w/h)
	//d "distance" to viewing plane is y/2/tan(fovy/2)

	glMatrixMode(GL_MODELVIEW);
}  


void HedronWindow::Refresh()
{
	if (!win_on) return;
	if (glXGetCurrentDrawable()!=xlib_window) glXMakeCurrent(app->dpy, xlib_window, glcontext);

	needtodraw=0;

	if (mode==MODE_Help) drawHelp();
	else {
		//-----------------
//		cureye='l';
//		reshape(0,0, win_w/2,win_h);
//		Refresh3d();
//
//		cureye='r';
//		reshape(win_w/2,0, win_w/2,win_h);
//		Refresh3d();
//
//		reshape(0,0, win_w,win_h);
		//-----------------
		Refresh3d();
		//-----------------
	}

	 //--all done drawing, now swap buffers
	glFlush();
	glXSwapBuffers(app->dpy,xlib_window);
}

void HedronWindow::drawHelp()
{
	char *helptext=NULL;

	DBG cerr <<"net "<<MODE_Net<<" old:"<<oldmode<<endl;

	if (!sc) GetShortcuts();

	ShortcutDefs *s;
	WindowActions *a;
	WindowAction *aa;
	ShortcutManager *m=GetDefaultShortcutManager();
	char buffer[100],str[400];
	s=sc->Shortcuts();
	a=sc->Actions();

	 //output all bound keys
	helptext=newstr("left mouse button  rolls shape\n"
			 "shift-l button  rotates along axis pointing at viewer\n"
			 "control-l button  shifts texture\n"
			 "control-shift-l button  rotates texture\n"
			 "right button  handles unwrapping\n"
			 "middle button  changes angle of unwrap\n"
			 "\n");
	if (s) {
		for (int c2=0; c2<s->n; c2++) {
			sprintf(str,"  %-15s ",m->ShortcutString(s->e[c2], buffer));
			if (a) aa=a->FindAction(s->e[c2]->action); else aa=NULL;
			if (aa) {
				 //print out string id and commented out description
				//sprintf(str+strlen(str),"%-20s",aa->name);
				if (!isblank(aa->description)) sprintf(str+strlen(str),"%s",aa->description);
				sprintf(str+strlen(str),"\n");
			} else sprintf(str+strlen(str),"%d\n",s->e[c2]->action); //print out number only
			appendstr(helptext,str);
		}
	}

	if (helptext) {
		int width=0, w;
		int numlines=0;
		int len=strlen(helptext);
		const char *eol=NULL, *str=helptext;

		while (str) {
			eol=strchr(str,'\n');
			if (eol) len=eol-str;
			else len=strlen(str);
			
			w=consolefont->Advance(str,len);
			if (w>width) width=w;
			
			numlines++;
			str=(eol?eol+1:NULL);
		}
		str=helptext;
		int y=helpoffset, offset=0;
		if (y==10000) y=win_h-fontsize;

		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		//glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

		glEnable (GL_LIGHTING);
		//glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE); <-- default
		glEnable(GL_COLOR_MATERIAL);
		//glEnable(GL_DEPTH_TEST);

		 //load the correct eye
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		//y=win_h/2 + numlines*consolefont->LineHeight()/2;
		//y=win_h/2 + numlines*fontsize/2;
		offset=win_w/2-width/2;
		for (int c=0; str && c<numlines; c++) {
			eol=strchr(str,'\n');
			if (eol) {
				len=eol-str;
			} else {
				len=strlen(str);
			}
			//offset=win_w/2 - lineextent/2; //to center

			consolefont->Render(str,len, FTPoint(offset,y));
			y-=fontsize;
			str= (eol?eol+1:NULL);
			//*** or pop up a messagebar window, that would be easy
		}

		delete[] helptext;
	}

}

//! Draw all the stuff in the scene
void HedronWindow::Refresh3d()
{
	int c;
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

	glEnable (GL_LIGHTING);
	//glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE); <-- default
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);

	 //load the correct eye
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (current_camera<0) return;
	EyeType *b=cameras.e[current_camera];
	if (cureye==0) {
		gluLookAt(b->m.p.x,b->m.p.y,b->m.p.z, 
				  //b->focus.x,b->focus.y,b->focus.z,
				  b->m.p.x-b->m.z.x, b->m.p.y-b->m.z.y, b->m.p.z-b->m.z.z,
				  b->m.y.x, b->m.y.y, b->m.y.z);
	} else if (cureye=='l') {
		gluLookAt(b->l.p.x,b->l.p.y,b->l.p.z, 
				  //b->focus.x,b->focus.y,b->focus.z,
				  b->l.p.x-b->l.z.x, b->l.p.y-b->l.z.y, b->l.p.z-b->l.z.z,
				  b->l.y.x, b->l.y.y, b->l.y.z);
	} else { //right eye
		gluLookAt(b->r.p.x,b->r.p.y,b->r.p.z, 
				  //b->focus.x,b->focus.y,b->focus.z,
				  b->r.p.x-b->r.z.x, b->r.p.y-b->r.z.y, b->r.p.z-b->r.z.z,
				  b->r.y.x, b->r.y.y, b->r.y.z);
	}
	
	
	if (draw_axes) drawaxes(10);
	
	glColor3f (0, 1.0, 0);

	
	 //----  Draw textured triangle
//	glPushMatrix();
//	//setmaterial(.8,.8,.8);
//	glEnable(GL_TEXTURE_2D);
//
//	glBindTexture(GL_TEXTURE_2D, spheretexture);
//
//	 //check if texture is resident
//	GLint yes;
//	glGetTexParameteriv(spheretexture,GL_TEXTURE_RESIDENT,&yes);
//	if (yes) cout <<"--refresh--------texture is resident"<<endl;
//	else cout <<"--refresh--------texture is NOT resident"<<endl;
//	int checkw=-1,checkh=-1;
//	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&checkw);
//	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&checkh);
//	cout <<"--refresh---------texture width,height: "<<checkw<<" x "<<checkh<<endl;
//
//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexImage2D(GL_TEXTURE_2D,   //target
//				 0,               //mipmap level
//				 GL_RGB,          //number of color components per pixel
//				 spheremap_width, //width
//				 spheremap_height,//height
//				 0,               //border with, must be 0 or 1
//				 GL_RGB,          //order of data
//				 GL_UNSIGNED_BYTE,//type of data
//				 spheremap_data); //pixel data
//
//	glBegin (GL_TRIANGLE_STRIP);
//		glTexCoord2f(1,0);
//		glVertex2f(25.0, 0);
//
//		glTexCoord2f(1,1);
//		glVertex2f(25.0, -25.0);
//
//		glTexCoord2f(0,0);
//		glVertex2f(0.0, 0.0);
//
//		glTexCoord2f(0,1);
//		glVertex2f(0, -25.0);
//	glEnd();
//	glDisable(GL_TEXTURE_2D);
//	glPopMatrix();


	 //---- draw things
	if (draw_texture) {
		DBG cerr <<"draw texture: "<<(spheremap_data?"yes ":"no ")<<" w="<<spheremap_width<<" h="<<spheremap_height<<endl;
		
		glEnable(GL_TEXTURE_2D);
	}

	for (c=0; c<things.n; c++) {
		//DBG cerr <<"drawing thing number "<<c<<endl;
		things.e[c]->Draw();
	}

	 //draw frame of hedron (without nets)
	if (draw_edges) {
		//GLUquadricObj *q;
		glDisable(GL_TEXTURE_2D);

		glPushMatrix();
		glMultMatrixf(hedron->m);
		//glScalef(hedron->scale[0],hedron->scale[1],hedron->scale[2]);
		spacepoint p1,p2,a,v;
		//double h;
		//double ang;

		glLineWidth(3);
		glBegin(GL_LINES);
		for (int c2=0; c2<poly->edges.n; c2++) {
			if (poly->edges.e[c2]->p1<0 || poly->edges.e[c2]->p2<0) continue;

			p1=poly->vertices[poly->edges.e[c2]->p1];
			p2=poly->vertices[poly->edges.e[c2]->p2];
			v=p1-p2;
			a=v/spacepoint(0,0,1);
			//h=norm(v);
			//a=spacepoint(0,0,1)/v;
			//ang=acos(v.z/h)/M_PI*180;

			glVertex3f(p1.x,p1.y,p1.z);
			glVertex3f(p2.x,p2.y,p2.z);

			//DBG cerr <<"edge: "<<p1.x<<','<<p1.y<<','<<p1.z<<" to "<<p2.x<<','<<p2.y<<','<<p2.z<<endl;
//				glPushMatrix();
//				q=gluNewQuadric();
//				glTranslatef(p1.x,p1.y,p1.z);
//				glRotatef(ang, a.x,a.y,a.z);
////				gluLookAt(p1.x,p1.y,p1.z,
////						  p2.x,p2.y,p2.z,
////						  0,0,1);
//				gluCylinder(q,
//						 	h*.03,  //base radius
//							h*.03,  //top radius
//							h, //height
//							8,  //num slices
//							1); //num stacks up z
//				gluDeleteQuadric(q);
//				glPopMatrix();
		}

		 //draw lines around any faces up in nets
		for (int c2=0; c2<poly->faces.n; c2++) {
			if (poly->faces.e[c2]->cache->facemode>=0) continue;
			for (int c3=0; c3<poly->faces.e[c2]->pn; c3++) {
				p1=poly->faces.e[c2]->cache->points3d[c3];
				p2=(c3==0?poly->faces.e[c2]->cache->points3d[poly->faces.e[c2]->pn-1]
						:poly->faces.e[c2]->cache->points3d[c3-1]);
				v=p1-p2;
				a=v/spacepoint(0,0,1);
				//h=norm(v);
				//a=spacepoint(0,0,1)/v;
				//ang=acos(v.z/h)/M_PI*180;

				glVertex3f(p1.x,p1.y,p1.z);
				glVertex3f(p2.x,p2.y,p2.z);
			}
		}
		glEnd();
		glLineWidth(1);

		glPopMatrix();
	}
	glDisable(GL_TEXTURE_2D);
	


	 //------draw red cylinders on seam edges--
	if ((draw_seams&1) && nets.n && (mode==MODE_Net || mode==MODE_Solid)) {
		int face;
		//basis bas=poly->basisOfFace(face);
		spacepoint p;
		flatpoint v;
		NetFaceEdge *edge;

		for (int n=0; n<nets.n; n++) {
		  for (int c=0; c<nets.e[n]->faces.n; c++) {
			if (nets.e[n]->faces.e[c]->tag!=FACE_Actual) continue;
			 //------draw red cylinders on seam edges--
			spacepoint p1,p2;
			for (int c2=0; c2<nets.e[n]->faces.e[c]->edges.n; c2++) {
				 //continue if edge is not a seam
				edge=nets.e[n]->faces.e[c]->edges.e[c2];
				if (!(edge->toface==-1 || (edge->toface>=0 && nets.e[n]->faces.e[edge->toface]->tag!=FACE_Actual)))
					continue;

				//------------------------
				face=nets.e[n]->faces.e[c]->original;
				p1=poly->VertexOfFace(face,c2,(mode==MODE_Net?1:0));
				p2=poly->VertexOfFace(face,(c2+1)%poly->faces.e[face]->pn,(mode==MODE_Net?1:0));
				//------------------------
				//v=transform_point(nets.e[n]->faces.e[c]->matrix, edge->points->p());
				//p1=bas.p + v.x*bas.x + v.y*bas.y;
				//v=transform_point(nets.e[n]->faces.e[c]->matrix, 
								  //nets.e[n]->faces.e[c]->edges.e[(c2+1)%nets.e[n]->faces.e[c]->edges.n]->points->p());
				//p2=bas.p + v.x*bas.x + v.y*bas.y;
				//------------------------

				setmaterial(1,.3,.3);
				drawCylinder(p1,p2,cylinderscale,hedron->m);
			}
		  }
		}
	}

	 //------draw green cylinders between faces--
	if ((draw_seams&2) && nets.n && (mode==MODE_Solid)) {
		int face;
		//Basis bas=poly->basisOfFace(face);
		spacepoint p;
		flatpoint v;
		NetFaceEdge *edge;
		Net *net;
		spacepoint p1,p2;

		for (int n=0; n<nets.n; n++) {
		  net=nets.e[n];
		  for (int c=0; c<net->faces.n; c++) {
			if (net->faces.e[c]->tag!=FACE_Actual) continue;
			for (int c2=0; c2<nets.e[n]->faces.e[c]->edges.n; c2++) {
				 //continue if edge is not a seam
				edge=nets.e[n]->faces.e[c]->edges.e[c2];
				if (edge->toface==-1 || (edge->toface>=0 && nets.e[n]->faces.e[edge->toface]->tag!=FACE_Actual))
					continue;

				 //perhaps replace with c1 + v||(c2-c1) + v||((c2-c1)/(p2-p1)), v=p1-c1
				face=nets.e[n]->faces.e[c]->original;
				p1=poly->CenterOfFace(face,(mode==MODE_Net?1:0));
				p2= (poly->VertexOfFace(face,c2,(mode==MODE_Net?1:0))
					+poly->VertexOfFace(face,(c2+1)%poly->faces.e[face]->pn,(mode==MODE_Net?1:0)))/2;

				setmaterial(.3,1,.3);
				drawCylinder(p1,p2,cylinderscale,hedron->m);
			}
		  }
		}
	}


	 //---- draw dotted face outlines for each potential face of currentnet
	if (currentnet && mode==MODE_Net) {
		int face=currentnet->info; //the seed face for the net
		Basis bas=poly->basisOfFace(face);
		spacepoint p;
		flatpoint v;

		for (int c=0; c<currentnet->faces.n; c++) {

			if (currentnet->faces.e[c]->tag!=FACE_Potential) continue;
			if (poly->faces.e[currentnet->faces.e[c]->original]->cache->facemode!=0) continue;

			glPushMatrix();
			glMultMatrixf(hedron->m);
			setmaterial(1,1,1);
			glLineStipple(1, 0x00ff);//factor, bit pattern 0=off 1=on
			glEnable(GL_LINE_STIPPLE);
			glBegin (GL_LINE_LOOP);

			for (int c2=0; c2<currentnet->faces.e[c]->edges.n; c2++) {
				v=transform_point(currentnet->faces.e[c]->matrix,currentnet->faces.e[c]->edges.e[c2]->points->p());
				p=bas.p + v.x*bas.x + v.y*bas.y;
				glVertex3f(p.x, p.y, p.z);
			}

			glEnd();
			glPopMatrix();
			glDisable(GL_LINE_STIPPLE);

		}

		if (currentpotential>=0) {
			 //draw textured polygon if hovering over a potential in current net
			drawPotential(currentnet,currentpotential);
		}

		if (draw_papers) {
			double w,h;
			double netmi[6];
			for (int c=0; c<papers.n; c++) {
				w=papers.e[c]->width;
				h=papers.e[c]->height;

				transform_invert(netmi,currentnet->m());
				glPushMatrix();
				glMultMatrixf(hedron->m);
				setmaterial(1,1,1);
				glBegin (GL_LINE_LOOP);

				// *** really need to figure out transforms, and use gl matrices instead

				v=transform_point(netmi,papers.e[c]->matrix.transformPoint(flatpoint(0,0)));
				p=bas.p + v.x*bas.x + v.y*bas.y;
				glVertex3f(p.x, p.y, p.z);

				v=transform_point(netmi,papers.e[c]->matrix.transformPoint(flatpoint(w,0)));
				p=bas.p + v.x*bas.x + v.y*bas.y;
				glVertex3f(p.x, p.y, p.z);

				v=transform_point(netmi,papers.e[c]->matrix.transformPoint(flatpoint(w,h)));
				p=bas.p + v.x*bas.x + v.y*bas.y;
				glVertex3f(p.x, p.y, p.z);

				v=transform_point(netmi,papers.e[c]->matrix.transformPoint(flatpoint(0,h)));
				p=bas.p + v.x*bas.x + v.y*bas.y;
				glVertex3f(p.x, p.y, p.z);

				glEnd(); //GL_LINE_LOOP
				glPopMatrix();
				glDisable(GL_LINE_STIPPLE);
			}
		}

		// *** need to draw outlines of other nets that share the current net's paper
	}


	 //---- draw cameras
	for (c=0; c<cameras.n; c++) {
		if (cameras.e[c]==cameras.e[current_camera]) continue;
		glPushMatrix();
		glMultMatrixf(cameras.e[c]->model);
		//cameras.e[c]->transformForDrawing();
		camera_shape.Draw();
		glPopMatrix();
	}
	


//	 //---- Draw sphere
//	glEnable(GL_TEXTURE_2D);
//	glBindTexture(GL_TEXTURE_2D, spheretexture);
//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
//
//	glPushMatrix();
//	glTranslatef(0,0,15);
//	GLUquadricObj *q=gluNewQuadric();
//	gluQuadricTexture(q,GL_TRUE);
//	//gluQuadricTexture(q,spheretexture);
//	gluSphere(q,4,10,10);
//	gluDeleteQuadric(q);
//	glPopMatrix();
//
//	glDisable(GL_TEXTURE_2D);

	 //---- Draw Lights
	SetMaterial(&lightmaterial,0);
	for (c=0; c<lights.n; c++) {
		glPushMatrix();
		glTranslatef(lights.e[c]->position[0],lights.e[c]->position[1],lights.e[c]->position[2]);
		glCallList(cubeCallList());
		glPopMatrix();
	}



	 //---draw current face
	if (buttondown.isdown(0,RIGHTBUTTON)) {
		if (rbdown>=0) transparentFace(rbdown,1,0,0,.3);
		if (currentface>=0 && rbdown!=currentface) transparentFace(currentface,0,1,0,.3);
	} else if (currentface>=0) {
		if (currentfacestatus==1) transparentFace(currentface,0,1,0,.3);//over a leaf
		else if (currentfacestatus==2) transparentFace(currentface,0,0,1,.3);
		else transparentFace(currentface,1,0,0,.3);
	}


	 //----write out text/menu overlay
	
	 //write at bottom of screen:  which face you are hovering over, whether the face is claimed in a net
	if (draw_info) {
		static char str[200];
		FTPoint ftpoint;
		if (currentface>=0) {
			sprintf(str,"face:%d  ",currentface);
			ftpoint=consolefont->Render(str,-1,ftpoint);
		}
		if (currentnet) { //show on net indicator
			int i=nets.findindex(currentnet);
			if (nets.e[i]->whatshape()) sprintf(str,"net %d: %s",i, nets.e[i]->whatshape());
			else sprintf(str,"net %d",i);

			DBG cerr <<str<<" face "<<currentface<<endl;
			consolefont->Render(str,-1,FTPoint(0,fontsize));
		}

		 //write at top of screen:  write currentmessage
		consolefont->Render(modename(mode),-1,FTPoint(0,win_h-fontsize));
		if (currentmessage) {
			consolefont->Render(currentmessage,-1,FTPoint(0,win_h-2*fontsize));
			messagetick++;
			if (messagetick>10) makestr(currentmessage,NULL);
		}
	}

	 //draw paper control overlays
	glFlush();
	if (mode==MODE_Net && draw_papers) {
		const char *text=NULL;
		Overlay *o=NULL;
		for (int c=0; c<paperoverlays.n; c++) {
			o=paperoverlays.e[c];
			text=o->Text();
			if (isblank(text)) continue;

			if (mouseover_group==OGROUP_Papers 
					&& mouseover_overlay==paperoverlays.e[c]->action
					&& mouseover_index==paperoverlays.e[c]->index) drawRect(*o, .4,.4,.4, .5,.5,.5, .5);
			else if (currentpaper>=0 && currentpaper==o->index) drawRect(*o, .5,.5,.5, .6,.6,.6, .5);
			else drawRect(*o, .2,.2,.2, .2,.2,.2, .5);

			consolefont->Render(text,-1, FTPoint(o->minx+pad/2, win_h-o->maxy+pad/2));
		}
	}

	 //draw overlay helpers
	glFlush();
	if (draw_overlays && overlays.n) {
		const char *text=NULL;
		Overlay *o=NULL;
		for (int c=0; c<overlays.n; c++) {
			o=overlays.e[c];
			text=o->Text();
			if (isblank(text)) continue;

			if (mouseover_group==OGROUP_TouchHelpers && mouseover_overlay==o->action) drawRect(*o, .5,.5,.5, .6,.6,.6, .5);
			else if (active_action==overlays.e[c]->action)  drawRect(*o, .5,.5,.5, .6,.6,.6, .5);
			else drawRect(*o, .2,.2,.2, .6,.6,.6, .5);
			//else drawRect(*o, 1,1,1, 1,1,1, .5);

			consolefont->Render(text,-1, FTPoint(o->minx+pad/2, win_h-o->maxy+pad/2));
		}
	}

}

//! Draw a rectangle in window coordinates with border, shading, and alpha.
void HedronWindow::drawRect(DoubleBBox &box, 
							double bg_r, double bg_g, double bg_b,
							double line_r, double line_g, double line_b,
							double alpha)
{
	//DBG cerr << "draw box ("<<box.minx<<','<<box.miny<<") -> ("<<box.maxx<<","<<box.maxy<<")"<<endl;


	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho (0, win_w, win_h, 0, 0, 1);

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

	glColor4f(bg_r,bg_g,bg_b, alpha);
	//setmaterial(bg_r,bg_g,bg_b);

	glBegin(GL_QUADS);
	  glVertex2f(box.minx, box.miny);
	  glVertex2f(box.maxx, box.miny);
	  glVertex2f(box.maxx, box.maxy);
	  glVertex2f(box.minx, box.maxy);
	glEnd();

	glColor4f(line_r,line_g,line_b,alpha);
	//setmaterial(line_r,line_g,line_b);
	glBegin(GL_LINE_LOOP);
	  glVertex2f(box.minx, box.miny);
	  glVertex2f(box.maxx, box.miny);
	  glVertex2f(box.maxx, box.maxy);
	  glVertex2f(box.minx, box.maxy);
	glEnd();

	glEnable (GL_LIGHTING);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	glMatrixMode (GL_PROJECTION);
	glPopMatrix();
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix();
}

//! Draw face as a colored transparent overlay (not textured).
void HedronWindow::transparentFace(int face, double r, double g, double b, double a)
{
	if (face<0 || face>=poly->faces.n) return;
	glPushMatrix();
	glMultMatrixf(hedron->m);

	 //enable transparent
	glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
	glColor4f(r,g,b,a);

	glBegin(GL_TRIANGLE_FAN);
	spacepoint center=1.001*poly->CenterOfFace(face,1);
	spacepoint normal=center/norm(center);
	spacepoint point;

	glNormal3f(normal.x,normal.y,normal.z);
	glVertex3f(center.x,center.y,center.z);
	for (int c2=0; c2<=poly->faces.e[face]->pn; c2++) {
		if (c2==poly->faces.e[face]->pn) point=poly->VertexOfFace(face,0,1);
		else point=poly->VertexOfFace(face, c2, 1);
		//DBG cerr <<"point: "<<point.x<<","<<point.y<<","<<point.z<<endl;
		point=1.001*point;
		normal=point/norm(point);
		glNormal3f(normal.x,normal.y,normal.z);
		glVertex3f(point.x,point.y,point.z);
	}
	glEnd();

	glDisable (GL_BLEND);

	glPopMatrix();
}

//! Currently draw face as a colored transparent overlay (not textured).
void HedronWindow::drawPotential(Net *net, int netface)
{
	DBG cerr <<"--drawing potential "<<netface<<endl;
	if (!net || netface<0 || netface>=net->faces.n) return;

	glPushMatrix();
	glMultMatrixf(hedron->m);

	int face=net->info; //the seed face for the net
	Basis bas=poly->basisOfFace(face);

	double r,g,b,a;
	r=1;
	b=1;
	g=1;
	a=.5;

	 //enable transparent
	glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
	glColor4f(r,g,b,a);

	glBegin(GL_TRIANGLE_FAN);
	flatpoint fp;
	for (int c=0; c<net->faces.e[netface]->edges.n; c++) {
		fp+=net->faces.e[netface]->edges.e[c]->points->p();
	}
	fp/=net->faces.e[netface]->edges.n;
	fp=transform_point(net->faces.e[netface]->matrix,fp);
	spacepoint center=bas.p + fp.x*bas.x + fp.y*bas.y;
	spacepoint normal=center/norm(center);
	spacepoint p;
	//flatpoint v;

	 //center of fan
	glNormal3f(normal.x,normal.y,normal.z);
	glVertex3f(center.x,center.y,center.z);

	 //rest of fan
	for (int c2=0; c2<=net->faces.e[netface]->edges.n; c2++) {
		fp=transform_point(net->faces.e[netface]->matrix,
						   net->faces.e[netface]->edges.e[c2%net->faces.e[netface]->edges.n]->points->p());
		p=bas.p + fp.x*bas.x + fp.y*bas.y;

		normal=p/norm(p);
		glNormal3f(normal.x,normal.y,normal.z);
		glVertex3f(p.x,p.y,p.z);
	}

	glEnd();

	glDisable (GL_BLEND);

	glPopMatrix();
}


int HedronWindow::event(XEvent *e)
{
	if (e->type==MapNotify) {
		glXMakeCurrent(app->dpy, xlib_window, glcontext);
		
		 // set viewport transform
		reshape(0,0, win_w,win_h);	// this is only gl commands
	
		if (firsttime) {
			firsttime=0;

			consolefont=setupfont(consolefontfile, fontsize);
			installOverlays();
			setlighting();

			 // set initial things 
			//makecar();
			makecube();
			makesphere();
			makecylinder();
			//makeshape();
			if (!hedron) {
				hedron=makeGLPolyhedron();
				things.push(hedron);
			}
			remapCache();

			 //set cameras
			makecameras();

			
			//eyem=cameras.e[current_camera]->model;
			//glMatrixMode(GL_MODELVIEW);
			//glPushMatrix();
			//glLoadIdentity();
			//gluLookAt(50,50,50, 0,0,0, 0,0,1);
			//glGetFloatv(GL_MODELVIEW_MATRIX,eyem);
			//glPopMatrix();
			
			 //define texture
			installSpheremapTexture(1);
		}

	}
	return anXWindow::event(e);
}

int HedronWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{
	if (mode==MODE_Help) {
		mode=oldmode;
		needtodraw=1;
		return 0;
	}

	int group=OGROUP_None;
	Overlay *overlay=scanOverlays(x,y, NULL,NULL,&group);
	int index=(overlay?overlay->id:-1);

	if (group==OGROUP_None) {
		int c=findCurrentPotential();
		if (c>=0) group=OGROUP_Potential;
	}

	if (group==OGROUP_None && draw_papers) {
		int c=scanPaper(x,y, index);
		DBG cerr <<"scan paper lbd: "<<c<<endl;
		if (c!=PAPER_None) {
			group=OGROUP_Paper;
			mouseover_index=c;
		}
	}

	mbdown=x;
	if (active_action==ACTION_Unwrap_Angle) mbdown=x;
	if (active_action==ACTION_Unwrap) rbdown=currentface;
	if (active_action==ACTION_Reseed) rbdown=currentface;

	buttondown.down(mouse->id,LEFTBUTTON,x,y, group,index);

	return 0;
}

int HedronWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *mouse)
{
	int overlayid=-1;
	int group=OGROUP_None, orig_group=OGROUP_None;
	//int dragged=
	buttondown.up(mouse->id,LEFTBUTTON, &orig_group,&overlayid);

	Overlay *overlay=scanOverlays(x,y, NULL,NULL,&group);
	if ((orig_group==OGROUP_TouchHelpers || orig_group==OGROUP_Papers) && overlayid>0) {
		if (overlay && overlay->id==overlayid) {
			if (group==OGROUP_TouchHelpers) {
				 //changing active_action
				if (active_action==overlay->action) {
					active_action=ACTION_None;
				} else active_action=overlay->action;
				needtodraw=1;
				return 0;

			} else if (group==OGROUP_Papers) {
				 //paper management buttons
				if (overlay->action==ACTION_AddPaper) {
					papers.push(new PaperBound(default_paper),1);
					remapPaperOverlays();
					needtodraw=1;
					return 0;
				}

				if (overlay->action==ACTION_Paper && overlay->index>=0) {
					if (currentnet) {
						currentnet->whichpaper=overlay->index;
						needtodraw=1;
						return 0;
					}
				} else if (overlay->action==ACTION_Paper && overlay->index<0) {
					draw_papers=0; //turn off showing of papers
					needtodraw=1;
					return 0;
				}

			} else if (group==OGROUP_ImageStack) {
				 //image stack management
				// ***
			}
		}
	}

	if (active_action==ACTION_Unwrap) RBUp(x,y,0,mouse);
	else if (active_action==ACTION_Reseed) RBUp(x,y,ControlMask,mouse);
	return 0;
}

int HedronWindow::MBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{
	if (mode==MODE_Help) {
		mode=oldmode;
		needtodraw=1;
		return 0;
	}

	buttondown.down(mouse->id,MIDDLEBUTTON,x,y);
	return 0;
}

int HedronWindow::MBUp(int x,int y,unsigned int state,const LaxMouse *mouse)
{
	buttondown.up(mouse->id,MIDDLEBUTTON);
	return 0;
}

int HedronWindow::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{
	if (mode==MODE_Help) {
		mode=oldmode;
		needtodraw=1;
		return 0;
	}

	rbdown=currentface;
	//currentface=-1;
	buttondown.down(mouse->id,RIGHTBUTTON,x,y,currentface);
	return 0;
}

int HedronWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *mouse)
{
	DBG cerr <<"HedronWindow::RBUp: rbdown=="<<rbdown<<"  currentface=="<<currentface<<endl;

	if (!(buttondown.isdown(mouse->id,RIGHTBUTTON))) return 0;

	int rbdown;
	buttondown.up(mouse->id,RIGHTBUTTON, &rbdown);
	//if (active_action!=ACTION_Unwrap && active_action!=ACTION_Reseed) return 0;

	if ((state&LAX_STATE_MASK)==ControlMask) {
		 //reseed net of currentface, if any
		if (rbdown==-1 || rbdown!=currentface) return 0;
		Reseed(currentface);
		remapCache();
		needtodraw=1;
		return 0;
	}

	if (mode!=MODE_Net) {
		rbdown=currentface=-1;
		return 0;
	}

	if (currentpotential>=0) {
		 //right click on a potential drops it down
		int orig=currentnet->faces.e[currentpotential]->original;
		poly->faces.e[orig]->cache->facemode=currentnet->object_id;
		currentnet->Drop(currentpotential);

		//remapCache();
		remapCache(orig,orig);
		needtodraw=1;
		rbdown=currentface=currentpotential=-1;
		return 0;

	} else if (rbdown!=-1 && rbdown==currentface) {
		 //right click down and up on same face...
		DBG cerr <<"...right click down and up on same face"<<endl;
		 
		 //if face is on hedron, not a seed, then create a new net with that face
		if (poly->faces.e[currentface]->cache->facemode==0) {
			DBG cerr <<"...establish a net on currentface"<<endl;

			 //unwrap as a seed when face is on polyhedron, not in net
			Net *net=establishNet(currentface);
			if (currentnet) {
				if (currentnet->numActual()==1) removeNet(currentnet);
				if (currentnet) currentnet->dec_count();
			}

			currentnet=net;//using count from above
			//remapCache(); //no remapping necessary since face stays on hedron
			needtodraw=1;
			rbdown=currentface=-1;
			return 0;

		} else if (poly->faces.e[currentface]->cache->facemode!=0) {
			DBG cerr <<"...make net of currentface the current net"<<endl;

			 //make current net the net of currentface
			Net *net=findNet(poly->faces.e[currentface]->cache->facemode);
			if (currentnet!=net) {
				if (currentnet && currentnet->numActual()==1) removeNet(currentnet);
				if (currentnet) currentnet->dec_count();
				currentnet=net;
				if (currentnet) currentnet->inc_count();
				needtodraw=1;
			}

			if (currentfacestatus==1) {
				 //face is a leaf. pick it up
				int i=-1;
				net->findOriginalFace(currentface,1,0,&i); //it is assumed the face is actually there
				net->PickUp(i,-1);
				poly->faces.e[currentface]->cache->facemode=0;
				remapCache(currentface,currentface);
				needtodraw=1;
			}
			rbdown=currentface=-1;
			return 0;

		}

	} else if (rbdown==-1 && currentface==-1) {
		if (currentnet) {
			 //If the net only has the seed face, and we are selecting off, then remove that net
			if (currentnet->numActual()==1) removeNet(currentnet);
			if (currentnet) { currentnet->dec_count(); currentnet=NULL; }
			needtodraw=1;
		}
	}
	rbdown=currentface=-1;
	return 0;
}

int HedronWindow::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{
	if (mode==MODE_Help) {
		if (helpoffset==10000) helpoffset=win_h-fontsize;
		else helpoffset+=fontsize;
		needtodraw=1;
		return 0;
	}

	int group=OGROUP_None;
	Overlay *overlay=scanOverlays(x,y, NULL,NULL,&group);
	
	if (overlay && group==OGROUP_Papers) {
		if (overlay->action==ACTION_Paper) {
			 //change paper type of overlay->index
			if (overlay->index>=0) changePaper(-2,overlay->index);
		}
		return 0;
	}


	cameras.e[current_camera]->m.p+=5*cameras.e[current_camera]->m.z;
	cameras.e[current_camera]->transformTo();

	needtodraw=1;
	return 0;
}

int HedronWindow::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{
	if (mode==MODE_Help) {
		if (helpoffset==10000) helpoffset=win_h-fontsize;
		else helpoffset-=fontsize;
		needtodraw=1;
		return 0;
	}

	int group=OGROUP_None;
	Overlay *overlay=scanOverlays(x,y, NULL,NULL,&group);
	
	if (overlay && group==OGROUP_Papers) {
		if (overlay->action==ACTION_Paper) {
			 //change paper type of overlay->index
			if (overlay->index>=0) changePaper(-1,overlay->index);
		}
		return 0;
	}

	cameras.e[current_camera]->m.p-=5*cameras.e[current_camera]->m.z;
	cameras.e[current_camera]->transformTo();

	needtodraw=1;
	return 0;
}

int HedronWindow::MouseMove(int x,int y,unsigned int state,const LaxMouse *mouse)
{
	 //first off, check if mouse in any overlays
	int action=0,index=-1,group=OGROUP_None;
	Overlay *in;
	in=scanOverlays(x,y, &action,&index,&group);

	if (mouseover_group!=group || mouseover_overlay!=action || mouseover_index!=index) {
		mouseover_group=group;
		mouseover_overlay=action;
		mouseover_index=index;
		DBG cerr <<"move group:"<<group<<" action:"<<action<<" index:"<<index<<endl;
		needtodraw=1;
	}
	if (in) return 0;


	if (grab_overlay>=0) return 0;

	 //map pointer to 3 space
	double d=win_h/2/tan(fovy/2);
	double sx,sy,sz;
	//map mouse point to a point in space 50 units out
	sz=50;
	sx=(x-win_w/2.)*sz/d;
	sy=(y-win_h/2.)*sz/d;

	tracker=cameras.e[current_camera]->m.p
		+ sx*cameras.e[current_camera]->m.x
		- sy*cameras.e[current_camera]->m.y
		- sz*cameras.e[current_camera]->m.z;

	pointer.p=cameras.e[current_camera]->m.p;
	pointer.v=tracker-pointer.p;
	

	 //no buttons pressed
	if (!buttondown.any()) {
		//search for mouse overs

		int c=-1;
		int index=-1;
		if (currentnet) {
			if (draw_papers) c=scanPaper(x,y, index);
			if (c>=0 && c!=PAPER_None) {
				mouseover_paper=currentnet->whichpaper;
				mouseover_group=OGROUP_Paper;
				mouseover_index=c;
				DBG cerr <<"   ==== scanned as over paper:"<<c<<endl;
				return 0;
			} else mouseover_paper=-1;
			 
			 //scan for mouse over potential faces
			c=findCurrentPotential(); //-1 for none, -2 for over paper
			DBG cerr <<"MouseMove found potential: "<<c<<endl;
			if (c>=0) {
				if (currentface!=-1) { 	currentface=-1; needtodraw=1; }
				if (c!=currentpotential) {
					currentpotential=c;
					needtodraw=1;
				}
				DBG cerr <<"current potential original: "<<currentpotential<<endl;

				return 0;

			} else if (c==-2) {
				 //mouse is over paper
				mouseover_paper=currentnet->whichpaper;
				currentpotential=-1;

			} else {
				if (currentpotential!=-1) {
					currentpotential=-1;
					needtodraw=1;
				}
			}

		} else {
			mouseover_paper=-1;
		}

		 //scan for mouse over actual faces
		c=findCurrentFace();

		DBG cerr<<"MouseMove findCurrentFace original: "<<c;
		DBG if (c>=0) {
		DBG		cerr <<" facemode:"<<poly->faces.e[c]->cache->facemode<<endl;
		DBG 	Net *net=findNet(poly->faces.e[c]->cache->facemode);
		DBG 	int i=-1;
		DBG 	if (net) {
		DBG			cerr <<"net found "<<nets.findindex(net)<<"..."<<endl;
		DBG			int status=net->findOriginalFace(c,1,0,&i); //it is assumed the face is actually there
		DBG			cerr <<" links: status:"<<status<<" on neti:"<<i<<"  links:"<<net->actualLink(i,-1)<<"   ";
		DBG		} else cerr <<"(not in a net) ";
		DBG		cerr <<"net face: "<<i<<endl;
		DBG } else cerr <<endl;

		if (c!=currentface) {
			needtodraw=1;
			currentface=c;
			if (currentface>=0) {
				currentfacestatus=0;
				//DBG cerr <<"face "<<currentface<<" facemode "<<poly->faces.e[currentface]->cache->facemode<<endl;
				if (poly->faces.e[currentface]->cache->facemode>0) {
					 //face is in a net. If it is a leaf in currentnet, color it green, rather then red
					Net *net=findNet(poly->faces.e[currentface]->cache->facemode);
					if (net && net==currentnet) {
						int i=-1;
						net->findOriginalFace(currentface,1,0,&i); //it is assumed the face is actually there

						DBG cerr <<"find original neti:"<<i<<" actual links:"<<net->actualLink(i,-1)<<endl;

						if (net->actualLink(i,-1)==1) {
							 //only touches 1 actual face
							currentfacestatus=1;
						}
					}
				} else if (poly->faces.e[currentface]->cache->facemode<0) currentfacestatus=2; //for seeds
			}
		}
		return 0;
	}

	//so below is for when button is down



	int lx,ly;
	int bgroup=OGROUP_None,bindex=-1;
	buttondown.move(mouse->id, x,y, &lx,&ly);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &bgroup,&bindex);
	flatpoint leftb=flatpoint(lx,ly);


	 //deal with 2 finger zoom first
	if (buttondown.isdown(0,LEFTBUTTON)==2 && buttondown.isdown(mouse->id,LEFTBUTTON)) {
		 //double move
		int xp1,yp1, xp2,yp2;
		int xc1,yc1, xc2,yc2;
		int device1=buttondown.whichdown(0,LEFTBUTTON);
		int device2=buttondown.whichdown(device1,LEFTBUTTON);
		buttondown.getinfo(device1,LEFTBUTTON, NULL,NULL, &xp1,&yp1, &xc1,&yc1);
		buttondown.getinfo(device2,LEFTBUTTON, NULL,NULL, &xp2,&yp2, &xc2,&yc2);

		double oldd=norm(flatpoint(xp1,yp1)-flatpoint(xp2,yp2));
		double newd=norm(flatpoint(xc1,yc1)-flatpoint(xc2,yc2));
		double zoom=newd/oldd;
		if (zoom==0) return 0;

		 //apply zoom
		double amount=10;
		//DBG cerr <<" ZZZZZZOOM d1:"<<device1<<" d2:"<<device2<<"    "<<amount<<"  z:"<<zoom<<endl;
		if (zoom>1) amount=-amount*(zoom-1);
		else amount=amount*(1/zoom-1);

		cameras.e[current_camera]->m.p+=amount*cameras.e[current_camera]->m.z;
		cameras.e[current_camera]->transformTo();

		 //apply rotation
//		flatvector v1=flatpoint(xc1,yc1)-flatpoint(xp1,yp1);
//		flatvector v2=flatpoint(xc2,yc2)-flatpoint(xp2,yp2);
//		double angle=0;
//		double epsilon=1e-10;
//		DBG cerr <<"   NNNNNORM  a:"<<norm(v1)<<"  b:"<<norm(v2)<<endl;
//		if (norm(v1)>epsilon) {
//			 //point 1 moved
//			v1=flatpoint(xc1,yc1)-flatpoint(xp2,yp2);
//			v2=flatpoint(xp1,yp1)-flatpoint(xp2,yp2);
//			angle=atan2(v2.y,v2.x)-atan2(v1.y,v1.x);
//		} else if (norm(v2)>epsilon) {
//			 //point 2 moved
//			v1=flatpoint(xc2,yc2)-flatpoint(xp1,yp1);
//			v2=flatpoint(xp2,yp2)-flatpoint(xp1,yp1);
//			angle=atan2(v2.y,v2.x)-atan2(v1.y,v1.x);
//		}
//		DBG  cerr <<" RRRRROTATE "<<angle<< "  deg:"<<angle*180/M_PI<<endl;
//
//		if (angle) {
//			spacepoint axis;
//			axis=cameras.e[current_camera]->m.z;
//			things.e[curobj]->RotateGlobal(angle*180/M_PI, axis.x,axis.y,axis.z);
//		}

		needtodraw=1;
		return 0;
	}



	if (!buttondown.isdown(mouse->id,LEFTBUTTON) && !buttondown.isdown(mouse->id,RIGHTBUTTON) && !buttondown.isdown(mouse->id,RIGHTBUTTON))
		return 0;

	if (bgroup==OGROUP_Paper && currentnet) {
		flatpoint fp,fpn;
		fpn=pointInNetPlane(x,y);
		fp =pointInNetPlane(lx,ly);

		PaperBound *paper=papers.e[bindex];
		if ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ShiftMask) {
			 //shift papers
			if ((state&LAX_STATE_MASK)==ShiftMask) {
				 //move net relative to papers
				currentnet->origin(currentnet->origin()-fpn+fp);
			} else {
				 //move paper relative to other papers
				flatpoint d=transform_point(currentnet->m(),fpn)-transform_point(currentnet->m(),fp);
				paper->matrix.Translate(d);
			}

		} else if ((state&LAX_STATE_MASK)==ControlMask) {
			 //scale papers
			double s=1;
			if (x>lx) s=.98;
			else s=1.02;
			for (int c=0; c<6; c++) {
				currentnet->m(c, currentnet->m(c)*s);
			}

		} else {
			 //rotate papers
			double s=0;
			if (x>lx) s=1./180*M_PI;
			else s=-1./180*M_PI;
			currentnet->xaxis(rotate(currentnet->xaxis(),s));
			currentnet->yaxis(rotate(currentnet->yaxis(),s));
		}

		needtodraw=1;
		return 0;
	}

	ActionType current_action=active_action;
	if (buttondown.isdown(0,MIDDLEBUTTON)) current_action=ACTION_Unwrap_Angle;
	else if (buttondown.isdown(0,RIGHTBUTTON)) current_action=ACTION_Unwrap;

	if (current_action==ACTION_Zoom) {
		 //zoom
		if (x-mbdown>10) { WheelUp(x,y,0,0,mouse); mbdown=x; }
		else if (mbdown-x>10) { WheelDown(x,y,0,0,mouse); mbdown=x; }

	}


	 //middle button: change unwrapangle
	if (current_action==ACTION_Unwrap_Angle) {
		cout <<" *** need to implement unwrap angle change!"<<endl;
		//if (nets.faces.n==0 || x==mbdown) return 0;

		unwrapangle+=(x-mbdown)*.1;
		if (unwrapangle<0) unwrapangle=0;
		else if (unwrapangle>1) unwrapangle=1;
		//remapCache(-1,-1);

		mbdown=x;
		return 0;
	}


	 //right button
	if (current_action==ACTION_Unwrap && rbdown>=0 && (mode==MODE_Net || mode==MODE_Solid)) {
		 //Unwrap
		int c=findCurrentFace();
		DBG cerr <<"rb move: "<<c<<endl;
		if (c==rbdown) { currentface=rbdown; return 0; }

		 //if c is attached to rbdown then we have a winner maybe
		int cc;

		//DBG cerr <<"faces attached to "<<rbdown<<": ";
		for (cc=0; cc<poly->faces.e[rbdown]->pn; cc++) {
			//DBG cerr<<poly->faces.e[rbdown]->f[cc]<<" ";

			if (poly->faces.e[rbdown]->f[cc]==c) break;
		}
		//DBG cerr <<endl;
		if (cc==poly->faces.e[rbdown]->pn) { 
			 //c is not attached to any edge of rbdown
			if (currentface!=-1) needtodraw=1;
			currentface=-1;
		} else {
			if (currentface!=c) needtodraw=1;
			currentface=c;
		}

		DBG cerr<<" rb-unwrap from "<<rbdown<<" to: "<<currentface<<endl;
		if (rbdown!=currentface && currentface>=0 && unwrapTo(rbdown,currentface)==0) {
			rbdown=currentface;
			currentface=-1;
			needtodraw=1;
		}
		DBG cerr<<" ..rb-unwrap done"<<endl;

		return 0;
	}

	if (current_action==ACTION_None) {
		if       ((state&LAX_STATE_MASK)== 0)                      current_action=ACTION_Roll;
		else if  ((state&LAX_STATE_MASK)== ShiftMask)              current_action=ACTION_Rotate;
		else if  ((state&LAX_STATE_MASK)== ControlMask)            current_action=ACTION_Shift_Texture;
		else if  ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) current_action=ACTION_Rotate_Texture;
	}

	currentface=-1;

	 //shift-drag -- rotates camera around axis through viewer
	if (current_action==ACTION_Rotate) {
		flatpoint d,p=flatpoint(x,y);
		d=p-leftb;

		//-----------rotate camera
		//			if (d.y) {
		//				rotate(cameras.e[current_camera]->m,    //basis
		//					   cameras.e[current_camera]->m.x,  //axis
		//					   d.y/5/180*M_PI); //angle
		//				cameras.e[current_camera]->transformTo();
		//				needtodraw=1;
		//			}
		//			if (d.x) {
		//				rotate(cameras.e[current_camera]->m,    //basis
		//					   cameras.e[current_camera]->m.y,  //axis
		//					   d.x/5/180*M_PI); //angle
		//				cameras.e[current_camera]->transformTo();
		//				needtodraw=1;
		//			}
		if (d.x) { //rotate thing around camera z
			spacepoint axis;
			axis=d.x*cameras.e[current_camera]->m.z;
			things.e[curobj]->RotateGlobal(norm(d)/5, axis.x,axis.y,axis.z);
			needtodraw=1;
		}
		leftb=p;

		//plain drag -- rolls shape
	} else if (current_action==ACTION_Roll) { //rotate current thing
		flatpoint d,p=flatpoint(x,y);
		d=p-leftb;

		spacepoint axis;
		axis=d.y*cameras.e[current_camera]->m.x + d.x*cameras.e[current_camera]->m.y;
		needtodraw=1;

		//-------------------------
		things.e[curobj]->RotateGlobal(norm(d)/5, axis.x,axis.y,axis.z);
		//things.e[curobj]->RotateLocal(norm(d)/5, axis.x,axis.y,axis.z);

		//-------------------------
		//			if (d.y) {
		//				things.e[curobj]->RotateLocal(d.y/5, 0,0,1);
		//				needtodraw=1;
		//			}
		//			if (d.x) {
		//				things.e[curobj]->RotateLocal(d.x/5, 0,1,0);
		//				needtodraw=1;
		//			}
		//-------------------------
		leftb=p;


		//shift-control-drag -- rotates texture around z
	} else if (current_action==ACTION_Rotate_Texture) { //rotate texture around z
		flatpoint d,p=flatpoint(x,y);
		d=p-leftb;

		if (d.x) {
			spacepoint axis;
			axis=d.x*cameras.e[current_camera]->m.z;
			things.e[curobj]->updateBasis();
			transform(axis,things.e[curobj]->bas);
			rotate(extra_basis,axis,norm(d)/5/180*M_PI);
			needtodraw=1;
			leftb=p;
			mapPolyhedronTexture(hedron);
		}

		// control-drag -- shift texture
	} else if (current_action==ACTION_Shift_Texture) { //rotate texture
		flatpoint d,p=flatpoint(x,y);
		d=p-leftb;

		spacepoint axis;
		axis=d.y*cameras.e[current_camera]->m.x + d.x*cameras.e[current_camera]->m.y;
		things.e[curobj]->updateBasis();
		transform(axis,things.e[curobj]->bas);
		rotate(extra_basis,axis,norm(d)/5/180*M_PI);

		mapPolyhedronTexture(hedron);
		needtodraw=1;
		leftb=p;
	}

	return 0;
}

//! Remap poly->faces.cache for faces in range [start,end] so that the unfolded sides are oriented correctly.
/*! This assumes that the face->cache->facemode is accurate. cache->points3d and cache->axis
 * will be remapped.
 *
 * If start<0 then start at 0, and if end<0 then end at poly->faces.n-1.
 *
 * \todo The faces will be tilted at the angle unwrapangle*(its normal dihedral angle).
 */
void HedronWindow::remapCache(int start, int end)
{
	if (!poly) return; //do not attempt if called too early!

	if (start<0) start=0;
	if (end<0) end=poly->faces.n-1;

	Net *net;
	ExtraFace *cache=NULL;
	if (mode==MODE_Net) {
	  if (unwrapangle==1) {
		for (int c=0; c<nets.n; c++) {
			 //remap all actual faces in nets to changed
			net=nets.e[c];
			int face=net->info; //the seed face for the net
			
			Basis bas=poly->basisOfFace(face); //basis of the seed face
			spacepoint p;
			flatpoint v;

			 //remap points
			for (int c=0; c<net->faces.n; c++) {
				if (net->faces.e[c]->tag!=FACE_Actual) continue;
				if (net->faces.e[c]->original<start || net->faces.e[c]->original>end) continue;
				cache=poly->faces.e[net->faces.e[c]->original]->cache;
				cache->axis=bas;
				cache->center=spacepoint();

				for (int c2=0; c2<net->faces.e[c]->edges.n; c2++) {
					v=transform_point(net->faces.e[c]->matrix, net->faces.e[c]->edges.e[c2]->points->p());
					p=bas.p + v.x*bas.x + v.y*bas.y;
					cache->points3d[c2]=p;
					cache->center+=p;

					 //map 2d points
					cache->points2d[c2]=flatten(p,cache->axis);
				}
				cache->center/=net->faces.e[c]->edges.n;
				 //remap face basis
				//poly->faces.e[net->faces.e[c]->original]->cache->axis=bas;
			}
		}
	  } else {
			 //unwrap angle is a partial angle (!= 0)
			int i, original;
			for (int c=0; c<nets.n; c++) {
				i=-1;
				original=nets.e[c]->info;
				nets.e[c]->resetTick(0);

				 //remap seed
				cache=poly->faces.e[c]->cache;
				cache->axis=poly->basisOfFace(original);
				for (int c2=0; c2<poly->faces.e[original]->pn; c2++) {
					cache->points3d[c2]=poly->VertexOfFace(original,c2,0);
					cache->points2d[c2]=flatten(cache->points3d[c2],cache->axis);
				}

				nets.e[c]->findOriginalFace(nets.e[c]->info,1,-1,&i);
				nets.e[c]->faces.e[i]->tick=1;
				recurseCache(nets.e[c],i);
			}
		
	  }
	}

	for (int c=start; c<=end; c++) {
		 //each facemode==0 reset to normal
		cache=poly->faces.e[c]->cache;
		if (mode==MODE_Net && cache->facemode!=0) continue;
		//****
		cache->axis=poly->basisOfFace(c);
		for (int c2=0; c2<poly->faces.e[c]->pn; c2++) {
			cache->points3d[c2]=poly->VertexOfFace(c,c2,0);
			cache->points2d[c2]=flatten(cache->points3d[c2],cache->axis);
		}
	}

	if (hedron) mapPolyhedronTexture(hedron);
}

/*! Unwrap each connected face with tick==0.
 */
void HedronWindow::recurseCache(Net *net,int neti)
{
	NetFace *face=net->faces.e[neti];
	//int o;
	for (int c=0; c<face->edges.n; c++) {
		if (face->edges.e[c]->tag!=FACE_Actual) continue;
		if (net->faces.e[face->edges.e[c]->toface]->tick==1) continue;
		//o=face->edges.e[c]->tooriginal;
		//***build face and transform to rotated position
	}
}

/*! A new net will be returned. it's count will have to be dec_count()'d.
 */
Net *HedronWindow::establishNet(int original)
{
	DBG cerr <<"create net around original "<<original<<endl;
	Net *net=new Net;
	net->_config=1;
	net->Basenet(poly);
	net->Unwrap(-1,original);
	poly->faces.e[original]->cache->facemode=-net->object_id;
	net->info=original;
	nets.push(net);

	return net;
}

//! Reseed the net containing original face, an index in poly->faces.
/*! Reseeding means to orient the net to appear stuck to the hedron at face original.
 * The origin of the net will be the origin of the seed face. The matrices of all the
 * faces will be changed accordingly.
 *
 * Return 0 for reseeded, or no reseeding necessary. Nonzero for error.
 *
 * NOTE: remapCache() is NOT called here.
 */
int HedronWindow::Reseed(int original)
{
	if (original<0 || original>=poly->faces.n) return 1;

	
	if (poly->faces.e[original]->cache->facemode==0) return 2; //not in a net
	if (poly->faces.e[original]->cache->facemode<0) return 0; // is already a seed

	 //reseed net 
	Net *net=findNet(poly->faces.e[original]->cache->facemode);

	DBG if (!net) {
	DBG 	cerr <<"******** null net for facemode "<<poly->faces.e[currentface]->cache->facemode<<"!!"<<endl;
	DBG 	exit(1);
	DBG }

	 //update poly<->net crosslinks
	poly->faces.e[net->info]->cache->facemode=net->object_id;
	poly->faces.e[original]->cache->facemode=-net->object_id;
	net->info=original;

	double m[6],m2[6];
	int neti;
	net->findOriginalFace(original,1,0,&neti);
	DBG cerr <<"--reseed: original:"<<currentface<<" to neti:"<<neti<<endl;

	if (neti<0) {
		DBG cerr <<"************WARNING!!!!! negative neti, this should NEVER happen!!"<<endl;
		return 0;
	}

	transform_invert(m,net->faces.e[neti]->matrix);
	for (int c=0; c<net->faces.n; c++) {
		//transform_mult(m2,m,net->faces.e[c]->matrix);
		transform_mult(m2,net->faces.e[c]->matrix,m);
		transform_copy(net->faces.e[c]->matrix,m2);
	}

	 //adjust paper transform, if any paper
	if (net->whichpaper>=-1) {
		double m3[6];
		transform_invert(m3,net->m());
		transform_mult(m2,m3,m);
		transform_invert(m3,m2);
		net->m(m3);
	}

	needtodraw=1;

	return 0;
}

//! Return 0 for net removed, or nonzero for not removed.
int HedronWindow::removeNet(Net *net)
{
	return removeNet(nets.findindex(net));
}

//! Remove net and set all faces to be on hedron (facemode==0).
int HedronWindow::removeNet(int netindex)
{
	if (netindex<0 || netindex>=nets.n) return 1;
	Net *net=nets.e[netindex];
	if (currentnet==net) {
		currentnet->dec_count();
		currentnet=NULL;
	}
	 //reset all faces in net to normal
	DBG int n=0;
	for (int c=0; c<net->faces.n; c++) {
		if (net->faces.e[c]->tag!=FACE_Actual) continue;
		DBG n++;
		poly->faces.e[net->faces.e[c]->original]->cache->facemode=0;
	}
	DBG cerr <<"removeNet() reset "<<n<<" faces"<<endl;
	nets.remove(netindex);
	return 0;
}

//! Change paper index to a different type of paper.
/*! towhich==-2 for previous paper, -1 for next, or >=0 for absolute paper num
 *
 * Return 1 for successful change. 0 for no change.
 */
int HedronWindow::changePaper(int towhich,int index)
{
	//default does nothing
	return 0;
}

//! Find the point in currentnet coordinates laying on screen point x,y.
flatpoint HedronWindow::pointInNetPlane(int x,int y)
{
	if (!currentnet) return flatpoint();

	spacevector spacev;
	spaceline line;
	spacepoint p;
	int err=0;
	//map mouse point to a point in space 50 units out
	double d=win_h/2/tan(fovy/2);
	double sz=50;
	double sx=(x-win_w/2.)*sz/d;
	double sy=(y-win_h/2.)*sz/d;
	hedron->updateBasis();

	spacev=cameras.e[current_camera]->m.p
		+ sx*cameras.e[current_camera]->m.x
		- sy*cameras.e[current_camera]->m.y
		- sz*cameras.e[current_camera]->m.z;

	line.p=cameras.e[current_camera]->m.p;
	line.v=spacev-line.p;

	flatpoint fp;
	transform(line,hedron->bas);
	Basis nbasis=poly->basisOfFace(currentnet->info); //basis of seed face on polyhedron

	p=intersection(line,Plane(nbasis.p,nbasis.z),err);
	if (err!=0) return flatpoint();
	fp=flatten(p,nbasis);

	return fp;
}

int HedronWindow::scanPaper(int x,int y, int &index)
{
	if (!currentnet || !papers.n) return PAPER_None;

	flatpoint fp=pointInNetPlane(x,y);
	flatpoint fpp;
	DBG cerr <<" ---- scanPaper from pointInNetPlane: netface:"<<currentnet->info<<"  "<<fp.x<<","<<fp.y<<endl;

	double pw,ph;
	for (index=0; index<papers.n; index++) {
		pw=papers.e[index]->width;
		ph=papers.e[index]->height;

		fpp=transform_point(currentnet->m(),fp);
		fpp=papers.e[index]->matrix.transformPointInverse(fpp);

		DBG cerr <<" ---- scanPaper "<<index<<" from pointInNetPlane: transf to paper: "<<fpp.x<<","<<fpp.y<<endl;
		DBG const double *nm=currentnet->m();
		DBG cerr <<" ----   netm: "<<nm[0]<<", "<<nm[1]<<", "<<nm[2]<<", "<<nm[3]<<", "<<nm[4]<<", "<<nm[5]<<", "<<endl;
		DBG nm=papers.e[index]->matrix.m();
		DBG cerr <<" ---- paperm: "<<nm[0]<<", "<<nm[1]<<", "<<nm[2]<<", "<<nm[3]<<", "<<nm[4]<<", "<<nm[5]<<", "<<endl;

		if (fpp.x>0 && fpp.x<pw*.1 && fpp.y>=0 && fpp.y<ph)
			return PAPER_Left;

		if (fpp.x>pw+-pw*.1 && fpp.x<pw && fpp.y>=0 && fpp.y<ph)
			return PAPER_Right;

		if (fpp.y>0 && fpp.y<ph*.1 && fpp.x>=0 && fpp.x<pw)
			return PAPER_Bottom;

		if (fpp.y>ph+-ph*.1 && fpp.y<ph && fpp.x>=0 && fpp.x<pw)
			return PAPER_Top;

		if (fpp.x>=0 && fpp.y>=0 && fpp.x<pw && fpp.y<ph)
			return PAPER_Inside;
	}

	return PAPER_None;
}

/*! Return index in currentnet->faces, or -2 over paper (if any) or -1 for not over anything.
 */
int HedronWindow::findCurrentPotential()
{
	if (!currentnet) return -1;

	spacepoint p;
	flatpoint fp;
	int err;
	int index=-1;

	spaceline line;
	hedron->updateBasis();
	line=pointer;
	transform(line,hedron->bas);
	Basis nbasis=poly->basisOfFace(currentnet->info); //basis of seed face on polyhedron

	p=intersection(line,Plane(nbasis.p,nbasis.z),err);
	if (err!=0) return -1;
	fp=flatten(p,nbasis);

	 //intersect with the net face's shape
	index=currentnet->pointinface(fp,1);

	if (index<0) {
		//not inside any potential face!
		return -1;
	}

	if (currentnet->faces.e[index]->tag!=FACE_Potential) return -1;
	if (poly->faces.e[currentnet->faces.e[index]->original]->cache->facemode!=0) return -1;

	return index;
}

//! Assuming the line pointer is set properly, find the closest face it intersects.
/*! Returns the face index or -1 if not found.
 */
int HedronWindow::findCurrentFace()
{
	 //using cached basis in each face, intersect pointer with that plane,
	 //then see if that face contains that point
	spacepoint p;
	flatpoint fp;
	int err;
	int index=-1;
	double dist=10000000,d;

	spaceline line;
	hedron->updateBasis();
	line=pointer;
	transform(line,hedron->bas);

	for (int c=0; c<poly->faces.n; c++) {
		 //intersect with the face's cached axis, against its cached 2-d points
		p=intersection(line,Plane(poly->faces.e[c]->cache->axis.p,poly->faces.e[c]->cache->axis.z),err);
		fp=flatten(p, poly->faces.e[c]->cache->axis);
		if (err!=0) continue;
		if (point_is_in(fp, poly->faces.e[c]->cache->points2d,poly->faces.e[c]->pn)) {
			d=(p-line.p)*(p-line.p);
			//DBG cerr <<"point in "<<c<<", dist: "<<d<<endl;
			if (d<dist) { index=c; dist=d; }
		}
	}
	//DBG cerr <<dist<<"  ";
	return index;
}

//! Find the net with object_id==id.
Net *HedronWindow::findNet(int id)
{
	if (id<0) id=-id;
	for (int c=0; c<nets.n; c++) if ((int)nets.e[c]->object_id==id) return nets.e[c];
	return NULL;
}

/*! If the faces are not adjacent, then return 1, else return 0 for successful unwrap.
 */
int HedronWindow::recurseUnwrap(Net *netf, int fromneti, Net *nett, int toneti)
{
	 //recursively traverse:
	 //  add face to nett by dropping at appropriate nett face and edge.
	 //  set tick corresponding to fromneti in netf to 1

	if (netf->faces.e[fromneti]->tick==1) return 0;

	 //mark this face as handled
	netf->faces.e[fromneti]->tick=1;

	int c=-1;
	 //find the edge in nett->toneti that touches fromneti. Since the from face is not in
	 //nett yet, the edge tag will be FACE_Potential
	for (c=0; c<nett->faces.e[toneti]->edges.n; c++) {
		//if (nett->faces.e[toneti]->edges.e[c]->tag!=
		if (netf->faces.e[fromneti]->original==nett->faces.e[toneti]->edges.e[c]->tooriginal) break;
	}
	if (c==nett->faces.e[toneti]->edges.n) {
		DBG cerr <<"Warning: recurseUnwrap() from and to are not adjacent"<<endl;
		return 1; //faces not adjacent
	}
	DBG cerr <<"recurseUnwrap: fromneti="<<fromneti<<" (orig="<<netf->faces.e[fromneti]->original
	DBG 	<<")  toneti="<<toneti<<" (orig="<<nett->faces.e[toneti]->original<<")"<<endl;

	nett->Unwrap(toneti,c); //attach face fromneti in netf
	DBG cerr <<"nett->toneti->toface:"<<nett->faces.e[toneti]->edges.e[c]->toface<<endl;
	DBG cerr <<"nett->toneti->tag:"<<nett->faces.e[toneti]->edges.e[c]->tag<<endl;
	int i=nett->findOriginalFace(netf->faces.e[fromneti]->original,1,0, &toneti);
	DBG cerr <<"recurseUnwrap newly laid face: find orig "<<netf->faces.e[fromneti]->original<<":neti="<<toneti<<", status="<<i<<endl;

	//-------------------------
	DBG for (int c=0; c<nett->faces.n; c++) {
	DBG 	if (nett->faces.e[c]->tag!=FACE_Actual) continue;
	DBG 	for (int c2=0; c2<nett->faces.e[c]->edges.n; c2++) {
	DBG 		if (nett->faces.e[c]->edges.e[c2]->tag==FACE_None) {
	DBG 			cerr <<"recurse:AAAAAAARRG! edge points to -1:"<<endl;
	DBG 			cerr <<"  net->face("<<c<<")->edge("<<c2<<")->tag="<<nett->faces.e[c]->edges.e[c2]->tag<<endl;
	DBG 		}
	DBG 	}
	DBG }
	//-------------------------



	if (toneti<=0) {
		cerr <<"****AARG! cannot find original "<<netf->faces.e[fromneti]->original<<", this should not happen, fix me!!"<<endl;
		return 0;
	}
	poly->faces.e[nett->faces.e[toneti]->original]->cache->facemode=nett->object_id;
	// now fromneti and toneti are the same face..


	for (c=0; c<netf->faces.e[fromneti]->edges.n; c++) {
		i=netf->faces.e[fromneti]->edges.e[c]->toface;
		if (i<0 || netf->faces.e[i]->tick==1 || netf->faces.e[i]->tag!=FACE_Actual) continue;

		 //now i is a face in netf at edge c of face fromneti
		recurseUnwrap(netf,i, nett,toneti);
	}

	return 0;
}

//! Mouse dragged: from -> to. Either unwrap face or net, or split a net.
/*! If both faces are seeds, then combine the nets. If one face is a seed,
 * and the other an adjacent net face, then that face becomes a seed of
 * a new net. All other cases ignored.
 *
 * from and to are both original face numbers.
 */
int HedronWindow::unwrapTo(int from,int to)
{
	if (from<0 || to<0) return 1;
	int reseed=-1;	

	int modef=poly->faces.e[from]->cache->facemode;
	int modet=poly->faces.e[ to ]->cache->facemode;

	 //we are mostly acting on seed to seed, so we need nets to begin with:
	if (modef==0) {
		 //from was an ordinary face, make from into a net
		Net *net=establishNet(from);
		modef=poly->faces.e[from]->cache->facemode=-net->object_id;
		net->dec_count();
	}
	if (modet==0) {
		 //to was an ordinary face, make to into a net
		Net *net=establishNet(to);
		modet=poly->faces.e[to]->cache->facemode=-net->object_id;
		net->dec_count();
	}

	//now modet and modef will not be 0

	if (mode==MODE_Solid && (modef>0 || modet>0) && modef!=-modet && modef!=modet) {
		 //reseed if connecting across nets in solid mode
		if (modef>0) Reseed(from);
		if (modet>0) Reseed(to);
	}

	if (modef<0 && modet<0) {
		 //both seeds, so combine
		Net *netf,*nett;
		netf=findNet(modef);
		nett=findNet(modet);

		if (netf->faces.n>nett->faces.n) {
			 //drop the smaller net onto the larger net to save time
			Net *nf=netf;
			netf=nett;
			nett=nf;

			reseed=modef;
			modef=modet;
			modet=reseed;

			reseed=from;
			from=to;
			to=reseed;

			reseed=netf->info;
		}

		int fromi,toi;
		int status=netf->findOriginalFace(from,1,0,&fromi);
		DBG cerr <<"from: find orig "<<from<<":"<<fromi<<", status="<<status<<endl;
		status=nett->findOriginalFace(to,1,0,&toi);
		DBG cerr <<"  to: find orig "<<to<<":"<<toi<<", status="<<status<<endl;

		netf->resetTick(0);
		recurseUnwrap(netf,fromi, nett,toi);

		DBG //------------------------
		DBG for (int c=0; c<nett->faces.n; c++) {
		DBG 	if (nett->faces.e[c]->tag!=FACE_Actual) continue;
		DBG 	for (int c2=0; c2<nett->faces.e[c]->edges.n; c2++) {
		DBG 		if (nett->faces.e[c]->edges.e[c2]->tag==FACE_None) {
		DBG 			cerr <<"AAAAAAARRG! edge points to null face:"<<endl;
		DBG 			cerr <<"  net->face("<<c<<")->edge("<<c2<<")"<<endl;
		DBG 			exit(1);
		DBG 		}
		DBG 	}
		DBG }
		DBG //------------------------

		removeNet(netf);

		 //make sure all faces in nett now have proper facemode
		for (int c=0; c<nett->faces.n; c++) {
			if (nett->faces.e[c]->tag!=FACE_Actual) continue;
			if (nett->faces.e[c]->original==to) //the seed face
				poly->faces.e[nett->faces.e[c]->original]->cache->facemode=-nett->object_id;
			else poly->faces.e[nett->faces.e[c]->original]->cache->facemode=nett->object_id;
		}
		if (reseed>=0) Reseed(reseed);
		remapCache();
		needtodraw=1;
		return 0;
		
	} else if (modef<0 && modet>0 && modet==-modef) {
		 //from a seed to a net face within the same net

		 //split net along edge between from and to
		cerr <<" *** must implement split nets!"<<endl;
	}
	return 1;
}

/*! Return 1 for in an overlay
 */
Overlay *HedronWindow::scanOverlays(int x,int y, int *action,int *index,int *group)
{
	if (draw_overlays && overlays.n) {
		int c=0;
		for (c=0; c<overlays.n; c++) if (overlays.e[c]->PointIn(x,y)) break;
		if (c!=overlays.n) {
			 //mouse is in an overlay
			if (action) *action=overlays.e[c]->action;
			if (index) *index=overlays.e[c]->index;
			if (group) *group=OGROUP_TouchHelpers;
			return overlays.e[c]; //mouse over overlay, do nothing else
		}
	}

	if (draw_overlays && paperoverlays.n) {
		int c=0;
		for (c=0; c<paperoverlays.n; c++) if (paperoverlays.e[c]->PointIn(x,y)) break;
		if (c!=paperoverlays.n) {
			 //mouse is in an overlay
			if (action) *action=paperoverlays.e[c]->action;
			if (index) *index=paperoverlays.e[c]->index;
			if (group) *group=OGROUP_Papers;
			return paperoverlays.e[c]; //mouse over overlay, do nothing else
		}
	}
	if (action) *action=0;
	if (index)  *index=-1;
	if (group)  *group=OGROUP_None;

	return NULL;
}

/*! Adds a duplicate.
 */
int HedronWindow::AddNet(Net *net)
{
	Net *newnet=net->duplicate();
	nets.push(newnet);
	newnet->dec_count();
	needtodraw=1;
	return 1;
}

/*! Copies paper. Return 0 for success, 1 for error.
 */
int HedronWindow::AddPaper(PaperBound *paper)
{
	if (!paper) return 1;
	papers.push(new PaperBound(*paper),1);
	remapPaperOverlays();
	needtodraw=1;
	return 1;
}

//! If non-null, only SETS files, does no loading of any kind.
int HedronWindow::SetFiles(const char *hedron, const char *image, const char *project)
{
	int n=0;
	if (hedron)  { n++; makestr(polyhedronfile,hedron); }
	if (image)   { n++; makestr(spherefile,image); }
	if (project) { n++; makestr(polyptychfile,project); }
	return n;
}

/*! Uses ph, does not duplicate.
 */
int HedronWindow::installPolyhedron(Polyhedron *ph)
{
	if (!ph) return 1;
	if (poly) poly->dec_count();
	poly=ph;
	ph->inc_count();

	poly->makeedges();
	poly->BuildExtra();
	
	nets.flush();
	if (currentnet) currentnet->dec_count();
	currentnet=NULL;
	remapCache();

	currentface=currentpotential=-1;
	needtodraw=1;
	return 0;
}

//! Install a polyhedron contained in file.
/*! Return 0 for success, 1 for failure.
 */
int HedronWindow::installPolyhedron(const char *file)
{
	DBG cerr <<"...installPolyhedron("<<file<<")"<<endl;

	char *error=NULL;
	if (poly->dumpInFile(file,&error)) { //file not polyhedron
		char message[200];
		sprintf(message,"Could not load polyhedron: %s",file);
		newMessage(message);
		return 1;
	}
	touch_recently_used_xbel(file, "application/x-polyptych-doc", NULL,NULL, "Polyptych", true,false, NULL);
	char str[200];
	sprintf(str,"Loaded new polyhedron: %s",lax_basename(file));
	newMessage(str);

	poly->makeedges();
	poly->BuildExtra();
	
	nets.flush();
	if (currentnet) currentnet->dec_count();
	currentnet=NULL;
	makestr(polyhedronfile,file);
	remapCache();
	currentface=currentpotential=-1;
	needtodraw=1;
	return 0;
}

/*! Return 0 for image installed, or nonzero for error installing, and old image kept.
 */
int HedronWindow::installImage(const char *file)
{
	DBG cerr <<"attempting to install image at "<<file<<endl;

	const char *error=NULL;
	Imlib_Image image,
				image_scaled;
	try {
		image=imlib_load_image(file);
		if (!image) throw _("Could not load ");
		
		int width,height;
		imlib_context_set_image(image);
		width=imlib_image_get_width();
		height=imlib_image_get_height();

		 //gl texture dimensions must be powers of 2
		if (width>2048) { spheremap_width=2048; spheremap_height=1024; }
		else if (width>1024) { spheremap_width=1024; spheremap_height=512; }
		else if (width>512) { spheremap_width=512; spheremap_height=256; }
		else { spheremap_width=256; spheremap_height=128; }

		if (spheremap_data) delete[] spheremap_data;
		spheremap_data=new unsigned char[spheremap_width*spheremap_height*3];

		imlib_context_set_image(image);
		image_scaled=imlib_create_cropped_scaled_image(0,0,width,height,spheremap_width,spheremap_height);
		imlib_context_set_image(image_scaled);
		unsigned char *data=(unsigned char *)imlib_image_get_data_for_reading_only();
		int i=0,i2=0;
		for (int c=0; c<spheremap_width; c++) {
		  for (int c2=0; c2<spheremap_height; c2++) {
			spheremap_data[i++]=data[i2++];
			spheremap_data[i++]=data[i2++];
			spheremap_data[i++]=data[i2++];
			i2++;
		  }
		}
		imlib_free_image();
		imlib_context_set_image(image);
		imlib_free_image();

		makestr(spherefile,file);
	} catch (const char *err) {
		error=err;
	}

	char e[400];
	if (error) sprintf(e,_("Error with %s: %s"),error,file);
	else {
		installSpheremapTexture(0);
		sprintf(e,_("Loaded image %s"),file);
	}

	if (error) app->postmessage(e);
	//else remapCache();

	DBG cerr <<"\n"
	DBG	 <<"Using sphere file:"<<spherefile<<endl
	DBG	 <<" scaled width: "<<spheremap_width<<endl
	DBG	 <<"       height: "<<spheremap_height<<endl;

	return error?1:0;
}

//! Zap the image to be a generic lattitude and longitude
/*! Pass in a number outside of [0..1] to use default for that color.
 *
 * This updates spheremap_data, spheremap_width, and spheremap_height.
 * It does not reapply the image to the GL texture area.
 */
void HedronWindow::UseGenericImageData(double fg_r, double fg_g, double fg_b,  double bg_r, double bg_g, double bg_b)
{
	cerr <<"Generating generic latitude and longitude lines..."<<endl;

	int fr=(fg_r*255+.5),
		fg=(fg_g*255+.5),
		fb=(fg_b*255+.5);
	int br=(bg_r*255+.5),
		bg=(bg_g*255+.5),
		bb=(bg_b*255+.5);
	
	if (fr<0 || fr>255) { fr=100; fg=100; fb=200; }
	if (br<0 || br>255) { br=0; bg=0; bb=0; }


	spheremap_width=512;
	spheremap_height=256;
	if (spheremap_data) delete[] spheremap_data;
	spheremap_data=new unsigned char[spheremap_width*spheremap_height*3];

	 //set background color
	//memset(spheremap_data,0, spheremap_width*spheremap_height*3);
	for (int y=0; y<spheremap_height; y++) {
		for (int x=0; x<spheremap_width; x++) {
			spheremap_data[3*(x+y*spheremap_width)  ]=bb;
			spheremap_data[3*(x+y*spheremap_width)+1]=bg;
			spheremap_data[3*(x+y*spheremap_width)+2]=br;
		}
	}

	 //draw longitude
	for (int x=0; x<spheremap_width; x+=(int)((double)spheremap_width/36)) {
		for (int y=0; y<spheremap_height; y++) {
			spheremap_data[3*(x+y*spheremap_width)  ]=fb;
			spheremap_data[3*(x+y*spheremap_width)+1]=fg;
			spheremap_data[3*(x+y*spheremap_width)+2]=fr;
		}
	}
	 //draw latitude
	for (int y=0; y<spheremap_height; y+=(int)((double)spheremap_height/18)) {
		for (int x=0; x<spheremap_width; x++) {
			spheremap_data[3*(x+y*spheremap_width)  ]=fb;
			spheremap_data[3*(x+y*spheremap_width)+1]=fg;
			spheremap_data[3*(x+y*spheremap_width)+2]=fr;
		}
	}
}

int HedronWindow::Event(const EventData *data,const char *mes)
{
	if (!strcmp(mes,"new poly")) {
		const StrEventData *s=dynamic_cast<const StrEventData*>(data);
		if (!s) return 1;
		installPolyhedron(s->str);
		return 0;
	}

	if (!strcmp(mes,"new image")) {
		const StrEventData *s=dynamic_cast<const StrEventData*>(data);
		if (!s) return 1;
		installImage(s->str);
		return 0;
	}

	if (!strcmp(mes,"saveas")) {
		const StrEventData *s=dynamic_cast<const StrEventData*>(data);
		if (!s || !s->str || !*s->str) return 1;

		makestr(polyptychfile,s->str);
		XStoreName(app->dpy,xlib_window,polyptychfile);
		//if (Save(s->str)==0) newMessage("%s saved",s->str);
		//else newMessage("Error saving %s, not saved!",s->str);

		char ss[strlen(s->str)+50];
		if (Save(s->str)==0) sprintf(ss,"%s saved",s->str);
		else sprintf(ss,"Error saving %s, not saved!",s->str);
		newMessage(ss);

		return 0;
	}

	if (!strcmp(mes,"renderto")) {
		cout <<"**** must implement background rendering, and multinet output!!"<<endl;

		const StrEventData *s=dynamic_cast<const StrEventData*>(data);
		if (!s || !s->str || !*s->str) {
			DBG cerr <<" Missing destination to render to."<<endl;
			return 1;
		}

		if (!currentnet) { 
			DBG cerr <<" Missing current net to render with."<<endl;
			return 0;
		}
		DBG cerr <<"\n\n-------Rendering, please wait-----------\n"<<endl;

		currentnet->rebuildLines();
		Laxkit::ErrorLog log;
		int status=SphereToPoly(spherefile,
				 poly,
				 currentnet, 
				 2300,      // Render images no more than this many pixels wide
				 s->str, // Say filebase="blah", files will be blah000.png, ...this creates default filebase name
				 OUT_SVG,          // Output format
				 3, // oversample*oversample per image pixel. 3 seems pretty good in practice.
				 draw_texture, // Whether to actually render images, or just output new svg, for intance.
				 !draw_texture, // Whether to only draw lines, or include file names too
				 &extra_basis,
				 papers.n,
				 papers.e,
				 &log
				);
		
		if (log.Errors() || status!=0) {
			char *err=log.FullMessageStr();
			if (err) {
				newMessage(err);
				delete[] err;
			} else     newMessage("Unknown error encountered during rendering. The developers need to account for this!!!");
		} else newMessage(_("Rendered."));

		DBG cerr <<"result of render call: "<<status<<endl;
		return 0;
	}

	return anXWindow::Event(data,mes);
}

enum HedronWindowAction {
	HEDA_Help,
	HEDA_Papers,
	HEDA_LoadPolyhedron,
	HEDA_LoadImage,
	HEDA_ToggleMode,
	HEDA_Render,
	HEDA_Save,
	HEDA_SaveAs,
	HEDA_ResetNets,
	HEDA_TotalUnwrap,
	HEDA_ThickenCylinder,
	HEDA_ThinCylinder,
	HEDA_DrawSeams,
	HEDA_ToggleTexture,
	HEDA_DrawEdges,
	HEDA_RenderMode,
	HEDA_Camera,
	HEDA_PanLeft,
	HEDA_PanRight,
	HEDA_PanUp,
	HEDA_PanDown,
	HEDA_CameraLeft,
	HEDA_CameraRight,
	HEDA_CameraUp,
	HEDA_CameraDown,
	HEDA_CameraForward,
	HEDA_CameraBack,
	HEDA_CameraRotate,
	HEDA_CameraRotateCC,
	HEDA_NextFace,
	HEDA_PrevFace,
	HEDA_NextObject,
	HEDA_ObjectPlusX,
	HEDA_ObjectMinusX,
	HEDA_ObjectPlusY,
	HEDA_ObjectMinusY,
	HEDA_ObjectPlusZ,
	HEDA_ObjectMinusZ,
	HEDA_ObjectRotateX,
	HEDA_ObjectRotateXr,
	HEDA_ObjectRotateY,
	HEDA_ObjectRotateYr,
	HEDA_ObjectRotateZ,
	HEDA_ObjectRotateZr,
	HEDA_ScaleUp,
	HEDA_ScaleDown,
	HEDA_ResetView,
	HEDA_Stereo,
	HEDA_MAX
};

Laxkit::ShortcutHandler *HedronWindow::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler(whattype());

	sc->Add(HEDA_Help,           'h',0,0,           _("Help"),        _("Display help"),NULL,0);
	sc->AddShortcut(LAX_F1,0,0, HEDA_Help);
	sc->Add(HEDA_Papers,         'p',0,0,           _("TogglePapers"),_("Toggle paper overlay"),NULL,0);
	sc->Add(HEDA_LoadPolyhedron, 'p',ControlMask,0, _("Hedron"),      _("Load new polyhedron"),NULL,0);
	sc->Add(HEDA_LoadImage,      'i',ControlMask,0, _("Image"),       _("Load image"),NULL,0);
	sc->Add(HEDA_ToggleMode,     '`',0,0,           _("Toggle mode"), _("Toggle between net and solid modes"),NULL,0);
	sc->Add(HEDA_Render,         'r',ControlMask,0, _("Render"),      _("Render"),NULL,0);
	sc->Add(HEDA_Save,           's',ControlMask,0, _("Save"),        _("Save"),NULL,0);
	sc->Add(HEDA_SaveAs,         'S',ControlMask|ShiftMask,0, _("SaveAs"), _("Save as"),NULL,0);
	sc->Add(HEDA_ResetNets,      'D',ShiftMask,0,   _("ResetNets"),    _("Delete all defined nets"),NULL,0);
	sc->Add(HEDA_TotalUnwrap,    'A',ShiftMask,0,   _("TotalUnwrap"),  _("Totally unwrap"),NULL,0);
	sc->Add(HEDA_ThickenCylinder,'[',0,0,           _("ThickenLines"), _("Thicken seam and path indicators"),NULL,0);
	sc->Add(HEDA_ThinCylinder,   ']',0,0,           _("ThinLines"),    _("Thin seam and path indicators"),NULL,0);
	sc->Add(HEDA_DrawSeams,      'n',0,0,           _("DrawSeams"),    _("Toggle drawing seams"),NULL,0);
	sc->Add(HEDA_ToggleTexture,  't',0,0,           _("ToggleTexture"),_("Toggle using texture"),NULL,0);
	sc->Add(HEDA_DrawEdges,      'l',0,0,           _("DrawEdges"),    _("Toggle drawing edges"),NULL,0);
	sc->Add(HEDA_RenderMode,     'm',0,0,           _("RenderMode"),   _("Toggle render mode"),NULL,0);
	sc->Add(HEDA_Camera,         'c',0,0,           _("Camera"),       _("Select next camera"),NULL,0);
	sc->Add(HEDA_PanLeft,        LAX_Left,0,0,      _("PanLeft"),      _("Pan camera left"),NULL,0);
	sc->Add(HEDA_PanRight,       LAX_Right,0,0,     _("PanRight"),     _("Pan camera right"),NULL,0);
	sc->Add(HEDA_PanUp,          LAX_Up,0,0,        _("PanUp"),        _("Pan camera up"),NULL,0);
	sc->Add(HEDA_PanDown,        LAX_Down,0,0,      _("PanDown"),      _("Pan camera down"),NULL,0);
	sc->Add(HEDA_CameraLeft,     LAX_Left,ControlMask,0,            _("CameraLeft"),    _("Move camera left"),NULL,0);
	sc->Add(HEDA_CameraRight,    LAX_Right,ControlMask,0,           _("CameraRight"),   _("Move camera right"),NULL,0);
	sc->Add(HEDA_CameraUp,       LAX_Up,ControlMask|ShiftMask,0,    _("CameraUp"),      _("Move camera up"),NULL,0);
	sc->Add(HEDA_CameraDown,     LAX_Down,ControlMask|ShiftMask,0,  _("CameraDown"),    _("Move camera down"),NULL,0);
	sc->Add(HEDA_CameraForward,  LAX_Up,ControlMask,0,              _("CameraForward"), _("Move camera forward"),NULL,0);
	sc->Add(HEDA_CameraBack,     LAX_Down,ControlMask,0,            _("CameraBack"),    _("Move camera back"),NULL,0);
	sc->Add(HEDA_CameraRotate,   LAX_Left,ControlMask|ShiftMask,0,  _("CameraRotate"),  _("Rotate camera clockwise"),NULL,0);
	sc->Add(HEDA_CameraRotateCC, LAX_Right,ControlMask|ShiftMask,0, _("CameraRotateCC"),_("Rotate camera counterclockwise"),NULL,0);
	sc->Add(HEDA_NextFace,       'w',0,0,        _("NextFace"),    _("Select next face"),NULL,0);
	sc->Add(HEDA_PrevFace,       'v',0,0,        _("PrevFace"),    _("Select previous face"),NULL,0);
	sc->Add(HEDA_NextObject,     '=',0,0,        _("NextObject"),  _("Select next object"),NULL,0);
	sc->Add(HEDA_ObjectPlusX,    'x',0,0,        _("ObjectPlusX"), _("Move object in its positive x direction"),NULL,0);
	sc->Add(HEDA_ObjectMinusX,   'X',ShiftMask,0,_("ObjectMinusX"),_("Move object in its negative x direction"),NULL,0);
	sc->Add(HEDA_ObjectPlusY,    'y',0,0,        _("ObjectPlusY"), _("Move object in its positive y direction"),NULL,0);
	sc->Add(HEDA_ObjectMinusY,   'Y',ShiftMask,0,_("ObjectMinusY"),_("Move object in its negative y direction"),NULL,0);
	sc->Add(HEDA_ObjectPlusZ,    'z',0,0,        _("ObjectPlusZ"), _("Move object in its positive z direction"),NULL,0);
	sc->Add(HEDA_ObjectMinusZ,   'Z',ShiftMask,0,_("ObjectMinusZ"),_("Move object in its negative z direction"),NULL,0);
	sc->Add(HEDA_ObjectRotateX,  'x',ControlMask,0,          _("ObjectRotateX"), _("Rotate object around x axis"),NULL,0);
	sc->Add(HEDA_ObjectRotateXr, 'X',ControlMask|ShiftMask,0,_("ObjectRotateXr"),_("Rotate object around x axis the other way"),NULL,0);
	sc->Add(HEDA_ObjectRotateY,  'y',ControlMask,0,          _("ObjectRotateY"), _("Rotate object around y axis"),NULL,0);
	sc->Add(HEDA_ObjectRotateYr, 'Y',ControlMask|ShiftMask,0,_("ObjectRotateYr"),_("Rotate object around y axis the other way"),NULL,0);
	sc->Add(HEDA_ObjectRotateZ,  'z',ControlMask,0,          _("ObjectRotateZ"), _("Rotate object around z axis"),NULL,0);
	sc->Add(HEDA_ObjectRotateZr, 'Z',ControlMask|ShiftMask,0,_("ObjectRotateZr"),_("Rotate object around z axis the other way"),NULL,0);
	sc->Add(HEDA_ScaleUp,        '0',0,0,          _("ScaleUp"),     _("Scale object up"),NULL,0);
	sc->Add(HEDA_ScaleDown,      '9',0,0,          _("ScaleDown"),   _("Scale object down"),NULL,0);
	sc->Add(HEDA_ResetView,      ' ',0,0,          _("ResetView"),   _("Make camera point at object from a reasonable distance"),NULL,0);
	sc->Add(HEDA_Stereo,         'e',ControlMask,0,_("ToggleStereo"),_("Toggle viewing in crosseye stereo, straight stereo, or no stereo"),NULL,0);

	manager->AddArea(whattype(),sc);
	return sc;
}

/*! Return 0 for action performed, else 1.
 */
int HedronWindow::PerformAction(int action)
{
	if (action==HEDA_Help) {
		oldmode=mode;
		mode=MODE_Help;
		needtodraw=1;
		DBG cerr <<"--switching to help mode"<<endl;
		return 0;

	} else if (action==HEDA_Papers) {
		draw_papers=!draw_papers;
		needtodraw=1;
		return 0;

	} else if (action==HEDA_LoadPolyhedron) {
		 //change polyhedron
		FileDialog *f= new FileDialog(NULL,
									_("New Polyhedron"),_("New Polyhedron"),
									ANXWIN_REMEMBER,
									0,0,800,600,0,
									object_id,"new poly",
									FILES_OPEN_ONE
									);
		f->RecentGroup("Polyhedron");
		app->rundialog(f);
		return 0;

	} else if (action==HEDA_LoadImage) {
		 //change image
		app->rundialog(new FileDialog(NULL,_("New Image"),_("New Image"),
									ANXWIN_REMEMBER,
									0,0,800,600,0, object_id,"new image",
									FILES_PREVIEW|FILES_OPEN_ONE
									));
		return 0;

	} else if (action==HEDA_ToggleMode) {
		 //toggle view solid mode
		if (mode==MODE_Net) { 
			mode=MODE_Solid; 
			if (currentnet && currentnet->numActual()==1) removeNet(currentnet);
			if (currentnet) currentnet->dec_count();
			currentnet=NULL;
			remapCache(); 
			needtodraw=1; 
		} else if (mode==MODE_Solid) { mode=MODE_Net; remapCache(); needtodraw=1; }
		return 0;

	} else if (action==HEDA_Render) {
		 //render with spheretopoly
		app->rundialog(new FileDialog(NULL, _("Select render directory"),_("Select render directory"),
									ANXWIN_REMEMBER,
									0,0,800,600,0, object_id,"renderto",
									FILES_SAVE_AS,
									"render###.png",polyptychfile));
		return 0;

	} else if (action==HEDA_Save || action==HEDA_SaveAs) {
		 //save to a polyptych file
		char *file=NULL;
		int saveas=(action==HEDA_SaveAs);

		if (!polyptychfile) {
			saveas=1;
			char *p=NULL;
			if (spherefile && *spherefile) {
				appendstr(file,spherefile);
				p=strrchr(file,'.');
				if (p) *p='\0';
				const char *bname=lax_basename(polyhedronfile);
				if (bname) {
					appendstr(file,"-");
					appendstr(file,bname);
				}

			} else {
				if (polyhedronfile) appendstr(file,polyhedronfile);
				else makestr(file,"default-cube");
			}
			const char *ptr=strrchr(file,'/');
			p=strrchr(file,'.');
			if (p>ptr) *p='\0';
			appendstr(file,".polyptych");
		} else makestr(file,polyptychfile);

		foreground_color(rgbcolor(255,255,255));
		textout(this,file,-1,0,app->defaultlaxfont->textheight(),LAX_LEFT|LAX_TOP);

		if (saveas) {
			 //saveas
			app->rundialog(new FileDialog(NULL,_("Save as..."),_("Save as..."),
							ANXWIN_REMEMBER,
							0,0,0,0,0, object_id, "saveas",
							FILES_SAVE_AS|FILES_ASK_TO_OVERWRITE,
							file));
		} else {
			 //normal save
			DBG cerr<<"Saving to "<<file<<endl;
			if (Save(file)==0) textout(this,"...Saved",-1,0,2*app->defaultlaxfont->textheight(),LAX_LEFT|LAX_TOP);
			else textout(this,"...ERROR!!! Not saved",-1,0,2*app->defaultlaxfont->textheight(),LAX_LEFT|LAX_TOP);
		}

		delete[] file;
		return 0;

	} else if (action==HEDA_ResetNets) {
		 //remove the current net, reverting all faces to no net
		if (!currentnet) return 0;
		for (int c=0; c<currentnet->faces.n; c++) {
			if (currentnet->faces.e[c]->tag!=FACE_Actual) continue;
			poly->faces.e[currentnet->faces.e[c]->original]->cache->facemode=0;
		}
		removeNet(currentnet);
		remapCache();
		needtodraw=1;
		return 0;

	} else if (action==HEDA_TotalUnwrap) {
		 //total unwrap
		while (nets.n>1) nets.remove(nets.n-1);
		if (nets.n==0) {
			Net *net=new Net;
			net->_config=1;
			net->Basenet(poly);
			net->Unwrap(-1,0);
			poly->faces.e[0]->cache->facemode=-net->object_id;
			net->info=0;

			nets.push(net);
			net->dec_count();
		}
		nets.e[0]->TotalUnwrap();
		for (int c=0; c<poly->faces.n; c++) {
			if (c==nets.e[0]->info) poly->faces.e[c]->cache->facemode=-nets.e[0]->object_id;
			poly->faces.e[c]->cache->facemode=nets.e[0]->object_id;
		}
		remapCache();
		needtodraw=1;
		return 0;

	} else if (action==HEDA_ThickenCylinder) {
		 //resize cylinder width
		cylinderscale*=.9;
		if (cylinderscale<=.001) cylinderscale=.001;
		needtodraw=1;
		return 0;

	} else if (action==HEDA_ThinCylinder) {
		 //resize cylinder width
		cylinderscale*=1.1;
		needtodraw=1;
		return 0;

	} else if (action==HEDA_DrawSeams) {
		draw_seams++;
		if (draw_seams>3) draw_seams=0;
		if (draw_seams==0) newMessage(_("Do not draw net edges or unwrap path."));
		else if (draw_seams==1) newMessage(_("Draw net edges, but not unwrap path."));
		else if (draw_seams==2) newMessage(_("Draw unwrap path, but not net edges."));
		else if (draw_seams==3) newMessage(_("Draw both net edges, and unwrap path."));
		needtodraw=1;
		return 0;

	} else if (action==HEDA_ToggleTexture) {
		draw_texture=!draw_texture;
		needtodraw=1;
		return 0;

	} else if (action==HEDA_DrawEdges) {
		draw_edges=!draw_edges;
		needtodraw=1;
		return 0;

	} else if (action==HEDA_RenderMode) {
		 //low level render mode
		if (rendermode==0) rendermode=1;
		else if (rendermode==1) rendermode=2;
		else rendermode=0;
		if (rendermode==0) glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);
		if (rendermode==1) glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		if (rendermode==2) glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_Camera) {
		 // change camera
		current_camera++;
		if (current_camera>=cameras.n) current_camera=0;
		DBG cerr<<"----new camera: "<<current_camera<<endl;
		//eyem=cameras.e[current_camera]->model;
		needtodraw=1;
		return 0;

	} else if (action==HEDA_PanLeft) {
		cameras.e[current_camera]->Rotate(-5./180*M_PI,'y');
		cameras.e[current_camera]->transformTo();
		//--------------
		//		rotate(cameras.e[current_camera]->m,    //basis
		//			   cameras.e[current_camera]->m.y,  //axis
		//			   5./180*M_PI); //angle
		//		cameras.e[current_camera]->transformTo();

		//		glPushMatrix();
		//		glLoadIdentity();
		//		glRotatef(-5,0,1,0);
		//		glMultMatrixf(eyem);
		//		glGetFloatv(GL_MODELVIEW_MATRIX,eyem);
		//		glPopMatrix();

		needtodraw=1; 
		return 0;

	} else if (action==HEDA_PanRight) {
		cameras.e[current_camera]->Rotate(5./180*M_PI,'y');
		cameras.e[current_camera]->transformTo();
		//--------------
//		rotate(cameras.e[current_camera]->m,    //basis
//			   cameras.e[current_camera]->m.y,  //axis
//			   -5./180*M_PI); //angle
//		cameras.e[current_camera]->transformTo();
		//--------------
		//glPushMatrix();
		//glLoadIdentity();
		//glRotatef(5,0,1,0);
		//glMultMatrixf(eyem);
		//glGetFloatv(GL_MODELVIEW_MATRIX,eyem);
		//glPopMatrix();

		needtodraw=1; 
		return 0;

	} else if (action==HEDA_PanUp) {
		cameras.e[current_camera]->Rotate(5./180*M_PI,'x');
		cameras.e[current_camera]->transformTo();
		//--------------
//		rotate(cameras.e[current_camera]->m,    //basis
//			   cameras.e[current_camera]->m.x,  //axis
//			   5./180*M_PI); //angle
//		cameras.e[current_camera]->transformTo();
		//glPushMatrix();
		//glLoadIdentity();
		//glRotatef(5,1,0,0);
		//glMultMatrixf(eyem);
		//glGetFloatv(GL_MODELVIEW_MATRIX,eyem);
		//glPopMatrix();
		needtodraw=1; 
		return 0;

	} else if (action==HEDA_PanDown) {
		cameras.e[current_camera]->Rotate(-5./180*M_PI,'x');
		cameras.e[current_camera]->transformTo();
		//--------------
//		rotate(cameras.e[current_camera]->m,    //basis
//			   cameras.e[current_camera]->m.x,  //axis
//			   -5./180*M_PI); //angle
//		cameras.e[current_camera]->transformTo();
		//----------------
		//glPushMatrix();
		//glLoadIdentity();
		//glRotatef(-5,1,0,0);
		//glMultMatrixf(eyem);
		//glGetFloatv(GL_MODELVIEW_MATRIX,eyem);
		//glPopMatrix();
		needtodraw=1; 
		return 0;

	} else if (action==HEDA_CameraLeft) {
		//eyem[11]-=1; // the x translation part
		cameras.e[current_camera]->m.p+=cameras.e[current_camera]->m.x; // the x translation part
		cameras.e[current_camera]->transformTo();
		needtodraw=1;
		return 0;
				
	} else if (action==HEDA_CameraRight) {
		//eyem[12]-=1; // the x translation part
		cameras.e[current_camera]->m.p-=cameras.e[current_camera]->m.x; // the x translation part
		cameras.e[current_camera]->transformTo();
		needtodraw=1;
		return 0;		

	} else if (action==HEDA_CameraUp) {
		//eyem[13]+=1; // the y translation part
		cameras.e[current_camera]->m.p+=cameras.e[current_camera]->m.y; // the x translation part
		cameras.e[current_camera]->transformTo();
		needtodraw=1;
		return 0;	
				
	} else if (action==HEDA_CameraDown) {
		//eyem[13]-=1; // the y translation part
		cameras.e[current_camera]->m.p-=cameras.e[current_camera]->m.y; // the x translation part
		cameras.e[current_camera]->transformTo();
		needtodraw=1;
		return 0;

	} else if (action==HEDA_CameraForward) {
		//eyem[14]+=1; // the z translation part
		cameras.e[current_camera]->m.p-=cameras.e[current_camera]->m.z; // the x translation part
		cameras.e[current_camera]->transformTo();
		needtodraw=1;
		return 0;
				
	} else if (action==HEDA_CameraBack) {
		//eyem[14]-=1; // the z translation part
		cameras.e[current_camera]->m.p+=cameras.e[current_camera]->m.z; // the x translation part
		cameras.e[current_camera]->transformTo();
		needtodraw=1;
		return 0;	

	} else if (action==HEDA_CameraRotate) {
		rotate(cameras.e[current_camera]->m,    //basis
			   cameras.e[current_camera]->m.z,  //axis
			   -5./180*M_PI); //angle
		cameras.e[current_camera]->transformTo();
		//glPushMatrix();
		//glLoadIdentity();
		//glRotatef(5,0,0,1);
		//glMultMatrixf(eyem);
		//glGetFloatv(GL_MODELVIEW_MATRIX,eyem);
		//glPopMatrix();
		needtodraw=1; 
		return 0;

	} else if (action==HEDA_CameraRotateCC) {
		rotate(cameras.e[current_camera]->m,    //basis
			   cameras.e[current_camera]->m.z,  //axis
			   5./180*M_PI); //angle
		cameras.e[current_camera]->transformTo();
		//glPushMatrix();
		//glLoadIdentity();
		//glRotatef(-5,0,0,1);
		//glMultMatrixf(eyem);
		//glGetFloatv(GL_MODELVIEW_MATRIX,eyem);
		//glPopMatrix();
		needtodraw=1; 
		return 0;

	} else if (action==HEDA_NextFace) {
		currentface++;
		if (currentface>=poly->faces.n) currentface=0;
		needtodraw=1;
		return 0;

	} else if (action==HEDA_PrevFace) {
		currentface--;
		if (currentface<0) currentface=poly->faces.n-1;
		needtodraw=1;
		return 0;

	} else if (action==HEDA_NextObject) {
		if (++curobj>=things.n) curobj=0;
		DBG cerr <<"curobj="<<curobj<<" id="<<things.e[curobj]->id<<endl;
		return 0;

	} else if (action==HEDA_ObjectPlusX) {
		things.e[curobj]->Translate(movestep, 0., 0.);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectMinusX) {
		things.e[curobj]->Translate(-movestep,0., 0.);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectPlusY) {
		things.e[curobj]->Translate(0., movestep, 0.);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectMinusY) {
		things.e[curobj]->Translate(0.,-movestep, 0.);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectPlusZ) {
		things.e[curobj]->Translate(0., 0., movestep);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectMinusZ) {
		things.e[curobj]->Translate(0., 0., -movestep);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectRotateX) {
		things.e[curobj]->RotateLocal(5., 0., 1., 0.);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectRotateXr) {
		things.e[curobj]->RotateLocal(-5., 0., 1., 0.);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectRotateY) {
		things.e[curobj]->RotateLocal(5., 0., 0., 1.);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectRotateYr) {
		things.e[curobj]->RotateLocal(-5., 0., 0., 1.);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectRotateZ) {
		things.e[curobj]->RotateLocal(-5., 1., 0., 0.);
		needtodraw=1;
		return 0;
			
	} else if (action==HEDA_ObjectRotateZr) {
		things.e[curobj]->RotateLocal(5., 1., 0., 0.);
		needtodraw=1;
		return 0;


	} else if (action==HEDA_ScaleUp) {
		hedron->SetScale(1.2,1.2,1.2);
		needtodraw=1;
		return 0; 

	} else if (action==HEDA_ScaleDown) {
		hedron->SetScale(.8,.8,.8);
		needtodraw=1;
		return 0; 

	} else if (action==HEDA_ResetView) {
		//***
		needtodraw=1;
		return 0;

	} else if (action==HEDA_Stereo) {
		if (instereo==1) instereo=-1;
		else if (instereo==-1) instereo=0;
		else if (instereo==0) instereo=1;
		needtodraw=1;
		return 0;

	}


//	} else if (ch=='N') {
//		 //stare right into currentnet or currentface
//		if (currentnet) {
//			cout <<"**** implement stare at currentnet!!"<<endl;
//			needtodraw=1;
//		} else if (currentface>=0) {
//			cout <<"**** implement stare at currentface!!"<<endl;
//			needtodraw=1;
//		}
//		return 0;
//	}




	return 1;
}

int HedronWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const LaxKeyboard *kb)
{
	if (!win_on) return 0;

	 //-----help mode, consumes any key press
	if (mode==MODE_Help) {
		//if (ch==LAX_Esc) { 
			mode=oldmode;
			needtodraw=1;
			return 0;
		//}
		return 0;

	} else 	if (ch==LAX_Esc) { 
		if (mode==MODE_Net && currentnet) {
			 //deactivate current net
			if (currentnet->numActual()==1) removeNet(currentnet);
			if (currentnet) currentnet->dec_count();
			currentnet=NULL;
			needtodraw=1;
		}
		return 0;
	}


	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	//cout <<"*** po ch:"<<ch<<"  action:"<<action<<endl;

	 //----chars available during all modes
	if (ch=='q' && (state&LAX_STATE_MASK)==ControlMask) { app->quit(); return 0; }



	return anXWindow::CharInput(ch,buffer,len,state,kb);
}


} //namespace Polyptych


