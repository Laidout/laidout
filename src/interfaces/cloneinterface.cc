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
// Copyright (C) 2013 by Tom Lechner
//


// Todo:
//  uniform tilings
//  uniform tiling colorings

//  tiling along a path -- frieze patterns
//  heesh patterns

//  penrose p1: pentagons, pentagrams, rhombus, sliced pentagram
//  penrose p2: kite and dart
//  penrose p3: 2 rhombs


#include "cloneinterface.h"
#include "../laidout.h"
#include "../dataobjects/datafactory.h"
#include "../drawdata.h"
#include "../viewwindow.h"

#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/interfaces/somedataref.h>

#include <lax/lists.cc>
#include <lax/refptrstack.cc>


using namespace Laxkit;
using namespace LaxInterfaces;


#define DBG
#include <iostream>
using namespace std;


namespace Laidout {


//------------------------------------- TileCloneInfo ------------------------------------

/*! \class TileCloneInfo
 * Instance of a clone in a Tiling pattern.
 */


//------------------------------------- TilingDest ------------------------------------

/*! \class TilingDest
 * Describes one destination of a TilingOp.
 */

TilingDest::TilingDest()
{
	cloneid=0; //of containing Tiling
	op_id=0; //of newly placed clone, can be any of uber parent Tiling::basecells
	parent_op_id=0; //of containing TilingOp

	is_progressive=false;

	 //conditions for traversal
	conditions=0; //1 use iterations, 2 use max size, 3 use min size, 4 use scripted
	max_iterations=1; // <0 for endless, use other constraints to control
	max_size=min_size=0;
	traversal_chance=1;
	scripted_condition=NULL;

	 //transform from current space to this destination cell
	scripted_transform=NULL;
}

TilingDest::~TilingDest()
{
	if (scripted_transform) scripted_transform->dec_count();
	delete[] scripted_condition;
}

//------------------------------------- TilingOp ------------------------------------

/*! \class TilingOp
 * Take one shape (described in celloutline), and transform it to other places in a Tiling.
 * The places to take celloutline to are described in a TilingDest.
 */


TilingOp::TilingOp()
{
	celloutline=NULL;
}

TilingOp::~TilingOp()
{
	if (celloutline) celloutline->dec_count();
}

/*! Returns pointer to the newly created and pushed dest.
 */
TilingDest *TilingOp::AddTransform(Affine &transform)
{
	TilingDest *dest=new TilingDest();
	dest->transform=transform;
	transforms.push(dest,1);

	return dest;
}

TilingDest *TilingOp::AddTransform(TilingDest *dest)
{
	transforms.push(dest,1);
	return dest;
}

Laxkit::Affine TilingOp::Transform(int which)
{
	if (which>=0 && which<transforms.n) return transforms.e[which]->transform;
	return Affine();
}

int TilingOp::isRecursive(int which)
{
	if (which>=0 && which<transforms.n) 
		return transforms.e[which]->max_iterations!=1 && transforms.e[which]->max_iterations!=0;
	return 0;
}




//------------------------------------- Tiling ------------------------------------

/*! \class Tiling
 * Repeat any number of base shapes into a pattern, and optionally repeat
 * that pattern linearly in 2 dimensions (a p1 wallpaper).
 *
 * You can define your own custom "dimensions", each of which may get
 * different tiling rules.
 *
 * \todo PtrStack<CellCombineRules> cell_combine_rules; for penrose, or heesch
 */


/*! \var Laxkit::PtrStack<TilingOp> Tiling::basecells
 *  Typically basecells must share same coordinate system,
 *  and must be defined such as they appear laid into the default unit
 */
/*! \var Laxkit::RefPtrStack<Tiling> dimensions;
 *
 * More flexible than built in overal P1 repeating.
 * 1st dimension is simply basecells stack. Additional dimensions are applied on top
 * of each object produced by the 1st dimension.
 */


Tiling::Tiling(const char *nname, const char *ncategory)
{
	required_interface=""; //if a special interface is needed, like for radial or penrose
	repeatable=3;
	default_unit.maxx=default_unit.maxy=1;

	name=newstr(nname);
	category=newstr(ncategory);
	icon=NULL;
}

Tiling::~Tiling()
{
	delete[] name;
	delete[] category;
	if (icon) icon->dec_count();
}

/*! Use laidout->icons.GetIcon() to search for "category__name".
 */
void Tiling::InstallDefaultIcon()
{
	if (icon) icon->dec_count();

	char *ifile=newstr(name);
	if (!isblank(category)) {
		prependstr(ifile,"__");
		prependstr(ifile,category);
	}
	for (unsigned int c=0; c<strlen(ifile); c++) if (ifile[c]==' ') ifile[c]='_';

	icon=laidout->icons.GetIcon(ifile);
	delete[] ifile;
}

/*! Return a new TilingOp with the specified base information.
 * The op is pushed onto *this->basecells.
 * Calling code can then add whatever transforms are relevant.
 */
TilingOp *Tiling::AddBase(PathsData *outline, int absorb_count, int lock_base)
{
	TilingOp *op=new TilingOp();
	op->basecell_is_editable=lock_base;
	op->celloutline=outline;
	if (!absorb_count) outline->inc_count();
	basecells.push(op,1);

	return op;
}

/*! There is some superset of basecells that makes the tiling repeatable in an evenly spaced grid.
 * Returns if it can repeat infinitely in the x axis.
 */
int Tiling::isXRepeatable()
{ return repeatable&1; }


/*! There is some superset of basecells that makes the tiling repeatable in an evenly spaced grid.
 * Returns if it can repeat infinitely in the y axis.
 */
int Tiling::isYRepeatable()
{ return repeatable&2; }


//! Transform applied after tiling to entire pattern, to squish around. Default is transform of default_unit.
Affine Tiling::finalTransform()
{ return Affine(repeat_basis); }



flatpoint Tiling::repeatOrigin()
{ return repeat_basis.origin(); }

flatpoint Tiling::repeatOrigin(flatpoint norigin)
{ repeat_basis.origin(norigin); return norigin; }

flatpoint Tiling::repeatXDir(flatpoint nx)
{ repeat_basis.xaxis(nx); return nx; }

flatpoint Tiling::repeatYDir(flatpoint ny)
{ repeat_basis.yaxis(ny); return ny; }

/*! When isRepeatable(). length is distance to translate, defaults to repeat_basis.xaxis().
 */
flatpoint Tiling::repeatXDir()
{ return repeat_basis.xaxis(); }


/*! When isRepeatable(). length is distance to translate, defaults to repeat_basis.yaxis().
 */
flatpoint Tiling::repeatYDir()
{ return repeat_basis.yaxis(); }




/*! Set default_unit to be sized to hold a hexagon with side_length, long axis vertical.
 * Sets xaxis to point right, yaxis points diagonally to upper right 60 degrees from x.
 */
void Tiling::DefaultHex(double side_length)
{
	repeatable=3;//repeat in x and y
	default_unit.maxy=2*side_length;
	default_unit.maxx=sqrt(3)*side_length;
	repeatYDir(flatpoint(side_length*sqrt(3)/2,side_length*1.5));
	repeatXDir(flatpoint(side_length*sqrt(3),0));
}

void Tiling::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
//***
//	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
//
//    if (what==-1) {
//        fprintf(f,"%sname Blah          #optional human readable name\n",spc);
//	}
//
}

void Tiling::dump_in_atts(LaxFiles::Attribute *att, int, Laxkit::anObject*)
{
//***
//    if (!att) return;
//    char *name,*value;
//    for (int c=0; c<att->attributes.n; c++) {
//        name= att->attributes.e[c]->name;
//        value=att->attributes.e[c]->value;
//
//        if (!strcmp(name,"name")) {
//            makestr(this->object_idstr,value);
//
//        } else if (!strcmp(name,"")) {
//		}
//	}
}

//! Create tiled clones.
/*! Install new objects as kids of parent_space. If NULL, create and return a new Group (else return parent_space).
 *
 * If trace is true, create path objects from the transformed base cells, in addition to cloning base objects in the base cells
 */
Group *Tiling::Render(Group *parent_space,
					   LaxInterfaces::SomeData *base_object_to_update, //!< If non-null, update relevant clones connected to base object
					   bool trace_cells,
					   int p1_minx, int p1_maxx, int p1_miny, int p1_maxy
					   )
{
	if (!parent_space) parent_space=new Group;

	if (p1_maxx<p1_minx) p1_maxx=p1_minx;
	if (p1_maxy<p1_miny) p1_maxy=p1_miny;
	if (!isXRepeatable()) p1_maxx=p1_minx;
	if (!isYRepeatable()) p1_maxy=p1_miny;
	Affine p1;


	SomeData *base=base_object_to_update; // *** need source object list, 1:1 with basecells
	SomeData *trace=NULL;
	if (trace_cells) {
		trace=basecells.e[0]->celloutline;
		if (trace) trace=trace->duplicate(NULL);
	}

	SomeDataRef *clone=NULL;
	TilingDest *dest;
	
	for (int x=p1_minx; x<=p1_maxx; x++) {
	  for (int y=p1_miny; y<=p1_maxy; y++) {
		for (int c=0; c<basecells.n; c++) {
		  for (int c2=0; c2<basecells.e[c]->transforms.n; c2++) {
			dest=basecells.e[c]->transforms.e[c2];

			p1.setIdentity();
			if (x==0 && y==0) {
				p1.origin(repeatOrigin());
			} else {
				p1.origin(repeatOrigin() + x*repeatXDir() + y*repeatYDir());
			}

			// *** need check that clone is within boundary if given

			if (base) {
				clone=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
				clone->Set(base,0);
				clone->Multiply(dest->transform);
				clone->Multiply(p1);
				parent_space->push(clone);
				clone->dec_count();
			}

			if (trace) {
				clone=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
				clone->Set(trace,0);
				clone->Multiply(dest->transform);
				clone->Multiply(p1);
				parent_space->push(clone);
				clone->dec_count();
			}

			if (dest->max_iterations>1) {
				cerr << " *** must implement recursive clone!"<<endl;
			}
		  }
		}
	  }
	}

	if (trace) trace->dec_count();

	return parent_space;
}


//------------------------------------ Tiling Creating Functions --------------------------------

enum TilingConditionTypes {
	TILING_Condition_None=0,
	TILING_Iterations = (1<<0),
	TILING_Max_Size = (1<<1),
	TILING_Min_Size = (1<<2),

	TILING_MAX
};

/*! Create a tiling pattern based on repeating something radially.
 * Creates a base cell that is part of a wedge, which is (end_angle-start_angle)/num_divisions wide.
 * 
 * \todo int inward..  If >0, then repeat with smaller clones toward circle center
 */
Tiling *CreateRadial(double start_angle, //!< radians
					 double end_angle,   //!< If end==start, or end==start+360, use full circle
					 double start_radius,//!< For purposes of defining a base cell outline
					 double end_radius,  //!< For purposes of defining a base cell outline
					 int num_divisions,  //!< divide total angle into this many sections. base+mirrored=1 section
					 int mirrored)       //!< Each repeated unit is the base plus a mirror of the base about a radius
{
	if (end_angle==start_angle) end_angle=start_angle+2*M_PI;
	double rotation_angle=(end_angle-start_angle)/num_divisions;

	Tiling *tiling=new Tiling(NULL,"Circular");
	tiling->repeatable=0;
	tiling->required_interface="radial";
	if (mirrored) makestr(tiling->name,"rm"); else makestr(tiling->name,"r");


	 //define cell outline
	PathsData *path=new PathsData;
	double a =start_angle;
	double a2=a+rotation_angle / (mirrored?2:1);
	path->append(start_radius*flatpoint(cos(a),sin(a)));
	path->append(  end_radius*flatpoint(cos(a),sin(a)));

	path->appendBezArc(flatpoint(0,0), rotation_angle, 1);
	path->append(start_radius*flatpoint(cos(a2),sin(a2)));

	path->appendBezArc(flatpoint(0,0), -rotation_angle, 1);
	path->close();


	 //set up a recursive transform limited by num_divisions
	TilingDest *dest=new TilingDest;
	dest->max_iterations=num_divisions;
	dest->conditions=TILING_Iterations;
	dest->transform.setRotation(rotation_angle);

	TilingOp *op=tiling->AddBase(path,1,1);
	op->AddTransform(dest);

	if (mirrored) {
		dest=new TilingDest;
		dest->max_iterations=num_divisions;
		dest->conditions=TILING_Iterations;
		dest->transform.Flip(flatpoint(0,0),
				   flatpoint(cos(start_angle+rotation_angle*1.5),sin(start_angle+rotation_angle*1.5)));
		op->AddTransform(dest);
	}



	 //icon key "Circular__r" or "Circular__rm"
	tiling->InstallDefaultIcon();


	return tiling;
}

/*! Return CreateRadial(0,0,0,1,5,mirrored).
 */
Tiling *CreateRadialSimple(int mirrored, int num_divisions)
{
	return CreateRadial(0,0,0,1,num_divisions,mirrored);
}

Tiling *CreateUniformColoring(const char *coloring, LaxInterfaces::SomeData *centered_on)
{
	Tiling *tiling=CreateWallpaper("p1",centered_on);
	makestr(tiling->category,"Uniform Coloring");
	makestr(tiling->name,coloring);
	tiling->InstallDefaultIcon();

	cerr << " *** HEY!!! FINISH IMPLEMENTING ME!!!!!!"<<endl;

	return tiling;
}


/*! Create a tiling based on a wallpaper group.
 * If centered_on!=NULL, then map the base cell to coincide with the bounding box of it.
 *
 * Note the definitions below are in some cases very brute force. It defines each transform
 * necessary to create an overall p1 cell.
 * It COULD be simplified by using iterated operations.
 */
Tiling *CreateWallpaper(const char *group, LaxInterfaces::SomeData *centered_on)
{
	if (isblank(group)) group="p1";
	

	Tiling *tiling=new Tiling(group,"Wallpaper");
	tiling->repeatable=3;

	tiling->default_unit.maxx=1;
	tiling->default_unit.maxy=1;

	PathsData *path;
	TilingOp *op;
	Affine affine;

	if (!strcasecmp(group,"p1")) {
		 //define single base cell, covers whole unit cell
		path=new PathsData;
		path->appendRect(0,0,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"p2")) {
		 //rotate on edge
		path=new PathsData;
		path->appendRect(0,0,.5,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Rotate(M_PI,flatpoint(.5,.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"pm")) {
		 //flip on edge
		path=new PathsData;
		path->appendRect(0,0,.5,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Flip(flatpoint(.5,0),flatpoint(.5,1));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"pg")) {
		 //glide reflect
		path=new PathsData;
		path->appendRect(0,0,.5,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Translate(flatpoint(.5,0));
		affine.Flip(flatpoint(0,.5),flatpoint(1,.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"cm")) {
		 //reflect vertically, glide reflect horizontally
		path=new PathsData;
		path->appendRect(0,0,.5,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Flip(flatpoint(0,.5),flatpoint(1,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Translate(flatpoint(.5,0));
		affine.Flip(flatpoint(0,.25),flatpoint(1,.25));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Translate(flatpoint(.5,.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"pmm")) {
		 //reflect vertically, reflect horizontally
		path=new PathsData;
		path->appendRect(0,0,.5,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Flip(flatpoint(0,.5),flatpoint(1,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(.5,0),flatpoint(.5,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(0,.5),flatpoint(1,.5));
		affine.Flip(flatpoint(.5,0),flatpoint(.5,1));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"pmg")) {
		 //flip vertically, rotate on right edge
		path=new PathsData;
		path->appendRect(0,0,.5,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Flip(flatpoint(0,.5),flatpoint(1,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI,flatpoint(.5,.25));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI,flatpoint(.5,.25));
		affine.Flip(flatpoint(0,.5),flatpoint(1,.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"pgg")) {
		 //rotate on top edge, glide reflect horizontally
		path=new PathsData;
		path->appendRect(0,0,.5,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Rotate(M_PI,flatpoint(.25,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Translate(flatpoint(.5,0));
		affine.Flip(flatpoint(0,.25),flatpoint(1,.25));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Translate(flatpoint(0,.5));
		affine.Flip(flatpoint(.5,0),flatpoint(.5,1));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"p4")) {
		 //rotate about center 3 times
		path=new PathsData;
		path->appendRect(0,0,.5,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Rotate(M_PI/2,flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI,flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI/2,flatpoint(.5,.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"cmm")) {
		 //8 components:
		 //reflect on right edge, then glide reflect those two
		 //reflect on top edge, with that one, reflect on right, glide reflect those top two
		path=new PathsData;
		path->appendRect(0,0,.25,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1); //0,0

		op->AddTransform(affine);

		affine.Flip(flatpoint(.25,0),flatpoint(.25,1)); //1,0
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Translate(flatpoint(.5,0));
		affine.Flip(flatpoint(0,.25),flatpoint(1,.25)); //2,0
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI,flatpoint(.5,.25)); //3,0
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(0,.5),flatpoint(1,.5)); //0,1
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI, flatpoint(.25,.5)); //1,1
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Translate(flatpoint(.5,.5)); //2,1
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(.5,0), flatpoint(.5,1));
		affine.Translate(flatpoint(0,.5)); //3,1
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"p4m")) {
		 //8 right isosceles triangles with hypotenuse at square center, reflected 
		 //all around center. is equivalent to flip once on hypotenuse, then 
		 //rotate those 2 around
		path=new PathsData;
		path->append(0,0);
		path->append(.5,0);
		path->append(.5,.5);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Flip(flatpoint(0,0), flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(0,0), flatpoint(.5,.5));
		affine.Rotate(M_PI/2, flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(0,0), flatpoint(.5,.5));
		affine.Rotate(M_PI, flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(0,0), flatpoint(.5,.5));
		affine.Rotate(-M_PI/2, flatpoint(.5,.5));
		op->AddTransform(affine);


		affine.setIdentity();
		affine.Rotate(M_PI/2, flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI, flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI/2, flatpoint(.5,.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"p4g")) {
		 //8 right isosceles triangles with hypotenuse NOT at square center.
		 //is equivalent to flip once on hypotenuse, then 
		 //rotate those 2 around
		path=new PathsData;
		path->append(0,0);
		path->append(.5,0);
		path->append(0,.5);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Flip(flatpoint(.5,0), flatpoint(0,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(.5,0), flatpoint(0,.5));
		affine.Rotate(M_PI/2, flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(.5,0), flatpoint(0,.5));
		affine.Rotate(M_PI, flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(.5,0), flatpoint(0,.5));
		affine.Rotate(-M_PI/2, flatpoint(.5,.5));
		op->AddTransform(affine);


		affine.setIdentity();
		affine.Rotate(M_PI/2, flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI, flatpoint(.5,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI/2, flatpoint(.5,.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"p3")) {
		 //rotate by 120 deg twice

		 //Remaining hex based ones are in box with width sqrt(3), height 2
		 //x horizontal, y 60 degrees off x
		tiling->DefaultHex(1);

		path=new PathsData;
		path->append(sqrt(3)/2,0);
		path->append(sqrt(3)/2,1);
		path->append(0,1.5);
		path->append(0,.5);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Rotate(M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);


	} else if (!strcasecmp(group,"p3m1")) {
		 //6 triangle hexagon, flip each around line through center
		tiling->DefaultHex(1);

		path=new PathsData;
		path->append(sqrt(3)/2,0);
		path->append(sqrt(3)/2,1);
		path->append(0,.5);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Rotate(M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(sqrt(3)/2,1),flatpoint(0,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(sqrt(3)/2,1),flatpoint(0,.5));
		affine.Rotate(M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Flip(flatpoint(sqrt(3)/2,1),flatpoint(0,.5));
		affine.Rotate(-M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"p31m")) {
		 //18 items, divide hexagon to 6 triangles, divide those into 3
		 //with triangle centers, which are rotation centers. Each cluster
		 //of 3 is flipped around as in p3m1
		tiling->DefaultHex(1);

		path=new PathsData;
		path->append(0,.5);
		path->append(1./2/sqrt(3),1);
		path->append(0,1.5);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		 //cluster 1 (with base)
		affine.setIdentity();
		affine.Rotate(M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		op->AddTransform(affine);

		 //cluster 2
		affine.setIdentity();
		affine.Flip(flatpoint(sqrt(3)/2,1),flatpoint(0,1.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		affine.Flip(flatpoint(sqrt(3)/2,1),flatpoint(0,1.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		affine.Flip(flatpoint(sqrt(3)/2,1),flatpoint(0,1.5));
		op->AddTransform(affine);

		 //cluster 3
		affine.setIdentity();
		affine.Rotate(M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		affine.Rotate(M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		affine.Rotate(M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		 //cluster 4
		affine.setIdentity();
		affine.Flip(flatpoint(sqrt(3)/2,0),flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		affine.Flip(flatpoint(sqrt(3)/2,0),flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		affine.Flip(flatpoint(sqrt(3)/2,0),flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		 //cluster 5
		affine.setIdentity();
		affine.Rotate(-M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		affine.Rotate(-M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		affine.Rotate(-M_PI*2/3, flatpoint(sqrt(3)/2,1));
		op->AddTransform(affine);

		 //cluster 6
		affine.setIdentity();
		affine.Flip(flatpoint(sqrt(3)/2,1),flatpoint(0,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		affine.Flip(flatpoint(sqrt(3)/2,1),flatpoint(0,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		affine.Flip(flatpoint(sqrt(3)/2,1),flatpoint(0,.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"p6")) {
		 //18 items, divide hexagon to 6 triangles, divide those into 3
		 //with triangle centers, which are rotation centers. Each cluster
		 //of 3 is rotated (not flipped) around hexagon center
		tiling->DefaultHex(1);

		path=new PathsData;
		path->append(0,.5);
		path->append(1./2/sqrt(3),1);
		path->append(0,1.5);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		 //cluster 1 (with base)
		affine.setIdentity();
		affine.Rotate(M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(-M_PI*2/3, flatpoint(1./2/sqrt(3),1));
		op->AddTransform(affine);

		for (int c=1; c<6; c++) {
			affine.setIdentity();
			affine.Rotate(c*M_PI/3, flatpoint(sqrt(3)/2,1));
			op->AddTransform(affine);

			affine.setIdentity();
			affine.Rotate(M_PI*2/3, flatpoint(1./2/sqrt(3),1));
			affine.Rotate(c*M_PI/3, flatpoint(sqrt(3)/2,1));
			op->AddTransform(affine);

			affine.setIdentity();
			affine.Rotate(-M_PI*2/3, flatpoint(1./2/sqrt(3),1));
			affine.Rotate(c*M_PI/3, flatpoint(sqrt(3)/2,1));
			op->AddTransform(affine);
		}

	} else if (!strcasecmp(group,"p6m")) {
		 //hexagon in 12 pieces, right 30-60-90 triangles, each flipped
		 //around egdges that touch the center
		tiling->DefaultHex(1);

		path=new PathsData;
		path->append(0,.5);
		path->append(sqrt(3)/2,1);
		path->append(0,1);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Flip(flatpoint(0,1),flatpoint(sqrt(3),1));
		op->AddTransform(affine);

		for (int c=1; c<6; c++) {
			affine.setIdentity();
			affine.Rotate(c*M_PI/3, flatpoint(sqrt(3)/2,1));
			op->AddTransform(affine);

			affine.setIdentity();
			affine.Flip(flatpoint(0,1),flatpoint(sqrt(3),1));
			affine.Rotate(c*M_PI/3, flatpoint(sqrt(3)/2,1));
			op->AddTransform(affine);
		}

	} else {
		tiling->dec_count();
		return NULL;
	}

	//if (centered_on) tiling->BaseObject(centered_on);

	 //find a matching icon...
	tiling->InstallDefaultIcon();

	return tiling;
}

//! Frieze groups. Basically some wallpaper groups, but only one dimensional expansion.
Tiling *CreateFrieze(const char *group, LaxInterfaces::SomeData *centered_on)
{
	const char *wall=NULL;
	if (!strcasecmp(group,"11")) wall="p1";
	else if (!strcasecmp(group,"1g")) wall="pg";
	else if (!strcasecmp(group,"m1")) wall="pm"; //in x dir
	else if (!strcasecmp(group,"12")) wall="p2";
	else if (!strcasecmp(group,"mg")) wall="pmg"; //in y
	else if (!strcasecmp(group,"1m")) wall="pm"; //in y
	else if (!strcasecmp(group,"mm")) wall="pmm";
	if (wall==NULL) return NULL;

	Tiling *tiling=CreateWallpaper(wall,centered_on);

	tiling->repeatable=1;
	makestr(tiling->name,group);
	makestr(tiling->category,"Frieze");
	tiling->InstallDefaultIcon();

	if (!strcasecmp(group,"mg") || !strcasecmp(group,"1m")) {
		 //uses the wallpaper group, but in y, not in x.
		tiling->repeatable=2;
		tiling->repeatXDir(flatpoint(0,-1));
		tiling->repeatYDir(flatpoint(1,0));
	}

	return tiling;
}
	

/*! Hardcoded built in tiling types.
 */
const char *BuiltinTiling[]=
		  {
			"Wallpaper/p1",
			"Wallpaper/p2",
			"Wallpaper/pm",
			"Wallpaper/pg",
			"Wallpaper/cm",
			"Wallpaper/pmm",
			"Wallpaper/pmg",
			"Wallpaper/pgg",
			"Wallpaper/cmm",
			"Wallpaper/p4",
			"Wallpaper/p4m",
			"Wallpaper/p4g",
			"Wallpaper/p3",
			"Wallpaper/p3m1",
			"Wallpaper/p31m",
			"Wallpaper/p6",
			"Wallpaper/p6m",

		    "Circular/r",
		    "Circular/rm",

			"Frieze/11",
			"Frieze/1g",
			"Frieze/m1",
			"Frieze/12",
			"Frieze/mg",
			"Frieze/1m",
			"Frieze/mm",

			"Uniform Coloring/elongated triangular",
			"Uniform Coloring/hexagonal 1",
			"Uniform Coloring/hexagonal 2",
			"Uniform Coloring/hexagonal 3",
			"Uniform Coloring/rhombi trihexagonal",
			"Uniform Coloring/snub hexagonal",
			"Uniform Coloring/snub square 1",
			"Uniform Coloring/snub square 2",
			"Uniform Coloring/square 1",
			"Uniform Coloring/square 2",
			"Uniform Coloring/square 3",
			"Uniform Coloring/square 4",
			"Uniform Coloring/square 5",
			"Uniform Coloring/square 6",
			"Uniform Coloring/square 7",
			"Uniform Coloring/square 8",
			"Uniform Coloring/square 9",
			"Uniform Coloring/triangular 1",
			"Uniform Coloring/triangular 2",
			"Uniform Coloring/triangular 3",
			"Uniform Coloring/triangular 4",
			"Uniform Coloring/triangular 5",
			"Uniform Coloring/triangular 6",
			"Uniform Coloring/triangular 7",
			"Uniform Coloring/triangular 8",
			"Uniform Coloring/triangular 9",
			"Uniform Coloring/trihexagonal 1",
			"Uniform Coloring/trihexagonal 2",
			"Uniform Coloring/truncated hexagonal",
			"Uniform Coloring/truncated square 1",
			"Uniform Coloring/truncated square 2",
			"Uniform Coloring/truncated trihexagonal",
		  NULL
		};

/*! Returns number of individual built in tilings.
 */
int NumBuiltinTilings()
{
	const char **s=BuiltinTiling;
	int num_tilings=0;
	while (*s) { num_tilings++; s++; }

	return num_tilings;
}

/*! Return a new char[] with something like "Wallpaper__p3" or "Uniform_Tiling__hexagonal_1".
 * Updates num to be actual num in BuiltinTiling array.
 */
char *GetBuiltinIconKey(int &num)
{
	int max=NumBuiltinTilings();
	if (num>=max || num<0) num=0;

	char *tile=replaceall(BuiltinTiling[num],"/","__");
	for (unsigned int c=0; c<strlen(tile); c++) if (tile[c]==' ') *tile='_';

	return tile;
}

Tiling *GetBuiltinTiling(int num, LaxInterfaces::SomeData *center_on)
{
	const char *tile=BuiltinTiling[num];

	if (strstr(tile,"Wallpaper")) return CreateWallpaper(tile+10, center_on);

	if (!strcmp(tile,"Circular/r"))  return CreateRadialSimple(0, 10);
	if (!strcmp(tile,"Circular/rm"))  return CreateRadialSimple(1, 5);

	if (strstr(tile,"Frieze"))    return CreateFrieze(tile+7, center_on);
	//if (strstr(tile,"Uniform"))   return CreateUniformColoring(tile+17, center_on);

	return NULL;
}



//------------------------------------- CloneInterface --------------------------------------


/*! \class CloneInterface
 *
 * Interface to define and maniplate sets of symmetric and recursive clone tiles.
 */



#define INTERFACE_CIRCLE  20

enum CloneInterfaceElements {
	CLONEI_None=0,
	CLONEI_Circle,
	CLONEI_Box,
	CLONEI_Tiling,
	CLONEI_Tiling_Label,

	CLONEI_MAX
};

enum TilingShortcutActions {
	CLONEIA_None=0,
	CLONEIA_Next_Tiling,
	CLONEIA_Previous_Tiling,
	CLONEIA_Toggle_Lines,

	CLONEIA_MAX
};


CloneInterface::CloneInterface(anInterface *nowner,int nid,Laxkit::Displayer *ndp)
  : anInterface(nowner,nid,ndp) 
{
	cloner_style=0;
	lastover=CLONEI_None;
	active=0;
	previewactive=1;


	sc=NULL;

	firsttime=1;
	uiscale=1;
	bg_color =rgbcolorf(.9,.9,.9);
	hbg_color=rgbcolorf(1.,1.,1.);
	fg_color =rgbcolorf(.1,.1,.1);
	activate_color=rgbcolorf(0.,.783,0.);
	deactivate_color=rgbcolorf(1.,.392,.392);

	tiling=NULL;
	trace_cells=1; // *** maybe 2 should be render outline AND install as new objects in doc?
	baseobj=NULL;
	source_objs=NULL; //a pool of objects to select from, rather than clone
					 //any needed beyond those is source_objs are then cloned
					 //from same list in order

	cur_tiling=-1;
	PerformAction(CLONEIA_Next_Tiling);
}

CloneInterface::~CloneInterface()
{
	if (baseobj) baseobj->dec_count();
	if (source_objs) source_objs->dec_count();
	if (sc) sc->dec_count();
	if (tiling) tiling->dec_count();
}

const char *CloneInterface::Name()
{
	return _("Cloner");
}


anInterface *CloneInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new CloneInterface(NULL,id,NULL);
	else if (!dynamic_cast<CloneInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}

void CloneInterface::Clear(LaxInterfaces::SomeData *d)
{ // ***
}

Laxkit::ShortcutHandler *CloneInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("CloneInterface");
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler("CloneInterface");

	sc->Add(CLONEIA_Next_Tiling,     LAX_Left,0,0,      "NextTiling",     _("Select next tiling"),    NULL,0);
	sc->Add(CLONEIA_Previous_Tiling, LAX_Right,0,0,     "PreviousTiling", _("Select previous tiling"),NULL,0);
	sc->Add(CLONEIA_Toggle_Lines,    'c',0,0,           "ToggleLines",    _("Toggle previewing cell lines"),NULL,0);

	//sc->Add(PAPERI_Decorations, 'd',0,0,         "Decs",    _("Toggle decorations"),NULL,0);
	//sc->Add(PAPERI_Delete,      LAX_Bksp,0,0,    "Delete",  _("Delete selected"),NULL,0);
	//sc->AddShortcut(LAX_Del,0,0, PAPERI_Delete);
	//sc->Add(PAPERI_Rectify,     'o',0,0,         "Rectify", _("Make the axes horizontal and vertical"),NULL,0);
	//sc->Add(PAPERI_Rotate,      'r',0,0,         "Rotate",  _("Rotate selected by 90 degrees"),NULL,0);
	//sc->Add(PAPERI_RotateCC,    'R',ShiftMask,0, "RotateCC",_("Rotate selected by 90 degrees in the other direction"),NULL,0);


	manager->AddArea("CloneInterface",sc);
	return sc;
}

int CloneInterface::PerformAction(int action)
{
	if (action==CLONEIA_Next_Tiling) {
		Tiling *newtiling=NULL;
		int maxtiling=NumBuiltinTilings();
		do {
			cur_tiling++;
			if (cur_tiling>=maxtiling) cur_tiling=0;
			newtiling=GetBuiltinTiling(cur_tiling, NULL);
		} while (!newtiling);

		if (tiling) tiling->dec_count();
		tiling=newtiling;

		if (active) Render();

		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Previous_Tiling) {
		Tiling *newtiling=NULL;
		int maxtiling=NumBuiltinTilings();
		do {
			cur_tiling--;
			if (cur_tiling<0) cur_tiling=maxtiling-1;
			newtiling=GetBuiltinTiling(cur_tiling, NULL);
		} while (!newtiling);

		if (tiling) tiling->dec_count();
		tiling=newtiling;

		if (active) Render();

		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Toggle_Lines) {
		trace_cells=!trace_cells;
		if (active) Render();
		PostMessage(trace_cells?_("Trace cell outlines"):_("Don't trace cells"));
		needtodraw=1;
		return 0;
	}

	return 1;
}

int CloneInterface::InterfaceOn()
{
	if (dynamic_cast<LaidoutViewport*>(viewport)) {
		LaidoutViewport *vp=dynamic_cast<LaidoutViewport*>(viewport);

		if (vp->curobj.obj) {
			if (baseobj) baseobj->dec_count();
			baseobj=vp->curobj.obj;
			baseobj->inc_count();
			Render();
		}
	}

	needtodraw=1;
	return 0;
}

int CloneInterface::InterfaceOff()
{
	needtodraw=1;
	return 0;
}


int CloneInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	double circle_radius=INTERFACE_CIRCLE*uiscale;

	if (firsttime) {
		firsttime=0;
		box.minx=100;
		box.maxx=100+4*circle_radius;
		box.miny=100;
		box.maxy=100+5*circle_radius + 2*dp->textheight();
	}



	 //draw clones
	if (preview.n() && active) {
		for (int c=0; c<preview.n(); c++) {
			Laidout::DrawData(dp, preview.e(c), NULL,NULL);
		}
	}


	dp->DrawScreen();

	 //--------draw control box------

	 //draw whole rect outline
	dp->LineAttributes(1,LineSolid,CapButt,JoinMiter);
	dp->NewBG(bg_color);
	dp->NewFG(fg_color);
	dp->drawrectangle(box.minx,box.miny, box.maxx-box.minx,box.maxy-box.miny, 2);
	if (lastover==CLONEI_Tiling) {
		double pad=(box.maxx-box.minx)*.1;
		dp->NewFG(hbg_color);
		dp->drawrectangle(box.minx+pad,box.miny+pad, box.maxx-box.minx-2*pad,box.maxy-box.miny-2*pad, 1);
	}


	 //draw circle
	flatpoint cc((box.minx+box.maxx)/2,box.maxy);
	if (active) dp->NewFG(activate_color); else dp->NewFG(deactivate_color);
	dp->LineAttributes(3,LineSolid, CapButt, JoinMiter);
	if (lastover==CLONEI_Circle) dp->NewBG(hbg_color); else dp->NewBG(bg_color);
	dp->drawellipse(cc.x,cc.y,
					circle_radius,circle_radius,
					0,2*M_PI,
					2);

	
	if (tiling) {
		 //draw icon
		double pad=(box.maxx-box.minx)*.1;
		if (tiling->icon) {
			double boxw=box.maxx-box.minx-2*pad;
			double w=boxw;
			double h=w*tiling->icon->h()/tiling->icon->w();
			if (h>w) {
				h=boxw;
				w=h*tiling->icon->w()/tiling->icon->h();
			}
			flatpoint offset=flatpoint(boxw/2-w/2, boxw/2-h/2);
			dp->imageout(tiling->icon, offset.x+box.minx+pad,offset.y+box.miny+pad, w,h);
		}

		 //draw tiling label
		if (tiling->name) {
			dp->NewFG(fg_color);
			dp->textout((box.minx+box.maxx)/2,box.maxy-circle_radius-pad, tiling->name,-1, LAX_HCENTER|LAX_BOTTOM);
			if (tiling->category) 
				dp->textout((box.minx+box.maxx)/2,box.maxy-circle_radius-pad-dp->textheight(),
						tiling->category,-1, LAX_HCENTER|LAX_BOTTOM);
		}
	}


	//--------draw clones-----------

//	 //draw default unit bounds and tiling region bounds
//	***
//
//	 //draw base cells
//	***
//
//	 //draw selected base objects
//	*** 


	dp->DrawReal();

	return 0;
}

int CloneInterface::scan(int x,int y)
{
	flatpoint cc((box.minx+box.maxx)/2,box.maxy);
	double circle_radius=INTERFACE_CIRCLE*uiscale;

	if (norm((cc-flatpoint(x,y)))<circle_radius) return CLONEI_Circle;

	if (box.boxcontains(x,y)) {
		double pad=(box.maxx-box.minx)*.15;
		if (x>box.minx+pad && x<box.maxx-pad && y>box.miny+pad && y<box.maxy-pad)
			return CLONEI_Tiling;
		return CLONEI_Box;
	}

	return CLONEI_None;
}

int CloneInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;

	int over=scan(x,y);
	if (over==CLONEI_None) return 1;

	buttondown.down(d->id,LEFTBUTTON,x,y, over,state);

	return 0;
}

int CloneInterface::Render()
{
	if (!tiling) return 1;

	preview.flush();
	//Group *ret=tiling->Render(&preview, baseobj, true, 0,3, 0,3);
	Group *ret=tiling->Render(&preview, baseobj, trace_cells, 0,3, 0,3);
	if (!ret) {
		PostMessage(_("Could not clone!"));
	}

	needtodraw=1;
	return 0;
}

/*! Returns whether active after toggling.
 */
int CloneInterface::ToggleActivated()
{
	if (!tiling) { active=0; return 0; }

	active=!active;
	if (active) Render();
	
	needtodraw=1;
	return active;
}

int CloneInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int firstover=CLONEI_None;
	//int dragged=
	buttondown.up(d->id,LEFTBUTTON, &firstover);

	int over=scan(x,y);

	//DBG flatpoint fp=dp->screentoreal(x,y);

	if (over==firstover) {
		if (over==CLONEI_Circle) {
			ToggleActivated();
			needtodraw=1;
			return 0;
		}
	}

	return 0;
}

int CloneInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	int over=scan(x,y);
	DBG cerr <<"over box: "<<over<<endl;

	if (!buttondown.any()) {
		if (lastover!=over) needtodraw=1;
		lastover=over;
		if (lastover==CLONEI_None) PostMessage(" ");
		return 0;
	}

	 //button is down on something...
	int lx,ly, oldover=CLONEI_None;
	buttondown.move(mouse->id,x,y, &lx,&ly);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &oldover);

	 //hijack others to box on dragging
	buttondown.moveinfo(mouse->id,LEFTBUTTON, CLONEI_Box);
	oldover=lastover=CLONEI_Box;
	if (oldover==CLONEI_Box) {
		 //move box
		int offx=x-lx;
		int offy=y-ly;
		box.minx+=offx;
		box.maxx+=offx;
		box.miny+=offy;
		box.maxy+=offy;
		needtodraw=1;
		return 0;
	}

	return 0;
}

int CloneInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int over=scan(x,y);
	DBG cerr <<"wheel up clone interface: "<<over<<endl;

	if (over==CLONEI_Tiling) {
		PerformAction(CLONEIA_Previous_Tiling);
		return 0;
	}
	return 1;
}

int CloneInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int over=scan(x,y);
	DBG cerr <<"wheel down clone interface: "<<over<<endl;

	if (over==CLONEI_Tiling) {
		PerformAction(CLONEIA_Next_Tiling);
		return 0;
	}
	return 1;
}

int CloneInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;
	
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1;
}



} //namespace Laidout

