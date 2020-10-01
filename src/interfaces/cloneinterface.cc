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
// Copyright (C) 2013-2015, 2017 by Tom Lechner
//

#include "cloneinterface.h"
#include "../version.h"
#include "../laidout.h"
#include "../core/drawdata.h"
#include "../ui/viewwindow.h"

#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/filedialog.h>
#include <lax/interfaces/somedataref.h>
#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/rectinterface.h>

//template implementation:
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
 * You can define your own custom "dimensions" (ToDO), each of which may get
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
	radial_divisions = 0; //this is a hint for the interface. Tiling itself does not autoupdate in response

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

/*! Use laidout->icons->GetIcon() to search for "category__name".
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

	icon=laidout->icons->GetIcon(ifile);
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

/*! Returns how many TilingDest objects have recursion. This many things will be
 * inputs under the normal control box.
 */
int Tiling::HasRecursion()
{
	int n=0;
	for (int c=0; c<basecells.n; c++) {
		for (int c2=0; c2<basecells.e[c]->transforms.n; c2++) {
			if (basecells.e[c]->transforms.e[c2]->max_iterations > 1) n++;
		}		
	}

	return n;
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

	
LaxFiles::Attribute *Tiling::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
    if (what==-1) {
		if (!att) att = new Attribute();

        att->push("name",     "Blah  #optional human readable name");
        att->push("category", "Blah  #optional human readable category name");
		return att;
	}

	// ***
	return NULL;
}

ObjectDef *Tiling::makeObjectDef()
{
	cerr << " *** need to implement Tiling::makeObjectDef()!!"<<endl;
	return NULL;
}

void Tiling::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

    if (what==-1) {
        fprintf(f,"%sname Blah          #optional human readable name\n",spc);
        fprintf(f,"%scategory Blah      #optional human readable category name\n",spc);
		fprintf(f,"%srepeatable no      #or x, y, \"x y\", whether to allow repeat in overall p1\n",spc);
		fprintf(f,"%srepeat_origin 1,1  #vector for origin of overall p1 arrangement\n",spc);
		fprintf(f,"%srepeat_x 1,0       #vector for x axis of overall p1 arrangement\n",spc);
		fprintf(f,"%srepeat_x 0,1       #vector for y axis of overall p1 arrangement\n",spc);
		fprintf(f,"%sradial_divisions 5 #Hint for how many slices to cut a cirlce into\n",spc);
		fprintf(f,"%sbasecell           #one or more of these, the guts of the tiling\n",spc);
		fprintf(f,"%s  shearable        #whether to allow shearing of this cell\n",spc);
		fprintf(f,"%s  flexible         #whether the cell can change aspect ratio without breaking things\n",spc);
		fprintf(f,"%s  outline          #path for this cell\n",spc);
		fprintf(f,"%s    ...\n",spc);
		fprintf(f,"%s  transform matrix(1,0,0,1,0,0)  #simple placement of outline to a certain orientation\n",spc);
		fprintf(f,"%s  clone            #recursive placement of outline clones\n",spc);
		fprintf(f,"%s    transform matrix(1,0,0,1,0,0)  #matrix for this placement\n",spc);
		fprintf(f,"%s    traceable      #Whether to render lines for this clone. Sometimes it is just a node for further cloning\n",spc);
		fprintf(f,"%s    iterations 22  #How many times to repeat this transform with outline\n",spc);
		fprintf(f,"%s    max_size       #(todo) Max size of clones after which repeating stops\n",spc);
		fprintf(f,"%s    min_size       #(todo) Min size of clones below which repeating stops\n",spc);
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

	if (radial_divisions>0) fprintf(f, "%sradial_divisions %d\n",spc,radial_divisions);
	if (properties.n()) {
		fprintf(f, "%sproperties\n", spc);
		properties.dump_out(f, indent+2, what, context);
		//for (int c=0; c< properties.n(); c++) {
		//	fprintf(f, "%s  %s\n", spc, properties.key(c)); 
		//}
	}

	if (required_interface != "") fprintf(f, "%srequired_interface %s\n", spc, required_interface.c_str());

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
				 //simple duplication
				const double *m=dest->transform.m();
				fprintf(f,"%s  transform matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\n",
						spc, m[0],m[1],m[2],m[3],m[4],m[5]);
			} else {
				 //assume recursive
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

void Tiling::dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context)
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
			LaxImage *nicon = ImageLoader::LoadImage(value);
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

        } else if (!strcmp(name,"properties")) {
			properties.dump_in_atts(att->attributes.e[c], flag, context);

        } else if (!strcmp(name,"radial_divisions")) {
			IntAttribute(value, &radial_divisions);
			if (radial_divisions < 0) radial_divisions = 0;

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
					op->celloutline=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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


//! Create tiled clones, EITHER trace the lines, OR clone sourceobjects.
/*! Install new objects as kids of parent_space. If NULL, create and return a new Group (else return parent_space).
 *
 * If source_objects is NULL, or has no objects, then create path outline objects from the transformed base cells instead.
 * If source_objects is not NULL and has objects, then render clones of the contents, and do NOT render base cell outlines.
 *
 * Install in parent_space. If parent_space==NULL, then return a new Group.
 *
 * If base_lines!=NULL, assume it is structured 1 group per tiling->basecells, and each of those groups contains
 * however many tiling->basecells->transforms there are.
 */
Group *Tiling::Render(Group *parent_space,
					   Group *source_objects, //!< If non-null, clone these. Each->property["tilingSource"] is the source base index
					   Affine *base_offsetm,  //!< Additional offset to place basecells from source_objects
					   Group *base_lines, //!< Optional base cells. If null, then create copies of tiling's default.
					   int p1_minx, int p1_maxx, int p1_miny, int p1_maxy,
					   LaxInterfaces::PathsData *boundary, //!< only render cells approximately within this
					   Affine *final_orient   //!< final transform to apply to clones
					 )
{
	bool trace_cells = (source_objects==NULL || (source_objects!=NULL && source_objects->n()==0));

	if (!parent_space) parent_space = new Group;


	 //figure out the maximum bounds of the render area that covers boundary
	if (p1_maxx<p1_minx)  p1_maxx = p1_minx;
	if (p1_maxy<p1_miny)  p1_maxy = p1_miny;
	if (!isXRepeatable()) p1_maxx = p1_minx;
	if (!isYRepeatable()) p1_maxy = p1_miny;
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
			pp = boundary->transformPoint(p->p());
			if (base_offsetm) pp = base_offsetm->transformPointInverse(pp);
			pp = repeattransform.transformPointInverse(pp);
			points.push(pp);
			bounds.addtobounds(pp);
			p = p->next;
		} while (p && p!=start);

		p1_minx = bounds.minx-.5;
		p1_miny = bounds.miny-.5;
		p1_maxx = bounds.maxx-.5;
		p1_maxy = bounds.maxy-.5;

	} else {
		points.push(flatpoint(p1_minx,p1_miny));
		points.push(flatpoint(p1_maxx,p1_miny));
		points.push(flatpoint(p1_maxx,p1_maxy));
		points.push(flatpoint(p1_minx,p1_maxy));
	}
	if (!isXRepeatable()) { p1_minx=p1_maxx=(p1_minx+p1_maxx)/2; }
	if (!isYRepeatable()) { p1_miny=p1_maxy=(p1_miny+p1_maxy)/2; }

	
	 //cache transform of source objects to base objects, if any
	Affine *sourcem =NULL;  //matrices of source objects
	Affine *sourcemi=NULL; //inverses of sourcem
	SomeData *base  =NULL;

	if (!trace_cells) { //we have source objects, cache some transforms
		Affine a;

		sourcem =new Affine[source_objects->n()];
		sourcemi=new Affine[source_objects->n()];

		for (int c=0; c<source_objects->n(); c++) {
			base = source_objects->e(c);

			if (dynamic_cast<DrawableObject*>(base)) 
				a=dynamic_cast<DrawableObject*>(base)->GetTransformToContext(false,0);

			sourcem[c].m(a.m());
			sourcemi[c] = sourcem[c];
			sourcemi[c].Invert();
		}
	}


	 //make outlines of original base cell complex.
	 //Note this is not the actual clones
	Group *trace=NULL;
	if (trace_cells) {
		if (base_lines) {
			trace = base_lines;
			trace->inc_count();

		} else {
			double w = basecells.e[0]->celloutline->MaxDimension()/50;
			LineStyle *ls = new LineStyle();
			ls->width = w;
			ls->capstyle = LAXCAP_Round;
			ls->joinstyle = LAXJOIN_Round;
			ls->Colorf(.5,.5,.5,1.);
			trace = new Group;
			trace->Id("base_cells");
			PathsData *d;

			for (int c=0; c<basecells.n; c++) {
				if (basecells.e[c]->celloutline) {
					d = dynamic_cast<PathsData*>(basecells.e[c]->celloutline->duplicate(NULL));
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
	}


	//SomeDataRef *clone=NULL;
	DrawableObject *obj;
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
		pp.x = x+.5;
		pp.y = y+.5;
		if ((p1_minx!=p1_maxx || p1_miny!=p1_maxy) && !point_is_in(pp, points.e, points.n)) continue;

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
			//	if (boundary.pointin(pp)) continue; ***
			//}

			if (trace_cells) { //the lines only
			  if (dest->traceable) {
				obj = dynamic_cast<DrawableObject*>(trace->e(c));
				if (base_lines == trace) {
					obj = dynamic_cast<DrawableObject*>(obj->e(0));
				}
				InsertClone(parent_space, obj, NULL, NULL, clonet, final_orient);
			  }

			} else { //for each source object in current base cell...
			  for (int s=0; s<source_objects->n(); s++) {
				obj = dynamic_cast<DrawableObject*>(source_objects->e(s));
				if (!obj || obj->properties.findInt("tilingSource") != c) continue;

				InsertClone(parent_space, obj, &sourcem[s], &basecellmi, clonet, final_orient);
			  }
			}

			 // For recursive destinations:
			for (int i=dest->max_iterations-1; i>0; i--) { //note doesn't run when max_itr==1
				clonet.PreMultiply(dest->transform);

				if (trace_cells) {
				  if (dest->traceable) {
					obj = dynamic_cast<DrawableObject*>(trace->e(c));
					if (base_lines == trace) {
						obj = dynamic_cast<DrawableObject*>(obj->e(0));
					}
					InsertClone(parent_space, obj, NULL, NULL, clonet, final_orient);
				  }

				} else { //if source objects..
				  for (int s=0; s<source_objects->n(); s++) {
					obj = dynamic_cast<DrawableObject*>(source_objects->e(s));
					if (!obj || obj->properties.findInt("tilingSource") != c) continue;

					InsertClone(parent_space, obj, &sourcem[s], &basecellmi, clonet, final_orient);
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

/*! Used during Render(), this simplifies insertion of clones to destination group.
 *
 * The clone will have transform: sourcem * basecellmi * clonet * final_orient
 */
void Tiling::InsertClone(Group *parent_space,  //!< clone into here
						 SomeData *object,     //!< the object to clone
						 Affine *sourcem,      //!< use this transform in place of identity
						 Affine *basecellmi,   //!< mapping to get source onto proper place for current base cell
						 Affine &clonet,       //!< current clone transform
						 Affine *final_orient  //!< final transform to apply to clone
						 )
{
	SomeDataRef *clone = dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory()->NewObject("SomeDataRef"));
	
	if (dynamic_cast<SomeDataRef*>(object)) object=dynamic_cast<SomeDataRef*>(object)->GetFinalObject();
	clone->Set(object,1); //the 1 means don't copy matrix also
	clone->FindBBox();
	if (sourcem) clone->m(sourcem->m()); //else starts out as identity

	if (basecellmi) clone->Multiply(*basecellmi);

	clone->Multiply(clonet);

	if (final_orient) clone->Multiply(*final_orient);


	parent_space->push(clone);
	clone->FindBBox();
	clone->dec_count();
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
					 int mirrored,       //!< Each repeated unit is the base plus a mirror of the base about a radius
					 Tiling *oldtiling)  //!< Update this one if not null, else return a new one
{
	if (end_angle==start_angle) end_angle=start_angle+2*M_PI;
	double rotation_angle=(end_angle-start_angle)/num_divisions;
	double cellangle=rotation_angle;
	if (mirrored) cellangle/=2;

	Tiling *tiling;
	if (oldtiling) {
		tiling = oldtiling;
		tiling->radial_divisions = num_divisions;
		tiling->basecells.flush();

	} else {
		tiling=new Tiling(NULL,"Circular");
		tiling->repeatable = 0;
		tiling->radial_divisions = num_divisions;
		tiling->required_interface="radial";
		if (mirrored) makestr(tiling->name,"rm"); else makestr(tiling->name,"r"); 
		tiling->InstallDefaultIcon(); //icon key "Circular__r" or "Circular__rm" 
	}


	 //define cell outline
	PathsData *path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
	double a =start_angle;
	double a2=a+cellangle;
	path->append(start_radius*flatpoint(cos(a),sin(a)));
	path->append(  end_radius*flatpoint(cos(a),sin(a)));

	path->appendBezArc(flatpoint(0,0), cellangle, 1);
	path->append(start_radius*flatpoint(cos(a2),sin(a2)));

	path->appendBezArc(flatpoint(0,0), -cellangle, 1);
	path->close();
	path->FindBBox();


	 //set up a recursive transform limited by num_divisions
	TilingDest *dest = new TilingDest;
	dest->max_iterations = num_divisions;
	dest->conditions = TILING_Iterations;
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



	return tiling;
}

/*! Return CreateRadial(0,0,0,1,5,mirrored).
 */
Tiling *CreateRadialSimple(int mirrored, int num_divisions)
{
	return CreateRadial(0,0,0,1,num_divisions,mirrored, NULL);
}

/*! Update the tiling to use new number of radial divisions.
 * Return 0 for success, or nonzero for error and could not update.
 */
int UpdateRadial(Tiling *tiling, int new_divisions)
{
	if (strcmp(tiling->category, "Circular")) return 1;
	if (new_divisions<1) return 2;

	if (!strcmp(tiling->name, "spiral")) {
		CreateSpiral(0, 4*M_PI, 5,3, new_divisions, tiling);

	} else if (!strcmp(tiling->name, "r")) {
		CreateRadial(0,0,0,1, new_divisions, 0, tiling);

	} else if (!strcmp(tiling->name, "rm")) {
		CreateRadial(0,0,0,1, new_divisions, 1, tiling);

	} else {
		return 3;
	}

	//if (!strcmp(tile,"Circular/r"))  return CreateRadialSimple(0, 10);
	//if (!strcmp(tile,"Circular/rm"))  return CreateRadialSimple(1, 5);

	return 0;
}

Tiling *CreateSpiral(double start_angle, //!< radians
					 double end_angle,   //!< If end==start, or end==start+360, use full circle
					 double start_radius,//!< For purposes of defining a base cell outline
					 double end_radius,  //!< For purposes of defining a base cell outline
					 int num_divisions,  //!< divide total angle into this many sections
					 Tiling *oldtiling   //!< If non-null, then update, don't create a new one
		)
{
	if (end_angle==start_angle) end_angle=start_angle+2*M_PI;
	double rotation_angle = (end_angle-start_angle)/num_divisions;
	double cellangle = rotation_angle;

	Tiling *tiling;
	int iterations = num_divisions;
	if (oldtiling) {
		tiling = oldtiling;
		tiling->radial_divisions = num_divisions;
		iterations = tiling->basecells.e[0]->transforms.e[0]->max_iterations;
		tiling->basecells.flush();

	} else {
		tiling=new Tiling("spiral","Circular");
		tiling->repeatable=0;
		tiling->radial_divisions = num_divisions;
		tiling->required_interface="radial";
		tiling->InstallDefaultIcon(); //icon key "Circular__spiral"
	} 


	//spiral equation: r(n)=r_outer*(r_inner/r_outer)^n, where n is number of winds
	//					   =r_outer*exp(n*ln(r_inner/r_outer))
	// if y=a^x, y'=ln(a)*a^x
	double nsr = start_radius*exp(cellangle/2/M_PI*log(end_radius/start_radius));
	double ner = end_radius  *exp(cellangle/2/M_PI*log(end_radius/start_radius));

	 //define cell outline
	PathsData *path = dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
	double a = start_angle;
	double a2= start_angle + cellangle;
	path->append(  end_radius*flatpoint(cos(a),sin(a)));
	path->append(start_radius*flatpoint(cos(a),sin(a)));

	//if (num_divisions%2==1) {
	//	 //need to have slightly modified base cell
	//	double msr = start_radius*exp(cellangle/2/2/M_PI*log(end_radius/start_radius));
	//	double mer = end_radius  *exp(cellangle/2/2/M_PI*log(end_radius/start_radius));
	//}

	path->append(nsr*flatpoint(cos(a2),sin(a2)));
	path->append(ner*flatpoint(cos(a2),sin(a2)));

	path->close();
	path->FindBBox();


	 //set up a recursive transform limited by num_divisions
	TilingDest *dest = new TilingDest;
	dest->max_iterations = iterations;
	//dest->max_iterations = num_divisions;
	dest->conditions = TILING_Iterations;
	dest->transform.setRotation(-rotation_angle);
	dest->transform.Scale(nsr/start_radius);

	TilingOp *op = tiling->AddBase(path,1,1);
	op->AddTransform(dest);


	return tiling;
}

/*! Create a tiling based on a wallpaper group.
 * If tiling==NULL, then return a new Tiling, else update tiling.
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->appendRect(0,0,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"p2")) {
		 //rotate on edge
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->appendRect(0,0,.5,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->flexible_aspect=true;

		op->AddTransform(affine);

		affine.Flip(flatpoint(.5,0),flatpoint(.5,1));
		op->AddTransform(affine);

	} else if (!strcasecmp(group,"pg")) {
		 //glide reflect
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->appendRect(0,0,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->appendRect(0,0,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		op->AddTransform(affine);

		if (!strcasecmp(coloring,"square 9")) {
			path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->appendRect(0,1,1,1);
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->shearable=true;
		op->flexible_aspect=true;
		affine.setIdentity();
		op->AddTransform(affine);
	
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		Coordinate *cc=CoordinatePolygon(flatpoint(sqrt(3)/2,1), 1, false, 6, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);

		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"hexagonal 2")) {
		tiling->repeatXDir(flatpoint(3*sqrt(3)/2,1.5));
		tiling->repeatYDir(flatpoint(0,3));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		Coordinate *cc=CoordinatePolygon(flatpoint(sqrt(3)/2,1), 1, false, 6, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);
		affine.Translate(flatpoint(sqrt(3),0));
		op->AddTransform(affine);

		cc=CoordinatePolygon(flatpoint(sqrt(3),2.5), 1, false, 6, 1);
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

	} else if (!strcasecmp(coloring,"hexagonal 3")) {
		tiling->repeatXDir(flatpoint(3*sqrt(3)/2,1.5));
		tiling->repeatYDir(flatpoint(0,3));

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		Coordinate *cc=CoordinatePolygon(flatpoint(sqrt(3)/2,1), 1, false, 6, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		cc=CoordinatePolygon(flatpoint(3*sqrt(3)/2,1), 1, false, 6, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		affine.setIdentity();
		op->AddTransform(affine);

		cc=CoordinatePolygon(flatpoint(sqrt(3),2.5), 1, false, 6, 1);
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
			path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1);
		op->AddTransform(affine);

		affine.Rotate(M_PI, flatpoint(.75,sqrt(3)/4));
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->append(0,0);
		path->append(1,0);
		path->append(.5,sqrt(3)/2);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		affine.Rotate(M_PI, flatpoint(.75,sqrt(3)/4));
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->appendCoord(cc);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		 //triangle
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->appendCoord(cc);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		 //triangles
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		path->appendCoord(cc);
		path->close();
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		 //squares
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		Coordinate *cc=CoordinatePolygon(flatpoint(0,0), r, false, 12, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		Coordinate *cc=CoordinatePolygon(flatpoint(0,0), r, false, 12, 1);
		path->appendCoord(cc);
		path->FindBBox();
		op=tiling->AddBase(path,1,1, false,false);
		op->AddTransform(affine);

		 //squares
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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
		path=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
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

	if (!strcmp(tile,"Circular/spiral"))  return CreateSpiral(0, 4*M_PI, 5,3, 20, NULL);
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

	 //thing to click on 
	CLONEI_Circle,
	CLONEI_Box,
	CLONEI_Inputs,
	CLONEI_Tiling,
	CLONEI_Tiling_Label,
	CLONEI_BaseCell,
	CLONEI_Source_Object,
	CLONEI_Boundary,

	 //shortcut actions
	CLONEIA_Edit,
	CLONEIA_Next_Tiling,
	CLONEIA_Previous_Tiling,
	CLONEIA_Next_Basecell,
	CLONEIA_Previous_Basecell,
	CLONEIA_Toggle_Lines,
	CLONEIA_Toggle_Auto_Base,
	CLONEIA_Toggle_Preview,
	CLONEIA_Toggle_Render,
	CLONEIA_Toggle_Orientations,
	CLONEIA_Select,
	CLONEIA_ColorFillOrStroke,
	
	 //interface modes
	CMODE_Normal,
	CMODE_Select,
	CMODE_Edit_Tiling,   // todo!
	CMODE_Edit_Boundary, // todo!
	CMODE_Edit_Base_Path,// todo!

	 //extra context menu things
	CLONEM_Clear_Base_Objects,
	CLONEM_Reset,
	CLONEM_Include_Lines,
	CLONEM_Load,
	CLONEM_Save,
	CLONEM_Select_Boundary,
	CLONEM_Select_Base,
	CLONEM_Select_Sources,
	CLONEM_Auto_Select_Cell,

	CLONEI_MAX
};


CloneInterface::CloneInterface(anInterface *nowner,int nid,Laxkit::Displayer *ndp)
  : anInterface(nowner,nid,ndp),
	extra_input_fields(LISTS_DELETE_Array),
	rectinterface(0,NULL)
{
	mode=CMODE_Normal;

	rectinterface.style|= RECT_CANTCREATE | RECT_OBJECT_SHUNT;
	rectinterface.owner=this;

	cloner_style = 0;
	lastover = CLONEI_None;
	lastoveri = -1;
	active = false;
	preview_orient = false;
	snap_to_base = true;
	color_to_stroke = true;
	show_p1 = false; 

	 //structure is:
	 //preview
	 //  base_cells
	 //    cell 1
	 //    cell 2
	 //    source_proxies
	 //      for base 1
	 //      for base 2
	previewoc = NULL;
	preview = NULL; 
	base_cells = NULL;
	source_proxies = NULL;

	char *str = make_id("tilinglines");
	lines = new Group;
	lines->Id(str);
	delete[] str;

	sc = NULL;

	firsttime = 1;
	uiscale = 1;
	bg_color =rgbcolorf(.9,.9,.9);
	hbg_color=rgbcolorf(1.,1.,1.);
	fg_color =rgbcolorf(.1,.1,.1);
	activate_color  =rgbcolorf(0.,.783,0.);
	deactivate_color=rgbcolorf(1.,.392,.392);

	preview_cell.Color(65535,0,0,65535);
	preview_cell.width=2;
	preview_cell.widthtype=0;
	preview_cell2.Color(35000,35000,35000,65535);
	preview_cell2.width=1;
	preview_cell2.widthtype=0;

	current_base=-1;
	ScreenColor col(0.,.7,0.,1.);
	boundary=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
	boundary->style=PathsData::PATHS_Ignore_Weights;
	boundary->line(2,-1,-1,&col);
	boundary->linestyle->widthtype = 0;
	boundary->appendRect(0,0,4,4);

	preempt_clear = false;

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
	if (base_cells) base_cells->dec_count();
	if (source_proxies) source_proxies->dec_count();
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
{
	if (preempt_clear) return;

	if (previewoc) {
		delete previewoc;
		previewoc=NULL;
	}

	if (preview)        { preview->dec_count();        preview = NULL;        }
	if (base_cells)     { base_cells->dec_count();     base_cells = NULL;     }
	if (source_proxies) { source_proxies->dec_count(); source_proxies = NULL; }
	if (lines) lines->flush();

	active = false;
}

/*! Return 1 if preview found, and state updated to it. Else return 0.
 *
 * From viewport->Selection, look for what appears to be a preview object.
 * This is just an object that has a "tiling" property that is a Tiling object.
 * If there are subobjects tagged with "tiling_base" or "tiling_sources", then 
 * use those.
 */
int CloneInterface::UpdateFromSelection()
{
	Selection *s = viewport->GetSelection();
	if (!s || s->n()==0) return 0;

	for (int c=0; c<s->n(); c++) {
		ObjectContext *oc = s->e(c);
		DrawableObject *obj = dynamic_cast<DrawableObject*>(oc->obj);

		while (obj) { 
			Tiling *t = dynamic_cast<Tiling*>(obj->properties.findObject("tiling"));
			if (t) {
				 //adjust to using obj as the preview object, and look for basecells and sources
				if (preview) Clear(NULL);
				active = true;
				preview = obj;
				preview->inc_count();
				previewoc = dynamic_cast<VObjContext*>(oc->duplicate());
				
				for (int c=0; c<obj->n(); c++) {
					DrawableObject *o = dynamic_cast<DrawableObject*>(obj->e(c));
					if (o->HasTag("tiling_base", 1)) {
						base_cells = o;
						base_cells->inc_count();
						break;
					}
				}

				if (base_cells) for (int c=0; c<base_cells->n(); c++) {
					DrawableObject *o = dynamic_cast<DrawableObject*>(base_cells->e(c));
					if (o->HasTag("tiling_sources", 1)) {
						source_proxies = o;
						source_proxies->inc_count();
						break;
					}
				}

				t->inc_count();
				SetTiling(t);
				return 1; 
			}
			obj = dynamic_cast<DrawableObject*>(obj->GetParent());
		}
	}

	return 0;
}

int CloneInterface::InterfaceOn()
{
	if (!preview) UpdateFromSelection();

	if (!preview) {
		if (cur_tiling<0) cur_tiling = 0;
		Tiling *newtiling = GetBuiltinTiling(cur_tiling);
		SetTiling(newtiling);
	}

	if (active) Render();

	needtodraw=1;
	return 0;
}

int CloneInterface::InterfaceOff()
{
	Clear(NULL);

	active=false;

	needtodraw=1;
	return 0;
}

/*! Replace old tiling with newtiling. Update base_cells control objects
 * Absorbs count of newtiling.
 *
 * Return 0 for success, nonzero error.
 */
int CloneInterface::SetTiling(Tiling *newtiling)
{
	if (!newtiling) return 1;

	Tiling *oldtiling = tiling;
	tiling = newtiling;

	extra_input_fields.flush();
	for (int c=0; c<tiling->basecells.n; c++) {
		for (int c2=0; c2<tiling->basecells.e[c]->transforms.n; c2++) {
			if (tiling->basecells.e[c]->transforms.e[c2]->max_iterations > 1) {
				char str[25];
				sprintf(str, "itr%d.%d",c,c2);
				extra_input_fields.push(newstr(str));
			}
		}		
	}
	if (tiling->radial_divisions>0) { extra_input_fields.push(newstr("radial")); }

	if (!preview) {
		preview = new Group;
		char *str=make_id("tiling");
		preview->Id(str);
		delete[] str;
	}

	UpdateBasecells();

	if (oldtiling && oldtiling != tiling) oldtiling->dec_count();

	preview->properties.pushObject("tiling", tiling);
	return 0;
}

/*! With current tiling, we need to flush the old base cells and install new ones.
 * This is usually in response to updating existing tiling's variables, or on creation of
 * a new tiling, so going in, base_cells refers to the previous tiling.
 */
int CloneInterface::UpdateBasecells()
{
	 //reset base_cells
	if (!base_cells) {
		base_cells = new Group;
		base_cells->Id("bases");
		base_cells->InsertTag("tiling_base", 1);
		preview->push(base_cells);

	} else {
		base_cells->flush();
		//for (int c=0; c < tiling->basecells.n && c<base_cells->n(); c++) { //remove base cell lines
		//	base_cells->remove(0);
		//}
	}

	base_cells->Unshear(1,0);
	base_cells->flags |= SOMEDATA_KEEP_ASPECT;

	//if (tiling->flexible_aspect) base_cells->flags &= ~SOMEDATA_KEEP_ASPECT;
	//else base_cells->flags |= SOMEDATA_KEEP_ASPECT;

	//if (tiling->shearable) base_cells->flags &= ~SOMEDATA_LOCK_SHEAR;
	//else base_cells->flags |= SOMEDATA_LOCK_SHEAR;


	if (source_proxies == NULL) {
		source_proxies = new Group();
		source_proxies->Id("sources");
		source_proxies->InsertTag("tiling_sources", 1);
	}

	 //push (or push back) sources
	base_cells->push(source_proxies);


	 //reassign out of bounds source objects to be cell 0
	//int error;
	Group *g0 = dynamic_cast<Group*>(source_proxies->e(0));
	for (int c = source_proxies->NumKids()-1; c >= tiling->basecells.n; c--) {
		Group *g = dynamic_cast<Group*>(source_proxies->e(c));

		while (g->n()) { 
			SomeData *obj = g->pop(-1);
			g0->push(obj);
			obj->dec_count();
		}
		source_proxies->remove(c);
	}

	 //base_cells gets installed as a child of preview.
	 //base_cells gets structure along these lines:
	 //0: basecell 0.. 
	 //  cell 0 path
	 //  repeat 1 for basecell 0 (the repeat 0 is just the original basecell)
	 //  repeat 2 for basecell 0
	 //1: basecell 1
	 //  cell 1 path
	 //  repeat 1 for basecell 1
	 //  repeat 2 for basecell 1
	 //  repeat 3 for basecell 1
	 //2: basecell 2
	 //  cell 2 path
	 //  repeat 1 for basecell 2
	 //source_proxies: 
	 // 0:
	 //  clone to source object 0, with property saying which base it belongs to
	 // 1:
	 //  clone to source object 1
	 // 2: (with nothing, for instance)


	LaxInterfaces::SomeData *o;
	LaxInterfaces::PathsData *po;
  	DrawableObject *d; 
	LineStyle *lstyle;
	ScreenColor c1(1.,0.,0.,1.), c2(0.,0.,1.,1.), ca; //new cells get a color along this range
	char scratch[50];

	 //install main base cells
	for (int c=0; c<tiling->basecells.n; c++) {
		if (tiling->basecells.n>1) coloravg(&ca, &c1,&c2, (double)c/(tiling->basecells.n-1));
		else ca=c1;

		Group *group = new Group;
		sprintf(scratch, "base%d", c);
		group->Id(scratch);

		base_cells->push(group, c);
		group->dec_count();

		if (source_proxies->n() <= c) {
			Group *src = new Group;
			sprintf(scratch, "sources%d", c);
			src->Id(scratch);
			src->properties.push("tilingSource", c);
			source_proxies->push(src);
			src->dec_count();
		}

		 //create the base outline
		d = dynamic_cast<DrawableObject*>(tiling->basecells.e[c]->celloutline->duplicate(NULL));
		d->properties.push("base",c);
		po= dynamic_cast<PathsData*>(d);
		po->style=PathsData::PATHS_Ignore_Weights;
		d->FindBBox();
		lstyle  = new LineStyle();
		*lstyle = preview_cell;
		lstyle->color = ca;
		lstyle->widthtype = 0;
		dynamic_cast<PathsData*>(d)->InstallLineStyle(lstyle);
		lstyle->dec_count();

		group->push(d);
		d->dec_count(); 

		 //install minor base cell copies
		for (int c2=1; c2<tiling->basecells.e[c]->transforms.n; c2++) {
			o=tiling->basecells.e[c]->celloutline->duplicate(NULL);
			o->FindBBox();
			o->Multiply(tiling->basecells.e[c]->transforms.e[c2]->transform);
			po = dynamic_cast<PathsData*>(o);
			po->InstallLineStyle(&preview_cell2);
			po->style=PathsData::PATHS_Ignore_Weights;
			group->push(o);
			o->dec_count();

			d=dynamic_cast<DrawableObject*>(o);
			d->properties.push("basecopy",c);
		}

		group->FindBBox();
	}

	base_cells->FindBBox();
	preview->FindBBox();

	current_base=0;


	if (active) Render(); 
	needtodraw=1;
	return 0;
}


Laxkit::ShortcutHandler *CloneInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("CloneInterface");
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler("CloneInterface");

	sc->Add(CLONEIA_Next_Tiling,        LAX_Right,0,0, "NextTiling",        _("Select next tiling"),    NULL,0);
	sc->Add(CLONEIA_Previous_Tiling,    LAX_Left,0,0,  "PreviousTiling",    _("Select previous tiling"),NULL,0);
	sc->Add(CLONEIA_Next_Basecell,      LAX_Up,0,0,    "NextBasecell",      _("Select next base cell"),    NULL,0);
	sc->Add(CLONEIA_Previous_Basecell,  LAX_Down,0,0,  "PreviousBasecell",  _("Select previous base cell"),    NULL,0);
	sc->Add(CLONEIA_Toggle_Lines,       'l',0,0,       "ToggleLines",       _("Toggle rendering cell lines"),NULL,0);
	sc->Add(CLONEIA_Toggle_Render,      LAX_Enter,0,0, "ToggleRender",      _("Toggle rendering"),NULL,0);
	sc->Add(CLONEIA_Toggle_Preview,     'p',0,0,       "TogglePreview",     _("Toggle preview of lines"),NULL,0);
	sc->Add(CLONEIA_Toggle_Orientations,'o',0,0,       "ToggleOrientations",_("Toggle preview of orientations"),NULL,0);
	sc->Add(CLONEIA_Edit,            'e',ControlMask,0,"Edit",              _("Edit"),NULL,0);
	sc->Add(CLONEIA_Select,             's',0,0,       "Select",            _("Select tile mode"),NULL,0);
	sc->Add(CLONEIA_ColorFillOrStroke,  'x',0,0,       "FillOrStroke",      _("Toggle sending color to fill or stroke"),NULL,0);

	//sc->AddShortcut(LAX_Del,0,0, PAPERI_Delete);


	manager->AddArea("CloneInterface",sc);
	return sc;
}

int CloneInterface::PerformAction(int action)
{
	if (action==CLONEIA_Next_Tiling) {
		Tiling *newtiling = NULL;
		int maxtiling = NumBuiltinTilings();
		do {
			cur_tiling++;
			if (cur_tiling >= maxtiling) cur_tiling=0;
			newtiling = GetBuiltinTiling(cur_tiling);
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

	} else if (action==CLONEIA_Toggle_Auto_Base) {
		snap_to_base = !snap_to_base;
		PostMessage(snap_to_base ? _("Auto select base cell") : _("Don't auto select base cell"));
		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_Toggle_Lines) {
		trace_cells=!trace_cells;
		if (active) Render();
		PostMessage(trace_cells ? _("Include cell outlines") : _("Don't include cells"));
		needtodraw=1;
		return 0;

	} else if (action==CLONEIA_ColorFillOrStroke) {
		color_to_stroke = !color_to_stroke;
		PostMessage(color_to_stroke ? _("Color to stroke") : _("Color to fill"));
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
		int base = current_base;
		if (base<0) base = 0; else base--;
		if (base<0) base = tiling->basecells.n-1;
		SetCurrentBase(base);
		return 0;

	} else if (action==CLONEIA_Previous_Basecell) {
		int base = current_base;
		if (base<0) base=0; else base++;
		if (base>=tiling->basecells.n) base=0;
		SetCurrentBase(base);
		return 0;

	} else if (action==CLONEIA_Edit) {
		cerr <<" *** need to implement Clone Tiling Edit!!"<<endl;
		return 0;

	} else if (action==CLONEIA_Select) {
		if (child) RemoveChild();
		Mode(CMODE_Select);
		return 0;
	}

	return 1;
}

/*! Return the current base afterwards. If which is not a valid base, then current_base is not changed.
 *
 * This just sets current_base to which, and sends message to viewport to update current color.
 */
int CloneInterface::SetCurrentBase(int which)
{
	if (which<0 || which >= tiling->basecells.n) return current_base;
	current_base = which;

	ScreenColor *col = BaseCellColor(current_base);
	if (col) {
		SimpleColorEventData *e=new SimpleColorEventData( 65535, col->red, col->green, col->blue, col->alpha, 0);
		app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);
	}

	needtodraw = 1;
	return current_base;
}

/*! Retrieve the color of which base cell, or NULL if not found.
 */
Laxkit::ScreenColor *CloneInterface::BaseCellColor(int which)
{
	if (!base_cells) return NULL;

	if (which < 0) which = current_base;
	if (which < 0) which = 0;
	if (which >= tiling->basecells.n) which = tiling->basecells.n-1;

	Group *group=dynamic_cast<DrawableObject*>(base_cells->e(which));
	PathsData *bpath=dynamic_cast<PathsData*>(group->e(0));

	//if (!bpath && group) {
	//	bpath=dynamic_cast<PathsData*>(group->e(0));//for groupified
	//}

	if (bpath) return &bpath->linestyle->color;
	return NULL;
}

Laxkit::MenuInfo *CloneInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!menu) menu=new MenuInfo(_("Clone Interface"));
	else menu->AddSep(_("Clone"));

    menu->AddItem(_("Select boundary"), CLONEM_Select_Boundary);
    menu->AddItem(_("Select base cells"), CLONEM_Select_Base);
    //menu->AddItem(_("Select source objects"), CLONEM_Select_Sources);
    menu->AddSep();
    menu->AddItem(_("Clear base objects"), CLONEM_Clear_Base_Objects);
    menu->AddItem(_("Reset all"), CLONEM_Reset);
    menu->AddSep();
    menu->AddToggleItem(_("Include lines"),        CLONEM_Include_Lines,    0, trace_cells );
	menu->AddToggleItem(_("Auto select base cell"),CLONEM_Auto_Select_Cell, 0, snap_to_base);
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
			 //remove all base objects
			if (current_base<0) current_base=0;

			if (source_proxies) {
				for (int c=0; c<source_proxies->n(); c++) {
					Group *g = dynamic_cast<Group*>(source_proxies->e(c));
					if (g) g->flush();
				}
			}
			current_selected = -1;

			if (child) RemoveChild();
			if (active) ToggleActivated();
            return 0;

		} else if (i==CLONEM_Select_Boundary) {
			Mode(CMODE_Normal);
			EditThis(boundary, _("Edit boundary"));
			return 0;

		} else if (i==CLONEM_Select_Base) {
			Mode(CMODE_Normal);
			EditThis(base_cells, _("Edit base cell placement"));
			return 0;

		} else if (i==CLONEM_Auto_Select_Cell) {
			PerformAction(CLONEIA_Toggle_Auto_Base);
			return 0;

		} else if (i==CLONEM_Include_Lines) {
			PerformAction(CLONEIA_Toggle_Lines);
			return 0;

		} else if (i==CLONEM_Reset) {
			Clear(NULL);

			Tiling *newtiling = GetBuiltinTiling(cur_tiling);
			SetTiling(newtiling);

			needtodraw=1;
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

	} else if (!strcmp(mes,"RectInterface")) {
		 //received when child rectinterface changes moves contents

		if (snap_to_base
				    && rectinterface.somedata != base_cells
				    && rectinterface.somedata != boundary
					) {
					//&& source_proxies->n()) {

			 //moving around a proxy image, need to check if we need to rebase
			int mx,my;
			mouseposition(0, curwindow, &mx,&my, NULL, NULL, NULL);
			flatpoint fp=dp->screentoreal(mx,my);
			int i = -1, dest=-1;
			int on = scanBasecells(fp, &i, &dest);

			if (on == CLONEI_BaseCell && i>=0 && dest>=0) {
				DrawableObject *d = dynamic_cast<DrawableObject*>(rectinterface.somedata);
				DrawableObject *parent = dynamic_cast<DrawableObject*>(d->GetParent());

				int error, base;
				base = parent->properties.findInt("tilingSource",-1,&error);

				if (error==0 && base != i) {
					//----------
					parent->pop(parent->findindex(d));
					dynamic_cast<Group*>(source_proxies->e(i))->push(d);
					//----------
					//d->properties.push("tilingSource", i);
					//----------

					SetCurrentBase(i);
					PostMessage(_("Moved to new base."));
					needtodraw=1;
				}
			}
		}
		return 0;

	} else if (!strncmp(mes,"setrecurse",10)) {
		int i = strtol(mes+10,NULL,10);
		if (i<0 || i >= extra_input_fields.n) return 0;

        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (isblank(s->str)) return 0;

		int itr = strtol(s->str, NULL, 10);
		if (itr<2) {
			PostMessage(_("Repeat must be 2 or more"));
		} else { 
			TilingDest *dest = GetDest(extra_input_fields.e[i]);
			if (dest) {
				dest->max_iterations = itr;
				if (active) Render();
			}
		}
		return 0;

	} else if (!strcmp(mes,"setradial")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (isblank(s->str)) return 0;

		int div = strtol(s->str, NULL, 10);
		if (div<1) {
			PostMessage(_("There must be 1 or more divisions"));
		} else { 
			if (div != tiling->radial_divisions) {
				UpdateRadial(tiling, div);
				UpdateBasecells();
				//if (active) Render(); <- called in UpdateBasecells()
			}
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

		LineStyle *l = dynamic_cast<LineStyle*>(ndata);
		PathsData *p = GetBasePath();
		if (p) {
			if (color_to_stroke) p->line(-1,-1,-1, &l->color);
			else p->fill(&l->color);
		}
		return 1;
	}
	return 0;
}

/*! Draw source object markers.
 */
void CloneInterface::DrawSelected()
{
	if (!source_proxies) return;

	int bcell;
	DrawableObject *data;
	Affine a;

	//dp->PushAndNewTransform(base_cells->m());

	for (int c=0; c<source_proxies->n(); c++) { 
		Group *g = dynamic_cast<Group*>(source_proxies->e(c));

		 //set color of relevant base cell
		//bcell = data->properties.findInt("tilingSource");
		bcell = c;
		dp->NewFG(BaseCellColor(bcell));
		dp->LineAttributes(-1, LineSolid,LAXCAP_Round,LAXJOIN_Round);

		for (int c2=0; c2<g->n(); c2++) { 
			data = dynamic_cast<DrawableObject*>(g->e(c2));
			a = data->GetTransformToContext(false,0);
			dp->PushAndNewTransform(a.m());

			dp->LineWidthScreen((bcell == current_base?3:1));


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

			//Laidout::DrawDataStraight(dp, data, NULL,NULL); // <- should be drawn as part of base_cells

			dp->PopAxes();
		}
	}

	//dp->PopAxes();
}

/*! Draw the big box of tiling icons to select other tilings.
 */
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
		f = GetBuiltinIconKey(i);
		img = laidout->icons->GetIcon(f);
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
		img->dec_count();

		x+=cell_pad+icon_width;
	}
}

/*! Return the main PathsData object for which base cell within the this->base_cells container.
 */
PathsData *CloneInterface::GetBasePath(int which)
{
	if (!base_cells) return NULL;
	if (which<0) which = current_base;
	if (which >= tiling->basecells.n) return NULL;
	Group *g = dynamic_cast<Group*>(base_cells->e(which));
	return dynamic_cast<PathsData*>(g->e(0));
}

/*! Convenience function to return the TilingDest of current tiling corresponding to a string from extra_input_fields.
 *
 * Currently, str should be in format "itr3.2", where you replace the 3 with which base cell to use,
 * and replace the 2 with the transform of that base to get.
 */
TilingDest *CloneInterface::GetDest(const char *str)
{
	if (!tiling) return NULL;

	int base, dest;
	if (sscanf(str, "itr%d.%d", &base, &dest) == 2) { 
		if (base >= 0 && base < tiling->basecells.n
				&& dest >= 0 && dest < tiling->basecells.e[base]->transforms.n) {
			return tiling->basecells.e[base]->transforms.e[dest];
		}
	}
	return NULL;
}

int CloneInterface::NumProxies()
{
	if (!source_proxies) return 0;
	int n=0;
	Group *g;
	for (int c=0; c<source_proxies->n(); c++) {
		g = dynamic_cast<Group*>(source_proxies->e(c));
		n += g->n();
	}
	return n;
}

/*! Return the source proxy for base, which.
 * If which<0 then return the base source proxy: source_proxies->e(base).
 * Else basically  source_proxies->e(base)->e(which)
 */
DrawableObject *CloneInterface::GetProxy(int base, int which)
{
	if (!source_proxies) return NULL;
	Group *g = dynamic_cast<Group*>(source_proxies->e(base));
	if (!g) return NULL;
	if (which<0) return g;
	return dynamic_cast<Group*>(g->e(which));
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

	// 
	// Includes:
	//  the control box
	//  base cells, a group of paths, where you can drag page objects into
	//  render outline
	//  rendered group
	//


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
	if (base_cells) {
		Laidout::DrawData(dp, base_cells, NULL,NULL);

		dp->PushAndNewTransform(base_cells->m());

		PathsData *pd;
		Group *base;
		for (int c = base_cells->n()-1; c>=0; c--) {
			base = dynamic_cast<Group*>(base_cells->e(c));

			//Laidout::DrawData(dp, base, NULL,NULL);

			if (preview_orient) {
				 //draw arrows to indicate orientions of base cells
				dp->LineWidthScreen(2);

				for (int c2=0; c2 < base->n(); c2++) {
					DrawableObject *o = dynamic_cast<DrawableObject*>(base->e(c2));
					flatpoint center  = o->ReferencePoint(LAX_MIDDLE,true);
					flatpoint v       = o->ReferencePoint(LAX_TOP_MIDDLE,true)-center;
					v*=.4;

					int error;
					int bc = o->properties.findInt("base",-1,&error);
					if (error != 0) bc = o->properties.findInt("baseCopy",-1,&error);
					if (error == 0) {
						pd = GetBasePath(bc);
						if (pd && pd->linestyle) dp->NewFG(&pd->linestyle->color);
						else dp->NewFG(&preview_cell2.color);
					}

					dp->drawline(center-v,center+v);
					dp->drawline(center+v,center+transpose(v)/2);
				}
			}
		}

		if (current_base >= 0) {
			 //make active base cell bolder, and draw on top of any others 
			Group *obj   = dynamic_cast<Group*>(base_cells->e(current_base));
			PathsData *p = NULL;
			if (obj) p = dynamic_cast<PathsData*>(obj->e(0));
			if (p) {
				p->linestyle->width=4;
				Laidout::DrawData(dp, p, NULL,NULL);
				p->linestyle->width=2;
			}
		}

		dp->PopAxes();
	} //if base_cells


	// //draw rect stuff after clone handles, but before the control box
	//if (rectinterface) { rectinterface.needtodraw=1; rectinterface.Refresh(); }


	 //--------draw control box------

	dp->DrawScreen(); 

	 //draw whole rect outline
	dp->LineAttributes(1,LineSolid,CapButt,JoinMiter);
	//dp->LineWidthScreen(1);
	dp->NewBG(bg_color);
	dp->NewFG(fg_color);
	dp->drawrectangle(box.minx,box.miny, box.maxx-box.minx,box.maxy-box.miny, 2);
	if (lastover==CLONEI_Tiling) {
		double pad=(box.maxx-box.minx)*.1;
		dp->NewFG(hbg_color);
		dp->drawrectangle(box.minx+pad,box.miny+pad, box.maxx-box.minx-2*pad,box.maxy-box.miny-2*pad, 1);
	}

	if (extra_input_fields.n) {
		 //draw extra recursion inputs
		dp->NewBG(bg_color);
		dp->NewFG(fg_color);
		double th=dp->textheight();
		double pad = th*.1;
		dp->drawrectangle(box.minx,box.maxy, box.maxx-box.minx, th/2 + 2*pad + circle_radius + (2*pad + th)*(extra_input_fields.n), 2);
		char scratch[100];
		double y=box.maxy + circle_radius + pad + th/2;
		TilingDest *dest;

		for (int c=0; c<extra_input_fields.n; c++) {
			dest = GetDest(extra_input_fields.e[c]);
			if (dest) sprintf(scratch, _("Rep: %d"), dest->max_iterations);
			else sprintf(scratch, _("Radial: %d"), tiling->radial_divisions);

			if (lastover == CLONEI_Inputs && lastoveri == c) {
				dp->NewFG(hbg_color);
				dp->drawrectangle(box.minx+pad,y-pad, box.maxx-box.minx-2*pad, th + 2*pad, 1);
				dp->NewFG(fg_color);
			}
			dp->textout((box.minx+box.maxx)/2,y, scratch,-1, LAX_HCENTER|LAX_TOP);
			y+=th+2*pad;
		}
	}

	 //draw circle
	flatpoint cc((box.minx+box.maxx)/2,box.maxy);
	if (active) dp->NewFG(activate_color); else dp->NewFG(deactivate_color);
	dp->LineWidthScreen(3);
	if (lastover==CLONEI_Circle) dp->NewBG(hbg_color); else dp->NewBG(bg_color);
	dp->drawellipse(cc.x,cc.y,
					circle_radius,circle_radius,
					0,2*M_PI,
					2);

	
	if (tiling) {
		 //draw icon
		double pad=(box.maxx-box.minx)*.1;
		double boxw=box.maxx-box.minx-2*pad;

		if (tiling->icon) {
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
			
			double w = dp->textextent(tiling->name,-1,NULL,NULL);
			double w2 = tiling->category ? dp->textextent(tiling->category,-1,NULL,NULL) : 0;
			if (w2>w) w = w2;

			double th = dp->textheight();
			if (w>boxw) {
				dp->fontsize(th*boxw/w);
			}
			dp->textout((box.minx+box.maxx)/2,box.maxy-circle_radius-pad, tiling->name,-1, LAX_HCENTER|LAX_BOTTOM);
			if (tiling->category) 
				dp->textout((box.minx+box.maxx)/2,box.maxy-circle_radius-pad-dp->textheight(),
						tiling->category,-1, LAX_HCENTER|LAX_BOTTOM);
			if (w>boxw) {
				dp->fontsize(th);
			}

		}
	}




	dp->DrawReal();

	return 0;
}

int CloneInterface::scan(int x,int y, int *i, int *dest)
{
	flatpoint cc((box.minx+box.maxx)/2,box.maxy);
	double circle_radius = INTERFACE_CIRCLE*uiscale;

	if (norm((cc-flatpoint(x,y)))<circle_radius) return CLONEI_Circle;

	 //check for things related to the tiling selector
	if (box.boxcontains(x,y)) {
		double pad=(box.maxx-box.minx)*.15;
		if (x>box.minx+pad && x<box.maxx-pad && y>box.miny+pad && y<box.maxy-pad)
			return CLONEI_Tiling;
		return CLONEI_Box;
	}

	if (extra_input_fields.n && x>=box.minx && x<=box.maxx && y>=box.maxy) {
		double th=dp->textheight();
		int yi = (y-(box.maxy + circle_radius + th/2 + th*.1))/(th*1.2);
		DBG cerr <<" --------------------extra input scan: "<<yi<<endl;

		if (yi>=0 && yi<extra_input_fields.n) {
			if (i) *i = yi;
			DBG cerr <<" --------------------extra input found! "<<yi<<endl;
			return CLONEI_Inputs;
		}
	}


	flatpoint fp=dp->screentoreal(x,y);

	if (source_proxies) {
		 //check for being inside a proxy image
		flatpoint pp = base_cells->transformPointInverse(fp);

		SomeData *obj;
		DBG cerr <<" ----- fpoint: "<<pp.x<<','<<pp.y<<endl;

		for (int c=0; c<source_proxies->NumKids(); c++) {
		  Group *g = dynamic_cast<Group*>(source_proxies->e(c));

		  for (int c2=0; c2<g->n(); c2++) {
			obj = g->e(c2);

			//DBG cerr <<" ----- source "<<c<<"/"<<source_proxies->n()<<" bbox: "<<obj->minx<<"  "<<obj->miny
			//DBG 	 <<"  "<<obj->maxx<<"  "<<obj->maxy<<endl;
			//DBG flatpoint p=transform_point_inverse(obj->m(), pp);
			//DBG cerr <<" ----- point: "<<p.x<<','<<p.y<<endl;

			if (obj->pointin(pp,1)) {
				if (i) *i = c;
				if (dest) *dest = c2;
				return CLONEI_Source_Object;
			}
		  }
		}
	}


	 //check for inside base cell outlines
	if (scanBasecells(fp, i, dest) == CLONEI_BaseCell) return CLONEI_BaseCell;

	 //check for inside boundary
	if (boundary && boundary->pointin(fp,1)) return CLONEI_Boundary;


	return CLONEI_None;
}

/*! fp is real point, such as dp->screentoreal(x,y) from a mouse event.
 *
 * If in a base cell, put which one in i, which TilingDest in dest, and return CLONEI_BaseCell.
 * Otherwise, return CLONEI_None.
 */
int CloneInterface::scanBasecells(flatpoint fp, int *i, int *dest)
{
	 //check for inside base cell outlines
	if (!base_cells || !base_cells->pointin(fp,1)) return CLONEI_None;


	flatpoint p = transform_point_inverse(base_cells->m(),fp);
	int which = -1, whichdest = -1;

	Group *base;
	for (int c=0; c<tiling->basecells.n; c++) {
		base = dynamic_cast<Group*>(base_cells->e(c));

		DBG cerr <<" ----- base cell "<<c<<"/"<<base_cells->n()<<" bbox: "
		DBG 	<<base_cells->e(c)->minx<<"  "<<base_cells->e(c)->miny<<"  "<<base_cells->e(c)->maxx<<"  "<<base_cells->e(c)->maxy<<endl;
		DBG cerr <<" ----- point: "<<p.x<<','<<p.y<<endl;

		for (int c2=0; c2<base->NumKids(); c2++) {
			if (!base->e(c2)->pointin(p,1)) continue;
			which = c;
			whichdest = c2;
			break;
		}
		if (which>=0) break;
	}

	if (i) *i = which;
	if (dest) *dest = whichdest;

	DBG cerr <<" --- over base cell: "<<which<<", "<<whichdest<<endl;
	if (which>=0) return CLONEI_BaseCell;
	return CLONEI_None;
}

/*! Scan for when the tiling selection panel is up.
 */
int CloneInterface::scanSelected(int x,int y)
{
	//double th=dp->textheight();
	//double pad=th;
	double boxh=num_rows*(icon_width*1.2)+icon_width*.2;
	double boxw=num_cols*(icon_width*1.2);
	int row,col;

	x -= (dp->Maxx+dp->Minx)/2-boxw/2;
	col = floor(x/icon_width/1.2);

	y -= selected_offset+(dp->Miny+dp->Maxy)/2-boxh/2+icon_width*.2;
	row = floor(y/icon_width/1.2);

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

/*! Return 1 for success and child interface added.
 *  Return 2 for success and child already there.
 *  else <=0 for error and could not edit object.
 */
int CloneInterface::EditThis(SomeData *object, const char *message)
{
	if (!object) return 0;

	if (object == base_cells) {
		transform_copy(base_lastm, object->m());
	}

	if (!child) {
		//RectInterface *rect=new RectInterface(0,dp);
		rectinterface.style|= RECT_CANTCREATE | RECT_OBJECT_SHUNT;
		rectinterface.owner=this;
		rectinterface.UseThis(object,0);

		if (object != boundary && object != base_cells) { //is source object, need an extra push.. *** note this fails on other page context...
			rectinterface.ExtraContext(base_cells->m());
		} else {
			rectinterface.ExtraContext(NULL);
		}

		child = &rectinterface;
		AddChild(&rectinterface,0,0);
		//AddChild(&rectinterface,0,1); //ensures *this gets first dibs for input events
		if (message) PostMessage(message);
		return 1; 
	}

	if (rectinterface.somedata != object) {
		child->UseThis(object,0);

		if (object != boundary && object != base_cells) { //is source object, need an extra push.. *** note this fails on other page context...
			rectinterface.ExtraContext(base_cells->m());
		} else {
			rectinterface.ExtraContext(NULL);
		}
		return 1;
	}
	return 2;
}

int CloneInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;

	DBG cerr <<"CloneInterface::LBDown()..."<<endl;

	if (mode==CMODE_Select) {
		//Mode(CMODE_Normal);
		int item=scanSelected(x,y);
		buttondown.down(d->id,LEFTBUTTON, x,y, item);
		return 0;
	}

	int i=-1, dest=-1;
	int over=scan(x,y,&i,&dest);

	 //on control box
	if (over==CLONEI_Tiling || over==CLONEI_Box || over==CLONEI_Inputs) {
		if (child) {
			RemoveChild();
			needtodraw=1;
		}
		buttondown.down(d->id,LEFTBUTTON,x,y, over,i);
		return 0;
	}

	if (over==CLONEI_BaseCell) {
		int status = EditThis(base_cells, _("Edit base cell placement"));
		if (status == 1) {
			 //fresh rectinterface
			dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
			return 0;
		} else if (status == 2) { //child existed already
			return 1;
		}
		over=CLONEI_None;
	}

	if (over==CLONEI_Boundary) {
		int status = EditThis(boundary, _("Edit boundary"));
		if (status == 1) {
			 //fresh rectinterface
			dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
			return 0;
		} else if (status == 2) { //child existed already
			return 1;
		}
		over=CLONEI_None;
	}

	if (over==CLONEI_Source_Object) {
		SomeData *obj = GetProxy(i,dest);

		int status = EditThis(obj, _("Move source object"));
		if (status == 1) {
			 //fresh rectinterface
			dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
			return 0;
		} else if (status == 2) { //child existed already
			return 1;
		}
		over=CLONEI_None;
	}

	if (over == CLONEI_None) {
		 //maybe add or remove a source object for a particular base cell
		SomeData *obj=NULL;
		ObjectContext *oc=NULL;
		int c=viewport->FindObject(x,y,NULL,NULL,1,&oc);
		if (c>0) obj=oc->obj;

		if (obj) {
			SomeData *o = obj;
			while (o) {
				 //don't allow selecting objects that are children of current cloning!
				if (o == preview) return 0;
				o=o->GetParent();
			}


			if (current_base<0) current_base=0;
			//sources.Add(oc,-1,current_base);

			 //set up proxy object
			//VObjContext *noc=dynamic_cast<VObjContext*>(oc->duplicate());
			//noc->clearToPage();
			SomeDataRef *ref=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory()->NewObject("SomeDataRef"));
			ref->Set(obj, false);
			ref->flags|=SOMEDATA_KEEP_ASPECT;
			double m[6];
			viewport->transformToContext(m,oc,0,1);
			ref->m(m);
			Affine mm;
			transform_invert(m, base_cells->m());
			mm.m(m);
			ref->Multiply(mm);
			Group *proxybase = GetProxy(current_base, -1);
			proxybase->push(ref);
			ref->dec_count();
			//DrawableObject *dobj = dynamic_cast<DrawableObject*>(ref);
			//dobj->properties.push("tilingSource",current_base);

			int status = EditThis(ref, _("Move source object"));
			if (status == 1) {
				 //fresh rectinterface
				dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
			} else if (status == 2) { //child existed already
			}

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

/*! Render lines and/or objects into preview.
 * If active, then also check to ensure that preview is properly installed in viewport.
 */
int CloneInterface::Render()
{
	if (!tiling) return 1;

	preview->flush(); //remove old clones
	preview->push(base_cells);


	 //render lines for preview only
	Group *ret=NULL;
	if (trace_cells || preview_lines) {
		lines->flush();
		ret = tiling->Render(lines, NULL, base_cells, NULL, 0,3, 0,3, boundary, base_cells);
		if (!ret) {
			PostMessage(_("Could not clone!"));
			return 0;
		} else lines->FindBBox();
	}


	int numproxies = NumProxies();
	bool render_lines = (trace_cells || numproxies==0);
	bool render_objects = (numproxies > 0);

	 //render source object clones
	if (render_objects) {
		//create individual groups to contain source objects per basecell.
		//This makes it easier to edit elsewhere

		Group *srcs = source_proxies;
		Group *layer = preview;
		if (render_lines) {
			 //put objects in their own layer, to isolate from lines
			layer = new Group;
			layer->Id("Objects"); // *** needs to be a unique id
			preview->push(layer);
			layer->dec_count();
		}

		ret = tiling->Render(layer, srcs, base_cells, NULL, 0,3, 0,3, boundary, base_cells);
		if (srcs != source_proxies) srcs->dec_count();

		if (!ret) {
			PostMessage(_("Could not clone!"));
			return 0;
		}

		layer->FindBBox();
		if (preview != layer) preview->FindBBox();
	}

	if (render_lines) { 
		 //render lines onto its own layer
		Group *layer = preview; 
		if (render_objects) {
			 //when there are source objects, the lines are not attached, so we need to add manually
			layer = new Group;
			layer->Id("Outlines");
			preview->push(layer);
			layer->dec_count();
		}
		ret = tiling->Render(layer, NULL, base_cells, base_cells, 0,3, 0,3, boundary, base_cells);
		layer->FindBBox();
		if (preview != layer) preview->FindBBox();
	}


	if (active) {
		 //make sure preview is installed in the target context in document
		if (!previewoc) {
			LaidoutViewport *vp=dynamic_cast<LaidoutViewport*>(viewport);
			VObjContext *voc = NULL;
			
			ObjectContext *noc=NULL;
			previewoc = dynamic_cast<VObjContext*>(vp->curobj.duplicate());

			previewoc->SetObject(NULL);
			previewoc->clearToPage();

			 //make sure previewoc aligns with viewport 
			vp->ChangeContext(previewoc);
			delete previewoc; previewoc=NULL;
			vp->NewData(preview,&noc);
			voc = dynamic_cast<VObjContext*>(noc->duplicate());
			previewoc = dynamic_cast<VObjContext*>(voc);
		}
	}

	needtodraw=1;
	return 0;
}

int CloneInterface::ToggleOrientations()
{
	preview_orient = !preview_orient;
	needtodraw=1;

	PostMessage(preview_orient ? _("Preview cell orientation") : _("Don't preview cell orientation"));
	return preview_orient;
}

int CloneInterface::TogglePreview()
{
	preview_lines = !preview_lines;
	needtodraw=1;

	PostMessage(preview_lines ? _("Preview lines") : _("Don't preview lines"));
	return preview_lines;
}

/*! Returns whether active after toggling.
 */
int CloneInterface::ToggleActivated()
{
	if (!tiling) { active = false; return 0; }

	active = !active;
	
	if (active) {
		Render();

	} else {
		 //remove from document if it is there
		if (previewoc) {
			preempt_clear = true;
			LaidoutViewport *vp=dynamic_cast<LaidoutViewport*>(viewport);
			vp->DeleteObject(previewoc);
			delete previewoc;
			previewoc=NULL;
			preempt_clear = false;
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


	int firstover=CLONEI_None, which=-1;
	int dragged = buttondown.up(d->id,LEFTBUTTON, &firstover, &which);

	int i=-1,dest=-1;
	int over=scan(x,y,&i,&dest);

	//DBG flatpoint fp=dp->screentoreal(x,y);

	if (over==firstover) {
		if (over==CLONEI_Circle) {
			ToggleActivated();
			needtodraw=1;
			return 0;

		} else if (over==CLONEI_Inputs) {
			if (dragged<2) {
				// pop up input box
				char scratch[50], mes[20];

				TilingDest *dest = GetDest(extra_input_fields.e[which]);
				if (dest) {
					sprintf(scratch, "%d", dest->max_iterations);
					sprintf(mes, "setrecurse%d", which);
				} else {
					sprintf(scratch, "%d", tiling->radial_divisions);
					strcpy(mes, "setradial");
				}

				double y = box.maxy + INTERFACE_CIRCLE*uiscale + dp->textheight()*.6;
				DoubleBBox bounds(box.minx,box.maxx, y,y+dp->textheight()*1.2);
				viewport->SetupInputBox(object_id, NULL, scratch, mes, bounds); 
			} //else {}
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
			if (i!=current_selected) {
				current_selected=i;
				if (i>=0) {
					PostMessage(BuiltinTiling[current_selected]);
				}
				needtodraw=1;
			}
		}
		return 0;
	}


	//--------CMODE_Normal:

	int i=-1, dest=-1;
	int over = scan(x,y,&i,&dest);
	DBG cerr <<"over box: "<<over<<endl;

	if (!buttondown.any()) {
		if (lastover!=over || lastoveri != i) needtodraw=1;
		lastover  = over;
		lastoveri = i;
		if (lastover == CLONEI_None) PostMessage(" ");
		return 0;
	}

	//if (rectinterface.buttondown.any()) return rectinterface.MouseMove(x,y,state,mouse);


	 //button is down on something...
	int lx,ly, oldover = CLONEI_None, oldwhich=-1;
	buttondown.move(mouse->id,x,y, &lx,&ly);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &oldover, &oldwhich);

	if (oldover == CLONEI_BaseCell) {
		return 0;
	}

	if (oldover == CLONEI_Inputs) {
		int ix,iy;
		double step = dp->textheight()/2;
		buttondown.getinitial(mouse->id, LEFTBUTTON, &ix,&iy);
		int oldi = (lx-ix-step/2)/step;
		int newi = (x-ix-step/2)/step;

		if (newi!=oldi) {
			TilingDest *dest = GetDest(extra_input_fields.e[oldwhich]);
			if (dest) {
				 //change max iterations
				int it = dest->max_iterations;
				it += (newi-oldi);
				if (it<2) it=2;
				if (it != dest->max_iterations) {
					dest->max_iterations = it;
					if (active) Render();
				}
			} else {
				 //change radial_divisions
				int it = tiling->radial_divisions;
				it += (newi-oldi);
				if (it<1) it=1;
				if (it != tiling->radial_divisions) {
					UpdateRadial(tiling, it);
					UpdateBasecells();
					//if (active) Render(); <- called in UpdateBasecells()
				}

			}
			needtodraw=1;
		}
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


	int over=scan(x,y,NULL,NULL);
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


	int over = scan(x,y,NULL,NULL);
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
		 //in the tiling box selection popup box

		int found=0;
		if (ch==LAX_Esc) {
			Mode(CMODE_Normal);
			return 0;

		} else if (ch==LAX_Enter) {
			Tiling *newtiling = GetBuiltinTiling(current_selected);
			if (newtiling) {
				cur_tiling = current_selected;
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

		return 0; //preempt any key propagation while the box is up
	}

	DBG if (ch=='d') {
	DBG 	FILE *f = fopen("tile.dump", "w");
	DBG 	if (f) {
	DBG 		if (preview) preview->dump_out(f,0, 0, NULL);
	DBG			else fprintf(f, "No preview!!");
	DBG 		fclose(f);
	DBG 	}
	DBG }

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

	if (ch==LAX_Del || ch==LAX_Bksp) {
		//intercept as deleting causes unexpected behavior if propagated.
		//If we are moving a source object, then remove that one

		if (child && rectinterface.somedata != base_cells && rectinterface.somedata != boundary) {
			 //pressing delete while source object is selected
			Group *g = GetProxy(current_base, -1);
			int i = -1;
			if (g) i = g->findindex(rectinterface.somedata);
			if (i >= 0) {
				g->remove(i);
			}

			RemoveChild();
			needtodraw=1;
		}
		return 0;
	}

	if (ch==LAX_Esc) {
		if (child) {
			RemoveChild();
			needtodraw=1;
			return 0;
		}

		if (active) ToggleActivated();
		return 0;
	}

	return 1;
}



} //namespace Laidout

