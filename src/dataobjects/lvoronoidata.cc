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
// Copyright (C) 2016 by Tom Lechner
//

#include "lvoronoidata.h"
#include "datafactory.h"
#include "group.h"
#include "../core/stylemanager.h"
#include "../language.h"
#include "../calculator/shortcuttodef.h"

#include <lax/interfaces/pathinterface.h>


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {




/*! \class LVoronoiData 
 * \brief Subclassing LaxInterfaces::VoronoiData
 */



LVoronoiData::LVoronoiData(LaxInterfaces::SomeData *refobj)
{
	Id();
}

LVoronoiData::~LVoronoiData()
{
}

void LVoronoiData::FindBBox()
{
	VoronoiData::FindBBox();
}

void LVoronoiData::ComputeAABB(const double *transform, DoubleBBox &box)
{
	VoronoiData::ComputeAABB(transform, box);
	DrawableObject::ComputeAABB(transform, box);
}

/*! Provide final pointin() definition.
 */
int LVoronoiData::pointin(flatpoint pp,int pin)
{
	return VoronoiData::pointin(pp,pin);
}

void LVoronoiData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	LaxFiles::Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
	// char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	// DrawableObject::dump_out(f,indent,what,context);
	// fprintf(f,"%sconfig\n",spc);
	// VoronoiData::dump_out(f,indent+2,what,context);
}

LaxFiles::Attribute *LVoronoiData::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	att = DrawableObject::dump_out_atts(att, what,context);
	LaxFiles::Attribute *att2 = att->pushSubAtt("config");
	VoronoiData::dump_out_atts(att2, what,context);
	return att;
}

void LVoronoiData::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			VoronoiData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) VoronoiData::dump_in_atts(att,flag,context);
}

LaxInterfaces::SomeData *LVoronoiData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LVoronoiData*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("VoronoiData"));
	VoronoiData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}

LaxInterfaces::SomeData *LVoronoiData::EquivalentObject()
{
	int n=0;
	if (show_delaunay) n++;
	if (show_points) n++;
	if (show_voronoi) n++;
	//if (show_numbers) n++;
	if (!n) return NULL;

	Group *group=NULL;
	if (n>1) group=dynamic_cast<Group*>(LaxInterfaces::somedatafactory()->NewObject("Group"));

	if (show_voronoi) {
		PathsData *paths=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		paths->line(width_voronoi, LAXCAP_Round,LAXJOIN_Round, &color_voronoi->screen);
		paths->fill(NULL);
		paths->Id("voronoi");

        for (int c=0; c<regions.n; c++) {
            if (regions.e[c].tris.n==0) continue;
			if (c>0) paths->pushEmpty();

            int i=regions.e[c].tris.e[0];
            if (i>=0) paths->lineTo(triangles.e[i].circumcenter);
            else paths->lineTo(inf_points.e[-i-1]);

            for (int c2=1; c2<regions.e[c].tris.n; c2++) {
                i=regions.e[c].tris.e[c2];
                if (i>=0) paths->lineTo(triangles.e[i].circumcenter);
                else paths->lineTo(inf_points.e[-i-1]);
            }
			// *** closed?
        }

		if (group) { group->push(paths); paths->dec_count(); }
		else return paths;
	} 

	if (show_delaunay) {
		PathsData *paths=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		paths->line(width_delaunay, LAXCAP_Round,LAXJOIN_Round, &color_delaunay->screen);
		paths->fill(NULL);
		paths->Id("delaunay");
		for (int c=0; c<triangles.n; c++) {
			paths->moveTo(points.e[triangles[c].p1]->p);
			paths->lineTo(points.e[triangles[c].p2]->p);
			paths->lineTo(points.e[triangles[c].p3]->p);
			paths->close();
		}	 

		if (group) { group->push(paths); paths->dec_count(); }
		else return paths;
	}

	if (show_points) {
		PathsData *paths=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
		//paths->InstallLineStyle(NULL);
		paths->line(width_points/10, LAXCAP_Round,LAXJOIN_Round, &color_points->screen);
		paths->fill(&color_points->screen);
		paths->Id("points");
		for (int c=0; c<points.n; c++) {
			if (c!=0) paths->pushEmpty();
			paths->appendEllipse(points.e[c]->p, width_points, width_points, 2*M_PI, 0, 4, 1);
			paths->close();
		}

		if (group) { group->push(paths); paths->dec_count(); }
		else return paths;
	}

	return group;
}


//------- VoronoiData.Value functions:

Value *LVoronoiData::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("VoronoiData"));
	VoronoiData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

Value *NewLVoronoiData() { return new LVoronoiData; }

ObjectDef *LVoronoiData::makeObjectDef()
{
	ObjectDef *sd=stylemanager.FindDef("VoronoiData");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *gdef = stylemanager.FindDef("Group");
	if (!gdef) {
		Group g;
		gdef = g.GetObjectDef();
	}
	sd = new ObjectDef(gdef,
			"VoronoiData",
            _("Voronoi Data"),
            _("Delaunay triangles and Voronoi regions"),
            NewLVoronoiData,NULL);
	stylemanager.AddObjectDef(sd, 0);

//	sd->pushFunction("FlipColors",_("Flip Colors"),_("Flip the order of colors"), NULL,
//					 NULL);
//
//	sd->pushVariable("p1", _("p1"),         _("The starting point"), "real",0, NULL,0);
//	sd->pushVariable("p2", _("p2"),         _("The ending point"),   "real",0, NULL,0);
//	sd->pushVariable("r1", _("Distance 1"), _("Radius or distance"), "real",0, NULL,0);
//	sd->pushVariable("r2", _("Distance 2"), _("Radius or distance"), "real",0, NULL,0);
	//sd->pushVariable("angle",_("Angle"),_("Angle gradient exists at"), NULL,0);

	//sd->pushVariable("stops",  _("Stops"),  _("The set of color positions"), NULL,0);

	return sd;
}

Value *LVoronoiData::dereference(const char *extstring, int len)
{
//	if (extequal(extstring,len, "p1")) {
//		return new DoubleValue(p1);
//	}
//
//	if (extequal(extstring,len, "p2")) {
//		return new DoubleValue(p2);
//	}
//
//	if (extequal(extstring,len, "r1")) {
//		return new DoubleValue(r1);
//	}
//
//	if (extequal(extstring,len, "r2")) {
//		return new DoubleValue(r2);
//	}

//	if (extequal(extstring,len, "colors")) {
//		return new DoubleValue(maxy);
//	}

	return DrawableObject::dereference(extstring, len);
}

int LVoronoiData::assign(FieldExtPlace *ext,Value *v)
{
//	if (ext && ext->n()==1) {
//		const char *str=ext->e(0);
//		int isnum;
//		double d;
//		if (str) {
//			if (!strcmp(str,"p1")) {
//				d=getNumberValue(v, &isnum);
//				if (!isnum) return 0;
//				p1=d;
//				FindBBox();
//				return 1;
//
//			} else if (!strcmp(str,"p2")) {
//				d=getNumberValue(v, &isnum);
//				if (!isnum) return 0;
//				p2=d;
//				FindBBox();
//				return 1;
//
//			} else if (!strcmp(str,"r1")) {
//				d=getNumberValue(v, &isnum);
//				if (!isnum) return 0;
//				r1=d;
//				FindBBox();
//				return 1;
//
//			} else if (!strcmp(str,"r2")) {
//				d=getNumberValue(v, &isnum);
//				if (!isnum) return 0;
//				r2=d;
//				FindBBox();
//				return 1;
//
//			}
//		}
//	}

	return DrawableObject::assign(ext,v);
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int LVoronoiData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, ErrorLog *log)
{
//	if (len==10 && !strncmp(func,"FlipColors",10)) {
//		//if (parameters && parameters->n()) return -1; *** if pp has this, then huh!
//
//		FlipColors();
//		return 0;
//	}

	return DrawableObject::Evaluate(func, len, context, parameters, settings, value_ret, log);
}



//------------------------------- LDelaunayInterface --------------------------------
/*! \class LDelaunayInterface
 * \brief add on a little custom behavior.
 */


LDelaunayInterface::LDelaunayInterface(int nid,Laxkit::Displayer *ndp)
  : DelaunayInterface(NULL, nid,ndp)
{
}


LaxInterfaces::anInterface *LDelaunayInterface::duplicate(LaxInterfaces::anInterface *dup)
{
	if (dup==NULL) dup=dynamic_cast<anInterface *>(new LDelaunayInterface(id,NULL));
	else if (!dynamic_cast<LDelaunayInterface *>(dup)) return NULL;

	return DelaunayInterface::duplicate(dup);
}


//! Returns this, but count is incremented.
Value *LDelaunayInterface::duplicate()
{
    this->inc_count();
    return this;
}


ObjectDef *LDelaunayInterface::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("DelaunayInterface");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd=new ObjectDef(NULL,"DelaunayInterface",
            _("Voronoi Interface"),
            _("Voronoi Interface"),
            "class",
            NULL,NULL);

	if (!sc) sc=GetShortcuts();
	ShortcutsToObjectDef(sc, sd);

	stylemanager.AddObjectDef(sd,0);
	return sd;
}


///*!
// * Return
// *  0 for success, value optionally returned.
// * -1 for no value returned due to incompatible parameters, which aids in function overloading.
// *  1 for parameters ok, but there was somehow an error, so no value returned.
// */
//int LDelaunayInterface::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//	                     Value **value_ret, ErrorLog *log)
//{
//	return 1;
//}

/*! *** for now, don't allow assignments
 *
 * If ext==NULL, then assign v to replace what exists in this.
 * Otherwise assign v to the value at the end of the extension.
 *  
 * Return 1 for success.
 *  2 for success, but other contents changed too.
 *  0 for total fail, as when v is wrong type.
 *  -1 for bad extension.
 */
int LDelaunayInterface::assign(FieldExtPlace *ext,Value *v)
{
	 //assignments not allowed
	return 0;
}

Value *LDelaunayInterface::dereference(const char *extstring, int len)
{
	return NULL;
}

void LDelaunayInterface::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	anInterface::dump_out(f,indent,what,context);
}

void LDelaunayInterface::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	anInterface::dump_in_atts(att,flag,context);
}

LaxFiles::Attribute *LDelaunayInterface::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	return att;
}

} //namespace Laidout

