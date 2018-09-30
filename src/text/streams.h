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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef STREAMS_H
#define STREAMS_H



namespace Laidout {


class TextEffect
{
  public:
	virtual ~TextEffect();
};

class TextUnderline : public TextEffect
{
  public:
	int linetype; //underline, double underline, strikethrough, etc
	int wordunderline; //nonzero if so
	int underlinestyle;
	double offset; //% of fontsize off baseline, either underline or strikethrough
	LineStyle *linestyle;
};

class DropShadowEffect : public TextEffect
{
  public:
	flatvector direction;
	double blur_amount;
	Color color; //color or color diff
};

class CharAdjust : public TextEffect
{
  public:
	 //glyph replacement effects:
	int ReplaceLetters(const char *in, char *out, int outbufsize);
	all caps
	no caps
	small caps

	 //glyph transformation effects:
	outline (artificial stroking of contour)
	bold   (artificial growing and filling of contour, or selection of bold font of same type)
	italic (artificial shearing)
};

class DropCaps : public TextEffect
{
  public:
	//drop caps at front of pp:
	int numlines;
	int wrap; //how to wrap
	//	hang initial punc.. like "Blah" ...
	//	                          Blah blah...
};

class LFont
{
  public:
	char *file;
	double font_size;
	const char *font_family, *font_style;
	virtual double Extent(const char *str,int len, double *real_ascent, double *real_descent);
	virtual double Em();
};

class MultiFont : public LFont
{
  public:
	class FontComponent
	{
	  public:
		LFont *font;
		Color *color;
	};

	PtrStack<FontComponent> fonts;
};

class CharacterStyle : public Style
{
  public:
	LFont *font;

	Color *color; //including alpha
	Color *bg_color; //highlighting

	double space_shrink;
	double space_grow;  //as percent of overriding font size
	double inner_shrink, inner_grow; //char spacing within words

	 //basic positioning info
	double kerning_before;
	double kerning_after;
	double h_scaling, v_scaling; //contentious glyph scaling

	 //more thorough text effects
	PtrStack<TextEffect> glyph_effects; //adjustments to characters or glyphs that changes metrics, making them different shapes or chars, like small caps, all caps, no caps, scramble, etc
	PtrStack<TextEffect> below_effects; //additions like highlighting, that exist under the glyphs
	PtrStack<TextEffect>  char_effects; //transforms to glyphs that do not change metrics, fake outline, fake bold, etc
	PtrStack<TextEffect> above_effects; //lines and such drawn after the glyphs are drawn

	CharacterStyle();
	virtual ~CharacterStyle();
};

enum LengthType {
	LEN_Percent_Text_Height,
	LEN_Percent_Parent,
	LEN_Absolute
};

class TabStopInfo
{
  public:
	int tabtype; //left, center, right, char
	char tab_char_utf8[10]; //if char type

	int positiontype; //automatic position, definite position, path
	double position; //if not path
	PathsData *path;

	TabStopInfo *next;
};

class ParagraphStyle : public Style
{
  public:
	int hyphens; //whether to have them or not
	double h_width,h_shrink,h_grow, h_penalty; //hyphen metrics, still use the font's character, though


	int gap_above_type; //0 for  % of font size, 1 for absolute number
	double gap_above;

	int gap_after_type; //0 for  % of font size, 1 for absolute number
	double gap_after;

	int gap_between_type; //0 for  % of font size, 1 for absolute number
	double gap_between_lines;


	int lines_of_first_indent; //default is 1
	double first_indent;
	double normal_indent;
	double far_margin; //for left to right, this would be the right margin for instance

	int tab_strategy; //regular intervals, by lines
	TabStopInfo *tabs;

	int justify_type; //0 for allow widow-like lines, 1 allow widows but do not fill space to edges, 2 for force justify all
	double justification; // (0..100) left=0, right=100, center=50, full justify
	int flow_direction;
	
	PtrStack<TextEffect> conditional_effects; //like initial drop caps, first line char style, etc

	CharacterStyle *default_charstyle;

	ParagraphStyle();
	virtual ~ParagraphStyle();
};


} // namespace Laidout

#endif

