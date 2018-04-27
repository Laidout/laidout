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
// Copyright (C) 2018 by Tom Lechner
//

#include <lax/interfaces/somedatafactory.h>

#include "lperspectiveinterface.h"
#include "../language.h"
#include "objectfilter.h"
#include "lpathsdata.h"


#include <iostream>
#define DBG


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {





//------------------------------- LPerspectiveInterface --------------------------------
/*! \class LPerspectiveInterface
 * \brief add on a little custom behavior.
 */


LPerspectiveInterface::LPerspectiveInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp)
  : PerspectiveInterface(nowner,nid,ndp)
{
}


LaxInterfaces::anInterface *LPerspectiveInterface::duplicate(LaxInterfaces::anInterface *dup)
{
	if (dup==NULL) dup=dynamic_cast<anInterface *>(new LPerspectiveInterface(NULL,id,NULL));
	else if (!dynamic_cast<LPerspectiveInterface *>(dup)) return NULL;

	return PerspectiveInterface::duplicate(dup);
}

int LPerspectiveInterface::UseThis(Laxkit::anObject *nobj,unsigned int mask)
{
	if (dynamic_cast<ObjectFilterInfo*>(nobj)) {
		dont_update_transform = true;

		ObjectFilterInfo *info = dynamic_cast<ObjectFilterInfo*>(nobj);
		PerspectiveNode *node = dynamic_cast<PerspectiveNode*>(info->node);
		if (!node) return 1;

		UseThisObject(info->oc);
		NodeProperty *out = info->node->FindProperty("out");
		DrawableObject *obj = dynamic_cast<DrawableObject*>(out->GetData());
		if (!obj) info->node->Update();
		if (!obj) {
			DBG cerr << " Warning! no data for out of perspectivenode!"<<endl;
			return 0;
		}

		 //replace actual data with the filtered data
		if (data) { data->dec_count(); data=NULL; }
		data = obj;
		data->inc_count();

		dont_update_transform = false;

		if (transform) transform->dec_count();
		transform = node->transform;
		transform->inc_count();

		return 0;
	}

	return PerspectiveInterface::UseThis(nobj, mask);
}


//------------------------ PerspectiveNode ------------------------

/*! \class PerspectiveNode
 * Filter that inputs one Laidout object, transforms and outputs
 * a changed Laidout object, perhaps of a totally different type.
 */


SingletonKeeper PerspectiveNode::keeper;


/*! Static return for a PerspectiveInterface.
 */
LaxInterfaces::PerspectiveInterface *PerspectiveNode::GetPerspectiveInterface()
{
	PerspectiveInterface *interf = dynamic_cast<PerspectiveInterface*>(keeper.GetObject());
	if (!interf) {
		interf = new LPerspectiveInterface(NULL, getUniqueNumber(), NULL);
		keeper.SetObject(interf, 1);
	}
	return interf;
}


PerspectiveNode::PerspectiveNode()
{
	makestr(Name, _("Perspective Filter"));
	makestr(type, "PerspectiveFilter");

	render_preview = true;
	render_dpi = 300;

	transform = new PerspectiveTransform;

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",  NULL,1, _("In"),  NULL,0, false));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "To1",  new FlatvectorValue(0,0),1, _("To 1")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "To2",  new FlatvectorValue(1,0),1, _("To 2")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "To3",  new FlatvectorValue(1,1),1, _("To 3")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "To4",  new FlatvectorValue(0,1),1, _("To 4")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"), NULL,0, false));
}

PerspectiveNode::~PerspectiveNode()
{
}

LaxInterfaces::anInterface *PerspectiveNode::ObjectFilterInterface()
{
	PerspectiveInterface *interf = GetPerspectiveInterface();
	interf->UseThis(transform);
	return interf;
}

DrawableObject *PerspectiveNode::ObjectFilterData()
{
	NodeProperty *prop = FindProperty("out");
	return dynamic_cast<DrawableObject*>(prop->GetData());
}

NodeBase *PerspectiveNode::Duplicate()
{
	PerspectiveNode *node = new PerspectiveNode();
	node->DuplicateBase(this);
	return node;
}

int PerspectiveNode::GetStatus()
{
	if (!transform->IsValid()) return -1;
	return NodeBase::GetStatus();
}

int PerspectiveNode::Update()
{
	makestr(error_message, NULL);

	NodeProperty *inprop = FindProperty("in");
	DrawableObject *orig = dynamic_cast<DrawableObject*>(inprop->GetData());

	NodeProperty *outprop = FindProperty("out");
	DrawableObject *out = dynamic_cast<DrawableObject*>(inprop->GetData());

	if (!orig) {
		outprop->SetData(NULL, 0);
		return 0;
	}

	if (IsMuted()) {
		outprop->SetData(inprop->GetData(), 0);
		return 0;
	}

	clock_t recent = MostRecentIn(NULL);
	if (recent <= out->modtime) return 0; //already up to date
	//if (recent <= out->modtime && transform->modtime <= out->modtime) return 0; //already up to date


	 // get input coordinates and:
	double v[4];
	flatvector to1, to2, to3, to4;
	if (isVectorType(properties.e[1]->GetData(), v) != 2) return -1;
	to1.set(v);
	if (isVectorType(properties.e[2]->GetData(), v) != 2) return -1;
	to2.set(v);
	if (isVectorType(properties.e[3]->GetData(), v) != 2) return -1;
	to3.set(v);
	if (isVectorType(properties.e[4]->GetData(), v) != 2) return -1;
	to4.set(v);

	transform->SetTo(to1, to2, to3, to4);
	transform->ComputeTransform();

	if (!transform->IsValid()) return -1;

	Affine toglobal(*orig), tolocal(*orig);
	tolocal.Invert();

	// *** vectors should be able to handle:
	//   base vector like PathsData
	//   clones to those
	//   groups composed of only vector objects

	//transform vectors:
	//  PathsData
	//  PatchData
	//  ColorPatchData
	//  EngraverFillData
	//  Voronoi
	//else transform rasterized

	if (dynamic_cast<PathsData*>(orig)) {
		 //go through point by point and transform.
		 //Try to preserve existing points to reduce allocations
		
		LPathsData *pathout = dynamic_cast<LPathsData*>(out);
		if (!pathout) {
			pathout = dynamic_cast<LPathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
			outprop->SetData(pathout,1);
		}

		PathsData *pathin = dynamic_cast<PathsData*>(orig);
		Coordinate *start, *p, *p2;
		Path *path, *path2;
		flatpoint fp;

		 //make sure in and out have same number of paths
		while (pathout->NumPaths() > pathin->NumPaths()) pathout->RemovePath(pathout->NumPaths()-1, NULL);
		while (pathout->NumPaths() < pathin->NumPaths()) pathout->pushEmpty();

		for (int c=0; c<pathin->NumPaths(); c++) {
			path  = pathin ->paths.e[c];
			path2 = pathout->paths.e[c];
			p  = path ->path;
			p2 = path2->path;
			start = p;

			if (p) {
				do {
					if (!p2) {
						p2 = p->duplicate();
						path2->append(p2);
					}

					fp = toglobal.transformPoint(p->fp);
					fp = transform->transform(fp);
					fp = tolocal.transformPoint(fp);
					p2->fp = fp;

					p  = p->next;
					p2 = p2->next;
				} while (p && p != start);

			} else {
				 //current path is an empty
				if (p2) path2->clear();
			}
		}

		out->modtime = recent;
		pathout->touchContents();
		return NodeBase::Update();
	}


	makestr(error_message, _("Can only use paths right now"));
	return -1;

//	if (render_preview) {
//		*** //transform rasterized at much less than print resolution
//	} else {
//		//use render_scale
//		***
//	}

}






} //namespace Laidout

