# Translating mods for Cataclysm: BN

## Disclaimer
This document aims to give a full, if brief, explanation on how to set up and operate
mod translation workflow for Cataclysm: Bright Nights

While it's possible to use Transifex or any other platform or software that supports gettext,
this document only gives examples on how to do it with [Poedit](https://poedit.net/).

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
### Installing software
This part will need to be done only once.
#### Linux
You'll need GNU gettext utilities, python and git. They can be installed with
```bash
sudo apt-get install gettext python git
```

#### Mac
You'll need GNU gettext utilities, python and git. They can be installed with
```bash
brew install gettext python git
```

#### Windows
If you have a compilation environment for the game set up on your computer, Cygwin or MSYS2,
then you should already have everything installed and ready.

Otherwise, setting up environment for string extraction follows the same steps as setting up compilation
environment - except you won't need to install as much packages.

##### Using Cygwin
Follow `Installation` and `Configuration` parts of [COMPILING-CYGWIN.md](COMPILING/COMPILING-CYGWIN.md) guide,
except instead of installing all packages required for compilation install only the ones required for cloning and string extraction:
```bash
apt-cyg install git gettext python3
```

##### Using MSYS2
Follow `Prerequisites` and `Installation` parts of [COMPILING-MSYS.md](COMPILING/COMPILING-MSYS.md) guide,
except instead of installing all packages required for compilation install only the ones required for cloning and string extraction:
```bash
pacman -S git gettext mingw-w64-x86_64-python
```

### Obtaining scripts
Scripts for extracting strings from JSON files are bundled with the game repository.

Either download source code for the game from latest release on github (it should be at the bottom of each release)
and extract it into an empty directory, or open the terminal and clone Cataclysm-BN repository with following command line:
**Note:**  Adding `--depth=1` will create a shallow clone, which marginally reduces repository download size and size on disk at the cost of repository history. However, a shallow clone is enough for our use case.

```bash
git clone https://github.com/cataclysmbnteam/Cataclysm-BN.git --depth=1
```

Once the operation is complete, your cloned repository should reside in newly created `Cataclysm-BN` folder.

## Extracting strings
Open the terminal in repository folder and run 
```bash
lang/update_mod_pot.sh path/to/your/mod/ output/file/name.pot
```
Where `path/to/your/mod/` is path to your mod folder (the one with `modinfo.json` in it),
and `output/file/name.pot` is the name of the resulting POT file.
If output file name is omitted, the script will output to `path/to/your/mod/lang/index.pot`.

## Creating new PO
Before creating PO file, you need to choose language id.

Open `data/raw/languages.json` to see the list of languages supported by the game.
In this list, each entry has its own id in form of `ln_LN`, where `ln` determines language and `LN` - dialect.

You can either use full `ln_LN` for exact language+dialect match,
or `ln` if you want the game to use your MO regardless of dialect.

1. Open the POT file with Poedit
2. Press "Create new translation" button (should show up near the bottom)
3. In language selection dialog, enter language id you chose
4. Save the file as `path/to/your/mod/lang/LANG.po` where `LANG` is the same language id

## Updating existing PO
1. Open the PO file with Poedit
2. Choose `Catalog->Update from POT file...` and select the new POT file
3. Save the file

## Compiling PO into MO
1. Open the PO file with Poedit
2. Make sure MO file will be encoded using UTF-8 (`Catalog->Properties->"Translation properties" tab->Charset`)
3. By default, each time PO file is saved Poedit automatically compiles it into MO, but the same can also be done explicitly via `File->Compile to MO...`

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

**Note:**  Storing PO and POT files in the same `lang` subdirectory may make it easier to keep track of them.
The game ignores these files, and your mod folder structure will look like this:

```
mods/
    YourMod/
        modinfo.json
        lang/
        	index.pot
        	es.po
            es.mo
            pt_BR.po
            pt_BR.mo
            zh_CN.po
            zh_CN.mo
```

## Miscellaneous notes
### MO load order
When loading MO files, the game looks for the file with exact language and dialect match.
If it is absent, then the game looks for a file without explicitly specified dialect.

For example, when using `Español (España)` the load order is
1. `es_ES.mo`
2. `es.mo`

And when using `Español (Argentina)` the load order is
1. `es_AR.mo`
2. `es.mo`

Thus, `es.mo` would be loaded for either dialect of Spanish if the exact translation files are not present.

### Reloading translations in a running game
Open debug menu and select `Info...->Reload translations`, and the game will reload all MO files from disk.

This makes it easy to see how translated string looks in game, provided the translator has a way to compile MO files.

Example workflow with Poedit:
1. Translate a string
2. Hit Ctrl+S
3. Alt+Tab into the game
4. Reload translation files via debug menu
5. The game now displays translated string
