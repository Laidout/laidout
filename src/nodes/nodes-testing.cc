
//*********** WORKS IN PROGRESS ****************

TODO:
  FitIn
  ObjectArrayNode
  MirrorPathNode
  AlignToBounds
  MergePaths
  Printf
  MenuValue

  EulerToQuaternion
  QuaternionToEuler
  Swizzle


//----------------------- GroupProxyNode ------------------------

 *** not sure if this class is worth it
/*! \class GroupProxyNode
 * Specific node to coordinate passing parameters across group nesting boundaries
 */
class GroupProxyNode : public NodeBase
{
  public:
	GroupProxyNode();
	virtual ~GroupProxyNode();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new GroupProxyNode(); }
};

GroupProxyNode::GroupProxyNode(bool ins)
{
	makestr(type, "GroupProxy");
	makestr(Name, ins ? _("Inputs") : _("Outputs"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,     _("Input"), _("A path or model")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "dir",   new FlatvectorValue(0,0,1),1,  _("Vector"),  _("Vector or path")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
}

GroupProxyNode::~GroupProxyNode()
{
}

int GroupProxyNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int GroupProxyNode::Update()
{

}



//----------------------- FitInNode ------------------------

/*! \class FitInNode
 *
 * Scale and move an object to fit within bounds of parent space.
 */
class FitInNode : public NodeBase
{
  public:
	FitInNode();
	virtual ~FitInNode();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new FitInNode(); }
};

FitInNode::FitInNode()
{
	makestr(type, "Drawable/FitIn");
	makestr(Name, _("Fit In"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in",     NULL,1,    _("Input"),  _("A DrawableObject")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "bounds", nullptr,1, _("Bounds"), nullptr));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "padding", new DoubleValue(0),1, _("Padding"), _("A number, or set of 4: [top, right, bottom, left]")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "space", new IntValue(1),1, _("Parent space"), _("Number of parents up to use as bounds space. -1 for top.")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "scaleup", new BooleanValue(false),1, _("If smaller that bounds, scale up"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "scaledown", new BooleanValue(true),1, _("If Larger than bounds, scale down"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "alignx", new DoubleValue(50),1, _("Align horizontally, 0..100"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "aligny", new DoubleValue(50),1, _("Align vertically, 0..100"), nullptr,0,true));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
}

FitInNode::~FitInNode()
{
}

int FitInNode::GetStatus()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;
	BBoxValue *b = dynamic_cast<BBoxValue*>(properties.e[1]->GetData());
	if (!b) return -1;
	if (!isNumberType(properties.e[2]->GetData()), nullptr) {
		if (!dynamic_cast<SetValue*>(properties.e[2]->GetData())) return -1;
	}
	if (!isNumberType(properties.e[3]->GetData()), nullptr) return -1;
	if (!isNumberType(properties.e[4]->GetData()), nullptr) return -1;
	if (!isNumberType(properties.e[5]->GetData()), nullptr) return -1;
	if (!isNumberType(properties.e[6]->GetData()), nullptr) return -1;
	if (!isNumberType(properties.e[7]->GetData()), nullptr) return -1;
	
	return NodeBase::GetStatus();
}

int FitInNode::Update()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;
	BBoxValue *bounds = dynamic_cast<BBoxValue*>(properties.e[1]->GetData());
	if (!bounds) return -1;
	double padding;
	SetValue *padset = nullptr;
	if (!isNumberType(properties.e[2]->GetData()), &padding) {
		padset = dynamic_cast<SetValue*>(properties.e[2]->GetData());
		if (!padset) return -1;
	}

	int parents = getNumberValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) return -1;
	bool scaleup = getNumberValue(properties.e[4]->GetData(), &isnum);
	if (!isnum) return -1;
	bool scaledown = getNumberValue(properties.e[5]->GetData(), &isnum);
	if (!isnum) return -1;
	double alignx = getNumberValue(properties.e[6]->GetData(), &isnum);
	if (!isnum) return -1;
	double alignx = getNumberValue(properties.e[7]->GetData(), &isnum);
	if (!isnum) return -1;

	***

	return NodeBase::Update();
}


//----------------------- ObjectArrayNode ------------------------

/*! \class ObjectArrayNode
 *
 * Analagous to Blender's array modifier.
 */
class ObjectArrayNode : public NodeBase
{
  public:
	ObjectArrayNode();
	virtual ~ObjectArrayNode();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ObjectArrayNode(); }
};

ObjectArrayNode::ObjectArrayNode()
{
	makestr(type, "Drawable/Array");
	makestr(Name, _("Object Array"));

	//obj in, or set of objects (as from previous array) set uses bbox of whole set
	//shift:
	//  x,y as bbox multiple
	//  x,y as absolute distance
	//  x,y scale around clone origin
	//  angle around clone origin
	//count 
	//merge if path
	//start cap
	//end cap

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,     _("Input"), _("A path or model")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "dir",   new FlatvectorValue(0,0,1),1,  _("Vector"),  _("Vector or path")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
}

ObjectArrayNode::~ObjectArrayNode()
{
}

int ObjectArrayNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int ObjectArrayNode::Update()
{

}



//------------------------------ concat --------------------------------------------

class PrintfNode : public NodeBase
{
  public:
	PrintfNode(const char *s1, const char *s2);
	virtual ~PrintfNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

PrintfNode::PrintfNode(const char *s1, const char *s2)
{
	makestr(Name, _("Printf"));
	makestr(type, "Strings/Printf");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "format", new StringValue(),1, ""));
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", new StringValue(ns),1, _("Out"), nullptr, 0, false));
}

PrintfNode::~PrintfNode()
{
}

NodeBase *PrintfNode::Duplicate()
{
	PrintfNode *newnode = new PrintfNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int PrintfNode::GetStatus()
{
	if (!dynamic_cast<StringValue*>(properties.e[0]->GetData())) return -1;
	return NodeBase::GetStatus(); //default checks mod times
}

int PrintfNode::SetProperties(const char *fmt)
{

}

int PrintfNode::Update()
{
	// NOT doing: %2$
	// NOT doing: length modifiers

	StringValue *fmtstr = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (!fmtstr) return -1;
	const char *fmt = fmtstr->str();

	if (modtime == 0 || modtime < properties.e[0]->modtime) SetProperties(fmt);

	Utf8String str, str2, str3;

	int cur_param = 0;
	int precision = 0;
	int fieldwidth = 0;
	int flags = 0;
	const char *ptr;
	const char *fstart = nullptr, *endptr;
	int i;
	char ch[2];
	ch[1] = '\0';

	#define ZEROPAD   (1<<0)
	#define LEFTJUST  (1<<1)
	#define BLANKSIGN (1<<2)
	#define PLUSMINUS (1<<3)

	//each input has: flags, field width, precision, type
	// flags: 
	//   0  zero padding
	//   -  make left justified within field
	//  ' ' insert a blank before positive number
	//   +  insert a + or - before numbers (never blank)
	//   #  NOT IMPLEMENTED -- force output with a decimal point, make first char a digit
	//   '  NOT IMPLEMENTED -- group by thounds
	//   I  NOT IMPLEMENTED -- locale alternate digits
	ptr = fmt;
	while (*ptr) {
		if (*ptr != '%') {
			ch[0] = *ptr;
			str.Append(ch);
			ptr++;
			continue;
		}

		if (ptr[1] == '%') {
			ch[0] = '%'
			str.Append(ch);
			ptr += 2;
			continue;
		}

		fstart = ptr;
		ptr++;
		precision = 1;
		fieldwidth = 1;
		flags = 0;

		while (strchr("0- +", *ptr)) {
			switch(*ptr) {
				case '0': flags |= ZEROPAD; break;
				case '-': flags |= LEFTJUST; break;
				case ' ': flags |= BLANKSIGN; break;
				case '+': flags |= PLUSMINUS; break;
			}
			ptr++;
		}

		if (isdigit(*ptr)) {
			// read in field width
			i = strtol(ptr, &endptr, 10);
			fieldwidth = i;
			ptr = endptr;
		}
		if (*ptr == '.') {
			// read in precision
			ptr++;
			if (isdigit(*ptr)) {
				i = strtol(ptr, &endptr, 10);
				precision = i;
				ptr = endptr;
			} else precision = 0;
		}

		if (!strchr("diouxXbeEfFgGcs", *ptr)) { //NOT doing aA (fractional hex), or p n m
			// add unformatted string, type was not recognized
			str.AppendN(fstart, ptr - fstart+1);
			ptr++;
		} else {
			//valid formatting, perform conversion
			char type = *ptr;
			ptr++;
			if (type == 'c') type = 's'; //we don't do chars per se
			if (type == 'b') {
				***; //our own special binary out

			} else if (type == 's') {
				//string/char
				str2.Sprintf("%%%s%s%ds", (flags&ZEROPAD) ? "0" : "", (flags&LEFTJUST) ? "-":"", fieldwidth);
				str3.Sprintf(str2.c_str(), sv->str);

			} else if (type == 'd' || type == 'i' || type == 'o' || type == 'u' || type == 'x' || type == 'X') {
				//integers
				str2.Sprintf("%%%s%s%d%c", (flags&ZEROPAD) ? "0" : "", (flags&LEFTJUST) ? "-":"", fieldwidth, type);
				str3.Sprintf(str2.c_str(), i);

			} else if (type == 'e' || type == 'E' || type == 'f' || type == 'F' || type == 'g' || type == 'G') {
				//floating point
				str2.Sprintf("%%%s%s%d", (flags&ZEROPAD) ? "0" : "", (flags&LEFTJUST) ? "-":"", fieldwidth);
				if (precision != 0) {
					str3.Sprintf(".%d", precision);
					str2.Append(str3);
				}
				ch[0] = type;
				str2.Append(ch);

				str3.Sprintf(str2.c_str(), d);
			}
		}
	}


	return NodeBase::Update();
}


Laxkit::anObject *newPrintfNode(int p, Laxkit::anObject *ref)
{
	return new PrintfNode(NULL,NULL);
}





//------------------------------ MenuValue ---------------------------------

/*! Value so that node properties can spawn custom selection windows, like popup menus or date selectors
 */
class MenuValue : public Value
{
  public:
  	Laxkit::MenuInfo *menu;
  	char *current;

  	MenuValue(MenuInfo *menu, bool absorb, const char *curvalue);
  	virtual ~MenuValue();

  	virtual void SetMenu(Laxkit::MenuInfo *nmenu, bool absorb);
  	virtual anXWindow *GetDialog();
};

MenuValue::MenuValue(MenuInfo *menu, bool absorb, const char *curvalue)
{
	this->menu = menu;
	if (!absorb && menu) menu->inc_count();
	current = nullptr;
	makestr(current, curvalue);
}

MenuValue::~MenuValue()
{
	if (menu) menu->dec_count();
	delete[] current;
}

void MenuValue::SetMenu(Laxkit::MenuInfo *nmenu, bool absorb)
{
	if (menu != nmenu) {
		if (menu) menu->dec_count();
		menu = nmenu;
	}
	if (!absorb && menu) menu->inc_count();
}

Laxkit::anXWindow *MenuValue::GetDialog(Laxkit::EventReceiver *proxy)
{
	if (!menu) return nullptr;

	PopupMenu *popup=new PopupMenu(NULL,menu->title, 0,
							0,0,0,0, 1,
							object_id,"prop_dialog_ret",
							0, //mouse to position near?
							menu,1, NULL,
							TREESEL_LEFT|TREESEL_SEND_PATH|TREESEL_LIVE_SEARCH);
	popup->WrapToMouse(0);
	return popup;
	// anXApp::app->rundialog(popup);
}


//------------------------------ MirrorNode ---------------------------------

class MirrorPathNode : public ObjectFilterNode
{
	static SingletonKeeper keeper; //the def for the op enum

  public:
	static LaxInterfaces::PerspectiveInterface *GetPerspectiveInterface();

	bool render_preview;
	double render_dpi;
	LaxInterfaces::PerspectiveTransform *transform;
	Laxkit::Affine render_transform;

	PerspectiveNode();
	virtual ~PerspectiveNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int UpdateTransform();
	virtual int GetStatus();

	virtual LaxInterfaces::anInterface *ObjectFilterInterface();
	virtual DrawableObject *ObjectFilterData();
	// virtual int Mute(bool yes=true);
	//virtual int Connected(NodeConnection *connection);

};

SingletonKeeper MirrorPathNode::keeper;

/*! Static return for a PerspectiveInterface.
 */
LaxInterfaces::MirrorInterface *MirrorPathNode::GetPerspectiveInterface()
{
	PerspectiveInterface *interf = dynamic_cast<PerspectiveInterface*>(keeper.GetObject());
	if (!interf) {
		interf = new LPerspectiveInterface(NULL, getUniqueNumber(), NULL);
		interf->Id("keeperpersp");
		interf->interface_flags |= LaxInterfaces::PerspectiveInterface::PERSP_Dont_Change_Object;
		keeper.SetObject(interf, 1);
	}
	return interf;
}


MirrorPathNode::MirrorPathNode()
{
	makestr(Name, _("Mirror"));
	makestr(type, "Filters/Mirror");


	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",  NULL,1, _("In"),  NULL,0, false));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "p1",  new FlatvectorValue(0,0),1, _("p1")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "p2",  new FlatvectorValue(1,0),1, _("p2")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"), NULL,0, false));
}

MirrorPathNode::~MirrorPathNode()
{
}

LaxInterfaces::anInterface *MirrorPathNode::ObjectFilterInterface()
{
	PerspectiveInterface *interf = GetPerspectiveInterface();
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

int MirrorPathNode::GetStatus()
{
	if (!dynamic_cast<PathsData*>(properties.e[0]->GetData())) return -1;
	return NodeBase::GetStatus();
}

int MirrorPathNode::UpdateMirror(flatpoint p1, flatpoint p2)
{
	NodeProperty *inprop = FindProperty("in");

	if (!properties.e[1]->IsConnected()) {
		FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData());
		if (!fv) {
			fv = new FlatvectorValue(p1);
			properties.e[1]->SetData(fv,1);
		} else fv->v = p1;
	}

	if (!properties.e[2]->IsConnected()) {
		FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[2]->GetData());
		if (!fv) {
			fv = new FlatvectorValue(p2);
			properties.e[2]->SetData(fv,1);
		} else fv->v = p2;
	}

	return 0;
}

int MirrorPathNode::Update()
{
	Error(nullptr);

	PathsData *orig = dynamic_cast<PathsData*>(properties.e[0]->GetData());
	if (!orig) { Error(_("Expected path")); return -1; }

	NodeProperty *outprop = FindProperty("out");
	PathsData *out = dynamic_cast<PathsData*>(outprop->GetData());

	if (IsMuted()) {
		outprop->SetData(orig, 0);
		return 0;
	} else if (out == orig) {
		//need to unset the mute
		outprop->SetData(nullptr, 0);
		out = nullptr;
	}

	FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData());
	if (!fv) { Error(_("Expected Vector2")); return -1; }

	FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[2]->GetData());
	if (!fv) { Error(_("Expected Vector2")); return -1; }

	if (out) out->set(*orig); //sets affine transform only


	if (dynamic_cast<PathsData*>(orig)) {
		 //go through point by point and transform.
		 //Try to preserve existing points to reduce allocations
		
		PathsData *pathin = dynamic_cast<PathsData*>(orig);

		LPathsData *pathout = dynamic_cast<LPathsData*>(out);
		if (!pathout) {
			pathout = dynamic_cast<LPathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
			pathout->Id("PerspFiltered");
			pathout->set(*pathin); //sets the affine transform
			pathout->InstallLineStyle(pathin->linestyle);
			pathout->InstallFillStyle(pathin->fillstyle);
			outprop->SetData(pathout,1);
			out = pathout;
		}

		DBG cerr << "pathout "<<pathout->Id()<<" previous bbox: "<<pathout->minx<<','<<pathout->maxx<<" "<<pathout->miny<<','<<pathout->maxy<<endl;

		Coordinate *start, *start2, *p, *p2;
		Path *path, *path2;
		flatpoint fp;

		 //make sure in and out have same number of paths
		while (pathout->NumPaths() > pathin->NumPaths()) pathout->RemovePath(pathout->NumPaths()-1, NULL);
		while (pathout->NumPaths() < pathin->NumPaths()) pathout->pushEmpty();

		for (int c=0; c<pathin->NumPaths(); c++) {
			path  = pathin ->paths.e[c];
			path2 = pathout->paths.e[c];
			path2->needtorecache = true;
		
			p  = path ->path;
			p2 = path2->path;
			start  = p;
			start2 = p2;

			if (p) {
				do {
					if (!p2) {
						p2 = p->duplicate();
						path2->append(p2);
						if (!start2) start2 = p2;
					} else {
						*p2 = *p;
					}

					fp = transform->transform(p->fp);
					p2->fp = fp;

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

			while (path2->pathweights.n > path->pathweights.n) path2->pathweights.remove(path2->pathweights.n-1);
			if (path2->pathweights.n < path->pathweights.n) {
				for (int c2 = 0; c2 < path->pathweights.n; c2++) {
					PathWeightNode *w1 = path->pathweights.e[c2];
					if (c2 >= path2->pathweights.n)
						path2->AddWeightNode(w1->t, w1->offset, w1->width, w1->angle);
					else *path2->pathweights.e[c2] = *w1;
				}
			}
			flatpoint tangent, point, wp1, wp2;
			for (int c2=0; c2<path->pathweights.n; c2++) {
				// virtual int PointAlongPath(double t, int tisdistance, flatpoint *point, flatpoint *tangent);
				PathWeightNode *w1 = path->pathweights.e[c2];
				PathWeightNode *w2 = path2->pathweights.e[c2];
				*w2 = *w1;
				path->PointAlongPath(w1->t, false, &point, &tangent);
				tangent.normalize();
				wp1 = transform->transform(point);
				wp2 = transform->transform(point + transpose(tangent) * .05);
				double scale = (wp1-wp2).norm() / .05;
				w2->offset *= scale;
				w2->width *= scale;
			}
		}


		pathout->FindBBox();
		DBG cerr << "pathout new bbox: "<<pathout->minx<<','<<pathout->maxx<<" "<<pathout->miny<<','<<pathout->maxy<<endl;
		outprop->Touch();
		pathout->touchContents();

		DBG cerr << "filter out:"<<endl;
		DBG pathout->dump_out(stderr, 10, 0, NULL);
		DBG cerr << "perspective transform:"<<endl;
		DBG transform->dump_out(stderr, 10, 0, NULL);

		return NodeBase::Update();

	// } else if (dynamic_cast<EngraverFillData*>(orig)) {
	// } else if (dynamic_cast<ColorPatchData*>(orig)) {
	// } else if (dynamic_cast<ImagePatchData*>(orig)) {
	// } else if (!strcmp(orig->whattype(), "PatchData")) {
	// } else if (dynamic_cast<VoronoiData*>(orig)) {
	// } else { // transform rasterized

	}

	Error(_("Cannot apply mirror to that type!"));
	return -1;


//	if (render_preview) {
//		*** //transform rasterized at much less than print resolution
//	} else {
//		//use render_scale
//		***
//	}

}



//------------------------ AlignToBoundsNode ------------------------

/*! \class Node to get a reference to other DrawableObjects in same page.
 */

class AlignToBoundsNode : public NodeBase
{
  public:
	AlignToBoundsNode();
	virtual ~AlignToBoundsNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref);

	// static SingletonKeeper keeper; //the def for domain enum
	// static ObjectDef *GetDef() { return dynamic_cast<ObjectDef*>(keeper.GetObject()); }
};

Laxkit::anObject *AlignToBoundsNode::NewNode(int p, Laxkit::anObject *ref)
{
	return new AlignToBoundsNode();
}

AlignToBoundsNode::AlignToBoundsNode()
{
	makestr(Name, _("Find Drawable"));
	makestr(type, "Drawable/AlignToBounds");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,  _("In"),     _("Object to align")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "bounds", NULL,1,  _("Bounds"), _("A bounding box to align in")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",  nullptr,1, _("Out"), nullptr, 0, false));
}

AlignToBoundsNode::~AlignToBoundsNode()
{
}

NodeBase *AlignToBoundsNode::Duplicate()
{
	AlignToBoundsNode *node = new AlignToBoundsNode();
	node->DuplicateBase(this);
	return node;
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int AlignToBoundsNode::Update()
{
	DrawableObject *in = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!in) return -1;

	DoubleBBox *bbox = dynamic_cast<DoubleBBox*>(properties.e[1]->GetData());
	if (!bbox) return -1;

	DrawableObject *out = in->duplicate();
	out->fitto
	
	properties.e[4]->SetData(in, 0);
	return NodeBase::Update();
}

int AlignToBoundsNode::GetStatus()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;
	StringValue *s = dynamic_cast<StringValue*>(properties.e[1]->GetData());
	if (!s) return -1;
	const char *pattern = s->str;
	if (isblank(pattern)) return -1;

	return NodeBase::GetStatus();
}





//------------------------ ResourceProperty ---------------------------------

/*! \class ResourceProperty
 * Node property that lets you select a Laidout resource.
 */

// [Resource (editable name)] [create new / create new from current] [remove ref to resource]
class ResourceProperty : public NodeProperty
{
  public:

	char *resource_type;
	Laxkit::Resource *resource;

	ResourceProperty(Resource *nresource, int absorb);
	virtual ~ResourceProperty();
};

ResourceProperty::ResourceProperty(Resource *nresource, int absorb)
{
	resource_type = nullptr;
}

ResourceProperty::~ResourceProperty()
{
	delete[] resource_type;
	if (resource) resource->dec_count();
}


//------------------------ NodePanel ------------------------

/*! \class NodePanel
 * Class to hold a definition for a standalone panel allowing random access to
 * individual properties of other nodes, so users can adjust very complicated
 * node arrangements via this much simpler panel.
 */


class NodePanel : public NodeBase
{
  public:
	class NodePanelRef
	{
	  public:
		int uihint;
		const char *text;
		NodeProperty *prop;
		NodeBase *node;

		NodePanelRef();
		virtual ~NodePanelRef();
	};

	PtrStack<NodePanelRef> props;

	NodePanel();
	NodePanel(NodeGroup *nowner, const char *nname=NULL, const char *nlabel=NULL, const char *ncomment=NULL);
	virtual ~NodePanel();
	virtual void HideAddButton();
	virtual void ShowAddButton();

	virtual int LinkProperty(NodeProperty *prop);

	// *** just another node? only with no in or out, all blocks, and changes relayed to refd props

}

NodePanel::NodePanel()
{
}

NodePanel::~NodePanel()
{
}

void NodePanel::HideAddButton()
{
}

void NodePanel::ShowAddButton()
{
}



//----------------------------- SimpleArrayValue ----------------------------------
template <class T>
class SimpleArrayValue : public Value
{
  public:
	NumStack<T> stack;

    SimpleArrayValue();
    virtual ~SimpleArrayValue();
    virtual const char *whattype() { return "SimpleArrayValue"; }
    virtual int getValueStr(char *buffer,int len);
    virtual Value *duplicate();
    virtual int type() { return VALUE_SimpleArray; }
    virtual ObjectDef *makeObjectDef();
};


typedef SimpleArrayValue<int> IntStackValue;
typedef SimpleArrayValue<double> DoubleStackValue;
typedef SimpleArrayValue<flatvector> Vector2StackValue;
typedef SimpleArrayValue<spacevector> Vector3StackValue;
typedef SimpleArrayValue<Quaternion> QuaternionStackValue;

typedef SimpleArrayValue<anObject> ObjectStackValue;


SimpleArrayValue<Int>::whattype() { return "IntStackValue"; }
SimpleArrayValue<Double>::whattype() { return "DoubleStackValue"; }
SimpleArrayValue<Vector2>::whattype() { return "Vector2StackValue"; }
SimpleArrayValue<Vector3>::whattype() { return "Vector3StackValue"; }
SimpleArrayValue<Quaternion>::whattype() { return "QuaternionStackValue"; }
SimpleArrayValue<String>::whattype() { return "StringStackValue"; }



class Vector2StackValue : public Value
{
  public:
	NumStack<flatvector> stack;

    Vector2StackValue();
    virtual ~SimpleArrayValue();
    virtual const char *whattype() { return "Vector2StackValue"; }
    virtual int getValueStr(char *buffer,int len);
    virtual Value *duplicate();
    virtual int type() { return VALUE_Vector2Stack; }
    virtual ObjectDef *makeObjectDef();
};





//------------------------ Path Effects ------------------------

// Input one or more paths. Do something to it. Output results


//------------------------ Model3D Hedron ------------------------

//Polyhedron class based:
//  name
//  vertices
//  edges
//  faces
//  uv(s)?
//  normals?
//
//Unwrap node:
//  click to pop up the unwrap window

//------------------------ Generators for 3d ------------------------


// Hedron operators:
//   dup spin
//   extrude faces
//   inset faces
//   deformation

class Path3dGenerator
{
  public:
	PathGenerator();
	virtual ~PathGenerator();

	
	NumStack<spacevector> points;

};

int CircleGenerator(double radius, spacevector center, Polyhedron *poly)
{
}

int GridPoints(
		double xwidth, double xstep,
		double ywidth, double ystep,
		double zwidth, double zstep,
		spacevector center, Polyhedron *poly)
{
	double pz, py;
	for (int zz = 0; zz < zdiv; zz++) {
		pz = zz / (double)zdiv * zwidth - zwidth/2;
		for (int yy = 0; yy < ydiv; yy++) {
			py = yy / (double)ydiv * ywidth - ywidth/2;
			for (int xx = 0; xx < xdiv; xx++) {
				poly->AddPoint(center + spacevector(xx / (double)xdiv * xwidth - xwidth/2, py, pz));
			}
		}
	}
}

/*! Cube centered at center.
 */
int Cube(double xwidth, double ywidth, double zwidth,
		 int xdiv, int ydiv, int zdiv,
		 spacevector center, Polyhedron *poly)
{
	double pz, py;
	for (int zz = 0; zz < zdiv; zz++) {
		pz = zz / (double)zdiv * zwidth - zwidth/2;
		for (int yy = 0; yy < ydiv; yy++) {
			py = yy / (double)ydiv * ywidth - ywidth/2;
			for (int xx = 0; xx < xdiv; xx++) {
				poly->AddPoint(center + spacevector(xx / (double)xdiv * xwidth - xwidth/2, py, pz));
			}
		}
	}
}


//----------------------- ScriptNode ------------------------

/*! \class ScriptNode
 * A runnable script with optional parameters.
 *
 * If the final value is a hash, then output links are created corresponding to those.
 * Otherwise, a single out property is made for the final value in the expression.
 */
class ScriptNode : public NodeBase
{
  public:
	PlainText *script;
	Interpreter *interpreter;
	Value *output;

	ScriptNode();
	virtual ~ScriptNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

ScriptNode::ScriptNode(const char *expression, const char *runner)
{
	//script = new PlainText(expression);
	//script = FindResource("PlainText", textobject);
	//if (script) script->inc_count();

	interpreter = GetInterpreter(runner);

	output = NULL;
}

ScriptNode::~ScriptNode()
{
	if (script) script->dec_count();
	if (interpreter) interpreter->dec_count();
	if (output) output->dec_count();
}

NodeBase *ScriptNode::Duplicate()
{
	ScriptNode *node = new ScriptNode(script, interpreter);

	***
	
	return node;
}

int ScriptNode::Update()
{
}

int ScriptNode::GetStatus()
{
	return NodeBase::GetStatus();
}

/*! Set up inputs. These can be accessed from the expression by name.
 */
int ScriptNode::AddParameters(const char *pname, const char *pName, const char *pTip, ...)
{
	***
}

/*! Create these as output properties. The script must return a hash with keys corresponding
 * to the pname.
 */
int ScriptNode::AddOutputs(const char *pname, const char *pName, const char *pTip, ...)
{
	***
}

int ScriptNode::Run()
{
}



//------------------------------ HistogramNode --------------------------------------------


/*! If to==NULL, then return a new LaxImage. Else pack into that one.
 * Divide color range into resolution steps.
 */
LaxImage *Histogram(LaxImage *from, LaxImage *to, int resolution, bool ignorealpha)
{
	int channels = 4;
	if (ignorealpha) channels--;

	if (!to) to = create_new_image(200,200);
	unsigned char *buffer = from->getImageBuffer(); //bgra

    int width  = from->w();
    int height = from->h();

	int h[channels][resolution];

	 //sample source
	int i=0;
	int v;
	int max  = 256; //1 more than greatest value
	int max1 = max-1; //1 more than greatest value

	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			for (int c=0; c<channels; c++) {
				v = buffer[i+c];
				if (v>0) {
					h[c][v * resolution / max]++;
				}
			}
			i += 4;
		}
	}
	from->doneWithBuffer(buffer)

	 //build histogram image
	int cmax[channels];
	int chmax = 0;
	for (int c=0; c<channels; c++) {
		cmax[c] = 0;
		for (int r=0; r<resolution; r++) {
			if (h[c][r] > cmax[c]) cmax[c] = h[c][r];
		}
		if (cmax[c] > chmax) chmax = cmax[c];
	}

	buffer = to->getImageBuffer(); //bgra
    width  = to->w();
    height = to->h();
	memset(buffer, 0, width*height*4);

	int xx;
	int yh;
	int w4 = 4*width;
	for (int x=0; x<width; x++) {
		xx = x*resolution / width;
		for (int c=0; c<channels; c++) {
			i = xx;
			yh = h[channels][xx] *height / chmax;
			for (int y=0; y < yh; y++) {
				buffer[i][c] = max1;

				i += w4;
			}
		}
	}

	to->doneWithBuffer(buffer)
	return to;
}

class HistogramNode : public NodeBase
{
  public:
	HistogramNode(int w, int h, int r);
	virtual ~HistogramNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();
};

HistogramNode::HistogramNode(int w, int h, int r)
{
	if (w==0) w=h;
	if (w==0) w=100;
	if (h==0) h=w;
	if (h==0) h=100;
	if (r<=0) r=256;

	makestr(Name, _("Histogram"));
	makestr(type, "Histogram");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "In",    NULL,1,  _("Image in"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Width",      new IntValue(w),1,   _("Width"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Height",     new IntValue(h),1,   _("Height"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Resolution", new IntValue(res),1, _("Resolution"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, false, "Out",  NULL,1, _("Out"))); 
}

HistogramNode::~HistogramNode()
{
}

NodeBase *HistogramNode::Duplicate()
{
	int isnum;
	int w = getNumberValue(properties.e[1]->GetData(), &isnum);
	int h = getNumberValue(properties.e[2]->GetData(), &isnum);
	int r = getNumberValue(properties.e[3]->GetData(), &isnum);
	HistogramNode *newnode = new HistogramNode(w,h,r);
	newnode->DuplicateBase(this);
	return newnode;
}

int HistogramNode::GetStatus()
{
	int isnum;
	int w = getNumberValue(properties.e[1]->GetData(), &isnum);  if (!isnum || w<=0) return -1;
	int h = getNumberValue(properties.e[2]->GetData(), &isnum);  if (!isnum || h<=0) return -1;
	int r = getNumberValue(properties.e[3]->GetData(), &isnum);  if (!isnum || r<=0) return -1;
	return NodeBase::GetStatus();
}

int HistogramNode::Update()
{
	int isnum;
	int w = getNumberValue(properties.e[1]->GetData(), &isnum);  if (!isnum || w<=0) return -1;
	int h = getNumberValue(properties.e[2]->GetData(), &isnum);  if (!isnum || h<=0) return -1;
	int r = getNumberValue(properties.e[3]->GetData(), &isnum);  if (!isnum || r<=0) return -1;

	LaxImage *in   = dynamic_cast<LaxImage*>(properties.e[0]->GetData());
	if (!in) return -1;

	LaxImage *himg = dynamic_cast<LaxImage*>(properties.e[4]->GetData());

	LaxImage *himgr = Histogram(in, himg, r, false);
	if (himgr) properties.e[4]->SetData(himgr, (himgr == himg ? 0 : 1));
	else return -1;

	return 0;
}


//------------------------------ ColorMapNode --------------------------------------------

class ColorMapNode : public NodeBase
{
  public:
	ColorMapNode();
	virtual ~ColorMapNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

ColorMapNode::ColorMapNode()
{ ***
}

ColorMapNode::~ColorMapNode()
{ ***
}

NodeBase *ColorMapNode::Duplicate()
{ ***
	ColorMapNode *node = new ColorMapNode(start, end, step);
	node->DuplicateBase(this);
	return node;
}

int ColorMapNode::Update()
{ ***
}

int ColorMapNode::GetStatus()
{ ***
	return NodeBase::GetStatus();
}


//------------------------------ CompileMath --------------------------------------------


/*! Take math nodes and create an equivalent expression.
 */
ScriptNode *CompileMath(RefPtrStack<NodeBase> &nodes, Interpreter *interpreter, RefPtrStack<NodeBase> &remainder)
{
}



//------------------------------ SwizzleNode --------------------------------------------

/*! \class SwizzleNode
 * Map arrays to other arrays using a special Swizzle interface.
 */

class SwizzleNode : public NodeBase
{
  public:
	SwizzleNode();
	virtual ~SwizzleNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

SwizzleNode::SwizzleNode()
{ ***
	makestr(type, "Swizzle");
	makestr(Name, NULL);
}

SwizzleNode::~SwizzleNode()
{ ***
}

NodeBase *SwizzleNode::Duplicate()
{ ***
	SwizzleNode *node = new SwizzleNode(start, end, step);
	node->DuplicateBase(this);
	return node;
}

int SwizzleNode::Update()
{ ***
}

int SwizzleNode::GetStatus()
{ ***
	return NodeBase::GetStatus();
}

Laxkit::anObject *newSwizzle(int p, Laxkit::anObject *ref)
{
	return new SwizzleNode();
}


//------------------------------ PathBooleanNode --------------------------------------------

/*! \class PathBooleanNode
 * Map arrays to other arrays using a special Boolean interface.
 */

class PathBooleanNode : public NodeBase
{
  public:
	static SingletonKeeper defkeeper; //the def for the op enum
	static ObjectDef *GetDef() { return dynamic_cast<ObjectDef*>(defkeeper.GetObject()); }

	PathBooleanNode();
	virtual ~PathBooleanNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

static SingletonKeeper PathBooleanNode::defkeeper;

static ObjectDef *PathBooleanNode::GetDef()
{
	ObjectDef *def = dynamic_cast<ObjectDef*>(defkeeper.GetObject());
	if (def) return def;

	def = new ObjectDef("PathBooleanDef", _("Path Boolean Def"), NULL,NULL,"enum", 0);

	def->pushEnumValue("Union",        _("Union"),         _("Union"),         BOOL_Union         );
	def->pushEnumValue("Intersection", _("Intersection"),  _("Intersection"),  BOOL_Intersection  );
	def->pushEnumValue("NotInSecond",  _("Not In Second"), _("Not In Second"), BOOL_NotInSecond   );
	def->pushEnumValue("NotInFirst",   _("Not In First"),  _("Not In First"),  BOOL_NotInFirst    );

	defkeeper.SetObject(def,1);
	return def;
}

PathBooleanNode::PathBooleanNode()
{
	makestr(type, "Filters/Path Boolean");
	makestr(Name, _("Path Boolean"));

	ObjectDef *enumdef = GetDef();

	EnumValue *e = new EnumValue(enumdef, 0);
	e->SetFromId(op);

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in",  NULL,1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in2", NULL,1, _("In2")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Block, true, "op", e,1, _("Op"),NULL,0, false));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output,true, "out", NULL,0, NULL, 0, false)); 
}

PathBooleanNode::~PathBooleanNode()
{
}

NodeBase *PathBooleanNode::Duplicate()
{ ***
	PathBooleanNode *node = new PathBooleanNode(op);
	node->DuplicateBase(this);
    node->DuplicateProperties(this);
	return node;
}

int PathBooleanNode::Update()
{ ***
}

int PathBooleanNode::GetStatus()
{ ***
	return NodeBase::GetStatus();
}

Laxkit::anObject *newPathBoolean(int p, Laxkit::anObject *ref)
{
	return new PathBooleanNode();
}


//------------------------------ ActionNode --------------------------------------------

/*! \class ActionNode
 * Thread class to perform some action.
 *
 * ----Action---
 * o  In
 *       Out o
 * [o] New through
 * [o] New In
 * str: Expression
 *     New Out [o]
 */



//----------------------- BoilerPlateNode ------------------------

/*! \class BoilerPlateNode
 *
 * Do stuff.
 */
class BoilerPlateNode : public NodeBase
{
  public:
	BoilerPlateNode();
	virtual ~BoilerPlateNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new BoilerPlateNode(); }
};

BoilerPlateNode::BoilerPlateNode()
{
	makestr(type, "Paths/Extrude");
	makestr(Name, _("Extrude"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,     _("Input"), _("A path or model")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "dir",   new FlatvectorValue(0,0,1),1,  _("Vector"),  _("Vector or path")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
}

BoilerPlateNode::~BoilerPlateNode()
{
}

NodeBase *BoilerPlateNode::Duplicate()
{
	BoilerPlateNode *newnode = new BoilerPlateNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int BoilerPlateNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int BoilerPlateNode::Update()
{
	return NodeBase::Update();
}

