//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/************* extras.cc ****************/

/*! \file
 *
 * \todo This file should ultimately hold the mechanism for installing and removing extras.
 *   Currently, the only thing sort of like an extra is the dump images facility,
 *   which ultimately should be in a different file....
 */

/*! \defgroup extras Extras
 * Extraneous addon type of things are documented here. These are functions and such
 * that are not considered core functionality, but are still hella useful.
 *
 * Currently:
 * 
 * dumpInImages() plops down a whole lot of images into the document. Specify the start page,
 * how many per page, and either a list of image files, a directory that contains the images,
 * or a list of ImageData objects.
 *
 * Planned but not implemented yet is applyPageNumbers, which automatically inserts
 * particular images representing the page numbers on each page. You would specify
 * the directory that contains the images, and the code does the rest..
 *
 * \todo *** must figure out how to integrate extras into menus, and incorporate callbacks?
 * \todo implement dumping in Images for page numbers at defined positions
 * \todo Still must work out good mechanism for being able to add on any extras via plugins...
 */

#include "extras.h"
#include <lax/attributes.h>
#include <lax/fileutils.h>
#include <dirent.h>


#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


//------------------------------------- ImagePlopInfo ------------------------------------
/*! \class ImagePlopInfo
 * \ingroup extras
 * \brief Class to simplify dumping multiple images from a list into a Document.
 */
/*! \var int ImagePlopInfo::page
 * 
 * -1 for try to fit on same page as previous page. If there is not enough room, increase
 *    the page counter by one, and start to fill that page.\n
 * -2 for force this image onto same page as previous image.
 */
/*! \var double *ImagePlopInfo::xywh
 * \brief A box with x,y,w,h in page coordinates to fit an image inside of.
 * 
 * The image is not distorted to fit the box. It is only scaled so that it is
 * contained within the box. This will override the dpi setting only if has to
 * be scaled down to fit.
 */

void ImagePlopInfo::add(ImageData *img, int ndpi, int npage, double *d)
{
	ImagePlopInfo *p=this;
	while (p->next) p=p->next;
	p->next=new ImagePlopInfo(img,ndpi,npage,d);
}

ImagePlopInfo::ImagePlopInfo(ImageData *img, int ndpi, int npage, double *d)
	: image(img), error(0), dpi(ndpi), page(npage), next(NULL)
{
	if (d) {
		xywh=new double[4];
		memcpy(xywh,d,4*sizeof(double));
	} else xywh=NULL;
}

ImagePlopInfo::~ImagePlopInfo()
{
	if (xywh) delete[] xywh;
	if (image) image->dec_count();
	if (next) delete next;
}

//--------------------------------- Dump In Images --------------------------------------------

//! Plop down images from the list contained in file.
/*! \ingroup extras
 * Returns the page index of the final page or -1 if error.
 * 
 * Any file in the list that does not appear to be an image will be installed as a broken image.
 * 
 * The file should be formated as follows, using the usual space (not tabs) indentation
 *  to indicate related data. Note that the "dir:///" thing is not yet implemented:
 * <pre>
 *    \#Laidout 0.04.1 Image List
 *    path /blah1/blah2  \#any subsequent "./file" becomes "/blah1/blah2/file"
 *    dpi 600    # default dpi, overridable per image
 *    perPage -1 # -1 == auto, as will fit on the page
 *                 # -2 == put all on same page
 *                 #  0 == ??
 *                 #  >0 == that many per page
 *    oneDirPerPage false
 *    page 3 # change current page to 3
 *    
 *     # Note that file sections must begin with "/" "./" or "../" "file://"
 *    dir:///path/to/a/directory \#dumps all in dir, no previews, always same dpi
 *    %file:///aoeuaoen  /path/to/preview/  name
 *    ./relative/file
 *    ../another/relative/file  /path/to/preview/  description
 *    /some/file                /path/to/preview/  description
 *    %file:///aoeuaoen          /path/to/preview/  description
 *      dpi 300 \#overrides the current dpi
 *      xywh 0 0 2 2  # fit image in this box, in page coordinates overrides dpi
 *                    # if too big for the box
 *    pagebreak # no more pictures for this page, goes to page 4
 *    %file:///aoeuaoen  
 *      preview /path/to/preview/  
 *      description blah blah blah
 *    page 5
 *    ...
 * </pre>
 *
 * \todo implement the "dir:///" thing 
 * \todo allow specifying Arrangements
 */
int dumpInImageList(Document *doc,const char *file, int startpage, int defaultdpi, int perpage)
{
	LaxFiles::Attribute att;
	if (att.dump_in(file)) return -1;
	char *dir=lax_dirname(file,0);
	if (!dir) {
		char *tdir=getcwd(NULL,0);
		makestr(dir,tdir);
		free(tdir);
	}
	att.push("path",dir,0);
	delete[] dir;
	return dumpInImageList(doc,&att,startpage,defaultdpi,perpage);
}

/*! \ingroup extras
 * Break a value from a file list file attribute into the preview and description.
 *
 * a line is something like "/path/to/preview  description blah blah".
 *
 * The preview and/or description can be quoted, in which case quotes within must
 * have been escaped. If the preview section does not begin
 * with "/" "./" or "../" or "file://", then the whole string is assumed to be 
 * the description.
 */
static void getPreviewAndDesc(const char *value,char **preview,char **desc)
{
	if (!value) {
		*preview=*desc=NULL;
		return;
	}
	char *e,*p=QuotedAttribute(value,&e);
	if (p) {
		if (!strncmp(p,"file://",7)) memmove(p,p+7,strlen(p)-7);
		if (!strncmp(p,"/",1) || !strncmp(p,"./",2) || !strncmp(p,"../",7)) {
			 // we have a preview file name
			*preview=p;
			value=(const char *)e;
		}
		if (value) *desc=WholeQuotedAttribute(value);
		else *desc=NULL;
	} else {
		*preview=*desc=NULL;
	}
}

//! Plop down images from the list contained in att.
/*! \ingroup extras
 *  Returns the page index of the final page or -1 if error.
 *
 *  The attribute must follow the format as described in 
 *  dumpInImageList(Document *,const char *, int, int, int). The order of the elements
 *  matters.
 *
 * \todo Right now, if an error occurs midstream, the document is still modified. should
 *    instead change doc only if no errors occur all through stream..
 * \todo implement dir:///some/dir/with/images/in/it 
 */
int dumpInImageList(Document *doc,LaxFiles::Attribute *att, int startpage, int defaultdpi, int perpage)
{
	if (!att) return -1;
	if (startpage<0) startpage=0;
	else if (startpage>=doc->pages.n) startpage=doc->pages.n-1;
	if (defaultdpi<1) defaultdpi=150;
	int curpage=startpage;
	int onedirperpage;
	char error=0;
	char *preview=NULL,*desc=NULL,*path=NULL;
	double xywh[4];
	char useplace,flush=-1;
	ImagePlopInfo *images=NULL;
	ImageData *image=NULL;
	int jumptopage;
	int numonpage=0;
	
	char *name,*value,*file=NULL;
	for (int c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"dpi")) {
			IntAttribute(value,&defaultdpi,NULL);
		} else if (!strcmp(name,"perPage")) {
			IntAttribute(value,&perpage);
			flush=1;
			jumptopage=-1;
		} else if (!strcmp(name,"oneDirPerPage")) {
			onedirperpage=BooleanAttribute(value);
		} else if (!strcmp(name,"path")) {
			if (value) makestr(path,value);
		} else if (!strncmp(name,"file://",7) || !strncmp(name,"/",1) || 
					!strncmp(name,"./",2) || !strncmp(name,"../",7)) {
 			 // single line variant:  file:///aoeuaoen  /path/to/preview/  name
			 // multi line has further options
			int curdpi=defaultdpi;
			useplace=0;
			getPreviewAndDesc(value,&preview,&desc);
			
			if (!strncmp(name,"file://",7)) name+=7;
			file=full_path_for_file(name,path,1); // expand "./" and "../" and "blah"=="./blah"
				
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++)  {
				name=att->attributes.e[c2]->name;
				value=att->attributes.e[c2]->value;
				if (!strcmp(name,"dpi")) {
					IntAttribute(value,&curdpi,NULL);
				} else if (!strcmp(name,"xywh")) {
					 //set up a box to fit the image into
					if (DoubleListAttribute(value,xywh,4,NULL)==4) useplace=1;
					else { error=1; break; }
				} else if (!strcmp(name,"preview")) {
					 //overrides any old preview
					makestr(preview,value);
				} else if (!strcmp(name,"description")) {
					 // append to any existing description, adding a newline if
					 // none exists at end of old preview.
					if (!value) continue;
					if (desc && desc[strlen(desc)-1]!='\n') appendstr(desc,"\n");
					appendstr(desc,value);
				}
			}
			
			 // ok, so we now have file, preview, desc, curdpi, xywh if useplace
			image=new ImageData(file,preview,0,0,0);
			image->SetDescription(desc);
			if (!image->image) {
				 //broken image, make it have dimensions curdpi x curdpi
				image->maxx=image->maxy=curdpi;
			}
			numonpage++;

			int pg;
			if (perpage==-1) pg=-1;
			else if (perpage==-2) pg=curpage;
			else {
				pg=curpage;
				if (numonpage==perpage) {
					flush=1;
					jumptopage=-1;
					numonpage=0;
				}
			}
			
			if (!images) images=new ImagePlopInfo(image,curdpi,pg,xywh);
			else images->add(image,curdpi,pg,xywh);
			
			if (preview) delete[] preview;
			if (desc) delete[] desc;
			preview=desc=NULL;
		} else if (!strcmp(name,"pagebreak")) {
			flush=1;
			jumptopage=-1;
		} else if (!strcmp(name,"page")) {
			if (IntAttribute(value,&jumptopage,NULL)) {
				if (jumptopage<0) { error=1; break; }
				flush=1;
			}
		} else {
			DBG cout <<" *** potential error in list, found unknown attribute"<<endl;
		}
		
		if (flush>0) {
			 // flush all pending insertions
			curpage=dumpInImages(doc,images,curpage);
			if (jumptopage>=0) curpage=jumptopage;
			else curpage++;
			delete images;
			images=NULL;
			flush=-1;
		}
	}
	if (flush==-1 && images) {
		curpage=dumpInImages(doc,images,curpage);
	} else curpage--;
	
	if (images) delete images;
	if (error) {
		return -1;
	}
	return curpage;
}


//! Plop all images in directory pathtoimagedir into the document.
/*! \ingroup extras
 * Grabs all the regular file names in pathtoimagedir and passes them to dumpInImages(...,char **,...).
 * Does not check to see that the files are actually image files here.
 *
 * Returns the page index of the final page or -1 if error.
 */
int dumpInImages(Document *doc, int startpage, const char *pathtoimagedir, int perpage, int ddpi)
{
	 // prepare to read all images in directory pathtoimagedir....
	if (pathtoimagedir==NULL) pathtoimagedir=".";
	struct dirent **dirents;
	int n=scandir(pathtoimagedir,&dirents, NULL,alphasort);
	char **imagefiles=new char*[n];
	int c,i=0;
	for (c=0; c<n; c++) {
		if (dirents[c]->d_type==DT_REG) {
			imagefiles[i]=NULL;
			makestr(imagefiles[i],pathtoimagedir);
			if (pathtoimagedir[strlen(pathtoimagedir)-1]!='/') 
				appendstr(imagefiles[i],"/"); 
			appendstr(imagefiles[i++],dirents[c]->d_name);
			//cout << "dump maybe image files: "<<imagefiles[i-1]<<endl;
		}
		free(dirents[c]);
	}
	free(dirents);
	c=dumpInImages(doc,startpage,(const char **)imagefiles,i,perpage,ddpi);
	deletestrs(imagefiles,i);
	return c;
}

//! Plop all images with paths in imagefiles into the document.
/*! \ingroup extras
 * Attempts to read in all the images among imagefiles and passes them
 * to dumpInImages(Document *doc, ImagePlopInfo *images, int startpage), 
 * where also their transform matrices are adjusted.
 * Any broken images are not inserted into the document as broken images. They are ignored.
 * 
 * perpage==-1 means insert until page is full. perpage==-2 means put them all on startpage.
 * 
 * Returns the page index of the final page or -1 if error.
 *
 * \todo *** should probably put in **previewimages..
 */
int dumpInImages(Document *doc, int startpage, const char **imagefiles, int nfiles, int perpage, int ddpi)
{
	ImagePlopInfo *images=NULL;
	int c,numonpage=0;
	LaxImage *image=NULL;
	ImageData *imaged;
	int curpage=startpage;
	
	for (c=0; c<nfiles; c++) {
		if (!imagefiles[c] || !strcmp(imagefiles[c],".") || !strcmp(imagefiles[c],"..")) continue;
		image=load_image(imagefiles[c]);
		if (image) {
			DBG cout << "dump image files: "<<imagefiles[c]<<endl;

			imaged=new ImageData;//creates with one count
			imaged->SetImage(image);

			numonpage++;
			int pg;
			if (perpage==-1) pg=-1;
			else if (perpage==-2) pg=curpage;
			else {
				pg=curpage;
				if (numonpage==perpage) {
					numonpage=0;
				}
			}
			if (!images) images=new ImagePlopInfo(imaged,ddpi,pg,NULL);
			else images->add(imaged,ddpi,pg,NULL);
			if (numonpage==0) curpage++;
		} else {
			DBG cout <<"** warning: bad image file "<<imagefiles[c]<<endl;
		}
	}
	if (images) c=dumpInImages(doc,images,startpage);
	delete images;
	return c;
}

//! Plop down images starting at startpage.
/*! \ingroup extras
 * If there are more images than pages, then add pages. Centers images on each page
 * with the page's default dpi (if the info's dpi==-1). (assumes that the layer is not transformed in any way)
 *
 * Does not delete the images list, the calling code should do that.
 *
 * Returns the page index of the final page or -1 if error. Note that this might not be the maximum
 * page number added. It is just the final page in the list.
 * 
 * \todo *** please note that i have big plans for this extra, involving being able to dump
 *   stuff into 'arrangements' which will be kind of like template pages. Each spot of an
 *   arrangement will be able to hold the item(s) centered, scaled to fit, scaled and rotated to fit,
 *   or flowed into the spot. ultimately, this should also
 *   be able to reasonably handle page shapes that are not rectangles..
 * \todo this function might take a while, so should incorporate a progress bar thing
 * \todo this can certainly be abstracted a bit, and use any object list, not just images,
 *   then this function would call some other layout object or something for align/distribute..
 */
int dumpInImages(Document *doc, ImagePlopInfo *images, int startpage)
{
	DBG cout<<"---dump in images from ImagePlopInfo list..."<<endl;
	if (!images) return -1;
	if (startpage<0) startpage=0;

	ImagePlopInfo *info=images,*last=NULL,*flow, *flow2;
	Group *g;
	int curpage;
	int dpi=doc->docstyle->imposition->paperstyle->dpi,curdpi=dpi;
	curpage=startpage;
	double x,y,w,h,t,   // temp info while computing each row
		   ww,hh,       // width and height of page
		   s,           // scaling == 1/dpi
		   rw,rh,
		   rrh;         // height of all rows found so far
	int n,         // total number of images
		nn;        // number of images in current row
	n=0; // total number of images placed, nn is placed for page
	SomeData *outline=NULL;
	SomeData *obj;

	while (info) { // one loop per page
		//***if (progressfunc) progressfunc(progressarg, curimgi/numimgs);
	
		DBG cout <<"  starting page "<<curpage+1<<endl;
		if (!info->image) { info=info->next; continue; }
		
		if (info->page>=0) curpage=info->page;
		
		if (curpage>=doc->pages.n) { 
			DBG cout <<" adding new page..."<<endl;
			doc->NewPages(-1,doc->pages.n-curpage+1); // add extra page(s) at end
		}
		
		 // figure out page characteristics: dpi, ww, hh, and scaling
		 //*** ultimately this will need to be reworked to more reasonably flow within
		 //    non-rectangular pages
		if (outline) { outline->dec_count(); outline=NULL; }
		outline=doc->docstyle->imposition->GetPage(curpage,0); //adds 1 count already
		ww=outline->maxx-outline->minx;
		hh=outline->maxy-outline->miny;;
		//DBG cout <<": ww,hh:"<<ww<<','<<hh<<"  x,y,w,h"<<x<<','<<y<<','<<w<<','<<h<<endl;
		
		if (info->dpi>0) curdpi=info->dpi; else curdpi=dpi;
		s=1./curdpi; 
		
		 // fit into box if necessary
		if (info->xywh) {
			if (s*info->image->maxx>info->xywh[2]) s=info->xywh[2]/info->image->maxx;
			if (s*info->image->maxy>info->xywh[3]) s=info->xywh[3]/info->image->maxy;
			info->image->xaxis(flatpoint(s,0));
			info->image->yaxis(flatpoint(0,s));
			info->image->origin(flatpoint(info->xywh[0],info->xywh[1]));
			//*** should probably center within box....
			g=doc->pages.e[curpage]->e(doc->pages.e[curpage]->layers.n()-1);
			g->push(info->image,0); //incs the obj's count
			info=info->next; 
			continue;
		}
		 
		 // flow onto page (into a rectangle)
		rw=rh=rrh=0;
		DBG int nr=0; // number of rows so far
		last=flow=info;
		
		 // find maxperpage
		int maxperpage=0;
		int flowtype=1; //0=as will fit, 1=force fit 
		while (last && (last->page==-1 || last->page==-2 || last->page==curpage)) {
			if (last->page==-1) flowtype=0;
			maxperpage++;
			last=last->next;
		}
			
		nn=0;
		do { //one loop per row,
			 //rows on a single page. if doesn't all fit, then break out of this loop
			 //and advance main loop one and start on a new page
			if (info->dpi>0) curdpi=info->dpi; else curdpi=dpi;
			s=1./curdpi; 
			
			if (flow!=info) flow=flow->next;
			last=flow;   // last is used to point to first image of a row
			
			rw=rh=0;
			DBG cout <<"  row number "<<++nr<<endl;
			while (flow) { 
				 // find all for a row
				obj=flow->image;
				obj->xaxis(flatpoint(s,0));
				obj->yaxis(flatpoint(0,s));
				w=(obj->maxx-obj->minx)*s;
				h=(obj->maxy-obj->miny)*s;
				t=(h>rh?h:rh);
				if (last!=flow && (rw+w>ww || rrh+t>hh)) break; // fit all that could be fit on row
				nn++;
				rw+=w;
				if (h>rh) rh=h;
				if (!flow->next || nn==maxperpage) break;
				flow=flow->next;
			}
			if (last!=flow && rrh+rh>hh && flowtype==0) 
				break; // end adding rows to page, row would be off page

			 // apply origin and scaling to all between and including last and flow
			x=(ww-rw)/2+outline->minx;
			y=hh-rrh-rh/2+outline->miny; // y is centerline for row
			rrh+=rh;
			for (flow2=last; flow2; ) {
				w=(flow2->image->maxx-flow2->image->minx)*s;
				h=(flow2->image->maxy-flow2->image->miny)*s;
				flow2->image->origin(flatpoint(x,y-h/2));
				x+=w;
				if (flow2==flow) break;
				flow2=flow2->next;
			}
		} while (flow->next && nn<maxperpage); // continue doing rows

		 // now do final arranging of nn images in range [info,flow]
		 // push images onto the page, adjusting their origins appropriately
		DBG cout <<"  add "<<nn<<" images to page "<<curpage<<endl;
		while (1) {
			DBG cout <<"   adding image ..."<<endl;
			info->image->origin(info->image->origin()+flatpoint(0,(rrh-hh)/2));
			g=doc->pages.e[curpage]->e(doc->pages.e[curpage]->layers.n()-1);
			g->push(info->image,0); //incs the obj's count
			if (info==flow) break;
			info=info->next;
		}
		info=info->next;
		n+=nn;
	} // end loop block for page

	DBG cout <<"-----------------end dump images[]----------------"<<endl;
	if (outline) { outline->dec_count(); outline=NULL; }
	return curpage;
}


//------------------------------ Images for page numbers------------------------------------

////! Apply images of page numbers to the pages of the document.
///*! Say you draw little pictures to be the page numbers, and put the images in the directory
// *  pathtonums, then use the files in there as page numbers..
// *
// * *** this must be an automatic placement for new pages!!
// */
//int applyPageNumbers(Document *doc,const char *pathtonums=NULL)
//{
//}

