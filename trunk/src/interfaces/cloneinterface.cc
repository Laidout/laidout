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
// Copyright (C) 2013-2014 by Tom Lechner
//


// Todo:

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
#include "../version.h"

#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/filedialog.h>
#include <lax/interfaces/somedataref.h>
#include <lax/interfaces/rectinterface.h>

#include <lax/lists.cc>
#include <lax/refptrstack.cc>


using namespace Laxkit;
using namespace LaxFiles;
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
	traceable=true;

	 //conditions for traversal
	conditions=0; //1 use iterations, 2 use max size, 3 use min size, 4 use scripted
	max_iterations=1; // <0 for endless, use other constraints to control
	recurse_objects=0; // 0 for dest only, 1 for all dests of base cell, 2 for whole set repeats
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


/*! Default is not shearable, and aspect locked.
 */
TilingOp::TilingOp()
{
	celloutline=NULL;
	shearable=false;
	flexible_aspect=false;
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

	boundary=NULL;

	name=newstr(nname);
	category=newstr(ncategory);
	icon=NULL;
}

Tiling::~Tiling()
{
	delete[] name;
	delete[] category;
	if (icon) icon->dec_count();
	if (boundary) boundary->dec_count();
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
TilingOp *Tiling::AddBase(PathsData *outline, int absorb_count, int lock_base, bool shearable, bool flexible_base)
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


//! Transform applied after tiling to entire pattern, to squish around.
Affine Tiling::finalTransform()
{ return final_transform; }



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




/*! Set repeatable to be sized to hold a hexagon with side_length, long axis vertical.
 * Sets xaxis to point right, yaxis points diagonally to upper right 60 degrees from x.
 */
void Tiling::DefaultHex(double side_length)
{
	repeatable=3;//repeat in x and y
	repeatYDir(flatpoint(side_length*sqrt(3)/2,side_length*1.5));
	repeatXDir(flatpoint(side_length*sqrt(3),0));
}

	
LaxFiles::Attribute *Tiling::dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *context)
{
	// ***
	return NULL;
}

ObjectDef *Tiling::makeObjectDef()
{
	cerr << " *** need to implement Tiling::makeObjectDef()!!"<<endl;
	return NULL;
}

void Tiling::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

    if (what==-1) {
        fprintf(f,"%sname Blah          #optional human readable name\n",spc);
        fprintf(f,"%scategory Blah      #optional human readable category name\n",spc);
		return;
	}

	fprintf(f,"%sname %s\n",spc,name?name:"");
	fprintf(f,"%scategory %s\n",spc,category?category:"");
	if (icon && icon->filename) fprintf(f,"%sicon_file %s\n",spc,icon->filename);

	if (repeatable==0) fprintf(f,"%srepeatable no\n",spc);
	else if (repeatable==1) fprintf(f,"%srepeatable x\n",spc);
	else if (repeatable==2) fprintf(f,"%srepeatable y\n",spc);
	else if (repeatable==3) fprintf(f,"%srepeatable x y\n",spc);

	if (repeatable) {
		flatpoint v=repeatOrigin();
		fprintf(f,"%srepeat_origin %.10g,%.10g\n",spc,v.x,v.y);
		v=repeatXDir();
		fprintf(f,"%srepeat_x %.10g,%.10g\n",spc,v.x,v.y);
		v=repeatYDir();
		fprintf(f,"%srepeat_y %.10g,%.10g\n",spc,v.x,v.y);
	}

	TilingOp *op;
	TilingDest *dest;
	for (int c=0; c<basecells.n; c++) {
		op=basecells.e[c];

		fprintf(f,"%sbasecell\n",spc);
		if (op->shearable) fprintf(f,"%s  shearable\n",spc);
		if (op->flexible_aspect) fprintf(f,"%s  flexible\n",spc);
		if (op->celloutline) {
			fprintf(f,"%s  outline\n",spc);
			op->celloutline->dump_out(f,indent+4,what,context);
		}
		for (int c2=0; c2<op->transforms.n; c2++) {
			dest=op->transforms.e[c2];
			if (dest->max_iterations==1) {
				const double *m=dest->transform.m();
				fprintf(f,"%s  transform matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\n",
						spc, m[0],m[1],m[2],m[3],m[4],m[5]);
			} else {
				fprintf(f,"%s  clone\n",spc);
				const double *m=dest->transform.m();
				fprintf(f,"%s    transform matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\n",
						spc, m[0],m[1],m[2],m[3],m[4],m[5]);

				if (dest->traceable) fprintf(f,"%s    traceable\n",spc);
				fprintf(f,"%s    iterations %d\n",spc, dest->max_iterations);
				fprintf(f,"%s    max_size %.10g\n",spc, dest->max_size);
				fprintf(f,"%s    min_size %.10g\n",spc, dest->min_size);
			}
		}
	}
}

void Tiling::dump_in_atts(LaxFiles::Attribute *att, int flag, Laxkit::anObject *context)
{
    if (!att) return;

    char *name,*value;
    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(name,"name")) {
            //makestr(this->object_idstr,value);
            makestr(this->name,value);

        } else if (!strcmp(name,"category")) {
            makestr(category,value);

        } else if (!strcmp(name,"icon_file")) {
			LaxImage *nicon=load_image(value);
			if (nicon) {
				if (icon) icon->dec_count();
				icon=nicon;
			}

        } else if (!strcmp(name,"repeatable")) {
			if (isblank(value)) repeatable=3;
			else {
				if (!strcasecmp(value,"true") || !strcasecmp(value,"yes")) repeatable=1;
				else {
					repeatable=0;
					if (strchr(value,'x')) repeatable|=1;
					if (strchr(value,'y')) repeatable|=2;
				}
			}

        } else if (!strcmp(name,"repeat_origin")) {
			flatpoint v;
			FlatvectorAttribute(value,&v);
			repeat_basis.origin(v);

        } else if (!strcmp(name,"repeat_x")) {
			flatpoint v;
			FlatvectorAttribute(value,&v);
			repeat_basis.xaxis(v);

        } else if (!strcmp(name,"repeat_y")) {
			flatpoint v;
			FlatvectorAttribute(value,&v);
			repeat_basis.yaxis(v);

        } else if (!strcmp(name,"required_interface")) {
			required_interface=value;

        } else if (!strcmp(name,"basecell")) {
			TilingOp *op=new TilingOp;

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;

		        if (!strcmp(name,"shearable")) {
					op->shearable=BooleanAttribute(value);

		        } else if (!strcmp(name,"flexible")) {
					op->flexible_aspect=BooleanAttribute(value);

		        } else if (!strcmp(name,"outline")) {
					op->celloutline=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
					op->celloutline->dump_in_atts(att->attributes.e[c]->attributes.e[c2], flag,context);

		        } else if (!strcmp(name,"transform")) {
					TilingDest *dest=new TilingDest;
					double m[6];
					transform_identity(m);
					TransformAttribute(value,m,NULL);
					dest->transform.m(m);
					op->transforms.push(dest,1);

		        } else if (!strcmp(name,"clone")) {
					TilingDest *dest=new TilingDest;

					for (int c3=0; c3<att->attributes.e[c]->attributes.e[c2]->attributes.n; c3++) {
						name= att->attributes.e[c]->attributes.e[c2]->attributes.e[c3]->name;
						value=att->attributes.e[c]->attributes.e[c2]->attributes.e[c3]->value;

						if (!strcmp(name,"traceable")) {
							dest->traceable=BooleanAttribute(value);

						} else if (!strcmp(name,"iterations")) {
							IntAttribute(value,&dest->max_iterations);

						} else if (!strcmp(name,"max_size")) {
							DoubleAttribute(value,&dest->max_size);

						} else if (!strcmp(name,"min_size")) {
							DoubleAttribute(value,&dest->min_size);

						} else if (!strcmp(name,"transform")) {
							double m[6];
							transform_identity(m);
							TransformAttribute(value,m,NULL);
							dest->transform.m(m);
						}
					}

					op->transforms.push(dest,1);
				}
			}

			basecells.push(op,1);
		}
	}
}

//void Tiling::RenderRecursive(TilingDest *dest, int iteration, int orig_basecell,
// 					   Affine current_space,
//					   Group *parent_space,
//					   LaxInterfaces::ObjectContext *base_object_to_update, //!< If non-null, update relevant clones connected to base object
//					   bool trace_cells,
//					   ViewportWindow *viewport
//					   )
//{
//	if (iteration>=dest->max_iterations) return;
//
//	current_space.PreMultiply(dest->transform);
//	***
//
//
//	RenderRecursive(dest, iterations-1, current_space,parent_space,base_object_to_update, trace_cells, viewport);
//}

//! Create tiled clones.
/*! Install new objects as kids of parent_space. If NULL, create and return a new Group (else return parent_space).
 *
 * If base_objects is NULL, then create path outline objects from the transformed base cells instead.
 * If base_objects is not NULL, then
 */
Group *Tiling::Render(Group *parent_space,
					   Selection *source_objects, //!< If non-null, update relevant clones connected to base object
					   Affine *base_offsetm, //!< Additional offset to place basecells from source_objects
					   int p1_minx, int p1_maxx, int p1_miny, int p1_maxy,
					   LaxInterfaces::PathsData *boundary,
					   Affine *final_orient
					 )
{
	bool trace_cells=(source_objects==NULL || (source_objects!=NULL && source_objects->n()==0));

	if (!parent_space) parent_space=new Group;

	if (p1_maxx<p1_minx) p1_maxx=p1_minx;
	if (p1_maxy<p1_miny) p1_maxy=p1_miny;
	if (!isXRepeatable()) p1_maxx=p1_minx;
	if (!isYRepeatable()) p1_maxy=p1_miny;
	Affine p1;

	NumStack<flatpoint> points;
	if (boundary) {
		Affine repeattransform;
		repeattransform.setBasis(repeatOrigin(),repeatXDir(),repeatYDir());
		Coordinate *p=boundary->paths.e[0]->path;
		Coordinate *start=p;
		DoubleBBox bounds;
		flatpoint pp;
		do {
			pp=boundary->transformPoint(p->p());
			if (base_offsetm) pp=base_offsetm->transformPointInverse(pp);
			pp=repeattransform.transformPointInverse(pp);
			points.push(pp);
			bounds.addtobounds(pp);
			p=p->next;
		} while (p && p!=start);

		p1_minx=bounds.minx-.5;
		p1_miny=bounds.miny-.5;
		p1_maxx=bounds.maxx-.5;
		p1_maxy=bounds.maxy-.5;
	} else {
		points.push(flatpoint(p1_minx,p1_miny));
		points.push(flatpoint(p1_maxx,p1_miny));
		points.push(flatpoint(p1_maxx,p1_maxy));
		points.push(flatpoint(p1_minx,p1_maxy));
	}
	if (!isXRepeatable()) { p1_minx=p1_maxx=(p1_minx+p1_maxx)/2; }
	if (!isYRepeatable()) { p1_miny=p1_maxy=(p1_miny+p1_maxy)/2; }

	
	 //cache transform to base objects, if any
	Affine *sourcem=NULL;
	Affine *sourcemi=NULL;
	SomeData *base=NULL;

	if (!trace_cells) { //we have source objects
		Affine a;

		sourcem =new Affine[source_objects->n()];
		sourcemi=new Affine[source_objects->n()];

		for (int c=0; c<source_objects->n(); c++) {
			ObjectContext *oc=source_objects->e(c);
			base=oc->obj;
			if (dynamic_cast<DrawableObject*>(base)) 
				a=dynamic_cast<DrawableObject*>(base)->GetTransformToContext(false,0);

			sourcem[c].m(a.m());
			sourcemi[c]=sourcem[c];
			sourcemi[c].Invert();
		}
	}


	Group *trace=NULL;
	if (trace_cells) {
		 //set up outlines 
		double w=norm( basecells.e[0]->celloutline->ReferencePoint(LAX_TOP_RIGHT,true)-
				basecells.e[0]->celloutline->ReferencePoint(LAX_BOTTOM_LEFT,true))/100;
		LineStyle *ls=new LineStyle(65535,0,0,65535, w, LAXCAP_Round,LAXJOIN_Round,0,LAXOP_Source);
		trace=new Group;
		trace->Id("base_cells");
		PathsData *d;
		for (int c=0; c<basecells.n; c++) {
			if (basecells.e[c]->celloutline) {
				d=dynamic_cast<PathsData*>(basecells.e[c]->celloutline->duplicate(NULL));
				d->InstallLineStyle(ls);
				d->FindBBox();
				trace->push(d);
				d->dec_count();
			}
		}
		trace->FindBBox();
		if (base_offsetm) trace->m(base_offsetm->m());
		parent_space->push(trace);
		ls->dec_count();
	}


	SomeDataRef *clone=NULL;
	Affine clonet, ttt;
	TilingDest *dest;
	//Affine basecellm;
	Affine basecellmi;
	if (base_offsetm) {
		basecellmi.m(base_offsetm->m());
		basecellmi.Invert();
	}
	
	flatpoint pp;
	for (int x=p1_minx; x<=p1_maxx; x++) {
	  for (int y=p1_miny; y<=p1_maxy; y++) {
		pp.x=x+.5;
		pp.y=y+.5;
		if ((p1_minx!=p1_maxx || p1_miny!=p1_maxy) && !point_is_in(pp,points.e,points.n)) continue;

		for (int c=0; c<basecells.n; c++) {
		  for (int c2=0; c2<basecells.e[c]->transforms.n; c2++) {
			dest=basecells.e[c]->transforms.e[c2];

			p1.setIdentity();
			if (x==0 && y==0) {
				p1.origin(repeatOrigin());
			} else {
				p1.origin(repeatOrigin() + x*repeatXDir() + y*repeatYDir());
			}
			clonet.set(dest->transform);
			clonet.Multiply(p1); //so clonet = (Dest) * (p1 x y transform)

			// *** need real boundary check if given.. above is good 1st approximation
			//if (boundary) {
			//}

			if (trace_cells) {
			  if (dest->traceable) {
				clone=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
				clone->Set(trace->e(c),0);
				clone->Multiply(clonet);
				clone->FindBBox();
				if (final_orient) clone->Multiply(*final_orient);
				parent_space->push(clone);
				clone->dec_count();
			  }

			} else { //for each source object in current base cell...
			  for (int s=0; s<source_objects->n(); s++) {
				if (source_objects->e_info(s)!=c) continue;

				clone=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
				base=source_objects->e(s)->obj;
				if (dynamic_cast<SomeDataRef*>(base)) base=dynamic_cast<SomeDataRef*>(base)->GetFinalObject();
				clone->Set(base,1); //the 1 means don't copy matrix also
				clone->m(sourcem[s].m());

				if (base_offsetm) clone->Multiply(basecellmi);
				clone->Multiply(clonet);
				if (final_orient) clone->Multiply(*final_orient);
				//clone->Multiply(sourcemi[s]);

				parent_space->push(clone);
				clone->dec_count();
			  }
			}

			 // For recursive destinations:
			for (int i=dest->max_iterations-1; i>0; i--) {
				clonet.PreMultiply(dest->transform);

				if (trace_cells) {
				  if (dest->traceable) {
					clone=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
					clone->Set(trace->e(c),0);
					clone->Multiply(clonet);
					clone->FindBBox();
					if (final_orient) clone->Multiply(*final_orient);
					parent_space->push(clone);
					clone->dec_count();
				  }

				} else { //if source objects..
				  for (int s=0; s<source_objects->n(); s++) {
					if (source_objects->e_info(s)!=c) continue;

					clone=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
					base=source_objects->e(s)->obj;
					if (dynamic_cast<SomeDataRef*>(base)) base=dynamic_cast<SomeDataRef*>(base)->GetFinalObject();
					clone->Set(base,1); //the 1 means don't copy matrix also
					clone->m(sourcem[s].m());

					if (base_offsetm) clone->Multiply(basecellmi);
					clone->Multiply(clonet);
					if (final_orient) clone->Multiply(*final_orient);
					//clone->Multiply(sourcemi[s]);

					parent_space->push(clone);
					clone->dec_count();
				  }
				}

			} //foreach iteration of current dest

		  } //basecells dests
		} //basecells
	  } //y
	} //x

	 //clean up
	if (trace) trace->dec_count();
	delete[] sourcem;
	delete[] sourcemi;

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
	double cellangle=rotation_angle;
	if (mirrored) cellangle/=2;

	Tiling *tiling=new Tiling(NULL,"Circular");
	tiling->repeatable=0;
	tiling->required_interface="radial";
	if (mirrored) makestr(tiling->name,"rm"); else makestr(tiling->name,"r");


	 //define cell outline
	PathsData *path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
	double a =start_angle;
	double a2=a+cellangle;
	path->append(start_radius*flatpoint(cos(a),sin(a)));
	path->append(  end_radius*flatpoint(cos(a),sin(a)));

	path->appendBezArc(flatpoint(0,0), cellangle, 1);
	path->append(start_radius*flatpoint(cos(a2),sin(a2)));

	path->appendBezArc(flatpoint(0,0), -cellangle, 1);
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
		//dest->max_iterations=num_divisions;
		dest->max_iterations=1;
		dest->conditions=TILING_Iterations;
		dest->transform.Flip(flatpoint(0,0),
				   flatpoint(cos(start_angle+cellangle),sin(start_angle+cellangle)));
		op->AddTransform(dest);

		Affine affine;
		affine.Flip(flatpoint(0,0), flatpoint(cos(start_angle+cellangle),sin(start_angle+cellangle)));
		for (int c=1; c<num_divisions; c++) {
			affine.Rotate(rotation_angle);
			op->AddTransform(affine);
		}
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

Tiling *CreateSpiral(double start_angle, //!< radians
					 double end_angle,   //!< If end==start, or end==start+360, use full circle
					 double start_radius,//!< For purposes of defining a base cell outline
					 double end_radius,  //!< For purposes of defining a base cell outline
					 int num_divisions   //!< divide total angle into this many sections
		)
{
	if (end_angle==start_angle) end_angle=start_angle+2*M_PI;
	double rotation_angle=(end_angle-start_angle)/num_divisions;
	double cellangle=rotation_angle;

	Tiling *tiling=new Tiling("spiral","Circular");
	tiling->repeatable=0;
	tiling->required_interface="radial";

	//spiral equation: r(n)=r_outer*(r_inner/r_outer)^n, where n is number of winds
	//					   =r_outer*exp(n*ln(r_inner/r_outer))
	// if y=a^x, y'=ln(a)*a^x
	double nsr=start_radius*exp(cellangle/2/M_PI*log(end_radius/start_radius));
	double ner=end_radius  *exp(cellangle/2/M_PI*log(end_radius/start_radius));

	 //define cell outline
	PathsData *path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
	double a = start_angle;
	double a2= start_angle + cellangle;
	path->append(  end_radius*flatpoint(cos(a),sin(a)));
	path->append(start_radius*flatpoint(cos(a),sin(a)));

	//flatpoint pp=start_radius*flatpoint(cos(a),sin(a));
	//double theta=cellangle;
	//double rp=log(end_radius/start_radius)*start_radius*exp(theta/2/M_PI*log(end_radius/start_radius));
	//double r=start_radius;
	//flatpoint v=flatpoint(rp*cos(theta)-r*sin(theta),rp*sin(theta)+r*cos(theta));

	path->append(nsr*flatpoint(cos(a2),sin(a2)));
	path->append(ner*flatpoint(cos(a2),sin(a2)));

	path->close();


	 //set up a recursive transform limited by num_divisions
	TilingDest *dest=new TilingDest;
	dest->max_iterations=num_divisions;
	dest->conditions=TILING_Iterations;
	dest->transform.setRotation(-rotation_angle);
	dest->transform.Scale(nsr/start_radius);

	TilingOp *op=tiling->AddBase(path,1,1);
	op->AddTransform(dest);



	 //icon key "Circular__r" or "Circular__rm"
	tiling->InstallDefaultIcon();


	return tiling;
}

/*! Create a tiling based on a wallpaper group.
 *
 * Note the definitions below are in some cases very brute force. It defines each transform
 * necessary to create an overall p1 cell.
 * It COULD be simplified by using iterated operations.
 */
Tiling *CreateWallpaper(Tiling *tiling,const char *group)
{
	if (isblank(group)) group="p1";
	

	if (!tiling) tiling=new Tiling(group,"Wallpaper");
	tiling->repeatable=3;

	PathsData *path;
	TilingOp *op;
	Affine affine;

	if (!strcasecmp(group,"p1")) {
		 //define single base cell, covers whole unit cell
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"p2")) {
		 //rotate on edge
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,.5,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;

		op->AddTransform(affine);

		affine.Rotate(M_PI,flatpoint(.5,.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"pm")) {
		 //flip on edge
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,.5,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->flexible_aspect=true;

		op->AddTransform(affine);

		affine.Flip(flatpoint(.5,0),flatpoint(.5,1));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"pg")) {
		 //glide reflect
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,.5,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->flexible_aspect=true;

		op->AddTransform(affine);

		affine.Translate(flatpoint(.5,0));
		affine.Flip(flatpoint(0,.5),flatpoint(1,.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"cm")) {
		 //reflect vertically, glide reflect horizontally
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,.5,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->flexible_aspect=true;

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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,.5,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->flexible_aspect=true;

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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,.5,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->flexible_aspect=true;

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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,.5,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->flexible_aspect=true;

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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,.25,.5);
		path->FindBBox();
		op=tiling->AddBase(path,1,1); //0,0
		op->flexible_aspect=true;

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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
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


	 //find a matching icon...
	tiling->InstallDefaultIcon();

	return tiling;
}

Tiling *CreateUniformColoring(const char *coloring)
{
	Tiling *tiling=new Tiling(coloring,"Uniform Coloring");
	tiling->InstallDefaultIcon();

	PathsData *path;
	TilingOp *op;
	Affine affine;

	if (!strcasecmp(coloring,"square 1")) {
		tiling=CreateWallpaper(tiling,"p1");

	} else if (!strcasecmp(coloring,"square 2")
			|| !strcasecmp(coloring,"square 3")) {
		// 2: ab ab    3: ab 
		//    aa aa       aa ab
		//                   aa

		tiling->repeatXDir(flatpoint(2,0));
		if (!strcasecmp(coloring,"square 3")) tiling->repeatYDir(flatpoint(1,2));
		else tiling->repeatYDir(flatpoint(0,2));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		op->AddTransform(affine);
	
		affine.Translate(flatpoint(1,0));
		op->AddTransform(affine);

		affine.Translate(flatpoint(-1,1));
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(1,1,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		affine.setIdentity();
		op->AddTransform(affine);
	
	} else if (!strcasecmp(coloring,"square 4")
			|| !strcasecmp(coloring,"square 5")) {

		tiling->repeatYDir(flatpoint(0,2));
		if (!strcasecmp(coloring,"square 5")) {
			tiling->repeatXDir(flatpoint(1,1));
		}

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,1,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"square 6")
			|| !strcasecmp(coloring,"square 7")
			|| !strcasecmp(coloring,"square 8")
			|| !strcasecmp(coloring,"square 9")) {
		// 6: ab ab   7: ab ab   8: ab ab  9:ab ab
		//    cc cc      cc cc      ca ca    cd cd
		//    ab ab       ab ab
		//    cc cc       cc cc

		tiling->repeatXDir(flatpoint(2,0));
		if (!strcasecmp(coloring,"square 7")) tiling->repeatYDir(flatpoint(1,2));
		else tiling->repeatYDir(flatpoint(0,2));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,0,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		op->AddTransform(affine);

		if (!strcasecmp(coloring,"square 9")) {
			path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
			path->appendRect(1,0,1,1);
			path->FindBBox();
			op=tiling->AddBase(path,1,1);
			op->shearable=true;
			op->flexible_aspect=true;
			op->AddTransform(affine);

		} else {
			if (!strcasecmp(coloring,"square 8")) affine.Translate(flatpoint(1,1));
			else affine.Translate(flatpoint(1,0));
			op->AddTransform(affine);
		}

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,1,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		affine.setIdentity();
		op->AddTransform(affine);
	
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(1,1,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		affine.setIdentity();
		if (!strcasecmp(coloring,"square 8")) {
			path->Translate(flatpoint(0,-1));
			path->ApplyTransform();
		}
		op->AddTransform(affine);
	
	} else if (!strcasecmp(coloring,"hexagonal 1")) {
		tiling->DefaultHex(1);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		Coordinate *cc=CoordinatePolygon(flatpoint(sqrt(3)/2,1), 1, false, 6, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);

		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"hexagonal 2")) {
		tiling->repeatXDir(flatpoint(3*sqrt(3)/2,1.5));
		tiling->repeatYDir(flatpoint(0,3));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		Coordinate *cc=CoordinatePolygon(flatpoint(sqrt(3)/2,1), 1, false, 6, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);
		affine.Translate(flatpoint(sqrt(3),0));
		op->AddTransform(affine);

		cc=CoordinatePolygon(flatpoint(sqrt(3),2.5), 1, false, 6, 1);
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"hexagonal 3")) {
		tiling->repeatXDir(flatpoint(3*sqrt(3)/2,1.5));
		tiling->repeatYDir(flatpoint(0,3));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		Coordinate *cc=CoordinatePolygon(flatpoint(sqrt(3)/2,1), 1, false, 6, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		cc=CoordinatePolygon(flatpoint(3*sqrt(3)/2,1), 1, false, 6, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

		cc=CoordinatePolygon(flatpoint(sqrt(3),2.5), 1, false, 6, 1);
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"snub square 1") 
			|| !strcasecmp(coloring,"snub square 2")) {

		tiling->repeatYDir(flatpoint(.5+sqrt(3)/2,.5+sqrt(3)/2));
		tiling->repeatXDir(flatpoint(1+sqrt(3),0));

		 //diamond
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,sqrt(3)/2);
		path->append(.5,0);
		path->append(1,sqrt(3)/2);
		path->append(.5,sqrt(3));
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);

		op->AddTransform(affine);

		affine.Rotate(M_PI/2,flatpoint(.5,sqrt(3)/2));
		affine.Translate(flatpoint(.5+sqrt(3)/2,0));
		op->AddTransform(affine);

		 //square1
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(.5,0);
		path->append(.5+sqrt(3)/2,-.5);
		path->append( 1+sqrt(3)/2,-.5+sqrt(3)/2);
		path->append(1,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		affine.setIdentity();
		op->AddTransform(affine);

		if (!strcasecmp(coloring,"snub square 1")) {
			affine.Rotate(-M_PI/3, flatpoint(1,sqrt(3)/2));
			affine.Translate(flatpoint(-.5,sqrt(3)/2));
			op->AddTransform(affine);

		} else {
			 //square2
			path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
			path->append(1,sqrt(3)/2);
			path->append(1+sqrt(3)/2,sqrt(3)/2+.5);
			path->append(.5+sqrt(3)/2, sqrt(3)+.5);
			path->append(.5,sqrt(3));
			path->close();
			path->FindBBox();
			op=tiling->AddBase(path,1,1);
			affine.setIdentity();
			op->AddTransform(affine);
		}

	} else if (!strcasecmp(coloring,"elongated triangular")) {
		tiling->repeatYDir(flatpoint(-.5,1+sqrt(3)/2));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->AddTransform(affine);

		affine.Rotate(M_PI, flatpoint(.75,sqrt(3)/4));
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(0,-1, 1,1);
		op=tiling->AddBase(path,1,1);
		affine.setIdentity();
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"truncated square 1")
			|| !strcasecmp(coloring,"truncated square 2")) {

		if (!strcasecmp(coloring,"truncated square 2")) {
			tiling->repeatXDir(flatpoint(2*(1+sqrt(2)),0));
			tiling->repeatYDir(flatpoint(1+sqrt(2),1+sqrt(2)));
		} else {
			tiling->repeatXDir(flatpoint(1+sqrt(2),0));
			tiling->repeatYDir(flatpoint(0,1+sqrt(2)));
		}

		 //first octagon
		double x=.5+1/sqrt(2);
		double r=sqrt(.25+x*x);
		Coordinate *cc=CoordinatePolygon(flatpoint(x,x), r, false, 8, 1);
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		if (!strcasecmp(coloring,"truncated square 2")) {
			 //second octagon
			path=dynamic_cast<PathsData*>(path->duplicate(NULL));
			path->Translate(flatpoint(1+sqrt(2),0));
			path->ApplyTransform();
			op=tiling->AddBase(path,1,1, false,false);
			op->AddTransform(affine);
		}

		 //the square
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		double a=1/sqrt(2);
		path->append(-a,0);
		path->append(0, a);
		path->append( a,0);
		path->append(0,-a);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

		if (!strcasecmp(coloring,"truncated square 2")) {
			affine.setIdentity();
			affine.Translate(flatpoint(1+sqrt(2),0));
			op->AddTransform(affine);
		}


	} else if (!strcasecmp(coloring,"triangular 1")) {
		tiling->repeatYDir(flatpoint(.5,-sqrt(3)/2));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		affine.Rotate(M_PI,flatpoint(.75,sqrt(3)/4));
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"triangular 2")) {
		tiling->repeatXDir(flatpoint(1.5,-sqrt(3)/2));
		tiling->repeatYDir(flatpoint(0,sqrt(3)));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		affine.Rotate(-M_PI/3,flatpoint(1,0));
		op->AddTransform(affine);

		affine.Rotate(-M_PI/3,flatpoint(1,0));
		op->AddTransform(affine);

		affine.Rotate(-M_PI/3,flatpoint(1,0));
		op->AddTransform(affine);

		affine.Rotate(-M_PI/3,flatpoint(1,0));
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(.5,sqrt(3)/2);
		path->append(1,0);
		path->append(1.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"triangular 3")) {
		tiling->repeatXDir(flatpoint(1,0));
		tiling->repeatYDir(flatpoint(.5,sqrt(3)/2));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(.5,sqrt(3)/2);
		path->append(1,0);
		path->append(1.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"triangular 4")) {
		tiling->repeatXDir(flatpoint(2,0));
		tiling->repeatYDir(flatpoint(0,sqrt(3)));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		affine.Rotate(M_PI, flatpoint(.75,sqrt(3)/4));
		op->AddTransform(affine);

		affine.Rotate(M_PI/3, flatpoint(1,0));
		affine.Translate(flatpoint(.5,-sqrt(3)/2));
		op->AddTransform(affine);

		affine.Rotate(M_PI, flatpoint(1.75,-sqrt(3)/4));
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,-sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

		affine.Rotate(M_PI, flatpoint(.75,-sqrt(3)/4));
		op->AddTransform(affine);

		affine.Rotate(-M_PI/3, flatpoint(1,0));
		affine.Translate(flatpoint(.5,sqrt(3)/2));
		op->AddTransform(affine);

		affine.Rotate(M_PI, flatpoint(1.75,sqrt(3)/4));
		op->AddTransform(affine);



	} else if (!strcasecmp(coloring,"triangular 5")
			|| !strcasecmp(coloring,"triangular 8")) {
		tiling->repeatXDir(flatpoint(1.5,-sqrt(3)/2));
		tiling->repeatYDir(flatpoint(0,sqrt(3)));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(1,0);
		path->append(1.5,sqrt(3)/2);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		if (!strcasecmp(coloring,"triangular 8")) {
			path=dynamic_cast<PathsData*>(path->duplicate(NULL));
			op=tiling->AddBase(path,1,1, false,false);
			path->Rotate(-2*M_PI/3, flatpoint(1,0));
			path->ApplyTransform();
			op->AddTransform(affine);
		} else {
			affine.Rotate(-2*M_PI/3, flatpoint(1,0));
			op->AddTransform(affine);
		}

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

		affine.Rotate(2*M_PI/3, flatpoint(1,0));
		op->AddTransform(affine);

		affine.Rotate(M_PI/3, flatpoint(1,0));
		op->AddTransform(affine);

		affine.Rotate(M_PI/3, flatpoint(1,0));
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"triangular 7")) {
		tiling->repeatXDir(flatpoint(1,0));
		tiling->repeatYDir(flatpoint(0,sqrt(3)));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		affine.Rotate(M_PI, flatpoint(.75,sqrt(3)/4));
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,-sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

		affine.Rotate(M_PI, flatpoint(.75,-sqrt(3)/4));
		op->AddTransform(affine);


	} else if (!strcasecmp(coloring,"triangular 6")
			|| !strcasecmp(coloring,"triangular 9")) {
		tiling->repeatXDir(flatpoint(1.5,-sqrt(3)/2));
		tiling->repeatYDir(flatpoint(0,sqrt(3)));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(.5,sqrt(3)/2);
		path->append(1,0);
		path->append(1.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(path->duplicate(NULL));
		path->Rotate(2*M_PI/3, flatpoint(1,0));
		path->ApplyTransform();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		if (!strcasecmp(coloring,"triangular 9")) {
			path=dynamic_cast<PathsData*>(path->duplicate(NULL));
			path->Rotate(2*M_PI/3, flatpoint(1,0));
			path->ApplyTransform();
			op=tiling->AddBase(path,1,1, false,false);
			op->AddTransform(affine);
		} else {
			affine.Translate(flatpoint(-1,0));
			op->AddTransform(affine);
		}

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

		affine.Rotate(2*M_PI/3, flatpoint(1,0));
		op->AddTransform(affine);

		affine.Rotate(2*M_PI/3, flatpoint(1,0));
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"trihexagonal 1")
			|| !strcasecmp(coloring,"trihexagonal 2")) {
		tiling->repeatXDir(flatpoint(2,0));
		tiling->repeatYDir(flatpoint(1,sqrt(3)));

		 //hexagon
		Coordinate *cc=CoordinatePolygon(flatpoint(1,0), 1, true, 6, 1);
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendCoord(cc);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		 //triangle
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(2,0);
		path->append(2.5,sqrt(3)/2);
		path->append(1.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		if (!strcasecmp(coloring,"trihexagonal 2")) {
			path=dynamic_cast<PathsData*>(path->duplicate(NULL));
			path->Rotate(M_PI,flatpoint(2,0));
			path->ApplyTransform();
			op=tiling->AddBase(path,1,1, false,false);
			op->AddTransform(affine);

		} else {
			affine.Rotate(M_PI,flatpoint(2,0));
			op->AddTransform(affine);
		}

	} else if (!strcasecmp(coloring,"snub hexagonal")) {
		tiling->repeatXDir(flatpoint(2.5,-sqrt(3)/2));
		tiling->repeatYDir(flatpoint(2,sqrt(3)));

		 //hexagon
		Coordinate *cc=CoordinatePolygon(flatpoint(1,0), 1, true, 6, 1);
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendCoord(cc);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		 //triangles
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(-.5,sqrt(3)/2);
		path->append(-1,0);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		affine.Translate(flatpoint(.5,sqrt(3)/2));
		op->AddTransform(affine);

		affine.Translate(flatpoint(1,0));
		op->AddTransform(affine);

		affine.Translate(flatpoint(1,0));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Rotate(M_PI,flatpoint(-.25,sqrt(3)/4));
		op->AddTransform(affine);

		affine.Translate(flatpoint(.5,sqrt(3)/2));
		op->AddTransform(affine);

		affine.Translate(flatpoint(1,0));
		op->AddTransform(affine);

		affine.Translate(flatpoint(.5,-sqrt(3)/2));
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"rhombi trihexagonal")) {
		tiling->repeatXDir(flatpoint((3+sqrt(3))/2,(1+sqrt(3))/2));
		tiling->repeatYDir(flatpoint(0,1+sqrt(3)));

		 //hexagon
		Coordinate *cc=CoordinatePolygon(flatpoint(1,0), 1, true, 6, 1);
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendCoord(cc);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		 //squares
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(0,0);
		path->append(.5,sqrt(3)/2);
		path->append(.5-sqrt(3)/2,.5+sqrt(3)/2);
		path->append(-sqrt(3)/2,.5);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		affine.Rotate(M_PI/3,flatpoint(.5,sqrt(3)/2));
		affine.Translate(flatpoint(1,0));
		op->AddTransform(affine);

		affine.Translate(flatpoint(1,0));
		affine.Rotate(M_PI/3,flatpoint(1.5,sqrt(3)/2));
		op->AddTransform(affine);

		 //triangles
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(.5,sqrt(3)/2);
		path->append(.5,1+sqrt(3)/2);
		path->append(.5-sqrt(3)/2,.5+sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

		affine.Rotate(M_PI/3,flatpoint(.5,sqrt(3)/2));
		affine.Translate(flatpoint(1,0));
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"truncated hexagonal")) {
		double r=.5/sin(M_PI/12);
		double rx=.5/tan(M_PI/12);
		tiling->repeatXDir(flatpoint(2*rx,0));
		tiling->repeatYDir(flatpoint(rx,rx*sqrt(3)));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		Coordinate *cc=CoordinatePolygon(flatpoint(0,0), r, false, 12, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->append(-.5,-rx);
		path->append(.5,-rx);
		path->append(0,-rx-sqrt(3)/2);
		path->close();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		affine.Rotate(M_PI,flatpoint(0,-rx));
		affine.Translate(flatpoint(rx,rx-sqrt(3)/2-.5));
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"truncated trihexagonal")) {
		double r=.5/sin(M_PI/12); //radius of 12 sided, with unit side length
		double rx=.5/tan(M_PI/12);//inradius
		tiling->repeatXDir(flatpoint((1+2*rx)*sqrt(3)/2,(1+2*rx)/2));
		tiling->repeatYDir(flatpoint(0,1+2*rx));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		Coordinate *cc=CoordinatePolygon(flatpoint(0,0), r, false, 12, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		 //squares
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		path->appendRect(-.5,rx, 1,1);
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		affine.Translate(flatpoint(rx-.5,.5-rx));
		affine.Rotate(M_PI/3, flatpoint(rx,.5));
		op->AddTransform(affine);

		affine.setIdentity();
		affine.Translate(flatpoint(.5-rx,.5-rx));
		affine.Rotate(-M_PI/3, flatpoint(-rx,.5));
		op->AddTransform(affine);

		 //hexagons
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
		cc=CoordinatePolygon(flatpoint(0,0), 1, false, 6, 1);
		path->appendCoord(cc);
		path->Translate(flatpoint(-(.5+sqrt(3)/2),rx+.5));
		path->ApplyTransform();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

		affine.Translate(flatpoint(1+sqrt(3),0));
		op->AddTransform(affine);


	} else {
		cerr << " *** HEY!!! FINISH IMPLEMENTING ME!!!!!! ("<<coloring<<")"<<endl;
		tiling->dec_count();
		return NULL;
	}

	tiling->InstallDefaultIcon();
	return tiling;
}

//! Frieze groups. Basically some wallpaper groups, but only one dimensional expansion.
Tiling *CreateFrieze(const char *group)
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

	Tiling *tiling=CreateWallpaper(NULL,wall);

	tiling->repeatable=1;
	makestr(tiling->name,group);
	makestr(tiling->category,"Frieze");
	tiling->InstallDefaultIcon();

	if (!strcasecmp(group,"mg") || !strcasecmp(group,"1m")) {
		 //uses the wallpaper group, but in y, not in x.
		tiling->repeatable=2;
		//tiling->repeatXDir(flatpoint(0,-1));
		//tiling->repeatYDir(flatpoint(1,0));
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

		    "Circular/spiral",
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
			"Uniform Coloring/truncated trihexagonal",
			"Uniform Coloring/truncated square 1",
			"Uniform Coloring/truncated square 2",
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
	for (unsigned int c=0; c<strlen(tile); c++) if (tile[c]==' ') tile[c]='_';

	return tile;
}

Tiling *GetBuiltinTiling(int num)
{
	const char *tile=BuiltinTiling[num];

	if (strstr(tile,"Wallpaper")) return CreateWallpaper(NULL,tile+10);

	if (!strcmp(tile,"Circular/spiral"))  return CreateSpiral(0, 4*M_PI, 5,3, 20);
	if (!strcmp(tile,"Circular/r"))  return CreateRadialSimple(0, 10);
	if (!strcmp(tile,"Circular/rm"))  return CreateRadialSimple(1, 5);

	if (strstr(tile,"Frieze"))    return CreateFrieze(tile+7);
	if (strstr(tile,"Uniform"))   return CreateUniformColoring(tile+17);

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
	CLONEI_BaseCell,
	CLONEI_Source_Object,
	CLONEI_Boundary,

	CLONEI_MAX
};

enum TilingShortcutActions {
	CLONEIA_None=0,
	CLONEIA_Edit,
	CLONEIA_Next_Tiling,
	CLONEIA_Previous_Tiling,
	CLONEIA_Next_Basecell,
	CLONEIA_Previous_Basecell,
	CLONEIA_Toggle_Lines,
	CLONEIA_Toggle_Preview,
	CLONEIA_Toggle_Render,
	CLONEIA_Toggle_Orientations,
	CLONEIA_Select,

	CLONEIA_MAX
};

#define CMODE_Normal  0
#define CMODE_Edit    1
#define CMODE_Select  2

CloneInterface::CloneInterface(anInterface *nowner,int nid,Laxkit::Displayer *ndp)
  : anInterface(nowner,nid,ndp),
	rectinterface(0,NULL)
{
	mode=CMODE_Normal;

	inrect=false;
	rectinterface.style|= RECT_CANTCREATE | RECT_OBJECT_SHUNT;
	rectinterface.owner=this;

	cloner_style=0;
	lastover=CLONEI_None;
	lastoveri=-1;
	active=false;
	preview_orient=false;

	previewoc=NULL;
	preview=new Group;
	char *str=make_id("tiling");
	preview->Id(str);
	delete[] str;

	str=make_id("tilinglines");
	lines=new Group;
	lines->Id(str);
	delete[] str;

	sc=NULL;

	firsttime=1;
	uiscale=1;
	bg_color =rgbcolorf(.9,.9,.9);
	hbg_color=rgbcolorf(1.,1.,1.);
	fg_color =rgbcolorf(.1,.1,.1);
	activate_color=rgbcolorf(0.,.783,0.);
	deactivate_color=rgbcolorf(1.,.392,.392);

	preview_cell.Color(65535,0,0,65535);
	preview_cell.width=2;
	preview_cell.widthtype=0;
	preview_cell2.Color(35000,35000,35000,65535);
	preview_cell2.width=1;
	preview_cell2.widthtype=0;

	current_base=-1;
	ScreenColor col(0.,.7,0.,1.);
	boundary=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory->newObject("PathsData"));
	boundary->line(-1,-1,-1,&col);
	boundary->appendRect(0,0,4,4);

	tiling=NULL;
	preview_lines=false;
	trace_cells=true; // *** maybe 2 should be render outline AND install as new objects in doc?
	//source_objs=NULL; //a pool of objects to select from, rather than clone
					 //any needed beyond those is source_objs are then cloned
					 //from same list in order

	cur_tiling=-1;
	PerformAction(CLONEIA_Next_Tiling);
}

CloneInterface::~CloneInterface()
{
	//if (source_objs) source_objs->dec_count();
	if (sc) sc->dec_count();
	if (tiling) tiling->dec_count();
	if (boundary) boundary->dec_count();
	if (preview) preview->dec_count();
	if (lines) lines->dec_count();
	if (previewoc) delete previewoc;
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

	sc->Add(CLONEIA_Next_Tiling,        LAX_Left,0,0,  "NextTiling",        _("Select next tiling"),    NULL,0);
	sc->Add(CLONEIA_Previous_Tiling,    LAX_Right,0,0, "PreviousTiling",    _("Select previous tiling"),NULL,0);
	sc->Add(CLONEIA_Next_Basecell,      LAX_Up,0,0,    "NextBasecell",      _("Select next base cell"),    NULL,0);
	sc->Add(CLONEIA_Previous_Basecell,  LAX_Down,0,0,  "PreviousBasecell",  _("Select previous base cell"),    NULL,0);
	sc->Add(CLONEIA_Toggle_Lines,       'l',0,0,       "ToggleLines",       _("Toggle rendering cell lines"),NULL,0);
	sc->Add(CLONEIA_Toggle_Render,      LAX_Enter,0,0, "ToggleRender",      _("Toggle rendering"),NULL,0);
	sc->Add(CLONEIA_Toggle_Preview,     'p',0,0,       "TogglePreview",     _("Toggle preview of lines"),NULL,0);
	sc->Add(CLONEIA_Toggle_Orientations,'o',0,0,       "ToggleOrientations",_("Toggle preview of orientations"),NULL,0);
	sc->Add(CLONEIA_Edit,            'e',ControlMask,0,"Edit",              _("Edit"),NULL,0);
	sc->Add(CLONEIA_Select,             's',0,0,       "Select",            _("Select tile mode"),NULL,0);

	//sc->AddShortcut(LAX_Del,0,0, PAPERI_Delete);


	manager->AddArea("CloneInterface",sc);
	return sc;
}

/*! Replace old tiling with newtiling. Update base_cells control objects
 * Absorbs count of newtiling.
 */
int CloneInterface::SetTiling(Tiling *newtiling)
{
	if (!newtiling) return 1;

	if (tiling) tiling->dec_count();
	tiling=newtiling;

	base_cells.Unshear(1,0);
	base_cells.flush();
	base_cells.flags|=SOMEDATA_KEEP_ASPECT;
	LaxInterfaces::SomeData *o;
  	DrawableObject *d;

	 //install main base cells
	LineStyle *lstyle;
	ScreenColor c1(1.,0.,0.,1.), c2(0.,0.,1.,1.), ca;

	for (int c=0; c<tiling->basecells.n; c++) {
		if (tiling->basecells.n>1) coloravg(&ca, &c1,&c2, (double)c/(tiling->basecells.n-1));
		else ca=c1;

		o=tiling->basecells.e[c]->celloutline->duplicate(NULL);
		o->FindBBox();
		lstyle=new LineStyle();
		*lstyle=preview_cell;
		lstyle->color=ca;
		dynamic_cast<PathsData*>(o)->InstallLineStyle(lstyle);
		lstyle->dec_count();
		base_cells.push(o);
		o->dec_count();

		d=dynamic_cast<DrawableObject*>(o);
		d->properties.push("base",c);
	}

	 //install minor base cell copies
	for (int c=0; c<tiling->basecells.n; c++) {
		if (tiling->basecells.e[c]->transforms.n<=1) continue;

		for (int c2=1; c2<tiling->basecells.e[c]->transforms.n; c2++) {
			o=tiling->basecells.e[c]->celloutline->duplicate(NULL);
			o->FindBBox();
			o->Multiply(tiling->basecells.e[c]->transforms.e[c2]->transform);
			dynamic_cast<PathsData*>(o)->InstallLineStyle(&preview_cell2);
			base_cells.push(o);
			o->dec_count();

			d=dynamic_cast<DrawableObject*>(o);
			d->properties.push("base",c);
		}
	}
	base_cells.FindBBox();

	 //remove any source images that can't map to current base cells
	current_base=0;
	for (int c=0; c<sources.n(); c++) {
		if (sources.e_info(c)>=base_cells.n()) {
			sources.Remove(c);
			source_proxies.Remove(c);
			c--;
		}
	}
//	-----move base_cell to coincide with one of the objects in sources
//	if (toc) {
//		double m[6];
//		viewport->transformToContext(m,toc,0,1);
//		base_cells.origin(flatpoint(m[4],m[5]));
//	}

	if (active) Render();

	needtodraw=1;
	return 0;
}


int CloneInterface::PerformAction(int action)
{
	if (action==CLONEIA_Next_Tiling) {
		Tiling *newtiling=NULL;
		int maxtiling=NumBuiltinTilings();
		do {
			cur_tiling++;
			if (cur_tiling>=maxtiling) cur_tiling=0;
			newtiling=GetBuiltinTiling(cur_tiling);
		} while (!newtiling);

		SetTiling(newtiling);

		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Previous_Tiling) {
		Tiling *newtiling=NULL;
		int maxtiling=NumBuiltinTilings();
		do {
			cur_tiling--;
			if (cur_tiling<0) cur_tiling=maxtiling-1;
			newtiling=GetBuiltinTiling(cur_tiling);
		} while (!newtiling);

		SetTiling(newtiling);

		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Toggle_Lines) {
		trace_cells=!trace_cells;
		if (active) Render();
		PostMessage(trace_cells ? _("Include cell outlines") : _("Don't include cells"));
		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Toggle_Render) {
		ToggleActivated();
		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Toggle_Preview) {
		TogglePreview();
		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Toggle_Orientations) {
		ToggleOrientations();
		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Next_Basecell) {
		if (current_base<0) current_base=0; else current_base--;
		if (current_base<0) current_base=tiling->basecells.n-1;
		DBG cerr <<" ***** current_base: "<<current_base<<endl;
		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Previous_Basecell) {
		if (current_base<0) current_base=0; else current_base++;
		if (current_base>=tiling->basecells.n) current_base=0;
		DBG cerr <<" ***** current_base: "<<current_base<<endl;
		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Edit) {
		cerr <<" *** need to implement Clone Tiling Edit!!"<<endl;
		return 0;

	} else if (action==CLONEIA_Select) {
		Mode(CMODE_Select);
		return 0;
	}

	return 1;
}

int CloneInterface::InterfaceOn()
{
	base_cells.setIdentity();
	if (active) Render();

	needtodraw=1;
	return 0;
}

int CloneInterface::InterfaceOff()
{
	if (previewoc) {
		delete previewoc;
		previewoc=NULL;
	}

	sources.Flush();
	source_proxies.Flush();
	if (lines) lines->flush();

	active=false;

	needtodraw=1;
	return 0;
}

enum CloneMenuItems {
	CLONEM_Clear_Base_Objects,
	CLONEM_Include_Lines,
	CLONEM_Load,
	CLONEM_Save
};

Laxkit::MenuInfo *CloneInterface::ContextMenu(int x,int y,int deviceid)
{
    MenuInfo *menu=new MenuInfo(_("Clone Interface"));

    menu->AddItem(_("Include lines"),      CLONEM_Include_Lines, LAX_ISTOGGLE|(trace_cells?LAX_CHECKED:0));
    menu->AddItem(_("Clear current base objects"), CLONEM_Clear_Base_Objects);
    menu->AddSep();
    menu->AddItem(_("Load resource"), CLONEM_Load);
    menu->AddItem(_("Save as resource"), CLONEM_Save);

    return menu;
}

int CloneInterface::Event(const Laxkit::EventData *e,const char *mes)
{
    if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
        int i =s->info2; //id of menu item
        //int ii=s->info4; //extra id, 1 for direction

        if (i==CLONEM_Clear_Base_Objects) {
			if (current_base<0) current_base=0;

			for (int c=0; c<source_proxies.n(); c++) {
				if (source_proxies.e_info(c)==current_base) {
					source_proxies.Remove(c);
					sources.Remove(c);
					c--;
				}
			}

			if (child) RemoveChild();
			if (active) ToggleActivated();
            return 0;

		} else if (i==CLONEM_Include_Lines) {
			PerformAction(CLONEIA_Toggle_Lines);
			return 0;

		} else if (i==CLONEM_Load) {
			app->rundialog(new FileDialog(NULL,"Load tiling",_("Load tiling"),ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
						                  object_id,"load",FILES_OPEN_ONE));
	        return 0;

		} else if (i==CLONEM_Save) {
			app->rundialog(new FileDialog(NULL,"Save tiling",_("Save tiling"),ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
						                  object_id,"save",FILES_SAVE));
	        return 0;
		}

		return 0;

	} else if (!strcmp(mes,"load")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (isblank(s->str)) return 0;

		Attribute att;
		att.dump_in(s->str);
		Tiling *ntiling=new Tiling;
		ntiling->dump_in_atts(&att,0,NULL);
		SetTiling(ntiling);

		return 0;

	} else if (!strcmp(mes,"save")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (isblank(s->str)) return 0;
		FILE *f=fopen(s->str,"w");
		if (!f) return 0;
		fprintf(f,"#Laidout %s Tiling\n\n", LAIDOUT_VERSION);
		tiling->dump_out(f,0,0,NULL);
		fclose(f);
		return 0;
	}

	return 1;
}

int CloneInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
	if (dynamic_cast<LineStyle*>(ndata)) {
		if (current_base<0) return 0;

		LineStyle *l=dynamic_cast<LineStyle*>(ndata);
		PathsData *p=dynamic_cast<PathsData*>(base_cells.e(current_base));
		p->line(-1,-1,-1, &l->color);
		return 1;
	}
	return 0;
}

void CloneInterface::DrawSelected()
{
	if (!source_proxies.n()) return;

	ObjectContext *toc;
	int bcell;
	PathsData *bpath;
	DrawableObject *data;
	Affine a;

	for (int c=0; c<source_proxies.n(); c++) {
		bcell=source_proxies.e_info(c);
		bpath=dynamic_cast<PathsData*>(base_cells.e(bcell));

		toc=source_proxies.e(c);
		data=dynamic_cast<DrawableObject*>(toc->obj);
		a=data->GetTransformToContext(false,0);
		dp->PushAndNewTransform(a.m());

		dp->NewFG(&bpath->linestyle->color);
		dp->LineAttributes((bcell==current_base?3:1),LineSolid,LAXCAP_Round,LAXJOIN_Round);

		 //draw corners just outside bounding box
		double o=5/dp->Getmag(), //5 pixels outside, 15 pixels long
			   ow=(data->maxx-data->minx)/15,
			   oh=(data->maxy-data->miny)/15;
		dp->drawline(data->minx-o,data->miny-o, data->minx+ow,data->miny-o);
		dp->drawline(data->minx-o,data->miny-o, data->minx-o,data->miny+oh);
		dp->drawline(data->minx-o,data->maxy+o, data->minx-o,data->maxy-oh);
		dp->drawline(data->minx-o,data->maxy+o, data->minx+ow,data->maxy+o);
		dp->drawline(data->maxx+o,data->maxy+o, data->maxx-ow,data->maxy+o);
		dp->drawline(data->maxx+o,data->maxy+o, data->maxx+o,data->maxy-oh);
		dp->drawline(data->maxx+o,data->miny-o, data->maxx-ow,data->miny-o);
		dp->drawline(data->maxx+o,data->miny-o, data->maxx+o,data->miny+oh);

		Laidout::DrawDataStraight(dp, data, NULL,NULL);

		dp->PopAxes();
	}
}

void CloneInterface::RefreshSelectMode()
{
	double th=dp->textheight();
	double pad=th;

	dp->LineAttributes(1,LineSolid,CapButt,JoinMiter);
	dp->NewBG(bg_color);
	dp->NewFG(fg_color);
	double boxh=num_rows*(icon_width*1.2)+icon_width*.2;
	double boxw=num_cols*(icon_width*1.2)-icon_width*.2;
	dp->drawrectangle(dp->Minx+pad,selected_offset+(dp->Miny+dp->Maxy)/2-boxh/2, dp->Maxx-dp->Minx-2*pad, boxh, 2);

	int builtin=NumBuiltinTilings();

	double cell_pad=icon_width*.2;
	
	double x=(dp->Maxx+dp->Minx)/2-boxw/2;
	double y=selected_offset+(dp->Miny+dp->Maxy)/2-boxh/2+cell_pad;
	double w,h;
	char *f=NULL;
	int i;
	LaxImage *img;
	for (int c=0; c<builtin; c++) {
		if (c>0 && c%num_cols==0) {
			x=(dp->Maxx+dp->Minx)/2-boxw/2;
			y+=cell_pad+icon_width;
		}
		i=c;
		f=GetBuiltinIconKey(i);
		img=laidout->icons.GetIcon(f);
		delete[] f;
		if (!img) continue;

		if (current_selected==c) {
			dp->NewFG(hbg_color);
			dp->drawrectangle(x-cell_pad+1,y-cell_pad+1, icon_width+2*cell_pad-2,icon_width+2*cell_pad-2, 1);
			dp->NewFG(fg_color);
			//const char *mid=strstr("/");
			////dp->textout(x+icon_width/2,y-th*.1, BuiltinTiling[c],-1, LAX_HCENTER|LAX_BOTTOM);
			//dp->textout(x+icon_width/2,y-th*.1, BuiltinTiling[c],mid-BuiltinTiling[c], LAX_HCENTER|LAX_BOTTOM);
			//dp->textout(x+icon_width/2,y-th*.1, mid+1,-1, LAX_HCENTER|LAX_BOTTOM);
		}

		w=icon_width;
		h=w*img->h()/img->w();
		if (h>w) {
			h=icon_width;
			w=h*img->w()/img->h();
		}
		flatpoint offset=flatpoint(icon_width/2-w/2, icon_width/2-h/2);
		dp->imageout(img, x+offset.x,y+offset.y+h, w,-h);

		x+=cell_pad+icon_width;
	}
}

int CloneInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	if (mode==CMODE_Select) {
		dp->DrawScreen();
		RefreshSelectMode();
		dp->DrawReal();
		return 0;
	}

	double circle_radius=INTERFACE_CIRCLE*uiscale;

	if (firsttime==1) {
		firsttime=0;
		box.minx=10;
		box.maxx=10+4*circle_radius*uiscale;
		box.miny=10;
		box.maxy=10+5*circle_radius*uiscale + 2*dp->textheight();
	} else if (firsttime==2) {
		 //remap control box size only, leave in same place
		firsttime=0;
		box.maxx=box.minx+4*circle_radius*uiscale;
		box.maxy=box.miny+5*circle_radius*uiscale + 2*dp->textheight();
	}




	 //preview lines when either not active, or active and not rendering lines
	if (lines->n() && preview_lines && (!active || (active && !trace_cells))) {
		dp->PushAndNewTransform(lines->m());
		for (int c=0; c<lines->n(); c++) {
			Laidout::DrawData(dp, lines->e(c), NULL,NULL);
		}
		dp->PopAxes();
	}


	 //draw indicators around source objects
	DrawSelected();


	 //draw default unit bounds and tiling region bounds
	if (boundary) {
		Laidout::DrawData(dp, boundary, NULL,NULL);
	}


	 //draw base cells
	dp->PushAndNewTransform(base_cells.m());
	PathsData *pd;
	for (int c=base_cells.n()-1; c>=0; c--) {
		Laidout::DrawData(dp, base_cells.e(c), NULL,NULL);

		if (preview_orient) {
			DrawableObject *o=dynamic_cast<DrawableObject*>(base_cells.e(c));
			flatpoint center=o->ReferencePoint(LAX_MIDDLE,true);
			flatpoint v=o->ReferencePoint(LAX_TOP_MIDDLE,true)-center;
			v*=.4;

			int bc=o->properties.findInt("base");
			pd=dynamic_cast<PathsData*>(base_cells.e(bc));
			dp->NewFG(&pd->linestyle->color);
			dp->drawline(center-v,center+v);
			dp->drawline(center+v,center+transpose(v)/2);
		}
	}
	if (current_base>=0) {
		 //make active base cell bolder
		PathsData *p=dynamic_cast<PathsData*>(base_cells.e(current_base));
		if (p) {
			p->linestyle->width=4;
			Laidout::DrawData(dp, p, NULL,NULL);
			p->linestyle->width=2;
		}
	}
	dp->PopAxes();


	// //draw rect stuff after clone handles, but before the control box
	//if (rectinterface) { rectinterface->needtodraw=1; rectinterface->Refresh(); }


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
			//dp->imageout(tiling->icon, offset.x+box.minx+pad,offset.y+box.miny+pad, w,h);
			dp->imageout(tiling->icon, offset.x+box.minx+pad,offset.y+box.miny+pad+h, w,-h);
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




	dp->DrawReal();

	return 0;
}

int CloneInterface::scan(int x,int y, int *i)
{
	flatpoint cc((box.minx+box.maxx)/2,box.maxy);
	double circle_radius=INTERFACE_CIRCLE*uiscale;

	if (norm((cc-flatpoint(x,y)))<circle_radius) return CLONEI_Circle;

	 //check for things related to the tiling selector
	if (box.boxcontains(x,y)) {
		double pad=(box.maxx-box.minx)*.15;
		if (x>box.minx+pad && x<box.maxx-pad && y>box.miny+pad && y<box.maxy-pad)
			return CLONEI_Tiling;
		return CLONEI_Box;
	}


	 //check for being inside a proxy image
	flatpoint fp=dp->screentoreal(x,y);
	flatpoint p;
	ObjectContext *oc;
	DBG cerr <<" ----- fpoint: "<<fp.x<<','<<fp.y<<endl;
	for (int c=0; c<source_proxies.n(); c++) {
		oc=source_proxies.e(c);

		DBG cerr <<" ----- source "<<c<<"/"<<source_proxies.n()<<" bbox: "<<oc->obj->minx<<"  "<<oc->obj->miny<<"  "<<oc->obj->maxx<<"  "<<oc->obj->maxy<<endl;
		DBG p=transform_point_inverse(oc->obj->m(), fp);
		DBG cerr <<" ----- point: "<<p.x<<','<<p.y<<endl;

		if (oc->obj->pointin(fp,1)) {
			if (i) *i=c;
			return CLONEI_Source_Object;
		}
	}


	 //check for inside base cell outlines
	p=transform_point_inverse(base_cells.m(),fp);
	for (int c=0; c<tiling->basecells.n; c++) {
		DBG cerr <<" ----- base cell "<<c<<"/"<<base_cells.n()<<" bbox: "<<base_cells.e(c)->minx<<"  "<<base_cells.e(c)->miny<<"  "<<base_cells.e(c)->maxx<<"  "<<base_cells.e(c)->maxy<<endl;
		DBG cerr <<" ----- point: "<<p.x<<','<<p.y<<endl;
		if (!base_cells.e(c)->pointin(p,1)) continue;
		if (i) *i=c;
		return CLONEI_BaseCell;
	}

	 //check for inside boundary
	if (boundary && boundary->pointin(fp,1)) return CLONEI_Boundary;


	return CLONEI_None;
}

int CloneInterface::scanSelected(int x,int y)
{
	//double th=dp->textheight();
	//double pad=th;
	double boxh=num_rows*(icon_width*1.2)+icon_width*.2;
	double boxw=num_cols*(icon_width*1.2);
	int row,col;

	x-=(dp->Maxx+dp->Minx)/2-boxw/2;
	col=x/icon_width/1.2;

	y+=(dp->Maxy+dp->Miny)/2-boxh/2-selected_offset;
	row=y/icon_width/1.2;

	DBG cerr <<" scanSelected r,c: "<<row<<','<<col<<endl;

	int i=-1;
	if (col>=0 && col<num_cols && row>=0 && row<num_rows) {
		i=col+row*num_cols;
		DBG cerr <<"   i="<<i<<endl;
		if  (i>=0 && i<NumBuiltinTilings()) {
			//found one
		} else i=-1;
	}

	return i;
}

int CloneInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;

	if (mode==CMODE_Select) {
		//Mode(CMODE_Normal);
		int item=scanSelected(x,y);
		buttondown.down(d->id,LEFTBUTTON, x,y, item);
		return 0;
	}

	int i=-1;
	int over=scan(x,y,&i);

	 //on control box
	if (over==CLONEI_Tiling || over==CLONEI_Box) {
		buttondown.down(d->id,LEFTBUTTON,x,y, over,state);
		return 0;
	}

	if (over==CLONEI_BaseCell) {
		if (!child) {
			RectInterface *rect=new RectInterface(0,dp);
			rect->style|= RECT_CANTCREATE | RECT_OBJECT_SHUNT;
			rect->owner=this;
			rect->UseThis(&base_cells,0);
			child=rect;
			AddChild(rect,0,1);
		} else {
			child->UseThis(&base_cells,0);
			return 1;
		}
		dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
		PostMessage(_("Edit base cell placements"));
		return 0;
	}

	if (over==CLONEI_Boundary) {
		if (boundary) {
			if (!child) {
				RectInterface *rect=new RectInterface(0,dp);
				rect->style|= RECT_CANTCREATE | RECT_OBJECT_SHUNT;
				rect->owner=this;
				rect->UseThis(boundary,0);
				child=rect;
				AddChild(rect,0,1);
			} else {
				child->UseThis(boundary,0);
				return 1;
			}
			dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
			PostMessage(_("Edit boundary"));
			return 0;
		}
		over=CLONEI_None;
	}

	if (over==CLONEI_Source_Object) {
		ObjectContext *oc=source_proxies.e(i);
		if (!child) {
			RectInterface *rect=new RectInterface(0,dp);
			rect->style|= RECT_CANTCREATE | RECT_OBJECT_SHUNT;
			rect->owner=this;
			rect->UseThis(oc->obj,0);
			child=rect;
			AddChild(rect,0,1);
		} else {
			child->UseThis(oc->obj,0);
		}
		dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
		PostMessage(_("Move source object"));
		return 0;
	}

	if (over==CLONEI_None) {
		 //maybe add or remove an object for a particular base cell
		SomeData *obj=NULL;
		ObjectContext *oc=NULL;
		int c=viewport->FindObject(x,y,NULL,NULL,1,&oc);
		if (c>0) obj=oc->obj;

		if (obj) {
			SomeData *o=obj;
			while (o) {
				if (o==preview) return 0;
				o=o->GetParent();
			}

			//if (sources.FindIndex(oc)>=0) return 0;

			if (current_base<0) current_base=0;
			sources.Add(oc,-1,current_base);

			 //set up proxy object
			VObjContext *noc=dynamic_cast<VObjContext*>(oc->duplicate());
			noc->clearToPage();
			SomeDataRef *ref=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
			ref->Set(obj, false);
			ref->flags|=SOMEDATA_KEEP_ASPECT;
			double m[6];
			viewport->transformToContext(m,oc,0,1);
			ref->m(m);
			noc->SetObject(ref);
			source_proxies.Add(noc,-1,current_base);
			delete noc;

			//if (child) RemoveChild();
			if (!child) {
				RectInterface *rect=new RectInterface(0,dp);
				rect->style|= RECT_CANTCREATE | RECT_OBJECT_SHUNT;
				rect->owner=this;
				rect->UseThis(ref,0);
				child=rect;
				AddChild(rect,0,1);
			} else {
				child->UseThis(ref,0);
				return 1;
			}

			dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
			if (active) Render();

			needtodraw=1;
			return 0;
		}

		return 1;
	}

	if (child) RemoveChild();

	 // else click down on something for overlay
	buttondown.down(d->id,LEFTBUTTON,x,y, over,state);

	return 0;
}

int CloneInterface::Render()
{
	if (!tiling) return 1;

	preview->flush(); //remove old clones

//	 //create array of offsets for base cells. This is used in conjunction
//	 //with the source objects
//	Affine bcellst[tiling->basecells.n];
//	for (int c=0; c<tiling->basecells.n; c++) {
//		bcellst[c]=base_cells;
//	}

	 //render lines
	Group *ret=NULL;
	if (trace_cells || preview_lines) {
		lines->flush();
		ret=tiling->Render(lines, NULL, &base_cells, 0,3, 0,3, boundary, &base_cells);
		if (!ret) {
			PostMessage(_("Could not clone!"));
			return 0;
		} else lines->FindBBox();
	}

	 //render clones
	ret=tiling->Render(preview, &source_proxies, &base_cells, 0,3, 0,3, boundary, &base_cells);
	if (!ret) {
		PostMessage(_("Could not clone!"));
		return 0;
	} else {
		preview->FindBBox();

		 //when there are source objects, the lines are not attached, so we need to add
		 //manually
		if (trace_cells && source_proxies.n()) {
			//SomeData *nlines=lines->duplicate(NULL);
			//preview->push(nlines);
			//nlines->dec_count();
			//-----
			ret=tiling->Render(preview, NULL, &base_cells, 0,3, 0,3, boundary, &base_cells);
		}
	}


	if (active) {
		 //make sure preview is installed in the target context
		if (!previewoc) {
			LaidoutViewport *vp=dynamic_cast<LaidoutViewport*>(viewport);
			
			ObjectContext *noc=NULL;
			if (sources.n()) {
				ObjectContext *toc=sources.e(0);
				noc=toc->duplicate();
				previewoc=dynamic_cast<VObjContext*>(noc);
				noc=NULL;
			} else {
				previewoc=dynamic_cast<VObjContext*>(vp->curobj.duplicate());
			}
			previewoc->SetObject(NULL);

			while (previewoc->context.n()>4) previewoc->pop(); //make context be on the page

			vp->ChangeContext(previewoc);
			delete previewoc; previewoc=NULL;
			vp->NewData(preview,&noc);
			noc=noc->duplicate();
			previewoc=dynamic_cast<VObjContext*>(noc);
		}
	} else {
		// *** note: if !active, shouldn't ever be here, removed in ToggleActivated()

		 //make sure preview is NOT in document tree
		if (previewoc) {
			LaidoutViewport *vp=dynamic_cast<LaidoutViewport*>(viewport);
			vp->DeleteObject(previewoc);
			delete previewoc;
			previewoc=NULL;
		}
	}

	needtodraw=1;
	return 0;
}

int CloneInterface::ToggleOrientations()
{
	preview_orient=!preview_orient;
	needtodraw=1;

	PostMessage(preview_orient ? _("Preview cell orientation") : _("Don't preview cell orientation"));
	return preview_orient;
}

int CloneInterface::TogglePreview()
{
	preview_lines=!preview_lines;
	needtodraw=1;

	PostMessage(preview_lines ? _("Preview lines") : _("Don't preview lines"));
	return preview_lines;
}

/*! Returns whether active after toggling.
 */
int CloneInterface::ToggleActivated()
{
	if (!tiling) { active=false; return 0; }

	active=!active;
	
	if (active) {
		Render();

	} else {
		 //remove from document if it is there
		if (previewoc) {
			LaidoutViewport *vp=dynamic_cast<LaidoutViewport*>(viewport);
			vp->DeleteObject(previewoc);
			delete previewoc;
			previewoc=NULL;
		}
	}
	
	needtodraw=1;
	return active;
}

int CloneInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	if (mode==CMODE_Select) {
		int item=-1;
		int dragged=buttondown.up(d->id,LEFTBUTTON, &item);
		if (!dragged) {
			if (item>=0) {
				Tiling *newtiling=GetBuiltinTiling(current_selected);
				if (newtiling) {
					cur_tiling=item;
					SetTiling(newtiling);
				}
			}
			Mode(CMODE_Normal);
		} else {
			double boxh=num_rows*(icon_width*1.2)+icon_width*.2;
			if (selected_offset+(dp->Maxy+dp->Miny)/2+boxh/2 < dp->Miny ||
				selected_offset+(dp->Maxy+dp->Miny)/2-boxh/2 > dp->Maxy) {
				Mode(CMODE_Normal);
			}
		}
		return 0;
	}


	int firstover=CLONEI_None;
	//int dragged=
	buttondown.up(d->id,LEFTBUTTON, &firstover);

	int i=-1;
	int over=scan(x,y,&i);

	//DBG flatpoint fp=dp->screentoreal(x,y);

	if (over==firstover) {
		if (over==CLONEI_Circle) {
			ToggleActivated();
			needtodraw=1;
			return 0;

		} else if (over==CLONEI_Tiling) {
			// *** switch to select tiling mode
			Mode(CMODE_Select);
			return 0;
		}
	}

	return 0;
}

int CloneInterface::Mode(int newmode)
{
	if (newmode==mode) return mode;

	if (newmode==CMODE_Normal) {
		mode=newmode;
		needtodraw=1;

	} else if (newmode==CMODE_Select) {
		int builtin=NumBuiltinTilings();

		double th=dp->textheight();
		double pad=th;
		icon_width=(box.maxx-box.minx)*.8;
		num_cols=(dp->Maxx-dp->Minx-2*pad-icon_width*.2)/(icon_width*1.2);
		num_rows=builtin/num_cols+1;
		selected_offset=0;
		current_selected=cur_tiling;
		mode=newmode;
		needtodraw=1;
	}

	return mode;
}

int CloneInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	if (mode==CMODE_Select) {
		if (buttondown.any()) {
			int ly;
			buttondown.move(mouse->id,x,y, NULL,&ly);
			selected_offset+=y-ly;
			needtodraw=1;
		} else {
			int i=scanSelected(x,y);
			if (i>=0 && i!=current_selected) {
				current_selected=i;
				PostMessage(BuiltinTiling[current_selected]);
				needtodraw=1;
			}
		}
		return 0;
	}


	//--------CMODE_Normal:

	int i=-1;
	int over=scan(x,y,&i);
	DBG cerr <<"over box: "<<over<<endl;

	if (!buttondown.any()) {
		if (lastover!=over) needtodraw=1;
		lastover=over;
		if (lastover==CLONEI_None) PostMessage(" ");
		return 0;
	}

	//if (rectinterface.buttondown.any()) return rectinterface.MouseMove(x,y,state,mouse);


	 //button is down on something...
	int lx,ly, oldover=CLONEI_None;
	buttondown.move(mouse->id,x,y, &lx,&ly);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &oldover);

	if (oldover==CLONEI_BaseCell) {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			 //scale
			int ix,iy;
			buttondown.getinitial(mouse->id,LEFTBUTTON, &ix,&iy);

			flatpoint op=dp->screentoreal(ix,iy);
			base_cells.Scale(op, (x-lx)>0?1.05:1/1.05);

		} else if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) {
			 //rotate
			int ix,iy;
			buttondown.getinitial(mouse->id,LEFTBUTTON, &ix,&iy);

			flatpoint op=dp->screentoreal(ix,iy);
			double angle=(x-lx)*M_PI/180;
			base_cells.Rotate(angle,op);
		} else base_cells.origin(base_cells.origin()+dp->screentoreal(x,y)-dp->screentoreal(lx,ly));
		needtodraw=1;
		return 0;
	}

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
	if (mode==CMODE_Select) {
		selected_offset+=(dp->Maxy-dp->Miny)/10;
		double boxh=num_rows*(icon_width*1.2)+icon_width*.2;
		if (selected_offset+(dp->Maxy+dp->Miny)/2-boxh/2 > dp->Maxy) {
			Mode(CMODE_Normal);
		}
		needtodraw=1;
		return 0;
	}


	int over=scan(x,y,NULL);
	DBG cerr <<"wheel up clone interface: "<<over<<endl;

	if (over==CLONEI_Tiling) {
		PerformAction(CLONEIA_Previous_Tiling);
		return 0;

	} else if (over==CLONEI_Box && (state&ControlMask)) {
		uiscale*=1.1;
		firsttime=2;
		needtodraw=1;
		return 0;
	}

	return 1;
}

int CloneInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (mode==CMODE_Select) {
		selected_offset-=(dp->Maxy-dp->Miny)/10;

		double boxh=num_rows*(icon_width*1.2)+icon_width*.2;
		if (selected_offset+(dp->Maxy+dp->Miny)/2+boxh/2 < dp->Miny) {
			Mode(CMODE_Normal);
		}
		needtodraw=1;
		return 0;
	}


	int over=scan(x,y,NULL);
	DBG cerr <<"wheel down clone interface: "<<over<<endl;

	if (over==CLONEI_Tiling) {
		PerformAction(CLONEIA_Next_Tiling);
		return 0;

	} else if (over==CLONEI_Box && (state&ControlMask)) {
		uiscale*=.9;
		firsttime=2;
		needtodraw=1;
		return 0;
	}

	return 1;
}

int CloneInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;
	
	if (mode==CMODE_Select) {
		int found=0;
		if (ch==LAX_Esc) {
			Mode(CMODE_Normal);
			return 0;

		} else if (ch==LAX_Enter) {
			Tiling *newtiling=GetBuiltinTiling(current_selected);
			if (newtiling) {
				cur_tiling=current_selected;
				SetTiling(newtiling);
				Mode(CMODE_Normal);
			}
			return 0;

		} else if (ch==LAX_Down) {
			current_selected+=num_cols;
			current_selected%=(num_cols*num_rows);
			if (current_selected>=NumBuiltinTilings()) current_selected=NumBuiltinTilings()-1;
			found=1;

		} else if (ch==LAX_Up) {
			current_selected-=num_cols;
			if (current_selected<0) current_selected+=(num_cols*num_rows);
			if (current_selected>=NumBuiltinTilings()) current_selected=NumBuiltinTilings()-1;
			found=1;

		} else if (ch==LAX_Left) {
			current_selected--;
			if (current_selected<0) current_selected+=(num_cols*num_rows);
			if (current_selected>=NumBuiltinTilings()) current_selected=NumBuiltinTilings()-1;
			found=1;

		} else if (ch==LAX_Right) {
			current_selected++;
			if (current_selected>=NumBuiltinTilings()) current_selected=0;
			found=1;
		}
		if (found) {
			PostMessage(BuiltinTiling[current_selected]);
			needtodraw=1;
			return 0;
		}
		return 1;
	}

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

//this is supposed to update down point for drag/scale/rotate
//	if (ch==LAX_Shift || ch==LAX_Control || ch==LAX_Alt || ch==LAX_Meta) {
//		if (buttondown.any(0,LEFTBUTTON)) {
//			int id=buttondown.whichdown(-1,LEFTBUTTON);
//			int x,y, lx,ly;
//			buttondown.getinfo(id,LEFTBUTTON, NULL,NULL, &lx,&ly, &x,&y);
//			buttondown.up(id,LEFTBUTTON);
//			buttondown.down(id,LEFTBUTTON, lx,ly);
//			buttondown.move(id, x,y);
//
//		}
//	}

	if (ch==LAX_Esc) {
		if (child) {
			RemoveChild();
			needtodraw=1;
			return 0;
		}

		if (inrect) {
			inrect=false;
			needtodraw=1;
			return 0;
		}

		if (source_proxies.n()) {
			sources.Flush();
			source_proxies.Flush();

			if (active) {
				active=false;
				preview->dec_count();
				preview=new Group;
				char *str=make_id("tiling");
				preview->Id(str);
				delete[] str;
			}

			if (previewoc) {
				delete previewoc;
				previewoc=NULL;
			}

			needtodraw=1;
			return 0;
		}
	}

	return 1;
}



} //namespace Laidout

