# In Falsus — English Re-translation Patch

A complete, re-translation of the Visual Novel portion of In Falsus, using Gemini Flash 3.8.

The goal of this re-translation is to replace lowiro's sub-par English script with an improved one. Mind you that this is done with AI, so I guess you could call this a band-aid solution, but in my opinion it is MILES better than the original.

The Japanese script was used as the baseline (since the voice acting is in Japanese), with the original English translation as reference.

---

## Changes
  - Complete overhaul of the English script, fixing a plethora of issues present in lowiro's localization.
  - Remove excessive uses of emdashes, colons, and semicolons and instead replace with natural sentence structures.
  - Japanese honorifics (-san, -chan, -nee, -sensei) are intentionally preserved throughout.

---

## Notes
  - Text that do not appear in the dialogue box and chat logs are not changed, due to technical limitations. You can see its retranslation in the logs.
  
---

## How to install
1. Extract the contexts of the zip file into the game
2. LINUX/STEAMOS ONLY: Add "WINEDLLOVERRIDES="version=n,b" %command%" in your launch options.
3. Run the game normally. The dialogue are patched on runtime.

It should survive game updates, but any new dialogue added will need a patch update.

## How to uninstall
- Remove the files "retranslation.dat" and "version.dll"
