# In Falsus — English Re-translation Patch

A complete, re-translation of the Visual Novel portion of In Falsus, using Gemini Flash 3.8.

The goal of this re-translation is to replace lowiro's sub-par English script with an improved one. Mind you that this is done with AI, so I guess you could call this a band-aid solution, but in my opinion it is MILES better than the original.

The Japanese script was used as the baseline (since the voice acting is in Japanese), with the original English translation as reference.

---

## Changes
  - Complete overhaul of the English script, fixing a plethora of issues present in lowiro's localization.
  - Remove excessive uses of emdashes, colons, and semicolons and instead replace with natural sentence structures.
  - Japanese honorifics (-san, -chan, -nee, -sensei) are intentionally preserved throughout.
  - Mute character dialogue formatted without quotation marks to reflect text-to-speech / digital screen communication.
  - Phone and chat app dialogues formatted cleanly without quotation marks, preserving authentic emoticons.

---

## Japanese Nuances & Wordplay (Non-Spoiler)

To help readers appreciate the worldbuilding and dialogue nuances that arise from the original Japanese voice acting and script, here are a few linguistic details preserved in this translation:

- **`4.10.7.2` & "Death and Summer" (*Goroawase*):**  
  In Japanese culture, numerical wordplay (*goroawase*) assigns phonetic readings of syllables to numbers. The hacker group's tag is read:
  - **4** (*shi*) = **死** (Death)
  - **10** (*to*) = **と** (and)
  - **7** (*na*) + **2** (*tsu*) = **夏** (Summer)  
  Together, `4.10.7.2` reads phonetically as *Shi-to-Natsu* (死と夏), literally translating to **"Death and Summer"**. In the story, characters treat the numerical string and the name interchangeably based on this exact pun.

- **Capitalized "Mute" (無口 / ミュート):**  
  In the world of *In Falsus*, "Mute" is not merely a descriptive adjective for physical silence. It is a recognized social designation (written in the script with the kanji `無口` glossed as `ミュート`) chosen by individuals who renounce spoken language in a society governed by truth detection. It is capitalized throughout to reflect this distinct societal identity, contrasting with ordinary actions like "to mute notifications."

- **"Diving" (潜る / ダイブ):**  
  The script pairs the native Japanese verb `潜る` (*moguru* — to plunge into water or submerge) with the phonetic reading `ダイブ` (*daibu* — Dive). This underpins the overarching aquatic and submersion metaphors used whenever characters access virtual networks or deeper mental layers.

- **AARC & The Ark (方舟 / アーク):**  
  AARC is written in the original script with the kanji `方舟` (*hakobune* — ark, as in Noah's Ark) accompanied by the reading `アーク` (*Āku* / Ark), symbolizing a vessel intended to preserve select lives through an impending calamity.

- **Japanese Honorifics:**  
  Suffixes like *-san*, *-chan*, *-nee*, and *-sensei* are preserved because interpersonal boundaries, respect levels, and relational warmth are heavily emphasized by the Japanese voice cast.

---

## Notes
  - Text that do not appear in the dialogue box and chat logs are not changed, due to technical limitations. You can see its retranslation in the logs.
  - Some lines will occupy the 4th row, due to them being longer than usual.
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
