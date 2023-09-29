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
// Copyright (C) 2023-present by Tom Lechner
//

#include "streams.h"
#include "../language.h"
#include "../dataobjects/fontvalue.h"
#include "../laidout.h"

#include <lax/units.h>
#include <lax/utf8utils.h>
#include <lax/cssutils.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/interfaces/somedatafactory.h>

#include <lax/debug.h>


using namespace Laxkit;


namespace Laidout {

/*! \file
 *
 * DrawableObjects can have text streams attached in various ways. general structure is:
 *
 * ```
 * DrawableObject
 *   StreamAttachment
 *     StreamCache - a particular rendering of Stream to the DrawableObject
 *     Stream
 *       top: StreamElement - a tree of Style objects, with leaf elements pointing to lists of StreamChunk
 *       chunk_start: a linked list of final leaf chunks
 */



//----------------------------------- General helper functions -----------------------------------

/*! -2 means type not found.
 *  -1 means auto.
 *  Else return one of LAX_LRTB, LAX_RLTB, LAX_LTBT, LAX_RLTB, LAX_ RLBT, LAX_TBLR, LAX_TBRL, LAX_BTLR, LAX_BTRL.
 */
int ParseFlowDirection(const char *str, int len)
{
	if (len < 0) len = strlen(str);
	int bidi = -2;

	if      (!strncasecmp(str,"ltr" , len)) bidi = LAX_LRTB;
	else if (!strncasecmp(str,"rtl" , len)) bidi = LAX_RLTB;
	else if (!strncasecmp(str,"auto", len)) bidi = -1;
	else if (!strncasecmp(str,"lrtb", len)) bidi = LAX_LRTB;
	else if (!strncasecmp(str,"lrbt", len)) bidi = LAX_LRBT;
	else if (!strncasecmp(str,"rltb", len)) bidi = LAX_RLTB;
	else if (!strncasecmp(str,"rlbt", len)) bidi = LAX_RLBT;
	else if (!strncasecmp(str,"tblr", len)) bidi = LAX_TBLR;
	else if (!strncasecmp(str,"tbrl", len)) bidi = LAX_TBRL;
	else if (!strncasecmp(str,"btlr", len)) bidi = LAX_BTLR;
	else if (!strncasecmp(str,"btrl", len)) bidi = LAX_BTRL;

	return bidi;
}


/*! Something with simple contents in parantheses, like:
 *  - local(blah)
 *  - local("Blah")
 *  - local('Blah')
 *  - url(https://thing)
 *
 *  Return 1 for successful parse, else 0.
 */
int ParseSimpleFofS(const char *value, int *f_len_ret, const char **s_ret, int *s_len_ret, const char **end_ret)
{
	//while (isspace(*value)) value++;
	const char *ptr = value;
	while (isalnum(*ptr)) ptr++;
	if (ptr == value) return 0;
	*f_len_ret = ptr - value;

	value = ptr;
	while (isspace(*value)) value++;
	if (*value != '(') return 0;
	value++;

	while (isspace(*value)) value++;
	char has_quote = '\0';
	if (*value == '"' || *value == '\'') { has_quote = *value; value++; }
	ptr = value;
	*s_ret = value;
	while (*ptr && *ptr != has_quote && *ptr != ')') ptr++;
	*s_len_ret = ptr - value;
	if (has_quote) {
		if (*ptr != has_quote) return 0;
		ptr++;
	}
	if (*ptr != ')') return 0;

	value = ptr+1; 
	*end_ret = value;
	return 1;
}


//----------------------------------- LengthValue -----------------------------------

class LengthValue : public Value
{
  public:
	double value;
	double v_cached; //cached context dependent computed absolute value. This will be set from outside LengthValue, such as when computing a StreamCache.

	//enum LengthType {
	//	LEN_Number, // absolute units are used
	//	LEN_Percent_Parent,
	//	LEN_Percent_Paper,
	//	LEN_em, // 1 = font height
	//	LEN_ex, // 1 = x-height in the font
	//	LEN_ch, // 1 = advance width of the '0' glyph of the font
	//	LEN_vw, // 100 = viewport width
	//	LEN_vh, // 100 = viewport height
	//	LEN_vmin, // 100 = minimum of viewport width, height
	//	LEN_vmax  // 100 = maximum of viewport width, height
	//};

	CSSName type;

	static Laxkit::UnitManager unit_manager;
	Laxkit::Unit units;

	LengthValue(); 
	LengthValue(double val, Laxkit::Unit unit, CSSName len_type);
	LengthValue(double val, Laxkit::Unit unit);
	LengthValue(const char *val, int len=-1, const char **endptr=nullptr);

	int ParseUnits(const char *str, int len);

	//Value overrides:
	virtual Value *duplicate();
 	virtual ObjectDef *makeObjectDef(); //calling code responsible for ref

	static LengthValue *Parse(const char *val, int len = -1, const char **endptr = nullptr);
};

/*! Static constructing function.
 */
LengthValue *LengthValue::Parse(const char *val, int len, const char **endptr)
{
	//TODO: it would be nice to do this with no allocation on error.
	LengthValue *v = new LengthValue(val,len);
	if (v->type == CSS_Error) {
		delete v;
		return nullptr;
	}
	return v;
}

LengthValue::LengthValue()
{
	value = 0;
	v_cached = 0;
	type = CSS_Physical; //LEN_Number;
}

LengthValue::LengthValue(const char *val, int len, const char **end_ptr)
  : LengthValue()
{
	//TODO: properly use len to truncate val if necessary

	if (isblank(val)) return;

	if (len < 0) len = strlen(val);
	char *endptr = nullptr;
	value = strtod(val, &endptr);

	if (endptr != val) {
		while (isspace(*endptr)) endptr++;

		if (*endptr == '%') {
			type = CSS_Percent; //LEN_Percent_Parent;
			endptr++;

		} else {
			// parse units
			const char *uptr = endptr;
			while (isalpha(*uptr)) uptr++;
			int _units = ParseUnits(endptr, uptr - endptr);
			if (_units != Laxkit::UNITS_None) {
				units = _units;
				if      (units == Laxkit::UNITS_em)   { type = CSS_em;   }
				else if (units == Laxkit::UNITS_ex)   { type = CSS_ex;   }
				else if (units == Laxkit::UNITS_ch)   { type = CSS_ch;   }
				else if (units == Laxkit::UNITS_vw)   { type = CSS_vw;   }
				else if (units == Laxkit::UNITS_vh)   { type = CSS_vh;   }
				else if (units == Laxkit::UNITS_vmin) { type = CSS_vmin; }
				else if (units == Laxkit::UNITS_vmax) { type = CSS_vmax; }
				else type = CSS_Physical;				
			}
		}
	} else {
		type = CSS_Error;
	}
	if (end_ptr) *end_ptr = endptr;
}


int LengthValue::ParseUnits(const char *str, int len)
{
	if (unit_manager.NumberOfUnits() == 0) {
		Laxkit::CreateDefaultUnits(&unit_manager);

		unit_manager.AddUnits(Laxkit::UNITS_Pixels, .0254/96, _("px"), _("pixel"), _("pixels"));
		unit_manager.AddUnits(Laxkit::UNITS_em,     1, _("em"), _("em"),    _("em"));

		unit_manager.AddUnits(Laxkit::UNITS_ex,     1, _("ex"), _("ex"),    _("ex"));
		unit_manager.AddUnits(Laxkit::UNITS_ch,     1, _("ch"), _("ch"),    _("ch"));
		unit_manager.AddUnits(Laxkit::UNITS_rem,    1, _("rem"),_("rem"),   _("rem"));
		unit_manager.AddUnits(Laxkit::UNITS_vw,     1, _("vw"), _("vw"),    _("vw"));
		unit_manager.AddUnits(Laxkit::UNITS_vh,     1, _("vh"), _("vh"),    _("vh"));
		unit_manager.AddUnits(Laxkit::UNITS_vmin,   1, _("vmin"), _("vmin"),_("vmin"));
		unit_manager.AddUnits(Laxkit::UNITS_vmax,   1, _("vmax"), _("vmax"),_("vmax"));
	}

	return unit_manager.UnitId(str, len);
}

Value *LengthValue::duplicate()
{
	LengthValue *dup = new LengthValue(value, units, type);
	return dup;
}

ObjectDef *LengthValue::makeObjectDef()
{
	//calling code responsible for ref
	DBGE("IMPLEMENT ME!!");
	return nullptr;
}

//----------------------------------- TabStopInfo -----------------------------------


TabStopInfo::TabStopInfo()
{
	alignment        = 0;
	use_char         = false;
	tab_char_utf8[0] = '\0';
	positiontype     = 0;
	position         = 0;
	path             = nullptr;
}

TabStopInfo::~TabStopInfo()
{
	if (path) path->dec_count();
}


//----------------------------------- Style -----------------------------------
/*! \class Style
 * \brief Class to hold cascading styles.
 *
 * Style objects are either temporary objects embedded in StreamElement, or they are project resources.
 * A Style may be based on a "parent" style. In this case, the parent style must be a project resource.
 */

Style::Style()
{
    parent = nullptr;
}

Style::Style(const char *new_name) //todo: *** name of what? instance? style type?
{
    parent = nullptr;
    Id(new_name);
}

Style::~Style()
{
}

/*! Create and return a new Style cascaded upward from this.
 * Values in *this override values in parents.
 */
Style *Style::Collapse()
{
    Style *s = new Style();
    Style *f = this;
    while (f) {
        s->MergeFromMissing(f);
        f = f->parent;
    }
    return s;
}

/*! Adds to *this any key+value in s that are not already included in this->values. It does not replace values.
 * Note ONLY checks against *this, NOT this->parent or this->kids.
 * Return number of items added.
 *
 * So, say you have:
 *     s:
 *       bold: true
 *       color: red
 *     this: 
 *       bold: false
 *       width: 5
 *
 * you will end up with:
 *     this:
 *       bold: false
 *       width: 5
 *       color: red
 */
int Style::MergeFromMissing(Style *s)
{
    const char *str;
    int n = 0;
    for (int c = 0; c < s->n(); c++) {
        str = s->key(c);
        if (findIndex(str) >= 0) continue; //don't use if we have it already
        push(str, s->e(c));
        n++;
    }
    return n;
}

/*! Adds or replaces any key+value from s into this->values.
 * Note ONLY checks against *this, NOT this->parent or this->kids.
 * Return number of items added.
 *
 * So, say you have:
 *     s:
 *       bold: true
 *       color: red
 *     this: 
 *       bold: false
 *       width: 5
 *
 * you will end up with:
 *     this:
 *       bold: true
 *       width: 5
 *       color: red
 */
int Style::MergeFrom(Style *s)
{
    const char *str;
    int n = 0;
    for (int c=0; c<s->n(); c++) {
        str = s->key(c);
        push(str, s->value(c)); //adds or replaces
        n++;
    }
    return n;
}


//----------------------------------- CharacterStyle -----------------------------------

class CharacterStyle : public Style
{
  public:
  	// // these member variables are cached from collapsed Style tree
	// FontValue *font;

	// Laxkit::Color *color; //including alpha
	// Laxkit::Color *bg_color; //highlighting

	// double space_shrink;
	// double space_grow;  //as percent of overriding font size
	// double inner_shrink, inner_grow; //char spacing within words

	//  //basic positioning info
	// double kerning_before;
	// double kerning_after;
	// double h_scaling, v_scaling; //contentious glyph scaling

	//  //more thorough text effects
	// PtrStack<TextEffect> glyph_effects; //adjustments to characters or glyphs that changes metrics, making them different shapes or chars, like small caps, all caps, no caps, scramble, etc
	// PtrStack<TextEffect> below_effects; //additions like highlighting, that exist under the glyphs
	// PtrStack<TextEffect>  char_effects; //transforms to glyphs that do not change metrics, fake outline, fake bold, etc
	// PtrStack<TextEffect> above_effects; //lines and such drawn after the glyphs are drawn

	CharacterStyle();
	virtual ~CharacterStyle();

	virtual void ParseFrom(StreamChunk *chunk);

	static Style *Default(Style *s = nullptr);
};

/*! Add to s if s!=nullptr.
 */
Style *CharacterStyle::Default(Style *s)
{
	if (!s) s = new Style("Character");

	FontValue *font = new FontValue(); //todo: *** this needs to be some configurable default font
	DBG cerr << __FILE__<<" #"<<__LINE__<<": fix me!"<<endl;

	s->push("font",       font, -1, true);
	s->push("fontfamily", "serif");
	s->push("fontstyle",  "normal");
	s->push("fontsize",   new LengthValue("11pt"), -1, true);
	s->push("color",      new ColorValue("black"), -1, true);
	s->push("bg_color",   (Value*)nullptr);
	s->push("kern_before",new LengthValue("0%"), -1, true);
	s->push("kern_after", new LengthValue("0%"), -1, true);
	// s->push("h_scaling", 1.0);
	// s->push("v_scaling", 1.0);
	s->push("space_shrink", 0.0);
	s->push("space_grow",   0.0);
	s->push("inner_shrink", 0.0);

	s->push("glpyh_effects", new SetValue(), -1, true);
	s->push("below_effects", new SetValue(), -1, true);
	s->push("char_effects",  new SetValue(), -1, true);
	s->push("above_effects", new SetValue(), -1, true);
	
	return s;
}


//----------------------------------- ParagraphStyle -----------------------------------

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
	TabStops *tabs;

	int justify_type; //0 for allow widow-like lines, 1 allow widows but do not fill space to edges, 2 for force justify all
	double justification; // (0..100) left=0, right=100, center=50, full justify
	int flow_direction;
	
	Laxkit::PtrStack<TextEffect> conditional_effects; //like initial drop caps, first line char style, etc

	CharacterStyle *default_charstyle;

	ParagraphStyle();
	virtual ~ParagraphStyle();

	virtual void ParseFrom(StreamChunk *chunk);

	static Style *Default(Style *s = nullptr);
};

/*! Add to s if s!=nullptr. Else return a new Style.
 */
Style *ParagraphStyle::Default(Style *s)
{
	if (!s) s = new Style("Paragraph");

	s->push("first_indent_lines", 1);
	s->push("first_indent", 0.0);
	s->push("normal_indent", 0.0);
	s->push("far_indent", 0.0); //for left to right, this would be the right margin for instance

	s->push("gap_above",   new LengthValue(1.0, UNITS_em), -1, true); //0 for  % of font size, 1 for absolute number
	s->push("gap_after",   new LengthValue(1.0, UNITS_em), -1, true); //0 for  % of font size, 1 for absolute number
	s->push("gap_between", new LengthValue(1.0, UNITS_em), -1, true); //0 for  % of font size, 1 for absolute number

	//todo: tabs
	//int tab_strategy; //regular intervals, by lines
	//TabStops *tabs;

	s->push("justify_type", 0); //0 for allow widow-like lines, 1 allow widows but do not fill space to edges, 2 for force justify all
	s->push("justification", 0.0); // (0..100) left=0, right=100, center=50, full justify
	s->push("flow_direction", 0); //*** need to implement all LAX_LRTB
	
	s->push("hyphens", new BooleanValue(false), -1, true); //whether to have them or not
	s->push("hyphen_width",   1.0);
	s->push("hyphen_shrink",  0.0);
	s->push("hyphen_grow",    0.0);
	s->push("hyphen_penalty", 1.0);


	//PtrStack<TextEffect> conditional_effects; //like initial drop caps, first line char style, etc

	s->push("default_charstyle", (Value*)nullptr);

	return s;
}


//----------------------------------- StreamElement -----------------------------------
/*! \class StreamElement
 * \brief Class to hold a stream broken down into a tree.
 * 
 * Please note this is different than general cascaded styles in that parents
 * AND kids exist as a specific hierarchy. Children are kept track of, and the order of children
 * depends on the associated stream. General Style objects don't care about particular
 * children, only parents from which they are derived.
 *
 * Streams need to traverse styles up and down when changing their contents. Leaves in
 * the tree are the actual content, and are derived from StreamChunk.
 *
 * StreamElement are very much like XML blocks, in that style holds the element attributes,
 * and kids holds the element content.
 *
 */

/*! Default constructor will create a new empty Style(). */
StreamElement::StreamElement()
{
	style = new Style();
	style_is_temp = true;
}

/*! nparent should have already added *this to its kids. */
StreamElement::StreamElement(StreamElement *nparent, Style *nstyle)
{
	treeparent = nparent;

	style = nstyle;
	if (style) style->inc_count();
	style_is_temp = true;
}

StreamElement::~StreamElement()
{
	if (style) style->dec_count();
}


//----------------------------------- StreamChunk ------------------------------------




StreamChunk::StreamChunk(StreamElement *nparent)
{
	parent = nparent;
	next = prev = nullptr;
}

/* Default do nothing.
 */
StreamChunk::~StreamChunk()
{
	//if (next) delete next; no deleting, as chunks must ALWAYS be owned somewhere by a single StreamElement.
}

/*! Insert a currently unstyled list of chunks after ourself.
 * 
 * Assumes chunk->prev == nullptr. Ok for chunk->next to not be nullptr.
 * Inserts between this and this->next.
 *
 * If chunk->parent->style == nullptr, then use same style as *this. 
 * Otherwise, integrate the contained styles into the StreamElement tree.
 * It is assumed that chunk has a self contained network of styles.
 *
 * Returns next most StreamChunk of chunk.
 */
StreamChunk *StreamChunk::AddAfter(StreamChunk *chunk)
{
	if (!chunk) {
		DBG cerr << __FILE__<<" #"<<__LINE__<<": fix me!"<<endl;
		return nullptr;
	}
	
	StreamChunk *chunkend = chunk;
	while (chunkend->next) chunkend = chunkend->next;

	if (!next) { next = chunk; chunk->prev = this; }
	else {
		chunkend->next = next;
		if (chunkend->next) chunkend->next->prev = chunkend;
		next = chunk;
		chunk->prev = this;
	}

	StreamElement *new_tree_parent = parent;
	StreamChunk *ch = chunk;
	do {
		if (ch->parent == nullptr) {
			ch->parent = new_tree_parent;
		} else { // it came with a parent style, so we need to child it within existing tree
			StreamElement *el = ch->parent;
			while (el->treeparent) {
				if (el == new_tree_parent) break; //chunk style has already been attached to tree
				el = el->treeparent;
			}
			if (el->treeparent == nullptr) el->treeparent = new_tree_parent;
		}

		ch = ch->next;
	} while (ch != chunkend->next);

	return chunkend;
}

/*! Assumes chunk->prev==nullptr. Ok for chunk->next to not be nullptr.
 * Inserts between this and this->next.
 *
 * If chunk->style==nullptr, then use same style as *this.
 */
void StreamChunk::AddBefore(StreamChunk *chunk)
{
	if (!chunk) return;
	
	StreamChunk *chunkend = chunk;
	while (chunkend->next) chunkend = chunkend->next;

	if (prev) prev->next = chunk;
	chunk->prev = prev;

	chunkend->next = this;
	prev = chunkend;

	StreamElement *new_tree_parent = parent;
	StreamChunk *ch = chunk;
	do {
		if (ch->parent == nullptr) {
			ch->parent = new_tree_parent;
		} else { // it came with a parent style, so we need to child it within existing tree
			StreamElement *el = ch->parent;
			while (el->treeparent) {
				if (el == new_tree_parent) break; //chunk style has already been attached to tree
				el = el->treeparent;
			}
			if (el->treeparent == nullptr) el->treeparent = new_tree_parent;
		}

		ch = ch->next;
	} while (ch != chunkend->next);
}


//----------------------------------- StreamBreak ------------------------------------


Laxkit::Attribute *StreamBreak::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att = new Laxkit::Attribute();
	if      (break_type == (int)StreamBreakTypes::BREAK_Paragraph) att->push("break", "pp");
	else if (break_type == (int)StreamBreakTypes::BREAK_Column)    att->push("break", "column");
	else if (break_type == (int)StreamBreakTypes::BREAK_Section)   att->push("break", "section");
	else if (break_type == (int)StreamBreakTypes::BREAK_Page)      att->push("break", "page");
	else if (break_type == (int)StreamBreakTypes::BREAK_Tab)       att->push("break", "tab");
	else if (break_type == (int)StreamBreakTypes::BREAK_Weak)      att->push("break", "weak");
	else if (break_type == (int)StreamBreakTypes::BREAK_Hyphen)    att->push("break", "hyphen");
	return att;
}

/*! NOTE! Uses whole attribute, not just kids. Meaning it compares att->value to various break types.
 */
void StreamBreak::dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if      (!strcmp(att->value,"pp"     )) break_type = (int)StreamBreakTypes::BREAK_Paragraph;
	else if (!strcmp(att->value,"column" )) break_type = (int)StreamBreakTypes::BREAK_Column;
	else if (!strcmp(att->value,"section")) break_type = (int)StreamBreakTypes::BREAK_Section;
	else if (!strcmp(att->value,"page"   )) break_type = (int)StreamBreakTypes::BREAK_Page;
	else if (!strcmp(att->value,"tab"    )) break_type = (int)StreamBreakTypes::BREAK_Tab;
	else if (!strcmp(att->value,"weak"   )) break_type = (int)StreamBreakTypes::BREAK_Weak;
	else if (!strcmp(att->value,"hyphen" )) break_type = (int)StreamBreakTypes::BREAK_Hyphen;
}


//----------------------------------- StreamImage ------------------------------------
/*! \class StreamImage
 * \brief Type of stream component made of a single DrawableObject.
 *
 * Note this can be ANY drawable object, not just images.
 */

StreamImage::StreamImage(DrawableObject *nimg,StreamElement *parent_el)
  : StreamChunk(parent_el)
{
	img = nimg;
	if (img) img->inc_count();
}

StreamImage::~StreamImage()
{
	if (img) img->dec_count();
}

Laxkit::Attribute *StreamImage::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!img) {
		att->push("object");
		return att;
	}
	Laxkit::Attribute *att2 = att->pushSubAtt("object", img->whattype());
	img->dump_out_atts(att2, what, context);
	return att;
}

void StreamImage::dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	DrawableObject *newimg = DrawableObject::CreateFromAttribute(att, att->value, context);

	if (img) img->dec_count();
	img = newimg;
}



//----------------------------------- StreamText ------------------------------------

/*! \class StreamText
 * \brief Type of stream chunk made only of simple text.
 *
 * One StreamText object contains a string of characters not including a newline.
 * This string is a single style of text. Any links, font changes, or anything else
 * that distinguishes this text from adjacent text requires a different StreamText.
 */

StreamText::StreamText(StreamElement *parent_el)
  : StreamChunk(parent_el)
{
	len = 0;
	text = nullptr;
}

StreamText::StreamText(const char *txt,int n, StreamElement *parent_el)
  : StreamChunk(parent_el)
{
	if (n < 0) n = strlen(txt);
	text = newnstr(txt, n);
	len  = n;
}

StreamText::~StreamText()
{
	delete[] text;
}

	//virtual int SetFromEntities(const char *cdata, int len);
int StreamText::SetFromEntities(const char *cdata, int cdata_len)
{
	//***TODO: compress whitespace

	//big json file with entities list: https://html.spec.whatwg.org/entities.json
	int new_n = 0;
	if (cdata_len < 0) cdata_len = strlen(cdata);
	char *new_text = new char[cdata_len+1];
	htmlchars_decode(cdata, new_text);
	delete[] text;
	text = new_text;
	len = new_n;
	return new_n;
}

//! Return the number of possible (not actual) breaks within the string of characters.
/*! This can include hyphen points, spaces, or newlines.
 */
int StreamText::NumBreaks()
{
	if (!text) return 0;

	const char *s = text;
	int n = 0;
	while (*s) {
		while (*s && !isspace(*s)) s = utf8fwd(s, s, text+len);
		if (*s == ' ') {
			n++;
			s++; //assumes all white space is ' '. Assumed '\n' and '\t' previously converted to explicit breaks
		}
	}
	return n;
}

/*! Return an indicator of the severity of the break.
 * 0 <= index < NumBreaks().
 * 
 * For instance, in between letters in a word there is almost no break potential,
 * hyphens have low break potential, spaces have a lot, but not as much
 * as line breaks.
 */
int StreamText::BreakInfo(int index)
{
	//assume we only have spaces, and that tabs and returns previously converted to explicit break chunks.
	return 1;
}


//------------------------------------- StreamImportData ----------------------------------

StreamImportData::~StreamImportData()
{
	if (importer) importer->dec_count();
	if (text_object) text_object->dec_count();
	if (file) file->dec_count();
}


//------------------------------------- Stream ----------------------------------

/*! \class Stream
 * \brief Hold a stream of things, usually text.
 * 
 * These are essentially heads of StreamElement + StreamChunk objects.
 */


Stream::Stream()
{
	modtime = 0;
}

Stream::~Stream()
{
	if (import_data) import_data->dec_count();
	//chunks are not explicitly deleted, since they exist somewhere in top tree
}

void Stream::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what,context);
	att.dump_out(f,indent);
}

/*! Starting with top, dump out the whole tree recursively with dump_out_recursive().
 *
 * <pre>
 *   streamdata
 *     style
 *       basedon laidout_default_style
 *       font-size     10
 *       line-spacing  110%
 *     chunks
 *     kids
 *       text "first text"
 *         style ...
 *         kids
 *           image /blah/blah.jpg
 *           text "Some text all up in here"
 *           dynamic context.page.label+"/"+string
 *           break line
 *           break column
 *           break section #or page
 *
 *   htmltext \\
 *     <span class="#laidout_default_style" style="font-size:10; line-spacing:110%;">first text<img file="/blah/blah.jpg"/>Some text all up in here</span>
 * </pre>
 */
Laxkit::Attribute *Stream::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!chunk_start) return nullptr;

	if (!att) att = new Attribute;

	Attribute *att2 = att->pushSubAtt("streamdata");
	dump_out_recursive(att2, &top, context);

	if (import_data) {
		if (import_data->file) att->push("file", import_data->file->filename);
		if (import_data->text_object) att->push("text_object", import_data->text_object->Id()); //assume the text object is a resource??
		if (import_data->importer) att->push("importer", import_data->importer->whattype());
	}

	return att;
}


void Stream::dump_out_recursive(Laxkit::Attribute *att, StreamElement *element, Laxkit::DumpContext *context)
{
	if (element->style) {
		if (!element->style_is_temp) {
			Utf8String str("resource:");
			str.Append(element->style->Id());
			att->push("style",str.c_str()); //is a style recorded somewhere

		} else {
			Attribute *att2 = att->pushSubAtt("style");
			element->style->dump_out_atts(att2, 0, context);
		}
	}

	if (element->kids.n) {
		Attribute *att2 = att->pushSubAtt("kids");
		for (int c=0; c < element->kids.n; c++) {
			dump_out_recursive(att2, element->kids.e[c], context);
		}
	}

	if (element->chunks.n) {
		Attribute *att2 = att->pushSubAtt("chunks");
		for (int c=0; c < element->chunks.n; c++) {
			Attribute *att3 = att2->pushSubAtt("chunk", element->chunks.e[c]->whattype());
			element->chunks.e[c]->dump_out_atts(att3, 0, context);
		}
	}
}



StreamChunk *NewStreamChunk(const char *type, StreamElement *parent)
{
	if (!strcmp(type, "text"))  return new StreamText(parent);
	if (!strcmp(type, "image")) return new StreamImage(parent);
	if (!strcmp(type, "break")) return new StreamBreak((int)StreamBreakTypes::BREAK_Unknown, parent);
	return nullptr;
}

/*! Return true for success, or false for some error.
 */
bool StreamElement::dump_in_att_stream(Laxkit::Attribute *att, Laxkit::DumpContext *context)
{
	const char *name, *value;

	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name,"style")) {
			if (!isblank(value)) {
				if (strstr(value, "resource:") == value) {
					//value += 9;
					// *** find resource!!
				}

			} else {
				if (!style) style = new Style();
				style->dump_in_atts(att->attributes.e[c], 0, context);
			}

		} else if (!strcmp(name,"kids")) {
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c++) {
				name  = att->attributes.e[c]->attributes.e[c2]->name;
				value = att->attributes.e[c]->attributes.e[c2]->value;

				StreamElement *el = new StreamElement(this, nullptr);
				kids.push(el);

				el->dump_in_att_stream(att->attributes.e[c]->attributes.e[c2], context);
			}
			
		} else if (!strcmp(name,"chunks")) {
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c++) {
				name  = att->attributes.e[c]->attributes.e[c2]->name;
				//value = att->attributes.e[c]->attributes.e[c2]->value;

				StreamChunk *chunk = NewStreamChunk(name, this);
				if (!chunk) {
					if (context->log) context->log->AddError(0,0,0, _("Unknown chunk type %s"), name);
					return false;
				}
				chunks.push(chunk);

				chunk->dump_in_atts(att->attributes.e[c]->attributes.e[c2], 0, context);
			}
		}
	}

	return true;
}

void Stream::dump_in_atts(Attribute *att,int flag, Laxkit::DumpContext *context)
{
	const char *name, *value;

	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name,"streamdata")) {
			 //default Laidout format.... maybe should just use htmltext for simplicity??
			top.dump_in_att_stream(att->attributes.e[c], context);

		} else if (!strcmp(name,"htmltext")) {
			ImportXML(value, -1, context->log);

		} else if (!strcmp(name,"text")) {
			ImportText(value,-1, nullptr, true, context->log);

		} else if (!strcmp(name,"markdown")) {
			ImportMarkdown(value,-1, nullptr,true, context->log);

		} else if (!strcmp(name,"file")) {
			 //Try to read in a file, which can be formatted any which way.
			 //Scan for hints about:
			 //  format, such as txt, html, etc
			 //  importer, from list of know StreamImporter objects
			 //  encoding, such as latin-1, utf8, etc
			 //
			 const char *file     = value;
			 const char *format   = nullptr;
			 const char *importer = nullptr;
			 const char *encoding = nullptr;

			 for (int c2 = 0; c2 < att->attributes.e[c]->attributes.n; c2++) {
				 name  = att->attributes.e[c]->attributes.e[c2]->name;
				 value = att->attributes.e[c]->attributes.e[c2]->value;

				 if (!strcmp(name, "format"))
					 format = value;
				 else if (!strcmp(name, "importer"))
					 importer = value;
				 else if (!strcmp(name, "encoding"))
					 encoding = value;
			}
			if (format && !strcasecmp(format,"txt")) format="text";

			DBG cerr << "Import text: "<<(format?format:"null")<<" "<<(importer?importer:"null")<<" "<<(encoding?encoding:"null")<<endl;

			int status = -2;

			if (importer) {
				 //find and try importer on file with format
				StreamImporter *importer_obj = nullptr; //FIXME *** laidout->GetStreamImporter(importer, format);
				if (importer_obj) status = importer_obj->In(this, file, context, nullptr,0);
				else {
					status = 1;
					if (context->log) context->log->AddWarning(0,0,0,_("Unknown importer %s, ignoring file %s"), importer, file);
				}

			} else { //importer not found
				 //try to guess from extension
				if (!format) {
					const char *ext = lax_extension(file);

					if (!ext)
						format = "text";
					else if (!strncasecmp(ext, "html", 4))
						format = "html";
					else if (!strncasecmp(ext, "md", 2))
						format = "markdown";
				}

				if (!format) format = "text";

				int len = 0;
				char *contents = read_in_whole_file(file, &len);
				if (!contents) {
					if (context->log) context->log->AddWarning(0,0,0, _("Cannot load stream file %s!"), file);

				} else {
					 //no importer found, try some defaults based on format
					if (!strcmp(format, "html")) {
						ImportXML(contents, len, context->log);

					} else if (!strcmp(format, "markdown")) {
						ImportMarkdown(contents, len, nullptr,true, context->log);

					} else { //text
						char *str = read_in_whole_file(file, &len);
						if (!isblank(str)) {
							ImportText(contents, len, nullptr,true, context->log);
						}
						delete[] str;
					}

					delete[] contents;
				}
			}

			if (status != 0) {
				if (context->log) context->log->AddWarning("Stream import failed or something");
			}
		}
	}

	tms tms_;
	modtime = times(&tms_);
}


int Stream::ImportMarkdown(const char *text, int n, StreamChunk *addto, bool after, Laxkit::ErrorLog *log)
{
	cerr << " *** must implement a markdown importer! defaulting to text"<<endl;

	// MarkdownImporter *importer = laidout->GetStreamImporter("markdown", nullptr);
	// importer->ImportTo(text, n, addto, after);

	return ImportText(text,n,addto,after, log);
}

void Stream::EstablishDefaultStyle()
{
	cerr << __FILE__<<" "<<__LINE__<<" IMPME!"<<endl;
}

/*! afterthis MUST be a chunk of *this or nullptr. No verification is done.
 * If nullptr, then add to end of chunks.
 *
 * Return 0 for success or nonzero for error.
 */
int Stream::ImportText(const char *text, int n, StreamChunk *addto, bool after, Laxkit::ErrorLog *log)
{
	if (!text) return 0;
	if (n < 0) n = strlen(text);
	if (n == 0) return 0;
	
	StreamText *txt = new StreamText(text,n, nullptr);

	if (addto) {
		if (after) addto->AddAfter(txt);
		else {
			addto->AddBefore(txt);
			//FIXME *** 
			//if (addto == chunks) chunks = addto; //reposition head
		}
	} else {
		if (!chunk_start) {
			 //blank stream! install first chunk...
			if (!top.style) EstablishDefaultStyle();
		   	chunk_start = txt;
			chunk_start->parent = &top;

	   	} else {
			 //append or prepend to addto
			if (!addto) {
				 //prepend to beginning of all, or append to end of all
				addto = chunk_start;
				if (after) while (addto->next) addto = addto->next;
			}
			if (after) {
				addto->AddAfter(txt); //AddAfter() and AddBefore() install a style
			} else {
				addto->AddBefore(txt);
				if (addto == chunk_start) chunk_start = addto; //reposition head
			}
		}
	}

	return 0;
}


/*! 
 * See https://www.w3.org/TR/css-fonts-3/#font-face-rule.
 *
 * This will be something akin to:
 * 
 *     @font-face {
 *       font-family: Oogabooga;
 *       src: "http://somewhere.fonts/oogabooga.woff";
 *     }
 *
 * Return nullptr on could not process.
 */
Style *ProcessCSSFontFace(Style *style, Attribute *att, Laxkit::ErrorLog *log)
{
	// CSS spec says font-family and src must be present. Otherwise ignore.
	// Given font-family overrides any name given in the actual font.

	if (!att) return style;

	const char *name;
	const char *value;
	int err = 0;

	try {
		for (int c=0; c<att->attributes.n; c++) {
			name  = att->attributes.e[c]->name;
			value = att->attributes.e[c]->value;

			if (!strcmp(name, "font-family")) {
				style->push("font-family", value);

			} else if (!strcmp(name, "src")) {
				//a comma separated list, go with first found:
				//
				// src: local("Some Name"),
				//	   local(SomeOtherNameWithoutQuotes),
				//	   url(/absolute/path),
				//	   url(rel/to/css/file),
				//	   url(file.svg#fontIdInFile)
				//	   url(fontcollection.woff2#fontId) format('woff2')
				//	   
				//format is optional, and string can be: woff, woff2, truetype, opentype, embedded-opentype, or svg

				const char *what = nullptr;
				int flen, slen;
				while (*value) {
					while (isspace(*value)) value++;
					const char *endptr = value;

					//int ParseSimpleFofS(const char *value, int *f_len_ret, const char **s_ret, int *s_len_ret, const char **end_ret)
					if (!strncmp(value, "local(", 6)) {
						if (ParseSimpleFofS(value, &flen, &what, &slen, &endptr)) {
							style->push("src-local", what,slen);
						} else {
							throw(_("Bad css"));
						};

					} else if (!strncmp(value, "url(", 4)) {
						if (ParseSimpleFofS(value, &flen, &what, &slen, &endptr)) {
							style->push("src", what,slen);
						} else {
							throw(_("Bad css"));
						};
					}
					value = endptr;

					while (isspace(*value)) value++;
					if (*value != ',') break;
					value++;
				}

			} else if (!strcmp(name, "unicode-range")) {
				//something like: unicode-range: U+0460-052F, U+1C80-1C88, U+20B4, U+2DE0-2DFF, U+A640-A69F, U+FE2E-FE2F;

				//single codepoint (e.g. U+416)
				//	a Unicode codepoint, represented as one to six hexadecimal digits
				//interval range (e.g. U+400-4ff)
				//	represented as two hyphen-separated Unicode codepoints indicating the inclusive start and end codepoints of a range
				//wildcard range (e.g. U+4??)
				//	defined by the set of codepoints implied when trailing ‘?’ characters signify any hexadeximal digit

				// ***  punting for now:
				style->push("unicode-range", value);

			} else if (!strcmp(name, "font-feature-settings")) {
				// ***  punting for now:
				style->push("font-feature-settings", value);

			} else if (!strcmp(name, "font-style")) {
				//Name: 	font-style
				//Value: 	normal | italic | oblique
				//Initial: 	normal

				const char *endptr = nullptr;
				int italic = CSSFontStyle(value, &endptr);
				if (italic >= 0) {
					style->push("italic", italic);
				} else {
					if (log) log->AddError(0,0,0, _("Bad value \"%s\" for font-style"), value);
					err = 1;
					break;
				}

			} else if (!strcmp(name, "font-weight")) {
				//like usual font-weight, but no relative tags "bolder" or "lighter"
				//font-weight: normal | bold | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit

				const char *endptr = nullptr;
				int weight = CSSFontWeight(value, &endptr, nullptr);
				value = endptr;
				
				if (weight >= 0) {
					style->push("font-weight", weight);
				} else {
					throw _("Bad font-weight");
				}

			} else if (!strcmp(name, "font-stretch")) {
				//Name: 	font-stretch
				//Value: 	normal | ultra-condensed | extra-condensed | condensed | semi-condensed | semi-expanded | expanded | extra-expanded | ultra-expanded
				//Initial: 	normal

                // Only works if font family has width-variant faces.
				//stretch refers to physically stretching the letters
				//CSS4 allows percentages >= 0.
				style->push(name, value); //always just a string
			}
		}

	} catch (const char *str) {
		if (log) log->AddError(str);
		return nullptr;
	}

	if (err != 0) return nullptr;

	return style;
}


class CSSParseCache
{
  public:
  	Laxkit::RefPtrStack<LaxFont> fonts;

};

/*! Convert a css block with selectors into a more accessible Attribute object.
 */
Attribute *CSSBlockToAttribute(Attribute *att, const char *cssvalue, CSSParseCache *parse_cache)
{
	if (!att) att = new Attribute();


	return att;
}

class CSSSelector
{
  public:
    char *name = nullptr;
	char *pseudo_class = nullptr;
	char *pseudo_element = nullptr;

	// any descended, ie "E F"
	// direct child, ie "E > F"
	char qualifier = 0; // relative to previous selector in stack, can be ' ' or '>', or nul.
	char type = 0;

	CSSSelector *next = nullptr;

    CSSSelector() {}
	~CSSSelector()
	{
		delete[] name;
		delete[] pseudo_class;
		delete[] pseudo_element;
		if (next) delete next;
	}

	bool IsType()  { return type != '.' && type != '#'; }
	bool IsClass() { return type == '.'; }
	bool IsID()    { return type == '>'; }
};

/*! Read in alphanum, return pointer to just after final character. */
const char *ParseName(const char *start, int *n_ret, const char *extra)
{
	*n_ret = 0;
	const char *pp = start;
	while (isalnum(*pp) || (extra && strchr(extra, *pp))) pp++;
	*n_ret = pp-start;
	return pp;
}

CSSSelector *ParseCSSSelector(const char *cssvalue, const char **endptr)
{
	const char *ptr = cssvalue;
	int n;

	CSSSelector *selector = new CSSSelector();
	CSSSelector *curselector = selector;

	while (*ptr && *ptr != '{') {
		while (isspace(*ptr)) ptr++;

		curselector->type = '\0';
		if (*ptr == '.' || *ptr == '#') {
			curselector->type = *ptr;
			ptr++;
		}

		// parse selector name
		if (isalnum(*ptr)) {
			const char *pp = ParseName(ptr, &n, "-");
			if (n) makenstr(curselector->name, ptr, n);
			ptr = pp;
		}

		if (*ptr == '[') {
			// match attribute
			// ***
			DBGW("skipping name[].. implement me!!");
			while (*ptr && *ptr != ']') ptr++; //*** just skip for now
		}

		if (*ptr == ':' && ptr[1] != ':') {
			// pseudo-class
			ptr++;
			while (isspace(*ptr)) ptr++;
			const char *pp = ParseName(ptr, &n, "-");
			if (n) makenstr(curselector->pseudo_class, ptr, n);
			ptr = pp;
		}

		if (*ptr == ':' && ptr[1] == ':') {
			// pseudo-element
			ptr += 2;
			while (isspace(*ptr)) ptr++;
			const char *pp = ParseName(ptr, &n, "-");
			if (n) makenstr(curselector->pseudo_element, ptr, n);
			ptr = pp;
		}

		if (*ptr != ',') break;

		ptr++; // advance past comma
		while (isspace(*ptr)) ptr++;
		if (isalnum(*ptr)) {
			curselector->next = new CSSSelector();
			curselector = curselector->next;
			curselector->qualifier = ' ';

		} else if (*ptr == '>') {
			while (isspace(*ptr)) ptr++;
			if (isalnum(*ptr)) {
				curselector->next = new CSSSelector();
				curselector = curselector->next;
				curselector->qualifier = '>';
			} else break; //seems to be malformed?

		} else break; //seems to be malformed?
	}

	return selector;
}


LaxFont *MatchCSSFont(const char *family_list, int italic, const char *variant, int weight, CSSParseCache *cache)
{
	return nullptr;
	// ***
	// // font_family is the css attribute:
	// //  font-family: Bookman, serif, ..... <-- use first one found
	// //    or generic:  serif  sans-serif  cursive  fantasy  monospace
	// Laxkit::LaxFont *font = nullptr;
	// const char *v;
	// while (value && *value) {
	// 	while (isspace(*value)) value++;
	// 	v = value;
	// 	while (*v && *v!=',' && !isspace(*v)) v++;
	// 	fontlist.push(newnstr(value,v-value+1));
	// 	font = MatchFont(value, v-value);
	// 	if (font) break;

	// 	while (isspace(*v)) v++;
	// 	if (*v == ',') v++;
	// 	value = v;
	// }

	// return font;
}

//forward declaration:
Style *ProcessCSSBlock(Style *existing_style, const char *cssvalue, const char **error_pos_ret, Laxkit::ErrorLog &log);


/*! Convert direct attributes of att to parts of current->style.
 * Look for "style", "class", "id".
 */
StreamElement *ParseCommonStyle(Laxkit::Attribute *att, StreamElement *current, Laxkit::ErrorLog *log)
{
	const char *name;
	const char *value;
	Style *style = new Style(); //todo: how is this even supposed to work

	 //Parse current element attributes: style class id dir="ltr|rtl|auto" title
	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name,"style")) {
			 //update newstyle with style found
			const char *endptr = nullptr;
			Style *style_ret = ProcessCSSBlock(style, value, &endptr, *log);
			if (!style_ret) {
				cerr << "error or something"<<endl;
			}

		} if (!strcmp(name,"class")) {
			 //*** update newstyle with style found
			cerr << " *** need to actually implement class import! lazy programmer!"<<endl;
			
			if (!isblank(value)) {
				if (!current) current = new StreamElement();
				current->style->set("class", new StringValue(value), true);
			}

		} if (!strcmp(name,"id")) {
			//if (!isblank(value)) current->Id(value);
			if (!isblank(value)) current->style->push("id", value);

		//} if (!strcmp(name,"title")) {
		//	makestr(title,value);

		} if (!strcmp(name,"dir")) {
			 // flow direction: html has "ltr" "rtl" "auto"
			 // 				-> extend with all 8 lrtb, lrbt, tblr, etc?

			int bidi = ParseFlowDirection(value,-1);

			if (!current) current = new StreamElement();
			if (bidi >= 0) current->style->set("bidi", new IntValue(bidi), true);

		} else {
			DBG cerr << "warning: ParseCommonStyle attribute "<<name<<" not handled"<<endl;
		}
	}

	return current;
}



/*! Returns style on success, or nullptr on error.
 * If `style == null`, then create a new Style object.
  */
Style *ProcessCSSBlock(Style *existing_style, const char *cssvalue, const char **error_pos_ret, Laxkit::ErrorLog &log)
{
	//CSS:
	// selectors:    selector[, selector] { ... }
	//    E     -> any E
	//    E F   -> any F that is descended from an E
	//    E F G -> any G descended from an F, which must be descended from E
	//    E > F -> any F that is a direct child of E
	//    E:first-child -> E when E is first child of parent
	//    E:link E:visited E:active E:hover E:focus   <- psuedo-class, must be before psuedo-element
	//    E:lang(c)  -> E when language is c
	//    E+F   -> any F occuring right after E
	//    E[foo] -> any with foo attribute
	//    E[foo="bar"] attribute equal to bar
	//    E[foo~="bar"] bar in foo attribute's list
	//    .class
	//    #id 
	//    p::first-line   <- psuedo-element
	//    p::first-letter
	//    p::before { content: "Blah"; }
	//     content: normal | none | [ <string> | <uri> | <counter> | attr(<identifier>) | open-quote | close-quote | no-open-quote | no-close-quote ]+ | inherit
	//    p::after
	//
	//  font-family: Bookman, serif, ..... <-- use first one found
	//    generic:
	//      serif  sans-serif  cursive  fantasy  monospace
	//  font-variant: 	normal | small-caps | inherit
	//  font-style:  normal | italic | oblique | inherit 
	//  font-weight: normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
	//       normal==400, bold==700
	//  font-size: 	<absolute-size> | <relative-size> | <length> | <percentage> | inherit
	//     absolute-size == xx-small | x-small | small | medium | large | x-large | xx-large 
	//     relative-size == larger | smaller
	//  font: 	[ [ <'font-style'> || <'font-variant'> || <'font-weight'> ]? <'font-size'> [ / <'line-height'> ]? <'font-family'> ] | caption | icon | menu | message-box | small-caption | status-bar | inherit
	//  text-indent: 5%  <-  % of containing block, or abs
	//  text-align: left | right | center | justify | justify-all | start | end | match-parent inherit | initial | unset    //start and end are relative to text direction
	//               //char-based alignment in a table column
	//              "." | "." center
	//  text-decoration:  	none | [ underline || overline || line-through || blink ] | inherit
	//  letter-spacing:  normal | <length> | inherit
	//  word-spacing:  normal | <length> | inherit  <- space in addition to default space
	//  text-transform:  capitalize | uppercase | lowercase | none | inherit
	//  white-space:  normal | pre | nowrap | pre-wrap | pre-line | inherit 
	//
	//  line-height: normal | <number> | <length> | <percentage> | none
	//
	//
	//lengths: ex = x-height, height of lowercase letters
	//         em = basically font size
	//   abs: in cm mm 
	//        pt = 1/72 inch
	//        pc = 1/6 inch, 12 pts
	//        px = .75 pt
	//  percents: 50%, but % items might refer to diff items, ie line-height: 120% means 120% of font-size
	//
	//  blah: url(http://...)
	//  blah: url(relative_url_to_this)
	//
	//colors:
	//  keywords:
	//		maroon #800000      red #ff0000   orange #ffA500  yellow #ffff00    olive #808000
	//		purple #800080  fuchsia #ff00ff    white #ffffff    lime #00ff00    green #008000
	//		  navy #000080     blue #0000ff     aqua #00ffff    teal #008080
	//  system colors: see http://www.w3.org/TR/CSS2/ui.html#system-colors
	//  color: #aabbcc  ==  color: #abc
	//  color: rgb(50%, 50%, 50%)
	//  color: rgb(255, 255, 255)  <- [0..255]
	//
	//boxes:  margin-border-padding-content
	//  margin-left  margin-right  margin-top  margin-bottom  margin: t r b l
	//   -> margins collapse with adjacent margins
	//  padding-left  padding-right  padding-top  padding-bottom  padding: t r b l
	//  border-left-width  border-right-width  border-top-width  border-bottom-width  border-width: t r b l
	//  border-left-color  padding-right-color  padding-top-color  padding-bottom-color  padding-color: t r b l,  <color> or transparent
	//  border-left-style  padding-right-style  padding-top-style  padding-bottom-style  padding-style: t r b l
	//   -> none hidden dotted dashed solid double groove ridge inset outset
	//  border-left  border-top  border-right  border-bottom  border: 	[ <border-width> || <border-style> || <'border-top-color'> ] | inherit
	//
	// list-style-type: disc | circle | square | decimal | decimal-leading-zero | lower-roman | upper-roman | lower-greek | lower-latin | upper-latin | armenian | georgian | lower-alpha | upper-alpha | none | inherit
	// list-style-image: 	<uri> | none | inherit
	// list-style-position:  	inside | outside | inherit
	// list-style:  	[ <'list-style-type'> || <'list-style-position'> || <'list-style-image'> ] | inherit
	//
	//default table styles for html:
	//  table    { display: table }
	//  tr       { display: table-row }
	//  thead    { display: table-header-group }
	//  tbody    { display: table-row-group }
	//  tfoot    { display: table-footer-group }
	//  col      { display: table-column }
	//  colgroup { display: table-column-group }
	//  td, th   { display: table-cell }
	//  caption  { display: table-caption }
	// 

	Style *style = existing_style;
	if (!existing_style) style = new Style;

	Attribute att;
	CSSParseCache parse_cache;
	CSSBlockToAttribute(&att, cssvalue, &parse_cache);

	const char *variant = nullptr;
	const char *familylist = nullptr;
	int weight = 400;
	int italic = -1;

	const char *name;
	const char *value;
	int err = 0;

	for (int c=0; c<att.attributes.n; c++) {
		name  = att.attributes.e[c]->name;
		value = att.attributes.e[c]->value;

		if (!strcmp(name, "font-family")) {
			//  font-family: Bookman, serif, ..... <-- use first one found
			//    or generic:  serif  sans-serif  cursive  fantasy  monospace
			familylist = value;			

		} else if (!strcmp(name, "font-variant")) {
			 //normal | small-caps | inherit
			 if (!strcmp(value,"small-caps")) variant=value;

		} else if (!strcmp(name, "font-style")) {
			 //normal | italic | oblique | inherit
			if (!strcmp(value,"inherit")) ;
			else if (!strcmp(value,"normal"))  italic=0; 
			else if (!strcmp(value,"italic"))  italic=1;
			else if (!strcmp(value,"oblique")) italic=1; //technically oblique is distorted normal, italic is actual new glyphs

			if (italic > 0) style->push("font-style", italic);
			 
		} else if (!strcmp(name, "font-weight")) {
			//normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
			const char *endptr;
			weight = CSSFontWeight(value, &endptr, nullptr); //-1 is inherit
			value = endptr;

			if (weight > 0) style->push("font-weight", weight);
			else {
				log.AddError(_("Bad font-weight value"));
				err = 1;
				break;
			}
			//TODO: if (weight > 0 && is_relative) style->pushInteger("font-weight-offset", w);

		} else if (!strcmp(name, "font-size")) {
			 //  font-size: 	<absolute-size> | <relative-size> | <length> | <percentage> | inherit
			 //     absolute-size == xx-small | x-small | small | medium | large | x-large | xx-large 
			 //     relative-size == larger | smaller
			double v=-1;
			CSSName type = CSSName::CSS_Physical;
			Unit units;
			//CSSName relative = CSS_Unknown;
			const char *endptr = nullptr;

			if (!CSSFontSize(value, &v, &type, &units, &endptr)) {
				// error parsing!
				log.AddError(_("Error parsing font-size"));
				err = 1;
				break;
			}

			if (v >= 0) style->push("font-size", new LengthValue(v, units, type), true);

		} else if (!strcmp(name, "font")) {
			//  font: 	[ [ <'font-style'> || <'font-variant'> || <'font-weight'> ]? <'font-size'> [ / <'line-height'> ]? <'font-family'> ] | caption | icon | menu | message-box | small-caption | status-bar | inherit
			//*** 

			if (!strcmp(value,"inherit")) {
				//do nothing
			} else if (!strcmp(value,"caption")) {
			} else if (!strcmp(value,"icon")) {
			} else if (!strcmp(value,"menu")) {
			} else if (!strcmp(value,"message-box")) {
			} else if (!strcmp(value,"small-caption")) {
			} else if (!strcmp(value,"status-bar")) {
			} else {
				// look for:  font-style || font-variant || font-weight

				const char *endptr = endptr;
				int font_style = CSSFontStyle(value, &endptr);
				int font_variant = -1;
				int font_weight = -1;

				if (endptr == value) {
					font_variant = CSSFontVariant(value, &endptr);
					if (endptr == value) {
						font_weight = CSSFontWeight(value, &endptr, nullptr);
						if (font_weight >= 0) {
							style->push("font-weight", new IntValue(font_weight), true);
						}
					} else {
						value = endptr+1;
						style->push("font-variant", new IntValue(font_variant), true);
					}
				} else {
					value = endptr+1;
					style->push("font-style", new IntValue(font_style), true);
				}

				
				double v = -1;
				Unit units;
				CSSName relative = CSS_Unknown;
				
				if (!CSSFontSize(value, &v,&relative,&units, &endptr) || v < 0) {
					log.AddError(_("Error parsing font-size for font"));
					err = 1;
					break;
				} else {
					if (v >= 0) style->push("font-size", new LengthValue(v,units,relative), true);
				}
			}

		} else if (!strcmp(name, "color")) {
			double colors[5];
			const char *endptr = nullptr;
			if (SimpleColorAttribute(value, colors, &endptr) == 0) {
				style->set("color", new ColorValue(colors[0], colors[1], colors[2], colors[3]));
			} else {
				log.AddError(_("Error parsing color"));
				err = 1;
				break;
			}

		} else if (!strcmp(name, "line-height")) {
			 //line-height: normal | <number>  | <length> | <percentage> | none 
			 //                      <number> == (the number)*(font-size) 
			 //                      percent is of font-size   
			 //                      none == font-size
			if (!strcmp(value,"none")) {
				//should be 100% of font-size
			} else {
				LengthValue *v = LengthValue::Parse(value,-1, nullptr);
				if (v) {
					style->set("line-height",v,1);
				} else {
					log.AddWarning(0,0,0, _("bad css attribute %s: %s"), "line-height", value?value:"");
				}
			}

		//} else if (!strcmp(name, "text-indent")) {
		//} else if (!strcmp(name, "text-align")) {
		//} else if (!strcmp(name, "text-decoration")) {  //<text-decoration-line> || <text-decoration-style> || <text-decoration-color>
		//      text-decoration-line:     none | [ underline || overline || line-through || blink ]
		//		text-underline-position:  auto | [ under || [ left | right ] ] 
		//		text-decoration-skip:     none | [ objects || spaces || ink || edges || box-decoration ]
		//      text-decoration-style:    solid | double | dotted | dashed | wavy 
		//      text-decoration-color:    <color>
		//
		//} else if (!strcmp(name, "letter-spacing ")) {   //  letter-spacing:  normal | <length> | inherit
		//} else if (!strcmp(name, "word-spacing")) {      //  word-spacing:  normal | <length> | inherit  <- space in addition to default space
		//} else if (!strcmp(name, "text-transform")) {  text-transform:  capitalize | uppercase | lowercase | none | inherit
		//} else if (!strcmp(name, "white-space")) {  white-space:  normal | pre | nowrap | pre-wrap | pre-line | inherit 

		//boxes:  margin-border-padding-content
		//  margin-left  margin-right  margin-top  margin-bottom  margin: t r b l
		//   -> margins collapse with adjacent margins
		//  padding-left  padding-right  padding-top  padding-bottom  padding: t r b l
		//  border-left-width  border-right-width  border-top-width  border-bottom-width  border-width: t r b l
		//  border-left-color  padding-right-color  padding-top-color  padding-bottom-color  padding-color: t r b l,  <color> or transparent
		//  border-left-style  padding-right-style  padding-top-style  padding-bottom-style  padding-style: t r b l
		//   -> none hidden dotted dashed solid double groove ridge inset outset
		//  border-left  border-top  border-right  border-bottom  border: 	[ <border-width> || <border-style> || <'border-top-color'> ] | inherit
		
		// list-style-type: disc | circle | square | decimal | decimal-leading-zero | lower-roman | upper-roman | lower-greek | lower-latin | upper-latin | armenian | georgian | lower-alpha | upper-alpha | none | inherit
		// list-style-image: 	<uri> | none | inherit
		// list-style-position:  	inside | outside | inherit
		// list-style:  	[ <'list-style-type'> || <'list-style-position'> || <'list-style-image'> ] | inherit
		//
		//default table styles for html:
		//  table    { display: table }
		//  tr       { display: table-row }
		//  thead    { display: table-header-group }
		//  tbody    { display: table-row-group }
		//  tfoot    { display: table-footer-group }
		//  col      { display: table-column }
		//  colgroup { display: table-column-group }
		//  td, th   { display: table-cell }
		//  caption  { display: table-caption }


		} else {
			DBG cerr << "Unimplemented attribute "<<name<<"!!"<<endl;
		}
	}

	if (familylist) {
		LaxFont *font = MatchCSSFont(familylist, italic, variant, weight, &parse_cache);
		if (font) {
			FontValue *fontv = new FontValue(font, true);
			style->push("font",fontv);
		}
	}

	if (err != 0) {
		if (!existing_style) delete style;
		return nullptr;
	}

	return style;
}


/*! Read in the file to a string, then call ImportXML(). Updates this->file.
 */
int Stream::ImportXMLFile(const char *nfile, Laxkit::ErrorLog *log)
{
	//makestr(file,nfile);
	Laxkit::Attribute att;
	XMLFileToAttribute(&att,nfile,nullptr);
	StreamChunk *last_chunk = nullptr;
	StreamElement *next_style = nullptr;
	return ImportXMLAtt(&att, last_chunk, next_style, nullptr, log);
}

/*! From string value, assume it is xml formatted text, and parse into StreamChunk objects, completely
 * replacing old stream chucks.
 *
 * Supported tags:
 * ```
 *    <b>bold</b> <i>italic</i>
 *    <br> <tab>
 *    <br section>
 *    <br page>
 *    <br column>
 *    <br tab>
 *    <img width="100" height="100" alt=".." src=".."   style class id ... />
 *    <hr>
 *    <div>stuff</div>
 *    <span>stuff</span>
 *    <code>stuff</code>
 *    <pre>stuff</pre>
 *    <ol> <li/> </ol>
 *    <ul> <li/> </ul>
 * ```
 */
int Stream::ImportXML(const char *value, int len, Laxkit::ErrorLog *log)
{
	long pos = 0;
	Laxkit::Attribute att;
	if (len < 0) len = strlen(value);
	XMLChunkToAttribute(&att, value, len, &pos, nullptr, nullptr);

	StreamChunk *last_chunk = nullptr;
	StreamElement *next_style = nullptr;
	return ImportXMLAtt(&att, last_chunk, next_style, nullptr, log);
}


/*! Import att as chunks under last_style_el, adding sub-StreamElement objects as needed.
 * last_chunk must be either null or a chunk within last_style_el.
 *
 * Return 0 for success, or nonzero for some kind of error.
 */
int Stream::ImportXMLAtt(Attribute *att, StreamChunk *&last_chunk, StreamElement *&next_element, StreamElement *last_style_el, Laxkit::ErrorLog *log)
{
	const char *name;
	const char *value;
	
	//char *title = nullptr;

	StreamChunk *chunk = nullptr;
	StreamElement *cur_style = ParseCommonStyle(att, nullptr, log);

	if (cur_style) {
		if (last_style_el) {
			// we need to install a subelement, since cur_style has additional styling.
			// if last_chunk is not at edge of last_style_el, we'll need to split it
		}
	}

	Attribute *elements = att->find("content:");
	if (!elements) return 0;

	for (int c=0; c<elements->attributes.n; c++) {
		name  = elements->attributes.e[c]->name;
		value = elements->attributes.e[c]->value;

		//br tab wbr img hr cdata:
		
		//
		//first process elements net expected to have kids ... currently: br wbr hr img tab "cdata:"
		//
		if (!strcmp(name,"br") || !strcmp(name,"tab")) {
			 //extended to have: <br section> <br chapter> <br column> <br page>
			 //also my tag: <tab>
	
			int type = (int)StreamBreakTypes::BREAK_Paragraph;

			if (!strcmp(name,"tab")) type = (int)StreamBreakTypes::BREAK_Tab;
			else {
				for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
					name =att->attributes.e[c]->attributes.e[c2]->name;
					value=att->attributes.e[c]->attributes.e[c2]->value;

					if      (!strcmp(name,"section")) type = (int)StreamBreakTypes::BREAK_Section;
					else if (!strcmp(name,"page"))    type = (int)StreamBreakTypes::BREAK_Page;
					else if (!strcmp(name,"column"))  type = (int)StreamBreakTypes::BREAK_Column;
					else if (!strcmp(name,"tab"))     type = (int)StreamBreakTypes::BREAK_Tab;
					else type = (int)StreamBreakTypes::BREAK_Unknown;
				}
			}
			 
			if (type != (int)StreamBreakTypes::BREAK_Unknown) {
				StreamBreak *br = new StreamBreak(type, nullptr);
				if (!chunk) { last_chunk = chunk = br; }
				else chunk = chunk->AddAfter(br);

			} else {
				if (log) {
					setlocale(LC_ALL, "");
					log->AddWarning(0,0,0, _("Unknown break %s"), value);
					setlocale(LC_ALL, "C");
				}
			}
			continue;

		} else if (!strcmp(name,"wbr")) {
			//html: Word Break Opportunity
			//*** if occurs between cdata blocks, then insert as weak break point in a combined StreamText
			cerr << " *** NEED TO IMPLEMENT wbr in Stream::ImportXMLAtt"<<endl;

		} else if (!strcmp(name,"img")) {
			//<img width="100" height="100" alt=".." src=".."   style class id ... />
			
			//double w=-1, h=-1;
			LengthValue *w_value  = nullptr;
			LengthValue *h_value  = nullptr;
			const char *alt       = nullptr;
			const char *src       = nullptr;
			const char *img_title = nullptr;

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name  = att->attributes.e[c]->attributes.e[c2]->name;
				value = att->attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name,"width")) {
					w_value = LengthValue::Parse(value);
					// DoubleAttribute(value,&w);

				} else if (!strcmp(name,"height")) {
					h_value = LengthValue::Parse(value);
					// DoubleAttribute(value,&h);

				} else if (!strcmp(name,"title")) {
					img_title = value;

				} else if (!strcmp(name,"alt")) {
					alt = value;

				} else if (!strcmp(name,"src")) {
					src = value;
				}
			}

			LaxInterfaces::ImageData *img = dynamic_cast<LaxInterfaces::ImageData*>(LaxInterfaces::somedatafactory()->NewObject(LaxInterfaces::LAX_IMAGEDATA));
			img->LoadImage(src,nullptr, 0,0,0,0);
			if (alt) makestr(img->description, alt);
			if (img_title) makestr(img->title, img_title);

			// img->maxx is pixel width, so we want w_value * xaxis().norm() == maxx
			cerr << "FIXME! do proper aspect ratio for inline image"<<endl;
			if (w_value) {
				img->xaxis(w_value->value / img->maxx * img->xaxis());
			}
			if (h_value) {
				img->yaxis(h_value->value / img->maxy * img->yaxis());
			}
			//if (h <= 0) *** //make same size as font (ascent+descent)?
			//*** if box style attributes, make a group with path border around image

			StreamImage *image = new StreamImage(dynamic_cast<DrawableObject*>(img), nullptr);
			if (!chunk) { last_chunk = chunk = image; }
			else chunk = chunk->AddAfter(image);
			continue;

		} else if (!strcmp(name,"hr")) {
			 //<hr>  add a filled path rectangle?
			cerr <<" *** <hr> not implemented!! Rats!"<<endl;
			continue;

		} else if (!strcmp(name,"cdata:")) {
			// *** &quot; and stuff

			StreamText *txt = new StreamText(nullptr,0, nullptr);
			txt->SetFromEntities(value,-1); // <- needs to compress whitespace, and change &stuff; to chars
			//char *text=ConvertEntitiesToChars(value);
			//delete[] text;

			if (!chunk) { last_chunk = chunk = txt; }
			else chunk = chunk->AddAfter(txt);
			continue;


		//
		//now process those that can have sub elements...
		//

		} else if (!strcmp(name,"div") || !strcmp(name,"p")) {
			//int Stream::ImportXMLAtt(Attribute *att, StreamChunk *&last_chunk, StreamElement *&next_style, StreamElement *last_style_el, Laxkit::ErrorLog *log)
			ImportXMLAtt(elements->attributes.e[c], chunk, next_element, nullptr, log);

			 //add a line break after div if not right after one
			 //*** should one be added before too? if first div should not be one
			if (chunk->Type()!=CHUNK_Break) {
				StreamBreak *br = new StreamBreak((int)StreamBreakTypes::BREAK_Paragraph, nullptr);
				if (!chunk) { last_chunk = chunk = br; }
				else chunk = chunk->AddAfter(br);
			}

		} else if (!strcmp(name,"span")) {
			ImportXMLAtt(elements->attributes.e[c], chunk, next_element, nullptr, log);

		} else if (!strcmp(name,"strong") || !strcmp(name,"b")) {
			//bold
			//search for a bold equivalent of current font
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"em") || !strcmp(name,"i")) {
			//italic
			//search for an italic equivalent of current font
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"blockquote")) {
			//*** //set paragraph indent
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"code") || !strcmp(name, "tt")) {
			//*** //use a monospace font
				//if "code" then do like a div?
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"pre")) {
			// *** convert \n chars to <br> elements
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"font")) { //deprecated as of html4.1
			//<font size="-1">blah</font>
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;
			
		} else if (!strcmp(name,"a")) {
			// *** anchors.. use for html links, bookmark designations, citations?
			//<a name="..." href="..." class="..." id="..">...</a>
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"dl")) {
			//<dl> <dt> <dd>
			cerr << " *** need to implement dl lists!"<<endl;

		} else if (!strcmp(name,"ol")) {
			//<ol> <li> ...</li> </ol>
			cerr << " *** need to implement ol lists!"<<endl;

		} else if (!strcmp(name,"ul")) {
			//<ul> <li> ...</li> </ul>
			cerr << " *** need to implement ul lists!"<<endl;

		} else if (!strcmp(name,"table")) {
			//<table><tr><td colspan rowspan>
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (name[0]=='h' && name[1]>='1' && name[1]<='6' && name[2]=='\0') {
			//int heading = name[1]-'1'+1;
			//***
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"math")) {
			//*** //mathml stuff
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"texmath")) {
			//*** math markup
			//<math> (for mathml) or not html: <latex> <tex> $latex stuff$  $$latex eqns$$
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else {
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;
		}


		//if (newstyle) newstyle->dec_count();
	}

	return 0;
}


//------------------------------------- StreamAttachment ----------------------------------


/*! Create this StreamAttachment as being owned by nobject.
 *  nstream will be inc_counted.
 *  cache is set to null, it is not computed here.
 */
StreamAttachment::StreamAttachment(DrawableObject *nobject, Stream *nstream)
{
    owner = nobject; //assume *this is owned by nobject

    stream = nstream;
    if (stream) stream->inc_count();

    cache = nullptr;
}

StreamAttachment::~StreamAttachment()
{
	if (stream) stream->dec_count();
	if (cache)  cache ->dec_count();
}


//------------------------------------- StreamCache ----------------------------------


StreamCache::StreamCache(StreamChunk *ch, long noffset, long nlen)
{
	modtime = 0;
	next    = nullptr;
	chunk   = ch;
	offset  = noffset;
	len     = nlen;
	element = nullptr;

	if (ch) element = ch->parent;
}

StreamCache::StreamCache()
{
	modtime = 0;
	next    = nullptr;
	chunk   = nullptr;
	element = nullptr;
	offset  = len = 0;
}

StreamCache::~StreamCache()
{
	if (next) delete next;
}


//------------------------------- Stream mapping ------------------------------------------


/*! Return path object when changed, else nullptr if unchanged.
 * If modify_in_place, then modify the area object to be the result.
 */
LaxInterfaces::PathsData *PathBooleanSubtract(LaxInterfaces::PathsData *area, LaxInterfaces::PathsData *to_remove, bool modify_in_place)
{
	cerr << __FILE__<<':'<<__LINE__<<"  IMPLEMENT ME!!"<<endl;
	return nullptr;
}



/*! Update cache for stream laid into target->GetInsetPath(), as blocked by appropriate objects in
 * the same page. Note that only page objects affect wrapping. Paper and limbo objects do not
 * affect wrapping.
 *
 * Overwrites anything in cache chain, adding more nodes if necessary, and deleting unused ones.
 * or create and return a new one if cache == nullptr.
 */
StreamCache *RemapAreaStream(DrawableObject *target, StreamAttachment *attachment, int start_chunck, int chunk_offset)
{
	if (!attachment) return nullptr;
	
	Stream      *stream = attachment->stream;
	StreamCache *cache  = attachment->cache;

	if (cache->modtime > 0 && stream->modtime < cache->modtime) return cache; //probably set ok


	 //-------- compute possible breaks if necessary
	 //***


	 //----------compute layout area:
	 //  start with target inset path
	 //  We need to remove the wrap path of any lesser siblings and aunts/uncles from that path
	LaxInterfaces::PathsData *area = dynamic_cast<LaxInterfaces::PathsData*>(target->GetInsetPath()->duplicate(nullptr));

	DrawableObject *so = target;
	DrawableObject *o = target->GetDrawableParent();
	while (o) {
		for (int c = 0; c < o->n() && o->e(c) != so; c++) {
			DrawableObject *dro = dynamic_cast<DrawableObject*>(o->e(c));
			PathBooleanSubtract(area, dro->GetWrapPath(), true);
			//area->CutOut(dro->GetWrapPath());
		}
		so = o;
		o = o->GetDrawableParent();
	}


	 //--------------compute actual breaks within area
	//***
	


	return cache;
}


} // namespace Laidout

