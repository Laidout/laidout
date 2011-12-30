//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2004-2007 by Tom Lechner
//

#include <lax/messagebar.h>
#include <lax/button.h>

#include "language.h"
#include "headwindow.h"
#include "helpwindow.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;


//------------------------ HelpWindow -------------------------
//
/*! \class HelpWindow
 * \brief Currently just a message box with the list of all the shortcuts.
 *
 * In the future, this class will be rather more than that. Ultimately, the
 * Laxkit will have ability to track the short cuts on the fly, so this
 * window will latch on to that, as well as provide other info...
 */  


//! If style!=0, then do no special sizing in preinit...
/*! \todo anyhow need to work out sizing in Laxkit::MessageBox!!
 */
HelpWindow::HelpWindow(int style)
	: MessageBox(NULL,NULL,"Help!",ANXWIN_ESCAPABLE, 0,0,500,600,0, NULL,0,NULL, NULL)
{
	s=style;
}

/*! The default MessageBox::init() sets m[1]=m[7]=10000, which is supposed 
 * to trigger a wrap to extent. However, if a window has a stretch of 2000, say
 * like the main messagebar, then that window is stretched
 * to that amount, which is silly. So, intercept this to be a more reasonable width.
 */
int HelpWindow::preinit()
{
	if (s) return 0;
	Screen *screen=DefaultScreenOfDisplay(app->dpy);
	
	m[1]=screen->width/2;
	m[7]=10000; //<-- this triggers a wrap in rowcol-figureDims
	//WrapToExtent: 
	arrangeBoxes(1);
	win_w=m[1];
	win_h=m[7];

	if (win_h>(int)(.9*screen->height)) { 
		win_h=(int)(.9*screen->height);
	}
	if (win_w>(int)(.9*screen->width)) { 
		win_w=(int)(.9*screen->width);
	}
	return 0;
}

/*! Pops up a box with the QUICKREF and an ok button.
 *
 * \todo when shortcuts are fully implemented, the text here will be basically automatic.
 */
int HelpWindow::init()
{
	//MessageBar *mesbar=new MessageBar(this,"helpmesbar",MB_LEFT|MB_MOVE, 0,0,0,0,0, "test");
	char *help=newstr(
		  _("---- Laidout Quick key reference ----\n"
			"\n"
			"Press escape to get rid of this window.\n"
			"Right click drag scrolls this help.\n"
			"Right click drag with shift or control scrolls faster.\n"
			////DBG "The keys with a '***' next to them are not implemented yet.\n"
			"\n"
			" + means Shift and ^ means control\n"
			"\n"
			"Window gutter:\n"
			"   ^left-click   Split window mouse was in last\n"
			"   +left-click   Join to adjacent window\n"
			"   right-click   Get a menu to split, join, or change\n"
			"\n"
			"\n"));
	appendstr(help,_(
			"In a viewer:\n"
			"   ^'s'          save file\n"
			"   ^+'s'         save as -- just change the file name?? (not imp)\n"
			"   'T' or left   prev tool \n"
			"   't' or right  next tool\n"
			"   '<'           previous page \n"
			"   '>'           next page\n"
            "   F1            popup this quick reference\n"
			"   F2            popup an About window\n"
			"   F5            popup new spread editor window\n"
			"\n"));
	appendstr(help,_(
			"   left      previous tool   \n"
			"   right     next tool       \n"
			"   ','       previous object \n"
			"   '.'       next object     \n"
			" +^'a'       deselect all currently selected objects\n"
			"   '<'       previous page   \n"
			"   '>'       next page       \n"
			" \n"));
	appendstr(help,_(
			"   'D'        toggle drawing of axes for each object\n"
			"   's'        toggle showing of the spread (shows only limbo)\n"
			"   'm'        move current selection to another page, pops up a dialog ***imp me!\n"
			"   ' '        center the spread\n"
			"   +' '       center the page\n"
			"   '=' or '+' zoom in around mouse\n"
			"   '-'        zoom out around mouse\n"
			"   'o'        clear any display rotation\n"
			"   'O'        center the viewer on the viewer origin\n"
			"\n"));
	appendstr(help,_(
			"    // these are like inkscape:\n"
			"   pgup      raise selection by 1 within layer\n"
			"   pgdown    lower selection by 1 within layer\n"
			"   home      bring selection to top within layer\n"
			"   end       drop selection to bottom within layer\n"
			////DBG "   +pgup     ***move selection up a layer\n"
			////DBG "   +pgdown   ***move selection down a layer\n"
			////DBG "   +^pgup    ***raise layer 1\n"
			////DBG "   +^pgdown  ***lower layer 1\n"
			////DBG "   +^home    ***layer to top\n"
			////DBG "   +^end     ***layer to bottom\n"
			"\n"
			"\n"));
	appendstr(help,_(
			"Signature Editor:\n"
			"  'h'/'H' Increase or decrease the number of horizontal folds\n"
			"  'v'/'V' Increase or decrease the number of vertical folds\n"
			"  'm'/'M' Increase or decrease page margin values\n"
			"  ^'m'    Switch which margin value is changed by keyboard controls\n"
			"  't'/'T' Increase or decrease page trim values\n"
			"  ^'t'    Switch which trim value is changed by keyboard controls\n"
			"  'i'/'I' Increase or decrease paper inset values\n"
			"  ^'i'    Switch which inset value is changed by keyboard controls\n"
			"  'x'/'X' Increase or decrease the number of tiles horizontally\n"
			"  'y'/'Y' Increase or decrease the number of tiles vertically\n"
			"  'g'/'G' Increase or decrease the size of the gap between tiles\n"
			"  'b'/'B' Move the binding edge\n"
			"\n"
			"\n"));
	appendstr(help,_(
			"SpreadEditor:\n"
			"   ' '    Center with all little spreads in view\n"
			"   'c'    toggle where the page labels go\n"
			"   'm'    toggle mark of current page\n"
			"   'M'    reverse toggle mark of current page\n"
			"   'p'    toggle drawing of thumbnails\n"
			"   'A'    toggle how to arrange the spreads\n"
			"  ^'A'    force arranging the spreads using current arrange style\n"
			"\n"
			"\n"));
	appendstr(help,_(
			"ObjectInterface:\n"
			"  'c'          toggle if drag resizes from center or opposite edge\n"
			"  'd'          toggle the showing of decorations\n"
			"  'n'          normalize, that is, make norm(xaxis)==norm(yaxis) and y=transpose(x)\n"
            "  'N'          like 'n', but also clear rotation\n"
			"  bksp or del  remove whichever of center, rotation handle, or shear handle\n"
			"                 is selected\n"
			"  ^left-click  define a center, rotation handle, or shear handle\n"
			"  \n"
			"  plain click on a center, rotation handle, or shear handle, then press control\n"
			"    to move the point, rather than modify the object\n"
			"\n"
			"\n"));
	appendstr(help,_(
			"ImageInterface:\n"
            "  'n'           normalize, that is, make norm(xaxis)==norm(yaxis) and y=transpose(x)\n"
            "  'N'           like 'n', but also clear rotation\n"
            "  'd'           Toggle drawing decorations\n"
            "  'f'           Toggle writing the filename next to the image\n"
            "  double-click  bring up the image properties dialog\n"
			"\n"
			"\n"));
	appendstr(help,_(
			"ColorPatchInterface and ImagePatchInterface:\n"
			"  'w'    warp the patch to an arc, rows are at radius, cols go from center\n"
			"  'm'    toggle between drawing just the grid, or draw full colors.\n"
			"  'a'    select all points, or deselect all if any are selected\n"
			"  'y'    constrain to y changes, or release the constraint\n"
			"  'x'    constrain to x changes, or release the constraint\n"
			"  'o'    decrease how much to recurse for rendering screen preview\n"
			"  'O'    increase how much to recurse for rendering screen preview\n"
			"  'R'    increase how many rows to divide each row into\n"
			" ^'R'    decrease how many rows to divide each row into\n"
			"  'r'    subdivide rows\n"
			"  'c'    subdivide columns\n"
			"  'C'    increase how many columns to divide each column into\n"
			" ^'C'    decrease how many columns to divide each column into\n"
			"  's'    subdivide rows and columns\n"
			"  'z'    reset to rectangular\n"
			"  'd'    toggle decorations\n"
			"  'h'    select all points adjacent horizontally to current points\n"
			"  'v'    select all points adjacent vertically to current points\n"
			"  'b'    toggle which control points are available\n"
			"  'B'    reverse of 'b'\n"
			"  'j'    toggle smooth node editing (j for jagged)\n"
			"  '1'    select corners:  0,0  0,3  3,0  3,3\n"
			"  '2'    select center controls: 1,1  1,2  2,1  2,2\n"
			"  '3'    select edge controls: 0,1  0,2  1,0  2,0  1,3  2,3  3,1  3,2\n"
			"  '4'    select top and bottom controls: 1,0  2,0  1,3  2,3\n"
			"  '5'    select left and right controls: 0,1  0,2  3,1  3,2\n"
			"  '8'    select a 3x3 group of points around each current point\n"
			"\n"
			"\n"));
	appendstr(help,_(
			"GradientInterface:\n"
			"  'r'   Radial gradient\n"
			"  'l'   Linear Gradient\n"
			"  'f'   flip the order of the colors\n"
			"  'd'   Toggle showing of decorations\n"
			"  left  Select next point\n"
			"  right Select previous point\n"
			"  \n"
			"  shift-left-click: add a new color spot\n"
			"\n"
			"\n"));
	appendstr(help,_(
			"PathInterface:\n"
			"  'o'    Select the next pathop.\n"
			"  left   Roll the curpoints one step previous.\n"
			"  right  Roll the curpoints one step next.\n"
			"  'A'    Toggle whether to add points after or before\n"
			"  'a'    Select all if none selected, else deselect all\n"
			"  'c'    Toggle closed path\n"
			"  'b'    Start a new PathsData\n"
			"  delete or bksp: Delete currently selected points.\n"
			"  'd'    Toggle displaying of decorations\n"
			"  '?'    Show some kind of help somewhere....?\n"
			"  'p'    Like a, but only in current part of a compound path\n"));

	MessageBar *mesbar=new MessageBar(this,"helpmesbar",NULL,MB_LEFT|MB_TOP|MB_MOVE, 0,0,0,0,0,help);
	delete[] help;
			
	mesbar->tooltip(_("Right click drag scrolls this help."));
	AddWin(mesbar,1,mesbar->win_w,mesbar->win_w*9/10,2000,50,0,
					mesbar->win_h,(mesbar->win_h>10?(mesbar->win_h-10):0),2000,50,0, -1);
	AddNull();
	AddButton(BUTTON_OK);
	
	MessageBox::init();

	return 0;
}

int HelpWindow::Event(const Laxkit::EventData *e,const char *mes)
{
	if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
	return MessageBox::Event(e,mes);
}

	
/*! Esc  dismiss the window.
 */
int HelpWindow::CharInput(unsigned int ch,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;
	}
	return 1;
}

