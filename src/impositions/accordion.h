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
// Copyright (C) 2025 by Tom Lechner
//
#ifndef ACCORDION_H
#define ACCORDION_H


#include "netimposition.h"
#include <lax/singletonkeeper.h>


namespace Laidout {


NetImposition *CreateAccordion(const char *which, double paper_width, double paper_height);


class Accordion : public NetImposition
{
  protected:
  	static Laxkit::SingletonKeeper presets; //the ObjectDef for the presets enum

  public:

  	enum AccordionPresets {
		Unknown,
		HalfFold,      // pamphlet just folded down the middle
		VerticalHalf,  // like HalfFold, but vertical
		TriFold,       // aka C-fold, 3 panels, one folded forward from right, one folded forward from left
		ZFold,         // 3 panel accordion
		ZFoldVertical, // 3 panel accordion, vertical
		FrenchQuarter, // fold down vertically in half, then horizontally in half
		Gate,          // aka Single Open, 3 panel, but center panel is twice the l and r panels
		DoubleGate,    // aka Double Open, 4 panels, fold in from right to center, fold from left to center, fold at center
		Roll,          // 4 panel, fold in from R, again from r, then l
		DoubleParallel,// fold in half from r, then fold in half again
		MapNxM,        // Generic grid of pages
		EasyZine,      // 2x4, with cut in the middle. Same as Accordion2xN with n=4
		Accordion1xN,  // 1 x n accordion. Not even numbers fold out nicely.
		Accordion2xN,  // n is even. fold in half vertically, cut across middle panels, lower left 2 panels are front/back
		Accordion4x4,  // cut in 3 panels horizontally from left, then right, then left
		AccordionNxM,  // zig zagging accordion, n and m must be even
		Miura          // 7x5 with slanting pages for rigid origami-like folding.
  	};

	AccordionPresets accordion_type = Unknown;
	int h_tiles = 2;
	int v_tiles = 1; // must be 1 or positive even
	int config_1 = 0;
	int config_2 = 0;
	bool double_sided = false;
	double inset_left=0, inset_right=0, inset_top=0, inset_bottom=0;
  	

  	Accordion();
  	const char *whattype() { return "AccordionImposition"; }

	bool Resize(double width, double height);

	static ObjectDef *GetPresets();
	static int NumParams(AccordionPresets type, int *default_1_ret, int *default_2_ret);

	static NetImposition *Build(AccordionPresets preset, double paper_width, double paper_height, int config1, int config2, NetImposition *existing_netimp);
	static NetImposition *BuildEasyZine(double paper_width, double paper_height, NetImposition *existing_netimp);
	static NetImposition *BuildAccordion1xN(int n, bool vertical, double paper_width, double paper_height, int variation, NetImposition *existing_netimp);
	static NetImposition *BuildAccordionNxM(int n, int m, double paper_width, double paper_height, int variation, NetImposition *existing_netimp);
	static NetImposition *BuildGate(double paper_width, double paper_height, NetImposition *existing_netimp);
	static NetImposition *BuildMiura(int n, int m, double angle_degrees, double paper_width, double paper_height, NetImposition *existing_netimp);
};


} //namespace Laidout

#endif
