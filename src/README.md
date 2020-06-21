Developing Laidout {#dev}
==================

Here are a few notes about the general layout of the Laidout code.

## In a nutshell

The [LaidoutApp](@ref Laidout::LaidoutApp) (`laidout.cc`, `laidout.h`) is what coordinates everything.
Main windows are composed of panes. Some pane types are viewports, which is
where interfaces operate. The basic visual elements that make up Laidout pages
are in dataobjects, as are interfaces directly relating to them.

If you add more source files and thus other object files that must be linked in,
you must modify `./Makefile -> otherobjs` to include the new object files, as well
as `(the dir)/Makefile -> objs` of where you are putting the file in.


## Directories

The source code is all in `src`:

| Directory     |  Description                                                                |
|---------------|-----------------------------------------------------------------------------|
| `api        ` |  Various functions and classes that can be used in the interpreter          |
| `calculator ` |  The guts of the internal interpreter                                       |
| `core       ` |  Core Laidout classes relating to Project, Document, and Page.              |
| `dataobjects` |  Data objects used in Laidout, and interfaces that directly operate on them |
| `extras     ` |  Extra plugins and such, not to be considered part of core functionality    |
| `filetypes  ` |  Export and import code                                                     |
| `impositions` |  All things impositions                                                     |
| `interfaces ` |  Interfaces not directly tied to particular object types                    |
| `nodes      ` |  Core classes relating to nodes                                             |
| `plugins    ` |  C++ based plugins                                                          |
| `printing   ` |  Code for printing, and postscript export                                   |
| `ui         ` |  Various window panes and dialogs                                           |
