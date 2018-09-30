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

#ifndef GLBASE_H
#define GLBASE_H


//#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <FTGL/ftgl.h>
//#include <unistd.h>
//#include <cstdlib>
//#include <cstdio>
//#include <cmath>

#include <lax/vectors.h>

#ifndef NULL
#define NULL 0
#endif


namespace Polyptych {


//-------------------------- Lower level gl utils -----------------------------

void dumpMatrix4(GLfloat *m,const char *str);

void addGLSphereTexpt(float x,float y,float z, Basis *extra_basis);
void makecylinder(void);
void drawCylinder(spacepoint p1, spacepoint p2, double scalew=-1,GLfloat *extram=NULL);
void makesphere(void);
GLuint sphereCallList();
void makecube(void);
GLuint cubeCallList();

void drawaxes(double scale, GLfloat *matrix=NULL);


//------------------------- FTGL helper functions ----------------------------
FTFont *setupfont(const char *fontfile, double facesize);


//--------------------- GlData ------------------------
class GlData
{
 public:
	GLfloat matrix[16];

	GlData();
	virtual ~GlData() {}
	virtual int isDrawable()=0;

};

/*--------------------- Light ------------------------*/

#define LIGHT_ALL                    (0xffff)
#define LIGHT_ALL_DIRECTIONAL        (1+2+4+8+128+256+512)
#define LIGHT_POSITION               (1<<0)
#define LIGHT_AMBIENT                (1<<1)
#define LIGHT_DIFFUSE                (1<<2)
#define LIGHT_SPECULAR               (1<<3)
#define LIGHT_SPOT_DIRECTION         (1<<4)
#define LIGHT_SPOT_EXPONENT          (1<<5)
#define LIGHT_SPOT_CUTOFF            (1<<6)
#define LIGHT_CONSTANT_ATTENUATION   (1<<7)
#define LIGHT_LINEAR_ATTENUATION     (1<<8)
#define LIGHT_QUADRATIC_ATTENUATION  (1<<9)
class Light 
{
 public:
	GLenum which; //GL_LIGHT0, ...
	unsigned int mask;
	GLfloat position[4]; // (x,y,z,w) w==0 means directional, 1 means position/spot
	GLfloat ambient[4];
	GLfloat specular[4];
	GLfloat diffuse[4];

	GLfloat spot_direction[4];
	GLfloat spot_exponent;
	GLfloat spot_cutoff; //[0,90] or 180
	GLfloat constant_attenuation;
	GLfloat linear_attenuation;
	GLfloat quadratic_attenuation;

	void Position(GLfloat x,GLfloat y,GLfloat z,GLfloat w) { position[0]=x; position[1]=y; position[2]=z; position[3]=w; }
	void Position(GLfloat *v) { position[0]=v[0]; position[1]=v[1]; position[2]=v[2]; position[3]=v[3]; }
	void Ambient(GLfloat x,GLfloat y,GLfloat z,GLfloat w) { ambient[0]=x; ambient[1]=y; ambient[2]=z; ambient[3]=w; }
	void Diffuse(GLfloat x,GLfloat y,GLfloat z,GLfloat w) { diffuse[0]=x; diffuse[1]=y; diffuse[2]=z; diffuse[3]=w; }
	void Specular(GLfloat x,GLfloat y,GLfloat z,GLfloat w) { specular[0]=x; specular[1]=y; specular[2]=z; specular[3]=w; }

	void Spots(GLfloat dirx,GLfloat diry,GLfloat dirz, GLfloat exp, GLfloat cutoff, GLfloat c, GLfloat l, GLfloat q);
};

void SetLight(Light *light,unsigned int mask,int enable);


/*--------------------- Material ------------------------*/

#define MAT_ALL        (0xffff) 
#define MAT_FACE       (1<<0)
#define MAT_AMBIENT    (1<<1)
#define MAT_SPECULAR   (1<<2)
#define MAT_EMISSION   (1<<3)
#define MAT_DIFFUSE    (1<<4)
#define MAT_SHININESS  (1<<5)
class Material
{
 public:
	unsigned int mask;
	
	GLenum face; // GL_FRONT/GL_BACK/GL_FRONT_AND_BACK
	
	GLfloat ambient[4];
	GLfloat diffuse[4];
	GLfloat specular[4];
	GLfloat emission[4];

	GLfloat shininess; // [0,128]
};

void SetMaterial(Material *mat,int colortoo);
void setmaterial(GLfloat r,GLfloat g,GLfloat b);


//-------------------- eyetype, from Displayer3d -------------------------
class EyeType
{
 public:
	Basis m,l,r; /* get x/y coords, z is away from focus */
	double left_fov, right_fov, middle_fov;
	spacepoint focus;
	double dist,fplane; /* dist=between eyes, flpane dist p to plane */
	GLfloat projection[16],model[16]; // gl transform matrix for projection view

	EyeType();
	void reset();
	void makestereo();
	void transformForDrawing();
	void transformTo();
	void Set(spacepoint atpoint, spacepoint nfocus, spacepoint up);
	void Rotate(double angle, char axis);
};

//--------------------- Thing ------------------------

class Thing
{
  protected:
	void virtual setfrom(const Thing &thing);

  public:
	GLuint id; // the calllist
	GLfloat scale[3]; // any additional scaling
	GLfloat color[4]; // any additional color to set before calling list(?)
	Material *mat; 
	GLfloat m[16]; // transform matrix to set before calling list
	Basis bas;
	//textures
	//animation, modified==1 triggers remap call list?

	Thing(GLuint nid=0);
	Thing(const Thing &thing);
	Thing &operator=(Thing &thing);
	virtual ~Thing() {}
	virtual void SetPos(GLfloat x,GLfloat y,GLfloat z);
	virtual void SetScale(GLfloat x,GLfloat y,GLfloat z);
	virtual void SetColor(GLfloat r,GLfloat g,GLfloat b,GLfloat a=1.0);
	virtual void Translate(GLfloat x,GLfloat y,GLfloat z);
	virtual void TranslateGlobal(GLfloat x,GLfloat y,GLfloat z);
	virtual void RotateLocal(GLfloat angle,GLfloat x,GLfloat y,GLfloat z);
	virtual void RotateGlobal(GLfloat angle,GLfloat x,GLfloat y,GLfloat z);
	virtual void updateBasis();
	virtual void Draw();
};

	
} //namespace Polyptych

	
#endif

