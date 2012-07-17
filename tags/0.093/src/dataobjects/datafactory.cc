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
// Copyright (C) 2009 by Tom Lechner
//










//---------------------------- LImagePatchData --------------------------------
/*! \class LImagePatchData 
 * \brief Subclassing LaxInterfaces::ImagePatchData
 */

class LImagePatchData : virtual public LaxInterfaces::ImagePatchData, 
						virtual public DrawableObject
{
 public:
	LImagePatchData() {}
	virtual ~LImagePatchData() {}
}:

LaxInterfaces::SomeData *createLImagePatchData()
{
	return new LImagePatchData();
}

//---------------------------- LImageData --------------------------------
/*! \class LImageData 
 * \brief Subclassing LaxInterfaces::ImageData
 */

LaxInterfaces::SomeData *createLImageData()
{
	return new LImageData();
}

//---------------------------- LColorPatchData --------------------------------
/*! \class LColorPatchData 
 * \brief Subclassing LaxInterfaces::ColorPatchData
 */

LaxInterfaces::SomeData *createLColorPatchData()
{
	return new LColorPatchData();
}

//---------------------------- LGradientData --------------------------------
/*! \class LGradientData 
 * \brief Subclassing LaxInterfaces::GradientData
 */

LaxInterfaces::SomeData *createLGradientData()
{
	return new LGradientData();
}

//---------------------------- LGradientData --------------------------------
/*! \class LGradientData 
 * \brief Subclassing LaxInterfaces::GradientData
 */

LaxInterfaces::SomeData *createLGradientData()
{
	return new LGradientData();
}




//---------------------------- SomeDataFactory Setup --------------------------

SomeDataFactory lobjectfactory;

void InitializeDataFactory()
{
	LaxInterfaces::somedatafatory=&lobjectfactory;

	lobjectfactory->DefineNewObject(LAX_IMAGEDATA,"ImageData",createLImageData,NULL);
	lobjectfactory->DefineNewObject(LAX_PATHSDATA,"PathsData",createLPathsData,NULL);
	lobjectfactory->DefineNewObject(LAX_IMAGEPATCHDATA,"ImagePatchData",createLImagePatchData,NULL);
	lobjectfactory->DefineNewObject(LAX_COLORPATCHDATA,"ColorPatchData",createLColorPatchData,NULL);
	lobjectfactory->DefineNewObject(LAX_GRADIENTDATA,"GradientData",createLGradientData,NULL);
}

