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
// Copyright (C) 2025-present by Tom Lechner
//

#include <cctype>

#include "cssparse.h"
#include <lax/cssutils.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/language.h>

#include "../dataobjects/fontvalue.h"
#include "lengthvalue.h"

#include <lax/debug.h>


using namespace Laxkit;

namespace Laidout {
namespace CSS {


//------------------------------ non-css specific parsing utils -----------------------------

/*! Something with simple contents in parentheses, like:
 *  - local(blah)
 *  - local("Blah")
 *  - local('Blah')
 *  - url(https://thing)
 *
 * If there are no parantheses, then 0 is returned.
 *
 * f_len_ret is the length of the function, so "local(blah)" will have f_len_ret == 5.
 *
 * s_ret will point to the start of the parameter. So "local(blah)" will have s_ret point to the 'b' in blah.
 * A parameter in quotes like `local("blah")` will have s_ret still point to the 'b' in blah. ***TODO: why? better to preserve whole param??
 *
 * s_len_ret will be the number of characters of the parameter, not including quotes.
 *
 * end_ret will point to the character right after the closing ')'.
 * 
 * Return 1 for successful parse, else 0.
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


/*! Read in alphanum and/or any of the characters in extra if any.
 * Return pointer to just after final character. */
const char *ParseName(const char *start, int *n_ret, const char *extra)
{
	*n_ret = 0;
	const char *pp = start;
	while (isalnum(*pp) || (extra && strchr(extra, *pp))) pp++;
	*n_ret = pp-start;
	return pp;
}


//------------------------------ css specific parsing utils -----------------------------

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
 * Return nullptr on could not parse.
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



/*! Convert a css block with selectors into a more accessible Attribute object.
 */
Attribute *CSSBlockToAttribute(Attribute *att, const char *cssvalue, CSSParseCache *parse_cache)
{
	if (!att) att = new Attribute();


	return att;
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
Style *ProcessCSSBlock(Style *existing_style, const char *cssvalue, const char **error_pos_ret, Laxkit::ErrorLog &log, int *error_status);


/*! Convert direct attributes of att to parts of current->style.
 * Look for "style", "class", "id".
 *
 * On parse success, return true, and style_ret gets the parsed Style object. If current != nullptr, then style_ret will be current.
 * If there were no style elements and current == nullptr, then style_ret will be nullptr.
 *
 * On parse failure, return false, style_ret = nullptr.
 */
bool ParseCommonStyle(Laxkit::Attribute *att, Style *current, Laxkit::ErrorLog &log, Style **style_ret)
{
	const char *name;
	const char *value;
	Style *style = current;
	
	 //Parse current element attributes: style class id dir="ltr|rtl|auto" title
	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name,"style")) { // inline style definition
			 //update newstyle with style found
			const char *endptr = nullptr;
			int error_status = 0;
			Style *style_ret = ProcessCSSBlock(style, value, &endptr, log, &error_status);
			if (error_status) {
				*style_ret = nullptr;
				if (style && !current) style->dec_count();
				return false;
			}

		} if (!strcmp(name,"class")) {
			if (!isblank(value)) {
				if (!style) style = new Style();
				style->set("class", new StringValue(value), true);
			}

		} if (!strcmp(name,"id")) {
			if (!isblank(value)) {
				if (!style) style = new Style();
				style->push("id", value);
			}

		//} if (!strcmp(name,"title")) {
		//	makestr(title,value);

		} if (!strcmp(name,"dir")) { // *** this shouldn't be here as it's not "common"?
			 // flow direction: html has "ltr" "rtl" "auto"
			 // 				-> extend with all 8 lrtb, lrbt, tblr, etc?

			//int bidi = ParseFlowDirection(value,-1);
			int bidi = Laxkit::flow_id(value);

			if (!style) style = new Style();
			if (bidi >= 0) style->set("bidi", new IntValue(bidi), true);

		} else {
			DBGW("warning: ParseCommonStyle attribute "<<name<<" not handled");
		}
	}

	*style_ret = style;
	return true;
}



/*! Returns existing_style on success, or nullptr on error.
 * If `style == null`, then create a new Style object.
  */
Style *ProcessCSSBlock(Style *existing_style, const char *cssvalue, const char **error_pos_ret, Laxkit::ErrorLog &log, int *error_status)
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
			DBGW("Unimplemented attribute "<<name<<"!!");
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

} // namespace CSS
} // namespace Laidout
