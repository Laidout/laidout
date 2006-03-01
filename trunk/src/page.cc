/********* page.cc ************/



#include <X11/Xutil.h>
#include <lax/lists.cc>
#include <lax/refcounter.h>
#include "page.h"
#include "drawdata.h"
using namespace LaxFiles;
using namespace Laxkit;


#include <iostream>
using namespace std;

using namespace Laxkit;
extern RefCounter<anObject> objectstack;
extern RefCounter<SomeData> datastack;

//----------------------- PageStyles ----------------------------

/*! \class PageStyle
 * \brief Basic Page style.
 *
 * flags can indicate whether margins clip, and whether contents from other pages are allowed to bleed onto this one.
 *
 * width and height indicate the bounding box for the page in page coordinates. For rectangular pages,
 * this is just the page outline, and otherwise is just the bbox, not the outline.
 *
 */
/*! \var double PageStyle::width
 * \brief The width of the bounding box of the page.
 */
/*! \var double PageStyle::height
 * \brief The height of the bounding box of the page.
 */
/*! \var unsigned int PageStyle::flags
 * \brief Can be or'd combination of MARGINS_CLIP and FACING_PAGES_BLEED.
 * 
 * MARGINS_CLIP==(1<<0) and FACING_PAGES_BLEED==(1<<1).
 */
//class PageStyle : public Style
//{
// public:
//	unsigned int flags; // marginsclip,facingpagesbleed;
//	double width,height; // these are to be considered the bounding box for non-rectangular pages
//	PageStyle() { flags=0; }
//	virtual const char *whattype() { return "PageStyle"; }
//	virtual StyleDef *makeStyleDef();
//	virtual Style *duplicate(Style *s=NULL);
//	virtual double w() { return width; }
//	virtual double h() { return height; }
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};

/*! Recognizes 'marginsclip', 'facingpagesbleed', 'width', and 'height'.
 * Discards all else.
 */
void PageStyle::dump_in_atts(LaxFiles::Attribute *att)
{
	if (!att) return;
	flags=0;
	width=height=0;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"marginsclip")) {
			if (BooleanAttribute(value)) flags|=MARGINS_CLIP;
		} else if (!strcmp(name,"facingpagesbleed")) {
			if (BooleanAttribute(value)) flags|=FACING_PAGES_BLEED;
		} else if (!strcmp(name,"width")) {
			DoubleAttribute(value,&width);
		} else if (!strcmp(name,"height")) {
			DoubleAttribute(value,&height);
		} else { 
			cout <<"PageStyle dump_in:*** unknown attribute!!"<<endl;
		}
	}
}

/*! Write out flags, width, height.
 * can be:
 * <pre>
 *  width 8.5
 *  height 11
 *  marginsclip
 *  facingpagesbleed
 * </pre>
 */
void PageStyle::dump_out(FILE *f,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	fprintf(f,"%swidth %.10g\n",spc,w());
	fprintf(f,"%sheight %.10g\n",spc,h());
	if (flags&MARGINS_CLIP) fprintf(f,"%smarginsclip\n",spc);
	if (flags&FACING_PAGES_BLEED) fprintf(f,"%sfacingpagesbleed\n",spc);
}

//! Copy over width, height, and flags.
Style *PageStyle::duplicate(Style *s)//s=NULL
{
	if (s==NULL) s=new PageStyle();
	else s=dynamic_cast<PageStyle *>(s);
	PageStyle *ps=dynamic_cast<PageStyle *>(s);
	if (!ps) return NULL;
	ps->flags=flags;
	ps->width=width;
	ps->height=height;
	return s;
}


 //! Return a pointer to a new StyleDef class with the PageStyle description.
StyleDef *PageStyle::makeStyleDef()
{
	//StyleDef(const char *nname,const char *nName,const char *ntp, const char *ndesc,unsigned int fflags=STYLEDEF_CAPPED);
	StyleDef *sd=new StyleDef(NULL,"pagestyle","Page","A page","A Page",STYLEDEF_FIELDS);

	//int StyleDef::push(const char *nfield,const char *ttip,const char *ndesc,StyleDef *nfields,unsigned int fflags);
	sd->push("marginsclip",
			"Margins Clip",
			"Whether a page's margins clip the contents",
			"Check this if you want the page's contents to be visible only if they are within the margins.",
			STYLEDEF_BIT,0);
	sd->push("facingpagesbleed",
			"Facing Pages Bleed",
			"Whether contents on a facing page are allowed on to the page",
			"Check this if you want the contents of pages to cross over onto other pages. "
			"What exactly bleeds over is determined from a page spread view. Any contents "
			"that leach out from a page's boundaries and cross onto other pages is shown. "
			"This is most useful for instance in a center fold of a booklet.",
			STYLEDEF_BIT,0);
	return sd;
}

//----------------------- RectPageStyle ----------------------------


/*! \class RectPageStyle
 * \brief Further info for rectangular pages
 *
 * Holds additional left, right, top, bottom margin insets, and
 * also whether the page is next to another, and so would have inside/outside
 * margins rather than left/right or top/bottom
 *
 * \code
 *   // left-right-top-bottom margins "rectpagestyle"
 *  #define RECTPAGE_LRTB      1
 *   // inside-outside-top-bottom (for booklet sort of style) "facingrectstyle"
 *  #define RECTPAGE_IOTB      2
 *   // left-right-inside-outside (for calendar sort of style) "topfacingrectstyle"
 *  #define RECTPAGE_LRIO      4
 *  #define RECTPAGE_LEFTPAGE  8
 *  #define RECTPAGE_RIGHTPAGE 16
 * \endcode
 */
//class RectPageStyle : public PageStyle
//{
// public:
//	unsigned int recttype; //LRTB, IOTB, LRIO
//	double ml,mr,mt,mb; // margins 
//	RectPageStyle(unsigned int ntype=RECTPAGE_LRTB,double l=0,double r=0,double t=0,double b=0);
//	virtual const char *whattype() { return "RectPageStyle"; }
//	virtual StyleDef *makeStyleDef();
//	virtual Style *duplicate(Style *s=NULL);
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};


//! Constructor, create new rectangular page style.
/*! ntype is RECTPAGE_LRTB for stand alone pages,
 * RECTPAGE_IOTB for a booklet sort of arrangement,
 * or RECTPAGE_LRIO for a calendar sort of arrangment.
 */
RectPageStyle::RectPageStyle(unsigned int ntype,double l,double r,double t,double b)
{
	recttype=ntype;
	ml=l;
	mr=r;
	mt=t;
	mb=b; //*** these would later start out at global default page margins.
}

/*! Recognizes  margin[lrtb], leftpage, rightpage, lrtb, iotb, lrio,
 * 'marginsclip', 'facingpagesbleed', 'width', and 'height'.
 * Discards all else.
 */
void RectPageStyle::dump_in_atts(LaxFiles::Attribute *att)
{
	ml=mr=mt=mb=0;
	recttype=0;
	char *name,*value;
	PageStyle::dump_in_atts(att);
	for (int c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"marginl")) {
			DoubleAttribute(value,&ml);
		} else if (!strcmp(name,"marginr")) {
			DoubleAttribute(value,&mr);
		} else if (!strcmp(name,"margint")) {
			DoubleAttribute(value,&mt);
		} else if (!strcmp(name,"marginb")) {
			DoubleAttribute(value,&mb);
		} else if (!strcmp(name,"leftpage")) {
			recttype=(recttype&~(RECTPAGE_LEFTPAGE|RECTPAGE_RIGHTPAGE))|RECTPAGE_LEFTPAGE;
		} else if (!strcmp(name,"rightpage")) {
			recttype=(recttype&~(RECTPAGE_LEFTPAGE|RECTPAGE_RIGHTPAGE))|RECTPAGE_RIGHTPAGE;
		} else if (!strcmp(name,"lrtb")) {
			recttype=(recttype&~(RECTPAGE_LRTB|RECTPAGE_IOTB|RECTPAGE_LRIO))|RECTPAGE_LRTB;
		} else if (!strcmp(name,"iotb")) {
			recttype=(recttype&~(RECTPAGE_LRTB|RECTPAGE_IOTB|RECTPAGE_LRIO))|RECTPAGE_IOTB;
		} else if (!strcmp(name,"lrio")) {
			recttype=(recttype&~(RECTPAGE_LRTB|RECTPAGE_IOTB|RECTPAGE_LRIO))|RECTPAGE_LRIO;
		} else { 
			cout <<"PageStyle dump_in:*** unknown attribute!!"<<endl;
		}
	}
}

/*! Write out marginsl/r/t/b, leftpage or rightpage, lrtb, iotb, lrio,
 * and call PageStyle::dump_out for flags, width, height.
 */
void RectPageStyle::dump_out(FILE *f,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	fprintf(f,"%smarginl %.10g\n",spc,ml);
	fprintf(f,"%smarginr %.10g\n",spc,mr);
	fprintf(f,"%smargint %.10g\n",spc,mt);
	fprintf(f,"%smarginb %.10g\n",spc,mb);
	if (recttype&RECTPAGE_LEFTPAGE)  fprintf(f,"%sleftpage\n", spc);
	if (recttype&RECTPAGE_RIGHTPAGE) fprintf(f,"%srightpage\n",spc);
	if (recttype&RECTPAGE_LRTB) fprintf(f,"%slrtb\n",spc);
	if (recttype&RECTPAGE_IOTB) fprintf(f,"%siotb\n",spc);
	if (recttype&RECTPAGE_LRIO) fprintf(f,"%slrio\n",spc);
	PageStyle::dump_out(f,indent);
}

//! Copy over ml,mr,mt,mb,recttype.
Style *RectPageStyle::duplicate(Style *s)//s=NULL
{
	if (s==NULL) s=new RectPageStyle(recttype);
	RectPageStyle *blah=dynamic_cast<RectPageStyle *>(s);
	if (!blah) return NULL;
	blah->recttype=recttype;
	blah->ml=ml;
	blah->mr=mr;
	blah->mt=mt;
	blah->mb=mb;
	return PageStyle::duplicate(s);
}

StyleDef *RectPageStyle::makeStyleDef() 
{
	StyleDef *sd;
	if (recttype&RECTPAGE_IOTB) 
		sd=new StyleDef("pagestyle","facingrectstyle","Rectangular Facing Page","Rectangular Facing Page",
						"Rectangular Facing Page",STYLEDEF_FIELDS);
	else if (recttype&RECTPAGE_LRIO) sd=new StyleDef("pagestyle","topfacingrectstyle",
						"Rectangular Top Facing Page","Rectangular Top Facing Page",
						"Rectangular Top Facing Page",STYLEDEF_FIELDS);
	else sd=new StyleDef("pagestyle","rectpagestyle","Rectangular Page","Rectangular Page",
					"Rectangular Page",STYLEDEF_FIELDS);
	
	 // the left or inside
	if (recttype&RECTPAGE_LRIO) 
		sd->push("insidemargin",
			"Inside Margin",
			"The inside margin",
			"How much space to put on the inside of facing pages.",
			STYLEDEF_REAL,
			0);
	else sd->push("leftmargin",
			"Left Margin",
			"The left margin",
			"How much space to put in the left margin.",
			STYLEDEF_REAL,
			0);
	
	 // right right or outside
	if (recttype&RECTPAGE_LRIO) 
		sd->push("outsidemargin",
			"Outside Margin",
			"The outside margin",
			"How much space to put on the outside of facing pages.",
			STYLEDEF_REAL,
			0);
	else sd->push("rightmargin",
			"Right Margin",
			"The right margin",
			"How much space to put in the right margin.",
			STYLEDEF_REAL,
			0);

	 // the top or inside
	if (recttype&RECTPAGE_IOTB) 
		sd->push("insidemargin",
			"Inside Margin",
			"The inside margin",
			"How much space to put on the inside of facing pages.",
			STYLEDEF_REAL,
			0);
	else sd->push("topmargin",
			"Top Margin",
			"The top margin",
			"How much space to put in the top margin.",
			STYLEDEF_REAL,
			0);

	 // the bottom or outside
	if (recttype&RECTPAGE_IOTB) 
		sd->push("outsidemargin",
			"Outside Margin",
			"The outside margin",
			"How much space to put on the outside of facing pages.",
			STYLEDEF_REAL,
			0);
	else sd->push("bottommargin",
			"Bottom Margin",
			"The bottom margin",
			"How much space to put in the bottom margin.",
			STYLEDEF_REAL,
			0);
	return sd;
}
 
//----------------------- Page ----------------------------

/*! \class Page
 * \brief Holds page number, thumbnail, a pagestyle, and the page's layers
 *
 * Pages fit into designated portions of spreads and papers. The sequence
 * of pages can be easily rearranged, usually via the spread editor.
 *
 * The ObjectContainer part returns the individual layers of the page.
 */
//class Page : public ObjectContainer
//{
// public:
//	int pagenumber;
//	Laxkit::ImageData *thumbnail;
//	clock_t thumbmodtime,modtime;
//	Laxkit::PtrStack<Group> layers;
//	PageStyle *pagestyle;
//	int psislocal;
//	Page(PageStyle *npagestyle=NULL,int pslocal=1,int num=-1); 
//	virtual ~Page(); 
//	virtual const char *whattype() { return "Page"; }
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//	virtual Laxkit::ImageData *Thumbnail();
//	virtual int InstallPageStyle(PageStyle *pstyle,int islocal=1);
//	virtual int n() { return layers.n(); }
//	virtual Laxkit::anObject *object_e(int i) 
//		{ if (i>=0 && i<layers.n) return layers.e[i]; return NULL; }
//};

//! Constructor, takes pointer, does not make copy of npagestyle, It deletes the pagestyle in destructor.
/*! If pslocal==0, then checkout npagestyle from the objectstack. If it is not there then push it on.
 *
 * Pushes 1 new Group onto layers stack.
 */
Page::Page(PageStyle *npagestyle,int pslocal,int num)
{
	thumbmodtime=0;
	modtime=times(NULL);
	pagestyle=npagestyle;
	psislocal=pslocal;
	if (psislocal==0 && pagestyle) {
		if (!objectstack.checkout(pagestyle)) objectstack.push(pagestyle,1,getUniqueNumber(),1);
	}
	thumbnail=0;
	pagenumber=num;
	layers.push(new Group,1);
}

//! Destructor, destroys the thumbnail, and pagestyle according to psislocal.
/*! If psislocal==1 then delete pagestyle. If psislocal=0, then objectstack.checkin(pagestyle). Otherwise,
 * don't touch pagestyle.
 */
Page::~Page()
{
	cout <<"  Page destructor"<<endl;
	if (thumbnail) delete thumbnail;
	if (psislocal==1) delete pagestyle;
	else if (psislocal==0) objectstack.checkin(pagestyle);
	layers.flush();
}

//! Delete (or checkin) old, checkout new.
/*! Return 0 for success, nonzero for error.
 */
int Page::InstallPageStyle(PageStyle *pstyle,int islocal)
{
	if (!pstyle) return 1;
	if (pagestyle) {
		if (psislocal==1) delete pagestyle;
		else if (psislocal==0) objectstack.checkin(pagestyle);
	}
	psislocal=islocal;
	pagestyle=pstyle;
	if (psislocal==0 && pagestyle) {
		if (!objectstack.checkout(pagestyle)) objectstack.push(pagestyle,1,getUniqueNumber(),1);
	}
	return 0;
}

//! Dump in page.
/*! Layers should have been flushed before coming here, and
 * pagestyle should have been set to the default page style for this page.
 *
 * \todo *** IMPORTANT: right now this is in hack stage, assumes RectPageStyle
 * is always the page style... need to have some sort of style object factory
 */
void Page::dump_in_atts(LaxFiles::Attribute *att)
{
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"pagestyle")) {
			PageStyle *ps;
			if (!strcmp(value,"RectPageStyle")) ps=new RectPageStyle();//***
			else ps=new PageStyle();
			ps->dump_in_atts(att->attributes.e[c]);
			InstallPageStyle(ps,0);
		} else if (!strcmp(name,"layer")) {
			Group *g=new Group;
			g->dump_in_atts(att->attributes.e[c]);
			layers.push(g,1);
		} else { 
			cout <<"Page dump_in:*** unknown attribute "<<(name?name:"(noname)")<<"!!"<<endl;
		}
	}
}

//! Write out pagestyle and layers. Ignore pagenumber.
/*! ***Each page writes out its own pagestyle.. this should probably be a reference
 * to somewhere in the file... too much duplication...
 */
void Page::dump_out(FILE *f,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (pagestyle) {
		fprintf(f,"%spagestyle %s\n",spc,pagestyle->whattype());
		pagestyle->dump_out(f,indent+2);
	}
	for (int c=0; c<layers.n; c++) {
		fprintf(f,"%slayer %d\n",spc,c);
		layers.e[c]->dump_out(f,indent+2);
	}
}

//! Update the thumbnail if necessary and return it.
/*! Creates thumbnail only if thumbmodtime<modtime.
 * 
 * These are used notably in the spreadeditor.
 *
 * *** later on should change this so that page width and height
 * are defined by some min/max, rather than origin to pagestyle->w(),h()
 */
ImageData *Page::Thumbnail()
{
	if (!pagestyle) return NULL;
	if (thumbmodtime>modtime) return thumbnail;
	double w=pagestyle->w(),
		   h=pagestyle->h();
	h=h*100./w;
	w=100.;
	cout <<"..----making thumbnail "<<w<<" x "<<h<<endl;
	if (!thumbnail) thumbnail=new ImageData(); 
	thumbnail->xaxis(flatpoint(pagestyle->w()/w,0));
	thumbnail->yaxis(flatpoint(0,pagestyle->w()/w));
	
	Pixmap pix=XCreatePixmap(anXApp::app->dpy,DefaultRootWindow(anXApp::app->dpy),
								(int)w,(int)h,XDefaultDepth(anXApp::app->dpy,0));
	Drawable d=imlib_context_get_drawable();
	imlib_context_set_drawable(pix);
	Displayer dp;
	
	 // setup dp to have proper scaling...
	dp.NewTransform(1.,0.,0.,-1.,0.,0.);
	dp.SetSpace(0,0,pagestyle->w(),pagestyle->h());
	dp.SetView(0,0,pagestyle->w(),pagestyle->h());
	dp.StartDrawing(pix);
		
	dp.NewBG(255,255,255); //*** this should be the paper color for paper the page is on...
	dp.NewFG(0,0,0);
	dp.m()[4]=0;
	dp.m()[5]=h;
	dp.Newmag(w/pagestyle->w());
	dp.ClearWindow();

	flatpoint p;
//	p=dp.realtoscreen(1,1);
//	dp.textout((int)p.x,(int)p.y,"++",2,LAX_CENTER);
//	p=dp.realtoscreen(1,-1);
//	dp.textout((int)p.x,(int)p.y,"+-",2,LAX_CENTER);
//	p=dp.realtoscreen(-1,1);
//	dp.textout((int)p.x,(int)p.y,"-+",2,LAX_CENTER);
//	p=dp.realtoscreen(-1,-1);
//	dp.textout((int)p.x,(int)p.y,"--",2,LAX_CENTER);

	for (int c=0; c<layers.n; c++) {
		//dp.PushAndNewTransform(layers.e[c]->m());
		DrawData(&dp,layers.e[c]);
		//dp.PopAxes();
	}
	dp.EndDrawing();
	Imlib_Image tnail=imlib_create_image_from_drawable(0,0,0,(int)w,(int)h,1);
	 
//	//***test output to thumb...
//	imlib_context_set_image(tnail);
//	imlib_blend_image_onto_image(thumbnail->imlibimage,0,0,0,100,100,0,0,100,100);
		
	thumbnail->SetImage(tnail); //*** must implement using diff size image than is in maxx,y
	//thumbnail->xaxis(flatpoint(pagestyle->w()/w,0));
	//thumbnail->yaxis(flatpoint(0,pagestyle->w()/w));
	XFreePixmap(anXApp::app->dpy,pix);
	
	cout <<"==--- Done updating thumbnail.."<<endl;
	imlib_context_set_drawable(d);
	cout <<"Thumbnail dump_out:"<<endl;
	thumbnail->dump_out(stdout,2);
	cout <<"  minx "<<thumbnail->minx<<endl;
	cout <<"  maxx "<<thumbnail->maxx<<endl;
	cout <<"  miny "<<thumbnail->miny<<endl;
	cout <<"  maxy "<<thumbnail->maxy<<endl;
	thumbmodtime=times(NULL);
	return thumbnail;
}



