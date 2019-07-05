
Laidout Feature History
=======================
http://www.laidout.org


New in Version 0.097  --  released 25 November 2018!
----------------------------------------------------
 - Node based interface
 - Improved clone tool
 - Save options now include save copy, save incremented copy
 - Autosave
 - Harfbuzz based text (work in progress)


New in Version 0.096  --  released 18 December 2015!
----------------------------------------------------
 - Caption text tool, for simple layers. Includes ability to use layered color fonts
 - Autosave, but you need to set manually in ~/.config/laidout/(version)/laidoutrc
 - More export options. This time to alternate each paper 180 deg, rotate each by increments of 90, export in reverse order


New in Version 0.095  --  released 23 April 2015!
-------------------------------------------------
 - Engraving tool, to create engraving style fields of lines
 - allow editing meshes based on a path
 - Variable width lines, offsets, and different kinds of line joins (bevel, round, miter)
 - More export options, such as export even or odd, or export in batches
 - Clickable page numbers to select page markers


New in Version 0.094  --  released 20 February 2014!
----------------------------------------------------
 - Cloning
 - Symmetric Clone Tiler
 - Shortcuts editor dialog
 - Signature editor: have variable num of pages per signature, stack multiple signatures
 - Graphical shell, kind of a searching calculator and function caller


New in Version 0.093  --  released 17 July 2012!
------------------------------------------------
 - On canvas flip controls
 - Align tool
 - Nup tool


New in Version 0.092  --  released 30 December 2011!
----------------------------------------------------
 - Path export
 - Gui for modifying Page Labels
 - Units, especially Allow changing units in rulers
 - OBJ file format in/out for polyhedra


New in Version 0.091  --  released 13 November 2010!
----------------------------------------------------

 - Impose-only mode, so as to allow easy plugin in access, to for instance do booklets inside scribus
 - Export PLAN files for podofoimpose
 - Interactive signature editor, fold and trim imposition


New in Version 0.09  -- released 21 February 2010!
--------------------------------------------------

 - Import Passepartout, Scribus, and very partially SVG.
Most data that Laidout doesn't know how to convert is stored as Mystery Data, which can then be exported
back out to the same format.
 - Rudimentary scripting interface, accessible from the command line.

 - You can now add, remove, load, and save different scratch spaces, as well as papergroups, so
you don't have to recreate them whenever you start Laidout.
 - Net impositions have been completely reprogrammed, to allow more adaptible import
of 3-d models. You can specify an <a href="http://shape.cs.princeton.edu/benchmark/documentation/off_format.html">OFF</a>
file to use as the basis for unwrapping, for instance.
 - Gradients, including color patch gradients, now display with transparency. Laidout's display mechanisms are
undergoing an overhall, so the state they are in now will be improved with the next version.


New in Version 0.08  -- released 15 September 2007!
---------------------------------------------------

 - A Paper Tiler, to spread one page on contents over many pieces of paper
 - Latin-1 internationalization support
 - Many exporters added, including images, pdf, and scribus
 - Limbo (scratch) spaces can be swapped between viewports


New in Version 0.07  -- released 2 June 2007!
---------------------------------------------

 - Limbo (scratch) spaces can be saved now
 - Otherwise, mostly bug fixes


New in Version 0.06  -- released 25 April 2007!
-----------------------------------------------

 - Group and ungroup, providing layer trees
 - New document creation from templates
 - An interface to associate previews with images, to make screen updating more rapid
 - Basic EPS import, opens up all kinds of down and dirty tricks using other programs, such as latex and lilypond.


New in Version 0.05  -- released 4 November 2006! 
-------------------------------------------------

 - Be able to work with preview images, rather than hundreds of 15M tiffs 
 - Import images from a list file, containing the images and a proper preview image if any 
 - Show adjacent pages in Net Singles view (prelude to being able to unwrap shapes any way they can) 
 - Use a ~/.laidout/0.05/laidoutrc 
 - Ability to select and resize multiple objects at the same time
 - Made ImageInterface scaling suck less, now distinguishes between no image and broken image 
 - A simple "configure" script to simplify setting up installation specifics before compiling 


New in Version 0.04  -- released 4 September 2006! 
--------------------------------------------------

 - Palette Window 
 - Simple multiple image import by selecting one or more from a directory 
 - Allow for window docking, floating, swapping panes, and temporary pane maximize 
 - Page Ranges controlling page number labels 
 

New in Version 0.03  -- released 13 May 2006! 
---------------------------------------------

 - Bezier patch with image for the color (this would be HOT for the gimp! a side project..) 
 - Insert a different image to an existing image object 
 - printing: produce masked images based on 50% threshhold of alpha channel of images 
 - Load and save window configurations 
 - EPS out to file###.eps by page not paper 
 - Ability to print a paper range 
 - Command prompt window 
 - SpreadEditor: drag to viewer to work on that page or spread 


New in Version 0.02 - released 8 April 2006! 
--------------------------------------------

 - Linear Gradients edit, save, and printout 
 - Circle Gradients edit, save, and printout 
 - ObjectInterface for resizing and shearing 
 - A help button to popup a list of all the otherwise unmentioned key shortcuts 


New in Version 0.01 -- released 12 March 2006! 
----------------------------------------------

 - Tiling in impositions, for booklets also  
 - Net Impositions for polyhedron "books" 
 - Implement page clipping for display AND for printing.. 
 - Images objects 
 - Color patches work enough to be getting on with 
 - Add and Delete Page buttons 
 - A Spread Editor to easily arrange the sequence of pages 
