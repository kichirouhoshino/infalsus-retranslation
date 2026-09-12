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

## Japanese Nuances (Non-Spoiler)
- **`4.10.7.2` / Death and Summer (死と夏):** Japanese number wordplay (*goroawase*): 4 (*shi* = death), 10 (*to* = and), 7+2 (*na-tsu* = summer).
- **Mute (無口 / ミュート):** Capitalized because it refers to a recognized social status/identity in this world, not just physical silence.
- **Diving (潜る / ダイブ):** Written with the kanji for "submerge" (`潜る`), establishing the story's aquatic network metaphors.
- **AARC (方舟 / アーク):** Written with the kanji for "Ark" (`方舟`), referencing a vessel built to survive a crisis.

---

## Notes
  - Text that do not appear in the dialogue box and chat logs are not changed, due to technical limitations. You can see its retranslation in the logs.
  - Some lines will occupy 4 or more rows, due to them being longer than usual. Some will be shortened to 4 rows in a later update.
  - This repo only contains the source code for the DLL used to inject the Re-translation. For probably legal reasons (I'm not a lawyer), I did not include the stuff I used to extract the text.
  - Expect bugs and future updates breaking the patch (but I will be updating it regularly)
  
---

## How to install
1. Download the zip file from "Releases"
2. Extract the contents of the zip file into the game
3. LINUX/STEAMOS ONLY: Add "WINEDLLOVERRIDES="version=n,b" %command%" in your launch options.
4. Run the game normally. The dialogue are patched on runtime.

It should survive game updates, but any new dialogue added will need a patch update.

## How to uninstall
- Remove the files "retranslation.dat" and "version.dll"
