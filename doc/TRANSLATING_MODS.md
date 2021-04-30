# Translating mods for Cataclysm: BN

- [Intro](#intro)
- [A short glossary](#a-short-glossary)
- [Workflow overview](#workflow-overview)
- [Setting up environment for string extraction](#setting-up-environment-for-string-extraction)
- [Extracting strings](#extracting-strings)
  * [Windows](#windows)
  * [Linux and OSX](#linux-and-osx)
- [Creating new PO](#creating-new-po)
- [Updating existing PO](#updating-existing-po)
- [Compiling PO into MO](#compiling-po-into-mo)
- [Adding MO file to the mod](#adding-mo-file-to-the-mod)
- [Miscellaneous notes](#miscellaneous-notes)

## Intro
This document aims to give a brief explanation on how to set up and operate
mod translation workflow for Cataclysm: Bright Nights.

While it's possible to use Transifex or any other platform or software that supports gettext,
this document only gives examples on how to do it with [Poedit](https://poedit.net/) and 
command-line [GNU gettext utilities](https://www.gnu.org/software/gettext/).

To get some generic tips on translating strings for Cataclysm: Bright Nights and its mods, see [TRANSLATING.md](TRANSLATING.md).

If you desire an in-depth explanation on PO/POT/MO files or how to work with them using GNU gettext utilities,
see [GNU gettext manual](https://www.gnu.org/software/gettext/manual/gettext.html).

## A short glossary
### POT file
Portable Object Template file (`.pot`).
This is a text file that contains original (most likely English) strings extracted from your mod JSON files.
The POT file is a template used for creating empty or updating existing PO files of any language.

### PO file
Portable Object file (`.po`).
This is a text file that contains translated strings for one language.
The PO files are what translators work with, and what will be compiled into a MO file.

### MO file
Machine Object file (`.mo`).
This is a binary file that contains translated strings for one language.
The MO files are what the game loads, and where it gets translated strings from.

## Workflow overview
The first translation workflow is as follows:

1. Extract strings from mod JSON files into a POT file
2. Create PO file for target language from this POT
3. Fill the PO file with translated strings
4. Compile PO into MO
5. Put the MO into your mod files

As the mod changes with time, so may change its strings.
Updating existing translations is done as follows:

1. Extract strings from mod JSON files into a new POT file
2. Update existing PO file from the new POT file
3. Add new or edit existing translated strings in the PO file
4. Compile PO into MO
5. Replace old MO in the mod files with new version

Step 1 in both workflows requires you to set up environment for string extraction (see below).

Steps 2-4 can be done using translation software either by the mod author/maintainer, or by the translator.

## Setting up environment for string extraction
You'll need Python 3 with `polib` library installed.
There are many tutorials online on how to install Python 3 and `pip`,
Python's package manager, and the `polib` library can be installed via
```bash
pip install polib
```
On Windows, make sure to add Python to the `PATH` variable (look for tutorials if you don't know how).

Scripts for extracting strings from JSON files can be found in the `lang` subdirectory of the game.

## Extracting strings
To extract strings, you'll need to run 2 python scripts, one of which requires additional configuration.

While it's easy to do once, you'll need to run them again in the future if the mod is under development and
you plan on keeping translations up-to-date. The easiest way to automate that is to create _another_ script
(batch or bash, depending on your OS) that would take care of the process for you.

### Windows
Create a `lang` folder inside the mod's folder, and put inside a batch script (`.bat` file) with the following contents:
```bat
python.exe C:\path\to\extract_json_strings.py -i .. -o index.pot --project YourModName
python.exe C:\path\to\dedup_pot_file.py index.pot
echo Done!
pause
```
Replace `C:\path\to\extract_json_strings.py` and `C:\path\to\dedup_pot_file.py` with actual paths to
`extract_json_strings.py` and `dedup_pot_file.py` scripts. If either paths or mod's name contain spaces,
surround them with quotes (e.g. `python.exe "C:\My Games\Cata\lang\dedup_pot_file.py" lang\index.pot`).
You can use relative paths: e.g. if the mod is inside `data\mods\`,
the paths would be `..\..\..\..\lang\extract_json_strings.py` and `..\..\..\..\lang\dedup_pot_file.py`.

Replace `YourModName` with the mod's name (doesn't have to match exactly).

To run the script, either double-click on it from File Explorer, or open command prompt and execute it from the mod's `lang` folder:
```bat
cd /d C:\path\to\your\mod\folder\lang
your_file.bat
```
Once the process completes, you should see a new translation template file `index.pot` that contains all strings extracted from the mod.

### Linux and OSX
Create a `lang` folder inside the mod's folder, and put inside a bash script (`.sh` file) with the following contents:
```bash
python /path/to/extract_json_strings.py -i .. -o index.pot --project YourModName
python /path/to/dedup_pot_file.py index.pot
echo Done!
```
Replace `/path/to/extract_json_strings.py` and `/path/to/dedup_pot_file.py` with actual paths to
`extract_json_strings.py` and `dedup_pot_file.py` scripts. If either paths or mod's name contain spaces,
surround them with quotes (e.g. `python "/path/with spaces/dedup_pot_file.py" lang/index.pot`).
You can use relative paths: e.g. if the mod is inside `data/mods/`,
the paths would be `../../../../lang/extract_json_strings.py` and `../../../../lang/dedup_pot_file.py`.

Replace `YourModName` with the mod's name (doesn't have to match exactly).

Don't forget to mark it as executable via file properties or terminal command:
```bash
chmod +x your_script.sh
```

To run the script, open the terminal in mod's `lang` folder and type in script name.

Once the process completes, you should see a new translation template file `index.pot` that contains all strings extracted from the mod.

## Creating new PO
Before creating PO file, you need to choose language id.

Open `data/raw/languages.json` to see the list of languages supported by the game.
In this list, each entry has its own id in form of `ln_LN`, where `ln` determines language and `LN` - dialect.

You can either use full `ln_LN` for exact language+dialect match,
or `ln` if you want the game to use your MO regardless of dialect.

### Poedit
1. Open the POT file with Poedit
2. Press "Create new translation" button (should show up near the bottom)
3. In language selection dialog, enter language id you chose
4. Save the file as `path/to/your/mod/lang/LANG.po` where `LANG` is the same language id

### msginit
```bash
msginit -i lang/index.pot -o lang/LANG.po -l LANG.UTF-8 --no-translator
```
Where `LANG` is the language id you chose

## Updating existing PO
### Poedit
1. Open the PO file with Poedit
2. Choose `Catalog->Update from POT file...` and select the new POT file
3. Save the file

### msgmerge
```bash
msgmerge lang/LANG.po lang/index.pot
```

## Compiling PO into MO
### Poedit
1. Open the PO file with Poedit
2. Make sure MO file will be encoded using UTF-8 (`Catalog->Properties->"Translation properties" tab->Charset`)
3. By default, each time PO file is saved Poedit automatically compiles it into MO,
   but the same can also be done explicitly via `File->Compile to MO...`

### msgfmt
```
msgfmt -o lang/LANG.mo lang/LANG.po
```

## Adding MO file to the mod
When loading mod files, the game automatically searches for MO files in `lang` mod subdirectory.
In other words, you typical mod folder structure will look like this:

```
mods/
    YourMod/
        modinfo.json
        lang/
            es.mo
            pt_BR.mo
            zh_CN.mo
```

**Note:**  Storing your script, and PO/POT files in the same `lang` subdirectory may make it easier to keep track of them.
The game ignores these files, and your mod folder structure will look like this:

```
mods/
    YourMod/
        modinfo.json
        lang/
            extract.bat  (extract.sh on MacOS/Linux)
            index.pot
            es.po
            es.mo
            pt_BR.po
            pt_BR.mo
            zh_CN.po
            zh_CN.mo
```

## Miscellaneous notes
### Is it possible to use arbitrary location or names for MO files?
No. The game looks for MO files with specific names that are located in the mod's `lang` directory.
If you'll put it anywhere else, or use you own file names (e.g. `french.mo`, `german.mo`), it won't work.

However, any mod will automatically try to use any other mod's translation files to translate it's strings.
This makes it possible to create mods that are purely "translation packs" for other mods (or mod collections),
if you'd rather for whatever reason keep the mod and MO separate.

### Is it necessary to run the script from the mod's lang folder, or keep it there?
No, it's just convenient to.

POT files allow saving references to each string's origin file,
and the extraction script prepends each reference with the input path passed via `-i` argument
(this is necessary because the script supports pulling strings from multiple input paths at the same time).

When running from mod's lang folder, the input path is very short `..`, and the resulting references look nice and clean.

Also, when keeping extraction script inside `lang` folder you don't have to worry about correct
working directory if you run it directly from the file explorer.

### Reloading translations in a running game
Open debug menu and select `Info...->Reload translations`, and the game will reload all MO files from disk.

This makes it easy to see how translated string looks in game, provided the translator has a way to compile MO files.

Example workflow with Poedit:
1. Translate a string
2. Hit Ctrl+S
3. Alt+Tab into the game
4. Reload translation files via debug menu
5. The game now displays translated string

### Dialects and MO load order
When loading MO files, the game looks for the file with exact language and dialect match.
If such file is absent, then it looks for a file with no dialect.

For example, when using `Español (España)` the load order is
1. `es_ES.mo`
2. `es.mo`

And when using `Español (Argentina)` the load order is
1. `es_AR.mo`
2. `es.mo`

Thus, `es.mo` would be loaded for either dialect of Spanish if the exact translation files are not present.

### What if 2 or more mods provide different translations for same string?
Then the game uses translation from the first such mod in the mod loading order.

The in-repo mods (including the core content "mod") are an exception: all of them use single MO file,
which is loaded at all times and always takes priority over 3-rd party translations.

If you want a different translation from the one in the base game, add a translation context to your string.
