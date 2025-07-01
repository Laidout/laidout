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
// Copyright (C) 2021 by Tom Lechner
//
#include <lax/interfaces/somedatafactory.h>
#include <lax/messagebox.h>
#include <lax/pointset.h>

#include "lmirrorinterface.h"
#include "../interfaces/objectfilterinterface.h"
#include "lpathsdata.h"
#include "../language.h"


#include <iostream>
#define DBG
using namespace std;

using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


//------------------------------- LMirrorInterface --------------------------------
/*! \class LMirrorInterface
 * \brief add on a little custom behavior.
 */


LMirrorInterface::LMirrorInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp)
  : MirrorInterface(nowner,nid,ndp)
{
	node = NULL;
	mirror_node_connected = false;
}

LMirrorInterface::~LMirrorInterface()
{
	if (node) node->dec_count();
}


LaxInterfaces::anInterface *LMirrorInterface::duplicate(LaxInterfaces::anInterface *dup)
{
	if (dup==NULL) {
		MirrorInterface *p = new LMirrorInterface(NULL,id,NULL);
		dup = p;
		p->interface_flags = interface_flags;

    } else if (!dynamic_cast<LMirrorInterface *>(dup)) return NULL;

	return MirrorInterface::duplicate(dup);
}

int LMirrorInterface::UseThis(Laxkit::anObject *nobj,unsigned int mask)
{
	if (dynamic_cast<ObjectFilterInfo*>(nobj)) {
		// dont_update_transform = true;

		ObjectFilterInfo *info = dynamic_cast<ObjectFilterInfo*>(nobj);
		MirrorPathNode *nnode = dynamic_cast<MirrorPathNode*>(info->node);
		if (!nnode) {
			DBG cerr << " Warning! no data coming into MirrorPathNode!"<<endl;
			return 1;
		}

		UseThisObject(info->oc); //sets base context
		//------
//		NodeProperty *out = info->node->FindProperty("out");
//		DrawableObject *obj = dynamic_cast<DrawableObject*>(out->GetData());
//		if (!obj) info->node->Update();
//		obj = dynamic_cast<DrawableObject*>(out->GetData());
//		if (!obj) {
//			DBG cerr << " Warning! no data for out of MirrorPathNode!"<<endl;
//			return 0;
//		}
		//------
		NodeProperty *in = info->node->FindProperty("in");
		DrawableObject *obj = dynamic_cast<DrawableObject*>(in->GetData());
		if (!obj) {
			DBG cerr << " Warning! no data coming into MirrorPathNode!"<<endl;
			return 0;
		}

		 //replace actual data with the filtered in data
		if (data) { data->dec_count(); data=NULL; }
		data = obj;
		data->inc_count();

		// dont_update_transform = false;

		

		if (node != nnode) {
			if (node) node->dec_count();
			node = nnode;
			if (node) node->inc_count();
		}

		NodeProperty *p1_prop = node->FindProperty("p1");
		NodeProperty *p2_prop = node->FindProperty("p2");

		FlatvectorValue *p1 = dynamic_cast<FlatvectorValue*>(p1_prop ? p1_prop->GetData() : nullptr);
		FlatvectorValue *p2 = dynamic_cast<FlatvectorValue*>(p2_prop ? p2_prop->GetData() : nullptr);
		
		if (mirrordata) { mirrordata->dec_count(); mirrordata = nullptr; }

		if (!p1 || !p2) {
			DBG cerr << "Missing p1 or p2 from mirror node!!"<<endl;
			return 0;
		}

		mirror_node_connected = (p1_prop->IsConnected() || p2_prop->IsConnected());

		mirrordata = new MirrorData();
		mirrordata->p1 = p1->v;
		mirrordata->p2 = p2->v;

		needtodraw=1;
		return 0;
	}

	return MirrorInterface::UseThis(nobj, mask);
}

#define BUTTON_Ok 0
#define BUTTON_Unlink 1
#define BUTTON_Edit 2

int LMirrorInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	int nhover = scan(x,y,state,count);
	if (nhover != MIRROR_None && mirror_node_connected) {
		// node is locked by node connections, ask user what they want
		PostMessage(_("Mirror has connected nodes, cannot edit."));

		MessageBox *box = new MessageBox("mirrorlocked", object_id, "mirrorlocked", _("Cannot edit while mirror is controlled with nodes."));
		box->AddButton(_("Ok"), BUTTON_Ok, nullptr);
		// box->AddButton(_("Unlink points"), BUTTON_Unlink, nullptr);
		box->AddButton(_("Edit nodes"), BUTTON_Edit, nullptr);

		app->rundialog(box);
		return 1;
	}

	return MirrorInterface::LBDown(x,y, state, count, d);
}

int LMirrorInterface::Event(const Laxkit::EventData *e, const char *mes)
{
	if (strcmp(mes, "mirrorlocked")) return MirrorInterface::Event(e,mes);

	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);

	if (s->info2 == BUTTON_Unlink) {
		PostMessage("IMPLEMENT UNLINK");

	} else if (s->info2 == BUTTON_Edit) {
		// PerformAction(MIRROR_Edit_Nodes);
		PostMessage("IMELEMENT EDIT FROM MIRROR INTERFACE");
		anInterface *objfilter = viewport->HasInterface("ObjectFilterInterface", nullptr);
		if (objfilter) {
			objfilter->PerformAction(ObjectFilterInterface::OFI_Edit_Nodes);
		}
	} // else just ignore ok

	return 0;
}

/*! Mirror has updated p1 or p2, so this updates node state with that information
 * whenever the interface thinks it has been modified.
 */
void LMirrorInterface::Modified(int level)
{
	if (!node) return;

	 //update pnode
	FlatvectorValue *v;

	// only update if not connected, then reverse update transform
	NodeProperty *prop = node->properties.e[1];
	v = dynamic_cast<FlatvectorValue *>(prop->GetData());

	// bool rev = false;
	if (v) {
		if (!prop->IsConnected()) {
			v->v = mirrordata->p1;
		} else {
			mirrordata->p1 = v->v;
			// rev = true;
		}		
	}
	prop = node->properties.e[2];
	v = dynamic_cast<FlatvectorValue *>(prop->GetData());
	if (v) {
		if (!prop->IsConnected()) {
			v->v = mirrordata->p2;
		} else {
			mirrordata->p2 = v->v;
			// rev = true;
		}
	}
	
	node->properties.e[1]->Touch();
	node->properties.e[2]->Touch();
	node->Update();
	node->PropagateUpdate();

	// make sure "base" object is still correct
	DrawableObject *obj = dynamic_cast<DrawableObject*>(node->properties.e[0]->GetData());
	if (obj != data) {
		if (obj) {
			if (data) data->dec_count();
			data = obj;
			data->inc_count(); //note oc will point to base object still
		}
	}

	needtodraw = 1;
}


//------------------------------ MirrorPathNode ---------------------------------


/*! \class MirrorPathNode
 * Node to mirror things across an axis.
 */


MirrorPathNode::MirrorPathNode()
{
	makestr(Name, _("Mirror"));
	makestr(type, "Filters/Mirror");


	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",  NULL,1, _("In"),  NULL,0, false));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "p1",  new FlatvectorValue(0,0),1, _("p1")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "p2",  new FlatvectorValue(1,0),1, _("p2")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "merge",  new BooleanValue(false),1, _("Merge"), _("Merge mirrored end points close to the mirror axis")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "merge_dist",  new DoubleValue(.01),1, _("Merge Distance"), _("When merging, the merge threshhold")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"), NULL,0, false));
}

MirrorPathNode::~MirrorPathNode()
{
}


SingletonKeeper MirrorPathNode::keeper;

/*! Static return for a MirrorInterface.
 */
LaxInterfaces::MirrorInterface *MirrorPathNode::GetMirrorInterface()
{
	MirrorInterface *interf = dynamic_cast<MirrorInterface*>(keeper.GetObject());
	if (!interf) {
		interf = new LMirrorInterface(NULL, getUniqueNumber(), NULL);
		interf->Id("keepermirror");
		//interf->interface_flags |= LaxInterfaces::MirrorInterface::PERSP_Dont_Change_Object;
		keeper.SetObject(interf, 1);
	}
	return interf;
}

LaxInterfaces::anInterface *MirrorPathNode::ObjectFilterInterface()
{
	MirrorInterface *interf = GetMirrorInterface();
	return interf;
}

DrawableObject *MirrorPathNode::ObjectFilterData()
{
	NodeProperty *prop = FindProperty("out");
	return dynamic_cast<DrawableObject*>(prop->GetData());
}

NodeBase *MirrorPathNode::Duplicate()
{
	MirrorPathNode *node = new MirrorPathNode();
	node->DuplicateBase(this);
	node->DuplicateProperties(this);
	return node;
}

/*! Install p1 and p2 into the node.
 */
int MirrorPathNode::UpdateMirror(flatpoint p1, flatpoint p2)
{
	// NodeProperty *inprop = FindProperty("in");

	if (!properties.e[1]->IsConnected()) {
		FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData());
		if (!fv) {
			fv = new FlatvectorValue(p1);
			properties.e[1]->SetData(fv,1);
		} else fv->v = p1;
		properties.e[1]->Touch();
	}

	if (!properties.e[2]->IsConnected()) {
		FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[2]->GetData());
		if (!fv) {
			fv = new FlatvectorValue(p2);
			properties.e[2]->SetData(fv,1);
		} else fv->v = p2;
		properties.e[2]->Touch();
	}

	return 0;
}

int MirrorPathNode::GetStatus()
{
	// should be able to mirror:
	//   set of points
	//   PointSet
	//   PathsData with merge
	//   Other Objects: make group with clones
	//   
	// should be able to flip:
	//   points
	//   set of points
	//   PointSet
	//   PathsData
	//   Other Objects: produce clones


	if (!dynamic_cast<PathsData*>(properties.e[0]->GetData())) return -1;
	return NodeBase::GetStatus();
}

void UpdatePath(Path *path, Path *path2, Affine *transform)
{
	path2->needtorecache = true;

	Coordinate *p  = path->path;
	Coordinate *p2 = path2->path;
	Coordinate *start  = p;
	Coordinate *start2 = p2;
	flatpoint fp;

	if (p) {
		do {
			if (!p2) {
				p2 = p->duplicate();
				path2->append(p2);
				if (!start2) start2 = p2;
			} else {
				*p2 = *p;
			}

			// fp = transform->transform(p->fp);
			p2->fp = (transform ? transform->transformPoint(p->fp) : p->fp);

			p  = p->next;
			p2 = p2->next;
			if (p2 == start2 && p != start) p2 = nullptr;
		} while (p && p != start);

		if (p == start && !p2) {
			path2->close();
		}

		if (p == start && p2 && p2 != start2) {
			 //too many points in path2! remove extra...
			 while (p2 && p2 != start2) {
			 	Coordinate *prev = p2->prev;
			 	path2->removePoint(p2, true);
			 	if (prev) p2 = prev->next;
			 	else p2 = nullptr;
			 }
		}

	} else {
		 //current path is an empty
		if (p2) path2->clear();
	}

	// copy over path weights
	while (path2->pathweights.n > path->pathweights.n) path2->pathweights.remove(path2->pathweights.n-1);
	for (int c2 = 0; c2 < path->pathweights.n; c2++) {
		PathWeightNode *w1 = path->pathweights.e[c2];
		if (c2 >= path2->pathweights.n)
			path2->AddWeightNode(w1->t, w1->offset, w1->width, w1->angle);
		else *path2->pathweights.e[c2] = *w1;
	}
}


int MirrorPathNode::Update()
{
	Error(nullptr);

	Value *origv = properties.e[0]->GetData();
	NodeProperty *outprop = FindProperty("out");
	Value *outv = outprop->GetData();

	if (IsMuted()) {
		outprop->SetData(origv, 0);
		return 0;
	} else if (outv == origv) {
		//need to unset the mute
		outprop->SetData(nullptr, 0);
		outv = nullptr;
	}

	int isnum = 0;
	bool merge = getIntValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) {
		Error(_("Expected boolean"));
		return -1;
	}

	double merge_distance = getNumberValue(properties.e[4]->GetData(), &isnum);
	if (!isnum) {
		Error(_("Expected number"));
		return -1;
	}


	// if (!origv || (origv->type() != LAX_PATHSDATA && origv->type() != LAX_VORONOIDATA)) { *** type is currently built in only
	if (!origv || !dynamic_cast<PathsData*>(origv)) {
		Error(_("Cannot apply mirror to that type!"));
		return -1;
	}

	FlatvectorValue *p1 = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData());
	if (!p1) { Error(_("Expected Vector2")); return -1; }

	FlatvectorValue *p2 = dynamic_cast<FlatvectorValue*>(properties.e[2]->GetData());
	if (!p2) { Error(_("Expected Vector2")); return -1; }


	Affine transform;
	transform.Flip(p1->v, p2->v);

	if (dynamic_cast<PathsData*>(origv)) {
		 //go through point by point and transform.
		 //Try to preserve existing points to reduce allocations
		
		LPathsData *pathin = dynamic_cast<LPathsData*>(origv);
		
		LPathsData *pathout = dynamic_cast<LPathsData*>(outv);
		if (pathout) {
			pathout->m(pathin->m()); //sets affine transform only

		} else if (!pathout) {
			pathout = dynamic_cast<LPathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
			pathout->Id("Mirrored");
			pathout->set(*pathin); //sets the affine transform
			pathout->InstallLineStyle(pathin->linestyle);
			pathout->InstallFillStyle(pathin->fillstyle);
			outprop->SetData(pathout,1);
			outv = pathout;
		}

		DBG cerr << "pathout "<<pathout->Id()<<" previous bbox: "<<pathout->minx<<','<<pathout->maxx<<" "<<pathout->miny<<','<<pathout->maxy<<endl;

		// Coordinate *start, *start2, *p, *p2;
		Path *path, *path2, *path2m;
		// flatpoint fp;

		 //make sure in and out have same number of paths
		while (pathout->NumPaths() > 2*pathin->NumPaths()) pathout->RemovePath(pathout->NumPaths()-1, NULL);
		while (pathout->NumPaths() < 2*pathin->NumPaths()) pathout->pushEmpty();

		for (int c=0; c<pathin->NumPaths(); c++) {
			path   = pathin ->paths.e[c];
			path2  = pathout->paths.e[c];
			path2m = pathout->paths.e[pathin->paths.n + c];
		
			UpdatePath(path, path2, nullptr);
			UpdatePath(path, path2m, &transform);
		}

		if (merge) {
			flatline line(p1->v, p2->v);
			int pathi = pathout->NumPaths()/2;
			for (int c=0; c<pathout->NumPaths()/2; c++) {
				path  = pathout->paths.e[c];
				if (!path->path || path->IsClosed()) continue;

				double dist = distance(path->path->fp, line);
				double dist2 = distance(path->lastPoint(1)->fp, line);

				bool p1_merged = false;
				if (dist < merge_distance) {
					p1_merged = pathout->MergeEndpoints(path->path,c, pathout->paths.e[pathi]->path,pathi);
					pathi--;
				}

				if (dist2 < merge_distance) {
					// might be different path or path indices now, so check
					if (p1_merged) {
						pathout->MergeEndpoints(path->path,c, path->lastPoint(1),c);
					} else {
						pathout->MergeEndpoints(path->lastPoint(1),c, pathout->paths.e[pathi]->lastPoint(1),pathi);
						pathi--;
					}
				}

				pathi++;
			}
		}

		pathout->FindBBox();
		DBG cerr << "pathout new bbox: "<<pathout->minx<<','<<pathout->maxx<<" "<<pathout->miny<<','<<pathout->maxy<<endl;
		outprop->Touch();
		pathout->touchContents();

		DBG cerr << "mirrored out:"<<endl;
		DBG pathout->dump_out(stderr, 10, 0, NULL);
		
		return NodeBase::Update();

	// } else if (dynamic_cast<VoronoiData*>(orig)) {
	// 	VoronoiData *vdata = dynamic_cast<VoronoiData*>(orig);

	// 	return NodeBase::Update();

	// } else if (dynamic_cast<EngraverFillData*>(orig)) {
	// } else if (dynamic_cast<ColorPatchData*>(orig)) {
	// } else if (dynamic_cast<ImagePatchData*>(orig)) {
	// } else if (!strcmp(orig->whattype(), "PatchData")) {
	// } else { // transform rasterized

	}

	Error(_("Cannot apply mirror to that type!"));
	return -1;
}



} //namespace Laidout

