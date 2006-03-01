/*************** dodecahedron.cc ********************/


#include "dispositioninst.h"
#include <lax/interfaces/pathinterface.h>
#include <lax/strmanip.h>

using namespace Laxkit;



//-------------------------- Dodecahedron ---------------------------------------------

/*! \class Dodecahedron
 * \brief Disposition for the 12 sided dodecahedron
 *
 * Each page is the non-rectanglar regular pentagon.
 *
 * \todo *** please note that this class is totally unimplemented, and what is here will soon
 * be transferred to be just one initializer for a NetDisposition
 */
//class Dodecahedron : public Disposition
//{
//  public:
//	double insetl,insetr,insett,insetb;
//	int pagesoncorners;
//	Dodecahedron();
//	virtual ~Dodecahedron() {}
//	virtual StyleDef *makeStyleDef();
//	
////	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
//	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle
//	virtual Laxkit::SomeData *GetPage(int pagenum,int local); // return outline of page in paper coords
//	virtual Spread *GetLittleSpread(int whichpage); 
//	virtual Spread *PageLayout(int whichpage); 
//	virtual Spread *PaperLayout(int whichpaper);
//	virtual Laxkit::DoubleBBox *GetDefaultPageSize(Laxkit::DoubleBBox *bbox=NULL);
//	virtual int *PrintingPapers(int frompage,int topage);
//	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
//	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
//	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
//	virtual Style *duplicate(Style *s=NULL);
//	virtual int SetPaperSize(PaperType *npaper);
//	virtual void setPage();
//
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//
//	virtual void DefineNet();
//	virtual void FitToData(Laxkit::SomeData *data,double margin);
//};

Dodecahedron::Dodecahedron() : Disposition("Dodecahedron")
{
	pagesoncorners=0;
	//paperstyle=*** global default paper style;
	paperstyle=new PaperType("letter",8.5,11.0,0,300);//***should read a global default
	setPage();
}

//! Set paper size, also reset the pagestyle. Duplicates npaper, not pointer transer.
int Dodecahedron::SetPaperSize(PaperType *npaper)
{
	if (!npaper) return 1;
	if (paperstyle) delete paperstyle;
	paperstyle=(PaperType *)npaper->duplicate();
	
	setPage();
	return 0;
}

//! Using the paperstyle and isvertical, create a new default pagestyle.
void Dodecahedron::setPage()
{***
	if (!paperstyle) return;
	if (pagestyle) delete pagestyle;
	pagestyle=new ***
}

//! *** imp me!
void Dodecahedron::dump_in_atts(LaxFiles::Attribute *att)
{}

//! Write out inset values, tile values, default paper, default page, numpages (not papers)
void Dodecahedron::dump_out(FILE *f,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (pagesoncorners) fprintf(f,"%spagesoncorners\n",spc);
	if (numpages) fprintf(f,"%snumpages %d\n",spc,numpages);
	if (pagestyle) {
		fprintf(f,"%sdefaultpagestyle\n",spc);
		pagestyle->dump_out(f,indent+2);
	}
	if (paperstyle) {
		fprintf(f,"%sdefaultpagestyle\n",spc);
		paperstyle->dump_out(f,indent+2);
	}
	***output the net
}

//! Duplicate this, or fill in this attributes.
Style *Dodecahedron::duplicate(Style *s)//s=NULL
{
	if (s==NULL) s=new Dodecahedron();
	else if (!dynamic_cast<Dodecahedron *>(s)) return NULL;
	Dodecahedron *d=dynamic_cast<Dodecahedron *>(s);
	if (!d) return NULL;
	d->pagesoncorners=pagesoncorners;
	*** duplicate the net
	return Disposition::duplicate(s);  
}

//! ***imp me! Make an instance of the Dodecahedron disposition styledef
/*  Required by Style...include the standard pagestyle, paperstyle, numpages and numpapers.
 *  They have their own styledefs stored in the style def manager 
 *  *** whatever and wherever that is!!!
 */
StyleDef *Dodecahedron::makeStyleDef()
{
	return NULL;
}

//! Create all necessary pages based on default pagestyle and numpages.
/*! Currently returns NULL terminated list of pages.
 */
Page **Dodecahedron::CreatePages(PageStyle *thispagestyle)//thispagestyle=NULL
{
	if (numpages==0) return NULL;
	Page **pages=new Page*[numpages+1];
	int c;
	PageStyle *ps=(PageStyle *)(thispagestyle?thispagestyle:pagestyle)->duplicate();
	for (c=0; c<numpages; c++) {
		pages[c]=new Page(ps,0,c);
	}
	pages[c]=NULL;
	return pages;
}

//! Return outline of paper in paper coords.
SomeData *Dodecahedron::GetPage(int papernum,int local)
{
	PathsData *newpath=new PathsData();
	double p,n,w,h,s;
	w=pagestyle->w();
	h=pagestyle->h();
	p=h/(1+sin(54./180*M_PI));
	n=p/sin(54./180*M_PI);
	for (int c=0; c<5; c++) {
		if (pagesoncorners) newpath->append(center.x+n*cos((-144+72*c)/180.*M_PI),
											center.y+n*sin((-144+72*c)/180.*M_PI));
		else newpath->append(center.x+n*cos((-126+72*c)/180.*M_PI),
							 center.y+n*sin((-126+72*c)/180.*M_PI));
	}
	newpath->close();
	***Apply transform so origin is at one of the corners
	if (local!=1) { datastack.push(newpath,1,newpath->object_id,1); local=0; }
	PageLocation *pl=new PageLocation(papernum,NULL,newpath,local,NULL);
	return pl;
}

//! Return a spread for use in the spread editor.
/*! This just returns a normal PageLayout. Perhaps some other
 * time I will make it so there are little dogeared corners. Hoo boy!
 */
Spread *Dodecahedron::GetLittleSpread(int whichpage)
{
	return PageLayout(whichpage);
}

//! Basically returns same as the paper layout, only without the paper outline.
Spread *Dodecahedron::PageLayout(int whichpage)
{
	Spread *spread=PaperLayout(PaperFromPage(whichpage));
	***remove paper outline
	return spread;
}

//! Return a paper spread with the net.
/*! The path created here is one path for the paper, and another for the possibly inset page.
 */
Spread *Dodecahedron::PaperLayout(int whichpaper)
{***
	Spread *spread=new Spread();
	spread->style=SPREAD_PAPER;
	spread->mask=SPREAD_PATH|SPREAD_PAGES|SPREAD_MINIMUM|SPREAD_MAXIMUM;
	
	 // define max/min points
	spread->minimum=flatpoint(paperstyle->w()/4,paperstyle->h()/2);
	spread->maximum=flatpoint(paperstyle->w()*3/4,paperstyle->h()/2);

	 // fill spread with paper and page outline
	PathsData *newpath=new PathsData();
	spread->path=(SomeData *)newpath;

	 // This automatically creates the new path in the PathsData.
	 // make the paper outline
	newpath->appendRect(0,0,paperstyle->w(),paperstyle->h());
	
	 // make the page outline
	newpath->pushEmpty(); //*** later be a certain linestyle
	newpath->appendRect(insetl,insetb, paperstyle->w()-insetl-insetr,paperstyle->h()-insett-insetb);
	
	 // setup spread->pagestack
	 // page width/height must map to proper area on page.
	SomeData *ntrans=new SomeData(0,pagestyle->w(), 0,pagestyle->h());
	ntrans->m(0,(paperstyle->w()-insetl-insetr)/(pagestyle->w())); // xaxis.x
	ntrans->m(3,(paperstyle->h()-insett-insetb)/(pagestyle->h())); //yaxis.y
	ntrans->origin(flatpoint(insetl,insetb));
	spread->pagestack.push(new PageLocation(whichpaper,NULL,ntrans,NULL));
		
	 // make printer marks if necessary
	 //*** make this more responsible lengths:
	if (insetr>0 || insetl>0 || insett>0 || insetb>0) {
		PathsData *marks=new PathsData();
		if (insetl>0) {
			marks->pushEmpty();
			marks->append(0,        paperstyle->h()-insett);
			marks->append(insetl*.9,paperstyle->h()-insett);
			marks->pushEmpty();
			marks->append(0,        insetb);
			marks->append(insetl*.9,insetb);
		}
		if (insetr>0) {
			marks->pushEmpty();
			marks->append(paperstyle->w(),          paperstyle->h()-insett);
			marks->append(paperstyle->w()-.9*insetr,paperstyle->h()-insett);
			marks->pushEmpty();
			marks->append(paperstyle->w(),          insetb);
			marks->append(paperstyle->w()-.9*insetr,insetb);
		}
		if (insetb>0) {
			marks->pushEmpty();
			marks->append(insetl,0);
			marks->append(insetl,.9*insetb);
			marks->pushEmpty();
			marks->append(paperstyle->w()-insetr,0);
			marks->append(paperstyle->w()-insetr,.9*insetb);
		}
		if (insett>0) {
			marks->pushEmpty();
			marks->append(insetl,paperstyle->h());
			marks->append(insetl,paperstyle->h()-.9*insett);
			marks->pushEmpty();
			marks->append(paperstyle->w()-insetr,paperstyle->h());
			marks->append(paperstyle->w()-insetr,paperstyle->h()-.9*insett);
		}
		spread->marks=marks;
	}

	 
	return spread;
}

//! Page size is the bounds of 1 pentagonthe paper size minus the inset values.
DoubleBBox *Dodecahedron::GetDefaultPageSize(Laxkit::DoubleBBox *bbox) 
{
	if (!paperstyle) return NULL;
	if (!bbox) bbox=new DoubleBBox;

	double s=***;
	double n=s/2/tan(36.*M_PI/180),
	       p=s/2/sin(36.*M_PI/180);
	if (pagesoncorners) { // base rests on y axis
		bbox.minx=0;
		bbox.maxx=p+n;
		bbox.miny=s/2-n*sin(18.*M_PI/180);
		bbox.maxy=s/2+n*sin(18.*M_PI/180);
	} else { // base rests on x axis
		bbox.minx=s/2-n*sin(18.*M_PI/180);
		bbox.maxx=s/2+n*sin(18.*M_PI/180);
		bbox.miny=0;
		bbox.maxy=p+n;
	}
	return bbox;
}

//! Returns like:{ frompage,topage, singlepage,-1,anothersinglepage,-1,-2 }
int *Dodecahedron::PrintingPapers(int frompage,int topage)
{
	if (frompage<topage) {
		int t=frompage;
		frompage=topage;
		topage=t;
	}
	int *blah=new int[3];
	blah[0]=frompage/12;
	blah[1]=topage/12;
	blah[2]=-2;
	return blah;
}

//! Just return pagenumber/12, since 12 pages==1 paper
int Dodecahedron::PaperFromPage(int pagenumber) // the paper number containing page pagenumber
{ return pagenumber/12; }

//! Return npapers*12.
int Dodecahedron::GetPagesNeeded(int npapers) 
{ return npapers*12; }

//! Return npages/12+1.
int Dodecahedron::GetPapersNeeded(int npages) 
{ return npages/12+1; } 



//--- net specific stuff:

//! Make *this fit inside bounding box of data (inset by margin).
/*! the net coords are static, but they are transform by an ***internal
 * matrix to get on the paper.
 */
void Dodecahedron::FitToData(Laxkit::SomeData *data,double margin)
{
	if (!data || !np) return;
	double wW=(data->maxx-data->minx-2*margin)/(maxx-minx);
	double hH=(data->maxy-data->miny-2*margin)/(maxy-miny);
	if (hH<wW) wW=hH; // pick the smaller of the two size ratios

	flatpoint mid=flatpoint((maxx+minx)/2,(maxy+miny)/2),
			midp=flatpoint((data->maxx+data->minx)/2,(data->maxy+data->miny)/2);
	xaxis(wW*data->xaxis());
	yaxis(wW*data->yaxis());
	origin(data->origin()+midp.x*data->xaxis()+midp.y*data->yaxis()-mid.x*xaxis()-mid.y*yaxis());
}

//! Define all the points in the net.
/*! Arbitrary nets can read in a generic net...
 */
void Dodecahedron::DefineNet()
{
	double s,a,b,c,h,v,i,j,k,q,r,m,n,tau,l;
	s=10;
	tau=(sqrt(5.)+1)/2;
	a=s*cos(54/180.*3.1415926535);
	b=s*sin(72/180.*3.1415926535);
	c=s/2*(1+cos(36/180.*3.1415926535))/sin(36/180.*3.1415926535);
	h=c*tau*tau;
	//v=s*(3*tau*tau+cos(72/180.*3.1415926535));
	v=s/2*(7*tau+5);
	i=s/2/tau;
	j=s/2;
	k=s*tau/2;
	q=s*tau*tau/2;
	r=s*tau;
	m=s/2*(tau+2);
	n=s*tau*tau*tau/2;
	l=s*tau*tau*tau;

	// total height=v, total width=h
	
	 // make points
	np=38;
	p=new flatpoint[38];
	p[0]=flatpoint(c,l);
	p[1]=flatpoint(b,l-k);
	p[2]=flatpoint(c,l-r);
	p[3]=flatpoint(a,l-q);
	p[4]=flatpoint(0,l/2);
	p[5]=flatpoint(a,q);
	p[6]=flatpoint(c,r);
	p[7]=flatpoint(b,k);
	p[8]=flatpoint(c,0);
	p[9]=flatpoint(h-c,i);
	p[10]=flatpoint(h-c,q);
	p[11]=flatpoint(h-b,j);
	p[12]=flatpoint(h,k);
	p[13]=flatpoint(h,m);
	p[14]=flatpoint(h-b,l/2);
	p[15]=flatpoint(h,l-m);
	p[16]=flatpoint(h,l-k);
	p[17]=flatpoint(h-b,l-j);
	p[18]=flatpoint(h-c,l-q);
	p[19]=flatpoint(h-c,l-i);
	p[20]=flatpoint(h-b,v-l+k);
	p[21]=flatpoint(h-c,v-l+r);
	p[22]=flatpoint(h-a,v-l+q);
	p[23]=flatpoint(h,v-l/2);
	p[24]=flatpoint(h-a,v-q);
	p[25]=flatpoint(h-c,v-r);
	p[26]=flatpoint(h-b,v-k);
	p[27]=flatpoint(h-c,v);
	p[28]=flatpoint(c,v-i);
	p[29]=flatpoint(c,v-q);
	p[30]=flatpoint(b,v-j);
	p[31]=flatpoint(0,v-k);
	p[32]=flatpoint(0,v-m);
	p[33]=flatpoint(b,v-l/2);
	p[34]=flatpoint(0,v-l+m);
	p[35]=flatpoint(0,v-l+k);
	p[36]=flatpoint(b,v-l+j);
	p[37]=flatpoint(c,v-l+q);
	for (int c=0; c<np; c++) cout <<"Dodecahedron point "<<c<<":"<<p[c].x<<','<<p[c].y<<endl;

	 // make lines
	nl=3;
	li=new int*[nl];
	li[0]=new int[7];
	li[0][0]=2;
	li[0][1]=6;
	li[0][2]=10;
	li[0][3]=14;
	li[0][4]=18;
	li[0][5]=2;
	li[0][6]=-1;

	li[1]=new int[3];
	li[1][0]=0;
	li[1][1]=19;
	li[1][2]=-1;
		
	li[2]=new int[7];
	li[2][0]=21;
	li[2][1]=25;
	li[2][2]=29;
	li[2][3]=33;
	li[2][4]=37;
	li[2][5]=21;
	li[2][6]=-1;
	
	 // make months
	nm=nf=12;
	mo=new int*[nm];
	faces=new int*[nf];
	for (int c=0; c<nm; c++) {
		mo[c]=new int[5];
		mo[c][0]=0; 
		mo[c][3]=c;//face index
		mo[c][4]=-1;
	}
	for (int c=0; c<nf; c++) {
		faces[c]=new int[6];
		faces[c][5]=-1;
	}
	IntListAttribute("25 21 37 33 29 -1",faces[0],6);
	IntListAttribute("37 21 20 19 0 -1", faces[1],6);
	IntListAttribute("33 37 36 35 34 -1",faces[2],6);
	IntListAttribute("29 33 32 31 30 -1",faces[3],6);
	IntListAttribute("25 29 28 27 26 -1",faces[4],6);
	IntListAttribute("21 25 24 23 22 -1",faces[5],6);
	IntListAttribute("6 2 18 14 10 -1",  faces[6],6);
	IntListAttribute("2 6 5 4 3 -1",     faces[7],6);
	IntListAttribute("6 10 9 8 7 -1",    faces[8],6);
	IntListAttribute("10 14 13 12 11 -1",faces[9],6);
	IntListAttribute("14 18 17 16 15 -1",faces[10],6);
	IntListAttribute("18 2 1 0 19 -1",faces[11],6);
	for (int c=0; c<nm; c++) {
		mo[c][1]=faces[c][0];
		mo[c][2]=faces[c][1];
	}
}
