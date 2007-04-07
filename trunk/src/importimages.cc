//
// $Id$
//	
// Laidout, for laying out
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


******************** NOTE: FILE NOT ACTIVE YET ************************

#include "importimages.h"


class ImportImageDialog : public Laxit::FileDialog
{
 public:
	double dpi;
};




// 	FileDialog(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
//			int xx,int yy,int ww,int hh,int brder, 
//			Window nowner,const char *nsend,
//			const char *nfile=NULL,const char *npath=NULL,const char *nmask=NULL);
ImportImageDialog::ImportImageDialog(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			Window nowner,const char *nsend,
			const char *nfile,const char *npath,const char *nmask,
			double defdpi)
	:FileDialog(parnt,ntitle,nstyle,xx,yy,ww,hh,brder,nowner,nsend,nfile,npath,nmask)
{
	dpi=defdpi;
}

int ImportImageDialog::init() 
{
	win_style=win_style&~(FILES_PREVIEW|FILES_PREVIEW2);
	FileDialog::init();

	*** set up the extra windows....
//LineInput::LineInput(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
//			int xx,int yy,int ww,int hh,unsigned int bordr,
//			anXWindow *prev,Window nowner,const char *nsend,
//			const char *newlabel,const char *newtext,unsigned int ntstyle,
//			int nlew,int nleh,int npadx,int npady,int npadlx,int npadly) // all after and inc newtext==0
	
	LineInput *linp;
	MessageBar *mesbar;
	CheckBox *check;
	char *temp;

	 //---------------------- to insert list, next to main dir listing ---------------------------
	// menuinfo that allows rearranging, and has other info about what page image will be on...
	//--- for future! ---

	 //---------------------- per image preview controls ---------------------------
	***filename:_____________
		previewfile:_________
		[[Re]Generate Preview]
		use temporary preview
		description:_________

	 //---------------------- extra image layout controls ---------------------------
	str=numtostr(dpi,0);
	linp=new LineInput(this,"DPI",0, 0,0,0,0,0,NULL,window,"dpi",
						"DPI:",str,0,
						0,0,0,0,0,0);
	delete[] str; str=NULL;
//	virtual int AddWin(anXWindow *win,int npw,int nws,int nwg,int nhalign, int nph,int nhs,int nhg,int nvalign);
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50);
	
	str=numtostr(start,0);
	linp=new LineInput(this,"StartPage",0, 0,0,0,0,0,NULL,window,"startpage",
						"Start Page:",str,0,
						0,0,0,0,0,0);
	delete[] str; str=NULL;
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50);
	
	str=numtostr(end,0);
	linp=new LineInput(this,"EndPage",0, 0,0,0,0,0,NULL,window,"endpage",
						"End Page:",str,0,
						0,0,0,0,0,0);
	delete[] str; str=NULL;
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50);

	 //------------------------------ Images Per Page
	***Images Per Page ____, or "as many as will fit", or "all on 1 page"
		
	mesbar=new MessageBar(this,"perpage",MB_MOVE, 0,0, 0,0, 0, "Images Per Page:");
	AddWin(mesbar, mesbar->win_w,0,0,50, mesbar->win_h,0,0,50);
	AddNull();
	
	check=new CheckBox(this,"perpagenum",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"perpagenum", NULL,5,5);
	check->State(LAX_ON);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
	linp=new LineInput(this,"NumPerPage",0, 0,0,0,0,0,NULL,window,"numperpage",
						"Exactly this",str,0,
						0,0,0,0,0,0);
	AddWin(linp);

	AddNull();
	check=new CheckBox(this,"perpagefit",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"perpagefit", "As many as will fit per page",5,5);
	check->State(LAX_OFF);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);

	AddNull();
	check=new CheckBox(this,"perpageall",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"perpageall", "All on one page",5,5);
	check->State(LAX_OFF);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
}
