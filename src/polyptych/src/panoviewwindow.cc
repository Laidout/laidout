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
// Copyright (C) 2012 by Tom Lechner
//



#include "panoviewwindow.h"
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


#define NUMLAT  50
#define NUMLONG 100

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



//--------------------------- PanoViewWindow -------------------------------

enum PanoViewWindowAction {
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
	HEDA_ZoomIn,
	HEDA_ZoomOut,
	HEDA_ResetView,
	HEDA_MAX
};


enum Mode {
	MODE_Solid,
	MODE_Help,
	MODE_Last
};

//! Return a string corresponding to the given Polyptych::Mode.
const char *modename(int mode)
{
	if (mode==MODE_Solid) return _("Solid");
	if (mode==MODE_Help) return _("Help");
	return " ";
}

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

/*! For reference, latitudes are "flat" sections parallel to the equator. Longitude
 * are great circles that go through the poles.
 */
Polyhedron *CreateGlobe(int numlatitude, int numlongitude)
{
	Polyhedron *hedron=new Polyhedron;

     //generate hedron
    double theta,gamma,r;
    spacepoint point;
    int newpointi;
    char face[200];
    for (int c=0; c<=numlongitude; c++) {
      for (int c2=0; c2<=numlatitude; c2++) {
        theta=2*M_PI*((double)c/numlongitude);
        gamma=M_PI/2 - M_PI*((double)c2/numlatitude);

        r=cos(gamma);
        point.x=r*cos(theta);
        point.y=r*sin(theta);
        point.z=sin(gamma);

        newpointi=hedron->AddPoint(point);
        if (c==0 || c2==0) continue;
        //if (c>2 || c2>2) break;

        sprintf(face,"%d %d %d %d",
                newpointi-1-(numlatitude+1),
                newpointi  -(numlatitude+1),
                newpointi,
                newpointi-1);
        hedron->AddFace(face);
      }
    }

    hedron->collapseVertices(1e-10);
	return hedron;
}


/*! If newpoly, then inc it's count. It does NOT create a copy from it current, so watch out.
 */
PanoViewWindow::PanoViewWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
							   int xx,int yy,int ww,int hh,int brder)
 	: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,NULL,0,0)
{
	poly=CreateGlobe(NUMLAT,NUMLONG);
	hedron=NULL;

	draw_texture=1;
	draw_axes=1;
	draw_info=1;
	draw_overlays=1;
	draw_edges=1;

	mouseover_overlay=-1;
	mouseover_index=-1;
	mouseover_group=OGROUP_None;
	mouseover_paper=-1;
	grab_overlay=-1; //mouse down on this overlay
	active_action=ACTION_None;

	oldmode=mode=MODE_Solid;

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
	view=0;
	firsttime=1;
	currentface=-1;
	currentpotential=-1;

	fovy=50*M_PI/180;
	current_camera=-1;
	
	sc=NULL;
}

PanoViewWindow::~PanoViewWindow()
{
	if (poly) poly->dec_count();
	if (spheremap_data) delete[] spheremap_data;
	if (spheremap_data_rotated) delete[] spheremap_data_rotated;

	if (spherefile) delete[] spherefile;
	if (polyptychfile) delete[] polyptychfile;

	if (sc) sc->dec_count();
}

//! Set the font size, if newfontsize>0.
/*! If fontfile!=NULL, then load a new font based on that file.
 * 
 * Returns the old font size, whether or not newfontsize>0.
 */
double PanoViewWindow::FontAndSize(const char *fontfile, double newfontsize)
{
	double f=fontsize;
	if (newfontsize>0) fontsize=newfontsize;
	if (fontfile) makestr(consolefontfile,fontfile);
	return f;
}

//! Update the current message displayed in the window to str.
void PanoViewWindow::newMessage(const char *str)
{
	messagetick=0;
	makestr(currentmessage,str);
	needtodraw=1;
}

//! Save a Polyptych file
/*! Return 0 for success or nonzero for error.
 */
int PanoViewWindow::Save(const char *saveto)
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

	fclose(f);
	touch_recently_used_xbel(saveto, "application/x-polyptych-doc", "Polyptych", "polyptych", "Polyptych", true, true, NULL);
	return 0;
}


//----------------------- polyhedron gl mapping

//! Break down a triangle into many, assigning spherical texture points along the way.
/*! This is akin to geodesic-izing, with n segments along each original triangle edge,
 *
 * Use p1,p2,p3 as the actual place of the triangle in space, and p1o,p2o,p3o
 * for where to put the triangle for texturizing.
 */
void PanoViewWindow::triangulate(spacepoint p1 ,spacepoint p2 ,spacepoint p3,
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
 */
void PanoViewWindow::mapPolyhedronTexture(Thing *thing)
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
		}
	}

	glEndList();
}

spacepoint PolarToRect(int c,int c2, double &theta,double &gamma)
{
	theta=2*M_PI*((double)c/NUMLONG);
	gamma=M_PI/2 - M_PI*((double)c2/NUMLAT);
	spacepoint point;
	double r=cos(gamma);
	point.x=r*cos(theta);
	point.y=r*sin(theta);
	point.z=sin(gamma);
	return point;
}

void addSpherePoint(spacepoint p,double theta,double gamma)
{
	theta/=2*M_PI;
	gamma/=M_PI;
	gamma+=.5;

	//if (theta<0) theta=0;
	//else if (theta>1) theta=1;
	//if (gamma<0) gamma=0;
	//else if (gamma>1) gamma=1;

	glTexCoord2f(theta,gamma);
	glNormal3f(p.x, p.y, p.z);
	glVertex3f(p.x, p.y, p.z);
}

void PanoViewWindow::mapPolyhedronTexture2(Thing *thing)
{
	glNewList(thing->id,GL_COMPILE);

     //generate hedron
    double theta,gamma;
    spacepoint p,p2,p3;

    for (int c=0; c<NUMLONG; c++) {
      for (int c2=0; c2<NUMLAT; c2++) {
		if (c2==0) {
			glBegin(GL_TRIANGLE_STRIP);

			p=PolarToRect(c,c2,theta,gamma);
			addSpherePoint(p,theta,gamma);

			p=PolarToRect(c+1,c2, theta,gamma);
			addSpherePoint(p,theta,gamma);
		}

		p=PolarToRect(c,c2+1, theta,gamma);
		addSpherePoint(p,theta,gamma);

		p=PolarToRect(c+1,c2+1, theta,gamma);
		addSpherePoint(p,theta,gamma);

		if (c2==NUMLAT-1) glEnd();
      }
    }

	glEnd();

	glEndList();
}

void PanoViewWindow::mapStereographicPlane(Thing *thing)
{
	glNewList(thing->id,GL_COMPILE);

     //generate hedron
    double theta,gamma;
    spacepoint p,p2,p3;

    for (int c=0; c<NUMLONG; c++) {
      for (int c2=0; c2<NUMLAT; c2++) {
		if (c2==0) {
			glBegin(GL_TRIANGLE_STRIP);

			p=PolarToRect(c,c2,theta,gamma);
			addSpherePoint(p,theta,gamma);

			p=PolarToRect(c+1,c2, theta,gamma);
			addSpherePoint(p,theta,gamma);
		}

		p=PolarToRect(c,c2+1, theta,gamma);
		addSpherePoint(p,theta,gamma);

		p=PolarToRect(c+1,c2+1, theta,gamma);
		addSpherePoint(p,theta,gamma);

		if (c2==NUMLAT-1) glEnd();
      }
    }

	glEnd();

	glEndList();
}

//! Create a new Thing with a new gl call list id.
/*! This is independent of any texture, or polyhedron coordinates.
 */
Thing *PanoViewWindow::makeGLPolyhedron()
{
	if (!poly) return NULL;
	
	GLuint hedronlist=glGenLists(1);
	Thing *thing=new Thing(hedronlist);

	thing->SetScale(5.,5.,5.);
	thing->SetColor(1., 1., 1., 1.0);
	thing->SetPos(0,0,0);

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
void PanoViewWindow::makecameras(void)
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


	eye=new EyeType();
	eye->Set(spacepoint(0,0,0),spacepoint(40,0,0),spacepoint(0,0,-40));
	eye->makestereo();
	cameras.push(eye);

	eye=new EyeType();
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

void PanoViewWindow::setlighting(void)
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
Polyhedron *PanoViewWindow::defineCube()
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

int PanoViewWindow::init()
{
	//---------------Polyhedron coordinate and texture final setup

	 // *** these two are static variables that really should be elsewhere
	if (!glvisual)  glvisual = glXChooseVisual(app->dpy, DefaultScreen(app->dpy), attributeListDouble);
	if (!glcontext) glcontext= glXCreateContext(app->dpy, glvisual, 0, GL_TRUE);

	if (!poly) {
		poly=defineCube();
		makestr(polyhedronfile,NULL);
		currentface=currentpotential=-1;
		needtodraw=1;
	}

	if (!spheremap_data) {
		UseGenericImageData();
	}



	 //-----------------Base window initialization
	anXWindow::init();


	//installSpheremapTexture(1);

	return 0;
}

//! Enable GL_TEXTURE_2D, define spheretexture id (if definetid), and load in spheremapdata.
void PanoViewWindow::installSpheremapTexture(int definetid)
{

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

void PanoViewWindow::installOverlays()
{
	return;
//	overlays.push(new Overlay("Roll",ACTION_Roll));
//	overlays.push(new Overlay("Unwrap",ACTION_Unwrap));
//	overlays.push(new Overlay("Reseed",ACTION_Reseed));
//	overlays.push(new Overlay("Shift Texture", ACTION_Shift_Texture));
//	overlays.push(new Overlay("Rotate Texture", ACTION_Rotate_Texture));
//	overlays.push(new Overlay("Zoom", ACTION_Zoom));
//
//	//ACTION_Rotate,
//	//ACTION_Pan,
//	//ACTION_Toggle_Mode,
//	//ACTION_Unwrap_Angle,
//	
//	 //---place overlays
//	int maxw=0, w, h;
//	FTBBox bbox;
//	FTPoint p1,p2;
//
//	 //determine bounds
//	for (int c=0; c<overlays.n; c++) {
//		if (isblank(overlays.e[c]->Text())) continue;
//
//		bbox=consolefont->BBox(overlays.e[c]->Text(),-1);
//		p1=bbox.Upper();
//		p2=bbox.Lower();
//		w=p1.X()-p2.X() + pad;
//		h=p1.Y()-p2.Y() + pad;
//
//		if (w>maxw) maxw=w;
//		overlays.e[c]->setbounds(0,w, 0,h);
//	}
//	placeOverlays();
}

//! Determine bounding box of the string with consolefont.
/*! Return value is the width, the same as what width variable gets set to.
 */
double PanoViewWindow::getExtent(const char *str,int len, double *width,double *height)
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
void PanoViewWindow::placeOverlays()
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
}

int PanoViewWindow::MoveResize(int x,int y,int w,int h)
{
	int c=anXWindow::MoveResize(x,y,w,h);
	reshape(win_w,win_h);
	return c;
}

int PanoViewWindow::Resize(int w,int h)
{
	int c=anXWindow::Resize(w,h);
	reshape(win_w,win_h);
	return c;
}

//! Setup camera based on new window dimensions, and placeOverlays().
/*! This is called when the window size is changed.
 */
void PanoViewWindow::reshape (int w, int h)
{
	helpoffset=10000;
	placeOverlays();

	glViewport (0, 0, (GLsizei) w, (GLsizei) h);

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


void PanoViewWindow::Refresh()
{
	if (!win_on) return;
	if (glXGetCurrentDrawable()!=xlib_window) glXMakeCurrent(app->dpy, xlib_window, glcontext);

	needtodraw=0;

	if (mode==MODE_Help) drawHelp();
	else Refresh3d();

	 //--all done drawing, now swap buffers
	glFlush();
	glXSwapBuffers(app->dpy,xlib_window);
}

void PanoViewWindow::drawHelp()
{
	char *helptext=NULL;

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
void PanoViewWindow::Refresh3d()
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
	gluLookAt(b->m.p.x,b->m.p.y,b->m.p.z, 
			  //b->focus.x,b->focus.y,b->focus.z,
			  b->m.p.x-b->m.z.x, b->m.p.y-b->m.z.y, b->m.p.z-b->m.z.z,
			  b->m.y.x, b->m.y.y, b->m.y.z);
	
	
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
		
		//glPushMatrix();
		//glMultMatrixf(hedron->m);

		glEnable(GL_TEXTURE_2D);
//		glBindTexture(GL_TEXTURE_2D, spheretexture);
//		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
//		//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexImage2D(GL_TEXTURE_2D,   //target
//					 0,               //mipmap level
//					 GL_RGB,          //number of color components per pixel
//					 spheremap_width, //width
//					 spheremap_height,//height
//					 0,               //border with, must be 0 or 1
//					 //GL_RGB,          //order of data
//					 GL_BGR,          //order of data
//					 GL_UNSIGNED_BYTE,//type of data
//					 spheremap_data); //pixel data
		//glPopMatrix();
	}

	for (c=0; c<things.n; c++) {
		//DBG cerr <<"drawing thing number "<<c<<endl;
		things.e[c]->Draw();
	}

	 //draw frame of hedron
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

//		 //draw lines around any faces
//		for (int c2=0; c2<poly->faces.n; c2++) {
//			if (poly->faces.e[c2]->cache->facemode>=0) continue;
//			for (int c3=0; c3<poly->faces.e[c2]->pn; c3++) {
//				p1=poly->faces.e[c2]->cache->points3d[c3];
//				p2=(c3==0?poly->faces.e[c2]->cache->points3d[poly->faces.e[c2]->pn-1]
//						:poly->faces.e[c2]->cache->points3d[c3-1]);
//				v=p1-p2;
//				a=v/spacepoint(0,0,1);
//				//h=norm(v);
//				//a=spacepoint(0,0,1)/v;
//				//ang=acos(v.z/h)/M_PI*180;
//
//				glVertex3f(p1.x,p1.y,p1.z);
//				glVertex3f(p2.x,p2.y,p2.z);
//			}
//		}
		glEnd();
		glLineWidth(1);

		glPopMatrix();
	}
	glDisable(GL_TEXTURE_2D);
	



//	 //---- draw cameras
//	for (c=0; c<cameras.n; c++) {
//		if (cameras.e[c]==cameras.e[current_camera]) continue;
//		glPushMatrix();
//		glMultMatrixf(cameras.e[c]->model);
//		//cameras.e[c]->transformForDrawing();
//		camera_shape.Draw();
//		glPopMatrix();
//	}
	


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

		 //write at top of screen:  write currentmessage
		//consolefont->Render(modename(mode),-1,FTPoint(0,win_h-fontsize));
		if (currentmessage) {
			consolefont->Render(currentmessage,-1,FTPoint(0,win_h-2*fontsize));
			messagetick++;
			if (messagetick>10) makestr(currentmessage,NULL);
		}
	}

	 //draw paper control overlays
	glFlush();

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
void PanoViewWindow::drawRect(DoubleBBox &box, 
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
void PanoViewWindow::transparentFace(int face, double r, double g, double b, double a)
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

int PanoViewWindow::event(XEvent *e)
{
	if (e->type==MapNotify) {
		glXMakeCurrent(app->dpy, xlib_window, glcontext);
		
		 // set viewport transform
		reshape(win_w,win_h);	// this is only gl commands
	
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

int PanoViewWindow::Idle(int tid)
{
	int x,y, lx,ly;
	int m=buttondown.whichdown(0);
	
	int state=0;
	buttondown.getextrainfo(m,LEFTBUTTON, NULL,&state);
	buttondown.getinitial(m,LEFTBUTTON, &lx,&ly);
	buttondown.getcurrent(m,LEFTBUTTON, &x,&y);

	flatpoint d,p=flatpoint(x,y);
	d=p-flatpoint(lx,ly);
	d/=10; //scale down a little

	if (active_action==ACTION_Roll) {
		spacepoint axis;
		axis=d.y*cameras.e[current_camera]->m.x + d.x*cameras.e[current_camera]->m.y;
		things.e[curobj]->RotateAbs(norm(d)/5, axis.x,axis.y,axis.z);
		needtodraw=1;

	} else if (active_action==ACTION_Rotate) {
		if (d.x) { //rotate thing around camera z
			spacepoint axis;
			axis=d.x*cameras.e[current_camera]->m.z;
			things.e[curobj]->RotateAbs(norm(d)/5, axis.x,axis.y,axis.z);
			needtodraw=1;
		}
	}
	return 0;
}

int PanoViewWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{
	if (mode==MODE_Help) {
		mode=oldmode;
		needtodraw=1;
		return 0;
	}

	int group=OGROUP_None;
	Overlay *overlay=scanOverlays(x,y, NULL,NULL,&group);
	int index=(overlay?overlay->id:-1);

	if (group==OGROUP_None) index=state;
	if (!buttondown.any()) timerid=app->addtimer(this, 30, 30, -1);

	buttondown.down(mouse->id,LEFTBUTTON,x,y, group,index);


	return 0;
}

int PanoViewWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *mouse)
{
	int overlayid=-1;
	int group=OGROUP_None, orig_group=OGROUP_None;
	//int dragged=
	buttondown.up(mouse->id,LEFTBUTTON, &orig_group,&overlayid);
	if (timerid) app->removetimer(this,timerid);

	Overlay *overlay=scanOverlays(x,y, NULL,NULL,&group);
	if ((orig_group==OGROUP_TouchHelpers) && overlayid>0) {
		if (overlay && overlay->id==overlayid) {
			if (group==OGROUP_TouchHelpers) {
				 //changing active_action
				if (active_action==overlay->action) {
					active_action=ACTION_None;
				} else active_action=overlay->action;
				needtodraw=1;
				return 0;
			}
		}
	}

	return 0;
}

int PanoViewWindow::MBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{
	if (mode==MODE_Help) {
		mode=oldmode;
		needtodraw=1;
		return 0;
	}

	buttondown.down(mouse->id,MIDDLEBUTTON,x,y);
	return 0;
}

int PanoViewWindow::MBUp(int x,int y,unsigned int state,const LaxMouse *mouse)
{
	buttondown.up(mouse->id,MIDDLEBUTTON);
	return 0;
}

int PanoViewWindow::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
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

int PanoViewWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *mouse)
{
	DBG cerr <<"PanoViewWindow::RBUp: rbdown=="<<rbdown<<"  currentface=="<<currentface<<endl;

	if (!(buttondown.isdown(mouse->id,RIGHTBUTTON))) return 0;

	buttondown.up(mouse->id,RIGHTBUTTON, &rbdown);
	return 0;
}

int PanoViewWindow::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{
	if (mode==MODE_Help) {
		if (helpoffset==10000) helpoffset=win_h-fontsize;
		else helpoffset+=fontsize;
		needtodraw=1;
		return 0;
	}

	//change fov
	PerformAction(HEDA_ZoomIn);

	needtodraw=1;
	return 0;
}

int PanoViewWindow::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{
	if (mode==MODE_Help) {
		if (helpoffset==10000) helpoffset=win_h-fontsize;
		else helpoffset-=fontsize;
		needtodraw=1;
		return 0;
	}

	//change fov
	PerformAction(HEDA_ZoomOut);

	needtodraw=1;
	return 0;
}

int PanoViewWindow::MouseMove(int x,int y,unsigned int state,const LaxMouse *mouse)
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

		return 0;
	}

	//so below is for when button is down



	int lx,ly;
	int bgroup=OGROUP_None,bindex=-1;
	buttondown.move(mouse->id, x,y, &lx,&ly);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &bgroup,&bindex);


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
//			things.e[curobj]->RotateAbs(angle*180/M_PI, axis.x,axis.y,axis.z);
//		}

		needtodraw=1;
		return 0;
	}



	if (!buttondown.isdown(mouse->id,LEFTBUTTON) && !buttondown.isdown(mouse->id,RIGHTBUTTON) && !buttondown.isdown(mouse->id,RIGHTBUTTON))
		return 0;


	ActionType current_action=active_action;
	if (buttondown.isdown(0,MIDDLEBUTTON)) current_action=ACTION_Unwrap_Angle;
	else if (buttondown.isdown(0,RIGHTBUTTON)) current_action=ACTION_Unwrap;

	if (current_action==ACTION_Zoom) {
		 //zoom
		if (x-mbdown>10) { WheelUp(x,y,0,0,mouse); mbdown=x; }
		else if (mbdown-x>10) { WheelDown(x,y,0,0,mouse); mbdown=x; }

	}





	if (active_action==ACTION_None || active_action==ACTION_Roll || active_action==ACTION_Rotate) {
		if       ((state&LAX_STATE_MASK)== 0)                      active_action=ACTION_Roll;
		else if  ((state&LAX_STATE_MASK)== ShiftMask)              active_action=ACTION_Rotate;
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
void PanoViewWindow::remapCache(int start, int end)
{
	if (!poly) return; //do not attempt if called too early!

	if (start<0) start=0;
	if (end<0) end=poly->faces.n-1;

	ExtraFace *cache=NULL;
	 

	for (int c=start; c<=end; c++) {
		 //each facemode==0 reset to normal
		cache=poly->faces.e[c]->cache;
		if (!cache) continue;

		//****
		cache->axis=poly->basisOfFace(c);
		for (int c2=0; c2<poly->faces.e[c]->pn; c2++) {
			cache->points3d[c2]=poly->VertexOfFace(c,c2,0);
			cache->points2d[c2]=flatten(cache->points3d[c2],cache->axis);
		}
	}

	if (hedron) mapPolyhedronTexture2(hedron);
}

/*! Return 1 for in an overlay
 */
Overlay *PanoViewWindow::scanOverlays(int x,int y, int *action,int *index,int *group)
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

/*! Return 0 for image installed, or nonzero for error installing, and old image kept.
 */
int PanoViewWindow::installImage(const char *file)
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
		if (width>4096) { spheremap_width=4096; spheremap_height=2048; }
		else if (width>2048) { spheremap_width=2048; spheremap_height=1024; }
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
void PanoViewWindow::UseGenericImageData(double fg_r, double fg_g, double fg_b,  double bg_r, double bg_g, double bg_b)
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
	for (float x=0; x<spheremap_width; x+=((float)spheremap_width/36)) {
		for (int y=0; y<spheremap_height; y++) {
			spheremap_data[3*((int)x+y*spheremap_width)  ]=fb;
			spheremap_data[3*((int)x+y*spheremap_width)+1]=fg;
			spheremap_data[3*((int)x+y*spheremap_width)+2]=fr;
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

int PanoViewWindow::Event(const EventData *data,const char *mes)
{
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

	return anXWindow::Event(data,mes);
}

Laxkit::ShortcutHandler *PanoViewWindow::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler(whattype());

	sc->Add(HEDA_Help,           'h',0,0,           _("Help"),        _("Display help"),NULL,0);
	sc->AddShortcut(LAX_F1,0,0, HEDA_Help);
	sc->Add(HEDA_LoadImage,      'i',ControlMask,0, _("Image"),       _("Load image"),NULL,0);
	sc->Add(HEDA_Save,           's',ControlMask,0, _("Save"),        _("Save"),NULL,0);
	sc->Add(HEDA_SaveAs,         'S',ControlMask|ShiftMask,0, _("SaveAs"), _("Save as"),NULL,0);
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
	sc->Add(HEDA_ZoomIn,         '=',0,0,        _("ZoomIn"),     _("Narrow field of view"),NULL,0);
	sc->Add(HEDA_ScaleDown,      '-',0,0,        _("ZoomOut"),   _("Widen field of view"),NULL,0);
	sc->Add(HEDA_ResetView,      ' ',0,0,        _("ResetView"),   _("Make camera point at object from a reasonable distance"),NULL,0);

	manager->AddArea(whattype(),sc);
	return sc;
}

/*! Return 0 for action performed, else 1.
 */
int PanoViewWindow::PerformAction(int action)
{
	if (action==HEDA_Help) {
		oldmode=mode;
		mode=MODE_Help;
		needtodraw=1;
		DBG cerr <<"--switching to help mode"<<endl;
		return 0;

	} else if (action==HEDA_LoadImage) {
		 //change image
		app->rundialog(new FileDialog(NULL,_("New Image"),_("New Image"),
									ANXWIN_REMEMBER,
									0,0,800,600,0, object_id,"new image",
									FILES_PREVIEW|FILES_OPEN_ONE
									));
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

	} else if (action==HEDA_ZoomIn) {
		 //narrow field of view
		fovy*=.98;
		reshape(win_w,win_h);
		needtodraw=1;
		return 0;

	} else if (action==HEDA_ZoomOut) {
		 //widen field of view
		fovy*=1.02;
		reshape(win_w,win_h);
		needtodraw=1;
		return 0;

	} else if (action==HEDA_ResetView) {
		EyeType *eye=cameras.e[current_camera];
		eye->Set(spacepoint(0,0,0),spacepoint(40,0,0),spacepoint(0,0,-40));
		eye->makestereo();
		needtodraw=1;
		return 0;
	}



	return 1;
}

int PanoViewWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const LaxKeyboard *kb)
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


