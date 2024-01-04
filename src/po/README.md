Translating Laidout
===================


## Note for translators

To translate Laidout, simply copy `laidout.pot` to `your_language.po` and
edit the strings in it. You can use a program like
[lokalize](https://userbase.kde.org/Lokalize) or [poedit](https://poedit.net/)
which may be easier if you don't like plain text editors.

The rest of this file describes the build process for translation files,
which translators can safely ignore.


## Translation file processing

All of this refers to the goings on in the src/po directory.
For more info on just what the hell
all these po things are, you can look up the gettext manual at:
  http://www.gnu.org/software/gettext/manual/gettext.html
 
Any po files you make MUST be encoded as UTF-8 characters.



<a name="updating-basic-info"/>

## Updating Basic Info

1. `POTFILES` is a list of files to grab translatable strings from.
   To regenerate that file, do:

       make potfiles

   Also, if you have a link to the laxkit source directory from the top laidout
   directory, then in the `laidout/po directory`, you can do:

       make potfiles-withlax

   which will target all the Laidout and Laxkit source files as having translatable strings.


2. `laidout.pot` is the base template file. Refresh with:

       make update

   This will make a file `laidout-freshdump.pot`, and merge it with any existing `laidout.pot`.
   `laidout-freshdump.pot` may be removed at this point. `laidout.pot` is the file you want.



<a name="making-new"/>

## Making New Translations

1. Update the basic info as above.

2. Combine with Laxkit strings, if you did not do so above, by appending the contents of
   `laxkit/lax/po/laxkit.pot` to `laidout/src/po/laidout.pot`.
   The Laxkit strings may have been translated to the desired
   language already. If so, you can add those to your new one with the `msgcat` command.
   For instance, for spanish (es), you could do this:

       msgcat es-laidout.po es-laxkit.po -o es-merged.po

   The file `es-merged.po` might have a bunch of lines around characters like `"#-#-#-#"`
   This means you must choose one section, and delete the other, including 
   the `"#-#-#-#"` parts.


3. Now make the necessary changes to that pot file, and save to the correct language
   name in this directory. For instance, Canadian english would be `en_CA.po`. You might
   use a program like [lokalize](https://userbase.kde.org/Lokalize) 
   to help edit the file. If you do it by hand, then you really
   just need to know that the things beginning with `msgid` are the original strings,
   and the `msgstr` things are the translated strings. For more info on just what the hell
   all these po things are, you can look up the gettext manual at:
   http://www.gnu.org/software/gettext/manual/gettext.html

4. Tell the developers there's a new translation available, and make sure they include 
   your `.po` file in the source tree.



Installation of the translation files 
-------------------------------------

1. Plain `make` will compile all the `.po` files found there.

2. `make install` will install the `.mo` files to `(prefix)/share/locale/*/LC_MESSAGES/laidout.mo`.
   This is called automatically when you type `make install` in the top Laidout directory.

3. Go learn some more languages and make new translations!


Updating Old Translation Files
------------------------------

1. Do the steps in [Updating Basic Info](#updating-basic-info) above.

2. Now run this command:

       msgmerge your_old_po_file.po laidout.pot > your_updated_po_file.po

   Now the `your_updated_po_file.po` file will contain all the currently needed strings, 
   preserving all the old translations, as long as they are still needed. Now just translate
   the new strings, and put it in place of `your_old_po_file.po`.
   There might be a few extra strings in the Laxkit po files, so you might also try the step
   in part 2 of [Making New Translations](#making-new) above.




What happens behind the scenes
------------------------------

For reference, here's the basics of translation file management:

1. `xgettext -C -d laidout  --files-from POTFILES -o laidout.pot --keyword=_ --flag=_:1:pass-c-format`

   This makes a pot file, usually from a file called `po/POTFILES`, with translatable strings listed.
   Then, edit that file to have a proper header, and make it a master po file.
   Translators make their own en_CA.po, es.po, etc based on the master pot file

2. `msgmerge old.pot new.pot > newest.pot` will merge new strings with an existing pot file

3. `msgfmt` converts the `.po` files into `.mo` (or `.gmo`) files

4. These are installed typically in `/usr/share/locale/en_CA/LC_MESSAGES/thedomainname.mo`,
   or `$(prefix)/share/locale/...`

       foreach *.mo  install $(prefix)/share/locale/`basename $NAME .po`/LC_MESSAGES/laidout.mo

5. Programs will know which translation file to use with a call to gettext's `bindtextdomain()`.

