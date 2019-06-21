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
// Copyright (C) 2019 by Tom Lechner
//

#include <cctype>

#include <lax/units.h>
#include <lax/strmanip.h>
#include <lax/attributes.h>

#include "cssutils.h"


using namespace Laxkit;
using namespace LaxFiles;


namespace Laidout {


static int num_css_colors = -1;

struct CssColorSpec {
	const char *name;
	const char *color;
};

static const CssColorSpec css_colors[] = {
	{"aliceblue", "#f0f8ff"},
	{"antiquewhite", "#faebd7"},
	{"aqua", "#00ffff"},
	{"aquamarine", "#7fffd4"},
	{"azure", "#f0ffff"},
	{"beige", "#f5f5dc"},
	{"bisque", "#ffe4c4"},
	{"black", "#000000"},
	{"blanchedalmond", "#ffebcd"},
	{"blue", "#0000ff"},
	{"blueviolet", "#8a2be2"},
	{"brown", "#a52a2a"},
	{"burlywood", "#deb887"},
	{"cadetblue", "#5f9ea0"},
	{"chartreuse", "#7fff00"},
	{"chocolate", "#d2691e"},
	{"coral", "#ff7f50"},
	{"cornflowerblue", "#6495ed"},
	{"cornsilk", "#fff8dc"},
	{"crimson", "#dc143c"},
	{"cyan", "#00ffff"},
	{"darkblue", "#00008b"},
	{"darkcyan", "#008b8b"},
	{"darkgoldenrod", "#b8860b"},
	{"darkgray", "#a9a9a9"},
	{"darkgreen", "#006400"},
	{"darkgrey", "#a9a9a9"},
	{"darkkhaki", "#bdb76b"},
	{"darkmagenta", "#8b008b"},
	{"darkolivegreen", "#556b2f"},
	{"darkorange", "#ff8c00"},
	{"darkorchid", "#9932cc"},
	{"darkred", "#8b0000"},
	{"darksalmon", "#e9967a"},
	{"darkseagreen", "#8fbc8f"},
	{"darkslateblue", "#483d8b"},
	{"darkslategray", "#2f4f4f"},
	{"darkslategrey", "#2f4f4f"},
	{"darkturquoise", "#00ced1"},
	{"darkviolet", "#9400d3"},
	{"deeppink", "#ff1493"},
	{"deepskyblue", "#00bfff"},
	{"dimgray", "#696969"},
	{"dimgrey", "#696969"},
	{"dodgerblue", "#1e90ff"},
	{"firebrick", "#b22222"},
	{"floralwhite", "#fffaf0"},
	{"forestgreen", "#228b22"},
	{"fuchsia", "#ff00ff"},
	{"gainsboro", "#dcdcdc"},
	{"ghostwhite", "#f8f8ff"},
	{"gold", "#ffd700"},
	{"goldenrod", "#daa520"},
	{"gray", "#808080"},
	{"green", "#008000"},
	{"greenyellow", "#adff2f"},
	{"grey", "#808080"},
	{"honeydew", "#f0fff0"},
	{"hotpink", "#ff69b4"},
	{"indianred", "#cd5c5c"},
	{"indigo", "#4b0082"},
	{"ivory", "#fffff0"},
	{"khaki", "#f0e68c"},
	{"lavender", "#e6e6fa"},
	{"lavenderblush", "#fff0f5"},
	{"lawngreen", "#7cfc00"},
	{"lemonchiffon", "#fffacd"},
	{"lightblue", "#add8e6"},
	{"lightcoral", "#f08080"},
	{"lightcyan", "#e0ffff"},
	{"lightgoldenrodyellow", "#fafad2"},
	{"lightgray", "#d3d3d3"},
	{"lightgreen", "#90ee90"},
	{"lightgrey", "#d3d3d3"},
	{"lightpink", "#ffb6c1"},
	{"lightsalmon", "#ffa07a"},
	{"lightseagreen", "#20b2aa"},
	{"lightskyblue", "#87cefa"},
	{"lightslategray", "#778899"},
	{"lightslategrey", "#778899"},
	{"lightsteelblue", "#b0c4de"},
	{"lightyellow", "#ffffe0"},
	{"lime", "#00ff00"},
	{"limegreen", "#32cd32"},
	{"linen", "#faf0e6"},
	{"magenta", "#ff00ff"},
	{"maroon", "#800000"},
	{"mediumaquamarine", "#66cdaa"},
	{"mediumblue", "#0000cd"},
	{"mediumorchid", "#ba55d3"},
	{"mediumpurple", "#9370db"},
	{"mediumseagreen", "#3cb371"},
	{"mediumslateblue", "#7b68ee"},
	{"mediumspringgreen", "#00fa9a"},
	{"mediumturquoise", "#48d1cc"},
	{"mediumvioletred", "#c71585"},
	{"midnightblue", "#191970"},
	{"mintcream", "#f5fffa"},
	{"mistyrose", "#ffe4e1"},
	{"moccasin", "#ffe4b5"},
	{"navajowhite", "#ffdead"},
	{"navy", "#000080"},
	{"oldlace", "#fdf5e6"},
	{"olive", "#808000"},
	{"olivedrab", "#6b8e23"},
	{"orange", "#ffa500"},
	{"orangered", "#ff4500"},
	{"orchid", "#da70d6"},
	{"palegoldenrod", "#eee8aa"},
	{"palegreen", "#98fb98"},
	{"paleturquoise", "#afeeee"},
	{"palevioletred", "#db7093"},
	{"papayawhip", "#ffefd5"},
	{"peachpuff", "#ffdab9"},
	{"peru", "#cd853f"},
	{"pink", "#ffc0cb"},
	{"plum", "#dda0dd"},
	{"powderblue", "#b0e0e6"},
	{"purple", "#800080"},
	{"red", "#ff0000"},
	{"rosybrown", "#bc8f8f"},
	{"royalblue", "#4169e1"},
	{"saddlebrown", "#8b4513"},
	{"salmon", "#fa8072"},
	{"sandybrown", "#f4a460"},
	{"seagreen", "#2e8b57"},
	{"seashell", "#fff5ee"},
	{"sienna", "#a0522d"},
	{"silver", "#c0c0c0"},
	{"skyblue", "#87ceeb"},
	{"slateblue", "#6a5acd"},
	{"slategray", "#708090"},
	{"slategrey", "#708090"},
	{"snow", "#fffafa"},
	{"springgreen", "#00ff7f"},
	{"steelblue", "#4682b4"},
	{"tan", "#d2b48c"},
	{"teal", "#008080"},
	{"thistle", "#d8bfd8"},
	{"tomato", "#ff6347"},
	{"turquoise", "#40e0d0"},
	{"violet", "#ee82ee"},
	{"wheat", "#f5deb3"},
	{"white", "#ffffff"},
	{"whitesmoke", "#f5f5f5"},
	{"yellow", "#ffff00"},
	{"yellowgreen", "#9acd32"},
	{nullptr, nullptr}
};

/*! Check against the 147 common css named colors.
 * Return nonzero for found, else 0 for not found.
 */
int CssNamedColor(const char *value, Laxkit::ScreenColor *scolor)
{
	if (num_css_colors == 0) {
		while (css_colors[num_css_colors].name) num_css_colors++;
	}

	//binary search
	int match = -1;
	int s = 0, e = num_css_colors, m, c;
	if (!strcasecmp(value, css_colors[s].name)) match = s;
	else if (!strcasecmp(value, css_colors[e].name)) match = e;

	while (match == -1) {
		m = (s+e)/2;
		if (m==s || m==e) break;

		c = strcasecmp(value, css_colors[m].name);

		if (c==0) { match = m; break; }
		if (c<0) {
			e = m-1;
			if (e<=s) break;

		} else { // c>0
			s = m+1;
			if (s>=e) break;
		}
	}

	if (match == -1) return 0;

	return HexColorAttributeRGB(css_colors[match].color, scolor, nullptr);
}


/*! Parse a css font size value.
 * relative_ret gets what a relative value is relative to. Percentage returns values where 1 is 100% and
 * applies to things like "100%", "larger" and "smaller".
 *
 * If there are units, then value is converted to css pixels (96 per inch),
 * EXCEPT when units_ret != nullptr. Then, don't convert, and return the units there.
 *
 * Returns 1 for success or 0 for parse error.
 */
int CssFontSize(const char *value, double *value_ret, CSSName *relative_ret, int *units_ret, const char **end_ptr) 
{
	 //  font-size: 	<absolute-size> | <relative-size> | <length> | <percentage> | inherit
	 //     absolute-size == xx-small | x-small | small | medium | large | x-large | xx-large 
	 //     relative-size == larger | smaller
	double v=-1;
	CSSName relative = CSS_Unknown;
	int units = UNITS_None;

	if (!strncmp(value,"inherit", 7)) ; //do nothing special

	 //named absolute sizes, these are relative to some platform specific table of numbers:
	else if (!strncmp(value,"xx-small", 8)) { relative = CSS_GlobalSize; v =.5;  value += 8; }
	else if (!strncmp(value,"x-small" , 7)) { relative = CSS_GlobalSize; v =.75; value += 7; }
	else if (!strncmp(value,"small"   , 5)) { relative = CSS_GlobalSize; v =.8;  value += 5; }
	else if (!strncmp(value,"medium"  , 6)) { relative = CSS_GlobalSize; v =1;   value += 6; }
	else if (!strncmp(value,"large"   , 5)) { relative = CSS_GlobalSize; v =1.2; value += 5; }
	else if (!strncmp(value,"x-large" , 7)) { relative = CSS_GlobalSize; v =1.4; value += 7; }
	else if (!strncmp(value,"xx-large", 8)) { relative = CSS_GlobalSize; v =1.7; value += 8; }

   	 //for relative size
	else if (!strncmp(value,"larger", 6)) { relative = CSS_Percent; v=1.2; value += 6; }
	else if (!strncmp(value,"smaller",7)) { relative = CSS_Percent; v=.8;  value += 7; }

	 //percentage, named relative units, or absolute length with units
	else {
		char *endptr = nullptr;
		DoubleAttribute(value,&v,&endptr); //relative size

		if (endptr && endptr != value) { //parse units
			value = endptr;
			while (isspace(*value)) value++;
			if (*value == '%') { relative = CSS_Percent; v /= 100; value++; }
			else if (!strncmp(value, "em" , 2)) { relative = CSS_em;   value += 2; }
			else if (!strncmp(value, "ex" , 2)) { relative = CSS_ex;   value += 2; }
			else if (!strncmp(value, "ch" , 2)) { relative = CSS_ch;   value += 2; }
			else if (!strncmp(value, "rem", 3)) { relative = CSS_rem;  value += 3; }
			else if (!strncmp(value, "vw" , 2)) { relative = CSS_vw;   value += 2; }
			else if (!strncmp(value, "vh" , 2)) { relative = CSS_vh;   value += 2; }
			else if (!strncmp(value, "vmin",4)) { relative = CSS_vmin; value += 4; }
			else if (!strncmp(value, "vmax",4)) { relative = CSS_vmax; value += 4; }
			else {
			    UnitManager *unitm = GetUnitManager();

				endptr = const_cast<char*>(value);
				while (isalnum(*endptr)) endptr++;
				units = unitm->UnitId(value, endptr - value);
				if (units != UNITS_None && !units_ret) {
					v = unitm->Convert(v, units, UNITS_CSSPoints, nullptr);
				}
				value = endptr;
			}
		}
	}

	*value_ret = v;
	if (relative_ret) *relative_ret = relative;
	if (units_ret) *units_ret = units;
	if (end_ptr) *end_ptr = value;

	return 1;
}


/*! Return integer weight, or -1 for error.
 *
 * <pre>
 *  normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
 * </pre>
 *
 * "bolder" and "lighter" are here arbitrarily mapped to 700 and 200 respectively and relative_ret is set to true.
 * Otherwise relative_ret is set to false.
 *
 */
int CSSFontWeight(const char *value, const char *&endptr, bool *relative_ret)
{
	int weight=-1;
	if (relative_ret) *relative_ret = false;

	if (!strncmp(value,"inherit",7))       { endptr = value+7; } //do nothing special
	else if (!strncmp(value,"normal",6))   { endptr = value+6; weight=400; }
	else if (!strncmp(value,"bold",4))     { endptr = value+4; weight=700; }
	else if (!strncmp(value,"bolder", 6))  { endptr = value+6; weight=700; if (relative_ret) *relative_ret = true; } //120% ?
	else if (!strncmp(value,"lighter", 7)) { endptr = value+7; weight=200; if (relative_ret) *relative_ret = true; } //80% ? 
	else if (value[0] >= '1' && value[0] <= '9' &&
			 value[1] >= '0' && value[1] <= '9' &&
			 value[2] >= '0' && value[2] <= '9') {
		//scan in any 3 digit integer between 100 and 999 inclusive... not really css compliant, but what the hay
		char *end_ptr;
		weight = strtol(value,&end_ptr,10);
		endptr = end_ptr;
	} else endptr = value;

	return weight;
}


} //namespace Laidout


