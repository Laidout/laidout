//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/************************ styles.cc **************************/

// This file is currently not used.
//

#ifndef DOXYGEN_SHOULD_IGNORE_THIS


class CharacterStyle : public Style
{
 public:
	
};

 /*! Character style has:
  *   font (name of, string)
  *   size (points)
  *   scaling x (percent)
  *   scaling y (percent)
  *   color
  *   hkern (kerns affect further positioning)
  *   vkern
  *   Characteristics (flags): 
  *     bold
  *     italic
  *     strikethrough
  *     outline
  *     non-breaking
  *     underline
  *     highlight
  *     blink
  *   hdisplacement (displacements do not affect further positioning)
  *   vdisplacement 
  *   Uppercase/Lowercase/Normal case/Smallcaps (enum)
  *   -----
  *   Superscript
  *   Subscript <-- these last two are really just instances of another style to it,
  *   				and are not really part of Character style itself
  */
StyleDef *CharacterStyleDef()
{
	StyleDef *sd=new StyleDef("charstyle","Character Style","Character Style",
							"This holds the characteristics of spans of characters.",
							NULL,0);
	***
	return sd;
}





//***--------------------------fairly old stuff follows

//---------------------------------------------------------------------------
///**********class ScreenStyle : public Style
//{
//	struct Wstuff {
//		XRect w;
//		int nb;
//		BoxStyle **boxes;
//		XRect *boxbounds;
//	} *wstuff;
//	int numwindows;
//	const char *docname;***not local, points to doc name in doc
//	
// 	const char *whattype() { return "Style.Screen"; }
//}**********/


class LayerStyle : public Style
{
	int locked,visible,dontprint;
	whattype() { return ".Style..Layer."; }
};

//--------------------------------- Drawable Object Styles --------------------------

class ImageStyle: public Style
{
	int dpi,rgborgrey,opacity?
	whattype() { return "ImageStyle"; }
};

class FontStyle : public Style
{
	int size,vshift,fontstyle; ***bold,normal,strikethrough,underline,italic
	char *fontname;
	const char *whattype() { return ".Style.Font."; } 
};

class ParagraphStyle : public Style
{
	int firstindent,leftindent,rightindent;
	int justification; /* left,right,center,centercurve */
	int prelead,postlead,midlead;
	const char *whattype() { return ".Style.Paragraph."; } 
};

class LineStyle : public Style
{
	int linestyle; ***none,solid,dash,dot,dashdot
	int thickness;
	int red,green,blue;
	const char *whattype() { return ".Style.Line."; }
	int CopyStyle(Style *astyle);
};


//-------------------------- Application and Window Styles -------------------------------

class AppStyle : public Style
{
	char **modules;
	DocStyle *defaultdoc;
	ScreenStyle *defaultscreen;
	const char *whattype() { return "Style.App"; }
};


class BoxStyle : public Style
{
	XRect boxbound;
	const char *whattype() { return "Style.Box"; }
};

class ColorStripStyle : public BoxStyle
{	const char *whattype() { return "Style.Box.ColorStrip"; }
};
class ColorStrip : public Box
{};

class ControlStripBoxStyle : public BoxStyle
{	const char *whattype() { return "Style.Box.ControlStrip"; }
};
class ControlStripBox : public Box
{};

class CurpageBoxStyle : public BoxStyle
{	const char *whattype() { return "Style.Box.Curpage"; }
};
class CurpageBox : public Box
{};

class DisplayBoxStyle : public BoxStyle 
{
	const char *whattype() { return "Style.Box.Display"; }
	double mag,centerx,centery,rot;
	DisplayBoxStyle() { mag=1; centerx=centery=0; rot=0; }
};
class DisplayBox : public Box
{ 
  protected:
  	DisplayBoxStyle *curstyle;
};

class LayerBoxStyle : public BoxStyle
{	const char *whattype() { return "Style.Box.Layer"; }
	char lx,ly,base;
	LayerBoxStyle() { lx=10; ly=2; base=lx; }
};
class LayerBox : public Box
{};

class MessageBoxStyle : public BoxStyle
{ 	const char *whattype() { return "Style.Box.Message"; }
};
class TheMessageBox : public MessageBox
{};

class PagenumberBoxStyle : public BoxStyle
{
	const char *whattype() { return "Style.Box.Pagenumber"; }
};
class PagenumberBox : public Box
{};

class XYBoxStyle : public BoxStyle
{	const char *whattype() { return "Style.Box.XY"; }
};
class XYBox : public Box
{
  protected:
};

class ZoomBoxStyle : public BoxStyle
{	const char *whattype() { return "Style.Box.Zoom"; }
};
class ZoomBox : public Box
{};

class ScreenStyle : public Style
{
	struct Wstuff {
		XRect wbounds;
		int nb;
		BoxStyle **boxes;
		XRect *boxbounds;
	} *wstuff;
	int numwindows;
	
 	const char *whattype() { return "Style.Screen"; }
};

#endif //DOXYGEN_SHOULD_IGNORE_THIS

