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
// Copyright (C) 2011-2012 by Tom Lechner
//


#ifndef POLYRENDER_H
#define POLYRENDER_H


#include <GraphicsMagick/Magick++.h>
#include <lax/transformmath.h>
#include <lax/errorlog.h>

namespace Polyptych {

//------------------------ Enums -------------------------------
enum OutFormat {
	OUT_NONE,
	OUT_LAIDOUT,
	OUT_SVG,
	OUT_CUBE_MAP,
	OUT_QTVR,
	OUT_QTVR_FACES,
	OUT_IMAGE
};

//--------------------------- PaperBound -------------------------------
class PaperBound
{
  public:
	int id;
	char *name;
	char *units;
	double width,height;
	Laxkit::Affine matrix;
	double r,g,b;
	PaperBound();
	PaperBound(const char *nname,double w,double h,const char *unit);
	PaperBound(const PaperBound &p);
	~PaperBound();
	PaperBound &operator=(PaperBound &p);
};

//------------------------ RenderConfig -------------------------------
class RenderConfig : public Laxkit::anObject
{
 public:
	char *spherefile; //corresponding to spheremap source
	Magick::Image spheremap;

	Polyhedron *poly;
	Basis extra_basis;

	Net *net;
	Laxkit::RefPtrStack<Net> nets;

	Laxkit::PtrStack<PaperBound> papers;

	int output;
	int oversample;
	int generate_images;
	
	int maxwidth;
	char *dest_imgfilebase;
	Magick::Image destination; //an equirectangular


	RenderConfig();
	virtual ~RenderConfig();
};


//--------------------------------- SphereMapper ------------------------------------

class SphereMapper
{
  public:
	RenderConfig *config;

	SphereMapper();
	virtual ~SphereMapper();
	virtual void SetRenderConfig(RenderConfig *newconfig);

	int ImageToSphere(Magick::Image image, int sx,int sy,int sw,int sh, double x_fov, double y_fov, 
				 double theta, double gamma, double rotation, 
				 const char *tofile, int oversample, 
				 Laxkit::ErrorLog *log   
				);
	int PolyWireframeToSphere(const char *tofile, 
				 double red,double green,double blue, double alpha, 
				 double edge_width, 
				 Polyhedron &poly,
				 int oversample,      
				 Basis *extra_basis, 
				 Laxkit::ErrorLog *log   
				);
	int RollSphere(double theta, double gamma, double rotation);
};


//--------------------------------- Functions ------------------------------------
int SaveSvgWithImages(Net *net,
					  const char *filename,
					  const char *filebase,
					  flatpoint *imagedims,
					  flatpoint *imageoffset,
					  double pixPerUnit,
				 	  int net_only,
					  int num_papers,
					  PaperBound **papers,
				 	  Laxkit::ErrorLog *log);

int SphereToPoly(const char *spherefile,
				 Polyhedron *poly,
				 Net *net, 
				 int maxwidth,
				 const char *filebase,
				 int output,
				 int oversample,
				 int generate_images,
				 int net_only,
				 Basis *extra_basis,
				 int num_papers,
				 PaperBound **papers,
			 	 Laxkit::ErrorLog *log
				);

int SphereToPoly(Magick::Image spheremap,
				 Polyhedron *poly,
				 Net *net, 
				 int maxwidth,
				 const char *filebase,
				 int output,
				 int oversample,
				 int generate_images,
				 int net_only,
				 Basis *extra_basis,
				 int num_papers,
				 PaperBound **papers,
				 Laxkit::ErrorLog *log
				);

int SphereToCubeMap(Magick::Image spheremap,
				 //spacepoint sphere_z,
				 //spacepoint sphere_x,
				 int defaultimagewidth, //of one side
				 const char *tofile,
				 int AA=2, //how much to oversample
				 const char *which="012345",
				 Laxkit::ErrorLog *log=NULL
				);


} //namespace Polyptych



#endif



