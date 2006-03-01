#ifndef NEWDOC_H
#define NEWDOC_H

#include <lax/rowframe.h>
#include <lax/lineedit.h>
#include <lax/lineinput.h>
#include <lax/checkbox.h>
#include <lax/menuselector.h>
#include <lax/strsliderpopup.h>
#include <lax/textbutton.h>
#include <lax/mesbar.h>
#include <lax/colorbox.h>

#include "laidout.h"
#include "papersizes.h"

class NewDocWindow : public Laxkit::RowFrame
{
	int mx,my;
	virtual void sendNewDoc();
 public:
	int curorientation;
	 // the names of each, so to change Left->Inside, Top->Inside (like calender), etc
	const char *marginl,*marginr,*margint,*marginb; 
	Disposition *disp;
	PaperType *papertype;
	
	Laxkit::PtrStack<PaperType> *papersizes;
	Laxkit::StrSliderPopup *dispsel;
	Laxkit::LineEdit *lineedit;
	Laxkit::LineInput *saveas,*paperx,*papery,*numpages;
	Laxkit::MessageBar *mesbar;
	Laxkit::CheckBox *defaultpage,*custompage;
 	NewDocWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder);
	virtual ~NewDocWindow();
	virtual int init();
//	virtual int Refresh();
//	virtual int CharInput(char ch,unsigned int state);
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
};


#endif

