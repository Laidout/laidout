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
// Copyright (C) 2011 by Tom Lechner
//



#include "glbase.h"

#include <iostream>
using namespace std;
#define DBG


namespace Polyptych {


/*! \defgroup gl Opengl helpers
 * @{
 */ 

void dumpMatrix4(GLfloat *m,const char *str)
{
	DBG cerr <<(str?str:"matrix")<<":"<<endl;
	int i,j;
	for (i=0; i<4; i++) {
	  cerr <<"  ";
	  for (j=0; j<4; j++) {
		cerr <<m[i*4+j]<<"  ";
	  }
	  cerr <<endl;
	}
}



//-------------------------- Lower level gl utils -----------------------------

//! From point x,y,z, add spherical texture coordinate.
/*! If trans, then transform the point by extra_basis.
 * Each texture coordinate maps to a point in an equirectangular sphere image.
 */
void addGLSphereTexpt(float x,float y,float z, Basis *extra_basis)
{
	if (extra_basis) {
		spacepoint p(x,y,z);
		transform(p,*extra_basis);
		x=p.x;
		y=p.y;
		z=p.z;
	}
	float r=sqrt(x*x+y*y);
	float theta=(atan2(y,x)/M_PI+1)/2; //theta
	float gamma=atan(z/r)/M_PI+.5;   //gamma
	if (theta<0) theta=0;
	else if (theta>1) theta=1;
	if (gamma<0) gamma=0;
	else if (gamma>1) gamma=1;

	glTexCoord2f(theta,gamma);
}



//-------------------- Basic cylinder ------------------------
GLuint THECYLINDER=0;
GLuint cylinderCallList()
{
	if (THECYLINDER==0) makecylinder();
	return THECYLINDER;
}

//! Define a gl call list for a basic cylinder that fits in a unit cube.
void makecylinder(void)
{
	THECYLINDER=glGenLists(1);
	GLUquadric *quad=NULL;
	quad=gluNewQuadric();

	glNewList(THECYLINDER,GL_COMPILE);
	//gluQuadricOrientation(quad, GLU_OUTSIDE);
	gluQuadricOrientation(quad, GLU_INSIDE);
	gluCylinder(quad, 
			.5, //base radius
			.5, //top radius
			1, //height, shape is 0..1
			8, //divisions around
			1); //divisions up
	glEndList();
	gluDeleteQuadric(quad);
}

//! Draw a basic cylinder whose flat sides are at p1 and p2.
void drawCylinder(spacepoint p1, spacepoint p2, double scalew,GLfloat *extram)
{
	glPushMatrix();
	if (extram) glMultMatrixf(extram);

	 //we can use any old "up" vector
	spacepoint v=p2-p1;
	double scalez=norm(v);
	if (scalew<=0) scalew=scalez/10;
	if (v.y==0 && v.z==0) v=spacepoint(0,0,1); else v=spacepoint(1,0,0);

	Basis b(p1,p2,p1+v);
	const double m[16]={  b.x.x,b.x.y,b.x.z, 0,
		 			b.y.x,b.y.y,b.y.z, 0,
					b.z.x,b.z.y,b.z.z, 0,
					b.p.x,b.p.y,b.p.z, 1};
	//bas.x.x=m[0];
	//bas.x.y=m[1];
	//bas.x.z=m[2];

	//bas.y.x=m[4];
	//bas.y.y=m[5];
	//bas.y.z=m[6];

	//bas.z.x=m[8];
	//bas.z.y=m[9];
	//bas.z.z=m[10];

	//bas.p.x=m[12];
	//bas.p.y=m[13];
	//bas.p.z=m[14];

	glMultMatrixd(m);
	glScalef(scalew, scalew, scalez);
	glCallList(THECYLINDER);

	glPopMatrix();
}

//-------------------- Basic sphere ------------------------
GLuint THESPHERE=0;
GLuint sphereCallList()
{
	if (THESPHERE==0) makesphere();
	return THESPHERE;
}


//! Define a gl call list for a basic sphere.
void makesphere(void)
{
	THESPHERE=glGenLists(1);
	GLUquadric *quad=NULL;
	quad=gluNewQuadric();

	glNewList(THESPHERE,GL_COMPILE);
	gluSphere(quad, 
			.1, //radius
			12, //slices
			6); //stack
	glEndList();
	gluDeleteQuadric(quad);
}

//-------------------- Basic open ended cube ------------------------

GLuint THECUBE=0;
GLuint cubeCallList()
{
	if (THECUBE==0) makecube();
	return THECUBE;
}

//! Define a gl call list for an open ended cube.
void makecube(void)
{
	THECUBE=glGenLists(1);
	glNewList(THECUBE,GL_COMPILE);
//		glColor3f(1,0,0);
		glBegin(GL_QUAD_STRIP);
	  		glNormal3f(1,-1,-1);
	  		glVertex3f(1,-1,-1);
	  		glNormal3f(1,1,-1);
	  		glVertex3f(1,1,-1);
	  		glNormal3f(1,-1,1);
	  		glVertex3f(1,-1,1);
	  		glNormal3f(1,1,1);
	  		glVertex3f(1,1,1);
			
	  		glNormal3f(-1,-1,1);
	  		glVertex3f(-1,-1,1);
	  		glNormal3f(-1,1,1);
	  		glVertex3f(-1,1,1);

	  		glNormal3f(-1,-1,-1);
	  		glVertex3f(-1,-1,-1);
	  		glNormal3f(-1,1,-1);
	  		glVertex3f(-1,1,-1);

	  		glNormal3f(1,-1,-1);
	  		glVertex3f(1,-1,-1);
	  		glNormal3f(1,1,-1);
	  		glVertex3f(1,1,-1);
		glEnd();
	glEndList();
}


//! Draw x (red), y (green), and z (blue) axes in gl.
void drawaxes(double scale, GLfloat *matrix)
{
	 //----  Draw Weird Line
	glPushMatrix();
	if (matrix) glMultMatrixf(matrix);

	setmaterial(1,0,0);
	glBegin (GL_LINES);
		 //x
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(scale, 0, 0);
	glEnd();
	setmaterial(0,1,0);
	glBegin (GL_LINES);
		 //y
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0, scale, 0);
	glEnd();
	setmaterial(0,0,1);
	glBegin (GL_LINES);
		 //z
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0,0,scale);
	glEnd();
	glPopMatrix();
}



//------------------------- FTGL helper functions ----------------------------

//! Allocate and return a new FTGLPixmapFont at the given size, based on the given file.
FTFont *setupfont(const char *fontfile, double facesize)
{
	FTFont *font=new FTGLPixmapFont(fontfile);
	//consolefont=new FTGLPolygonFont(consolefontfile);

	// If something went wrong, bail out.
	if(font->Error()) {
		DBG cerr <<"*** Cannot load font "<<fontfile<<"!"<<endl;
		exit(1);
	}

	// Set the font size and render a small text.
	font->FaceSize(facesize);
	return font;
}

//--------------------------- GlData ----------------------------
// **** is this used???
GlData::GlData()
{
	 //set matrix to Identity
	int i,j;
	for (i=0; i<4; i++) {
	  for (j=0; j<4; j++) {
		if (i==j) matrix[i*4+j]=1;
		else      matrix[i*4+j]=0;
	  }
	}
}

//-------------------------- EyeType -------------------------------

/*! \class EyeType
 * \brief Holds and synchronizes eyes for potential stereo vision.
 *
 * Composed of left, right and middle eyes. all three look toward
 * focus. dist is the distance between the left and right eyes.
 * The focal plane is distance p to the point of whichever eye is used.
 */

EyeType::EyeType()
{
	left_fov=90; //<- need to implement these!
	right_fov=90;
	middle_fov=90;

	reset();

	 //set matrices to Identity
	int i,j;
	for (i=0; i<4; i++) {
	  for (j=0; j<4; j++) {
		if (i==j) { projection[i*4+j]=model[i*4+j]=1; }
		else      { projection[i*4+j]=model[i*4+j]=0; }
	  }
	}
}

/*! Initialize so camera z is roughly upward in world z,
 * put camera at (5,5,5) pointing to origin, dist=1.5,
 * and fplane=9. 
 *
 * These values are totally arbitrary. Should probably have some 
 * default adjustible config object to reset to.
 *
 * \todo WARNING! This is currently the incorrect "toe in" method of positioning.
 *   Correct, more comfortable style is such that the view planes intersect,
 *   but the direction of sight is parallel for all of l,m,r.
 *   More research needed, I don't quite understand it.
 */
void EyeType::reset()
{
	dist=1.5; fplane=9; focus=spacevector();
	m=Basis(spacepoint(5,5,5),spacepoint(6,6,6),spacepoint(0,5,5));
	makestereo();
//	r=Basis(spacepoint(9,.75,0),spacepoint(10,.75,0),spacepoint(9,1,0));
//      l=Basis(spacepoint(9,-.75,0),spacepoint(10,-.75,0),spacepoint(9,0,0));
}

/*! Assuming m is positioned correctly, and we are looking at focus,
 * then update l and r to be positioned and oriented correctly.
 *
 * assumes m,focus,dist,fplane defined.
 */
void EyeType::makestereo()
{
	spacepoint pr,pl;
	pr=m.p+dist/2*m.x;
	pl=m.p-dist/2*m.x;
	l=Basis(pl,2*pl-focus,pl+m.x);
	r=Basis(pr,2*pr-focus,pr+m.x);
}

void EyeType::transformForDrawing()
{//***
}

/*! Regenerate model and projection matrix based on what's in m.
 */
void EyeType::transformTo()
{
//	glMatrixMode(GL_MODELVIEW);
//	glPushMatrix();
//	glLoadIdentity();
//	gluLookAt(m.p.x,m.p.y,m.p.z, focus.x,focus.y,focus.z,  m.p.x+m.y.x, m.p.y+m.y.y, m.p.z+m.y.z);
//	glGetFloatv(GL_MODELVIEW_MATRIX,model);
//	glPopMatrix();

	model[ 0]=m.x.x;  model[ 1]=m.x.y;  model[ 2]=m.x.z;  model[ 3]=0;
	model[ 4]=m.y.x;  model[ 5]=m.y.y;  model[ 6]=m.y.z;  model[ 7]=0;
	model[ 8]=m.z.x;  model[ 9]=m.z.y;  model[10]=m.z.z;  model[11]=0;
	model[12]=m.p.x;  model[13]=m.p.y;  model[14]=m.p.z;  model[15]=1;

	projection[ 0]=m.x.x;  projection[ 1]=m.x.y;  projection[ 2]=m.x.z;  projection[ 3]=0;
	projection[ 4]=m.y.x;  projection[ 5]=m.y.y;  projection[ 6]=m.y.z;  projection[ 7]=0;
	projection[ 8]=m.z.x;  projection[ 9]=m.z.y;  projection[10]=m.z.z;  projection[11]=0;
	projection[12]=m.p.x;  projection[13]=m.p.y;  projection[14]=m.p.z;  projection[15]=1;
}

void EyeType::Rotate(double angle, char axis)
{
	rotate(m,axis,angle,0);
	transformTo();
}

/*! Like gluLookAt, position the middle basis at atpoint, looking toward
 * nfocus, with the positive y direction the way of up. The negative Z axis points from
 * atpoint toward nfocus.
 *
 * \todo *** change up to be vector rather than absolute point!!!!
 */
void EyeType::Set(spacepoint atpoint, spacepoint nfocus, spacepoint up)
{
	focus=nfocus;
	m.Set(atpoint, nfocus-2*(nfocus-atpoint), atpoint+(up-atpoint)/(atpoint-nfocus));
	makestereo();
	transformTo();

	DBG dumpMatrix4(model,"eye");
}

/*--------------------- Light and Material structures ------------------------*/


void Light::Spots(GLfloat dirx,GLfloat diry,GLfloat dirz, GLfloat exp, GLfloat cutoff, GLfloat c, GLfloat l, GLfloat q)
{
	spot_direction[0]=dirx;
	spot_direction[1]=diry;
	spot_direction[2]=dirz;
	spot_direction[3]=0;

	spot_exponent=exp;
	spot_cutoff=cutoff;
	constant_attenuation=c;
	linear_attenuation=l;
	quadratic_attenuation=q;
}

//-----------

 //! enable=1 call enable, 0=call disable, other=don't enable or disable
void SetLight(Light *light,unsigned int mask,int enable)
{
	if (!light) return;
	if (mask==0) mask=light->mask;
	if (mask&LIGHT_POSITION) glLightfv (light->which, GL_POSITION, light->position);
	if (mask&LIGHT_AMBIENT)  glLightfv (light->which, GL_AMBIENT,  light->ambient);
	if (mask&LIGHT_DIFFUSE)  glLightfv (light->which, GL_DIFFUSE,  light->diffuse);
	if (mask&LIGHT_SPECULAR) glLightfv (light->which, GL_SPECULAR, light->specular);

	if (mask&LIGHT_SPOT_DIRECTION) glLightfv(light->which, GL_SPOT_DIRECTION, light->spot_direction);
	if (mask&LIGHT_SPOT_EXPONENT)  glLightf (light->which, GL_SPOT_EXPONENT, light->spot_exponent);
	if (mask&LIGHT_SPOT_CUTOFF)    glLightf (light->which, GL_SPOT_CUTOFF, light->spot_cutoff);
	
	if (mask&LIGHT_CONSTANT_ATTENUATION)  glLightf (light->which, GL_CONSTANT_ATTENUATION,  light->constant_attenuation);
	if (mask&LIGHT_LINEAR_ATTENUATION)    glLightf (light->which, GL_LINEAR_ATTENUATION,    light->linear_attenuation);
	if (mask&LIGHT_QUADRATIC_ATTENUATION) glLightf (light->which, GL_QUADRATIC_ATTENUATION, light->quadratic_attenuation);
	
	if (enable==1) glEnable (light->which);
	else if (enable==0) glDisable (light->which);
}
/*--------------------- Materials ------------------------*/

//#define MAT_ALL        (0xffff) 
//#define MAT_FACE       (1<<0)
//#define MAT_AMBIENT    (1<<1)
//#define MAT_SPECULAR   (1<<2)
//#define MAT_EMISSION   (1<<3)
//#define MAT_DIFFUSE    (1<<4)
//#define MAT_SHININESS  (1<<5)

void SetMaterial(Material *mat,int colortoo)
{
	if (!mat) return;
	if (colortoo) glColor3f(mat->diffuse[0],mat->diffuse[1],mat->diffuse[2]);
	if (mat->mask&MAT_AMBIENT)   glMaterialfv(mat->face, GL_AMBIENT, mat->ambient);
	if (mat->mask&MAT_DIFFUSE)   glMaterialfv(mat->face, GL_DIFFUSE, mat->diffuse);
	if (mat->mask&MAT_SPECULAR)  glMaterialfv(mat->face, GL_SPECULAR, mat->specular);
	if (mat->mask&MAT_EMISSION)  glMaterialfv(mat->face, GL_EMISSION, mat->emission);
	if (mat->mask&MAT_SHININESS) glMaterialf (mat->face, GL_SHININESS, mat->shininess);
}

void setmaterial(GLfloat r,GLfloat g,GLfloat b)
{
	glColor3f(r,g,b);

	GLfloat mat[4] = {1.0, 1.0, 1.0, 1.0};
	glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, mat);
	  
	mat[0] = r; mat[1] = g; mat[2] = b;
	glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, mat);
	  
	mat[0] = 0.3; mat[1] = 0.3; mat[2] = 0.3;
	glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat);
	  
	glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 0.3*128.0);

	mat[0] = 0; mat[1] = 0; mat[2] = 0;
	glMaterialfv (GL_FRONT_AND_BACK, GL_EMISSION, mat);
}

//------------------------------ Thing ----------------------------------------

/*! \class Thing
 * \brief Generic class holding a transform for a gl object.
 */
Thing::Thing(const Thing &thing)
{
	setfrom(thing);
}

//*** note that since this is called from Thing constructor, redefined versions will not be called
void Thing::setfrom(const Thing &thing)
{
	id=thing.id;
	int c;
	for (c=0; c<3; c++) {
		scale[c]=thing.scale[c];
		color[c]=thing.color[c];
	}
	color[3]=thing.color[3];
	for (c=0; c<16; c++) m[c]=thing.m[c];
}

Thing &Thing::operator=(Thing &thing)
{
	setfrom(thing);
	return *this;
}

Thing::Thing(GLuint nid)
{ 
	id=nid; 
	mat=NULL; 
	 // load identity in m
	for (int c=0; c<16; c++) m[c]=0;
	m[0]=m[5]=m[10]=m[15]=1.0;

	color[0]=color[1]=color[2]=color[3]=1.0;
	scale[0]=scale[1]=scale[2]=1.0;
}

void Thing::SetScale(GLfloat x,GLfloat y,GLfloat z)
{
	//scale[0]+=x;
	//scale[1]+=y;
	//scale[2]+=z;
	//--------------------
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	//glLoadMatrixf(m);
	//glScalef(x,y,z);
	//----
	glLoadIdentity();
	glScalef(x,y,z);
	glMultMatrixf(m);
	
	glGetFloatv(GL_MODELVIEW_MATRIX,m);
	glPopMatrix();
}

void Thing::SetColor(GLfloat r,GLfloat g,GLfloat b,GLfloat a) //a=1
{
	color[0]=r;
	color[1]=g;
	color[2]=b;
	color[3]=a;
}


//! This rotates in absolute coords. angle in degrees.
void Thing::RotateGlobal(GLfloat angle,GLfloat x,GLfloat y,GLfloat z)
{
	glPushMatrix();
	glLoadIdentity();
	glRotatef(angle,x,y,z);
	GLfloat tx,ty,tz;
	tx=m[12];
	ty=m[13];
	tz=m[14];
	m[12]=m[13]=m[14]=0;
	glMultMatrixf(m);
	glGetFloatv(GL_MODELVIEW_MATRIX,m);
	m[12]=tx;
	m[13]=ty;
	m[14]=tz;
	glPopMatrix();
}

//! Rotate in thing's local coords.
void Thing::RotateLocal(GLfloat angle,GLfloat x,GLfloat y,GLfloat z)
{
	glPushMatrix();
	glLoadMatrixf(m);
	glRotatef(angle,x,y,z);
	glGetFloatv(GL_MODELVIEW_MATRIX,m);
	glPopMatrix();
}

// this translates absolute coordinates, 
void Thing::TranslateGlobal(GLfloat x,GLfloat y,GLfloat z)
{
	m[12] += x;
	m[13] += y;
	m[14] += z;
}

// Translate relative to thing's orientation
void Thing::Translate(GLfloat x,GLfloat y,GLfloat z)
{
	glPushMatrix();
	glLoadMatrixf(m);
	glTranslatef(x,y,z);
	glGetFloatv(GL_MODELVIEW_MATRIX,m);
	glPopMatrix();
}

//! Assumes model view, and can push it
void Thing::SetPos(GLfloat x,GLfloat y,GLfloat z)
{
	m[12]=x;
	m[13]=y;
	m[14]=z;
}

void Thing::Draw()
{
	if (!id) return;
	glPushMatrix();
//	glLoadIdentity();
	//*** load any other initializers to modelview
	
	//glLoadMatrixf(m);
	glMultMatrixf(m);
	//glScalef(scale[0],scale[1],scale[2]);
	glColor4f(color[0],color[1],color[2],color[3]);
	if (mat) SetMaterial(mat,1);
	glCallList(id);
	glPopMatrix();
}

//! Update bas from whatever is in m.
void Thing::updateBasis()
{
	bas.x.x = m[0];
	bas.x.y = m[1];
	bas.x.z = m[2];

	bas.y.x = m[4];
	bas.y.y = m[5];
	bas.y.z = m[6];

	bas.z.x = m[8];
	bas.z.y = m[9];
	bas.z.z = m[10];

	bas.p.x = m[12];
	bas.p.y = m[13];
	bas.p.z = m[14];
}

//doxygen group gl
//@}

} //namespace Polyptych


