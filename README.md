# TR-808 / TR-909 — Clone VST3

Plugin VST3 + Standalone développé en C++ avec le framework JUCE 9.
Reproduit le son et l'ergonomie des TR-808 et TR-909 de Roland.

## 🎛️ Caractéristiques

- **Double moteur** : TR-808 (crème/orange) + TR-909 (gris/rouge) avec sélecteur en UI
- **11 voix synthétisées en DSP** : kick, snare, rimshot, clap, 3 toms, HH closed/open, crash, ride, + cowbell + clave (808)
- **Step sequencer 16×11** avec accent par step, shuffle paramétrable, 16 patterns internes
- **MIDI Learn** sur tous les paramètres
- **Sync MIDI Clock / transport DAW** via AudioPlayHead (PPQ position)
- **Disto interne** sur le bus de sortie (drive paramétrable)
- **State save/restore** via APVTS (presets persistants dans le DAW)
- Interface en français, style vintage fidèle aux originaux

## 🔨 Build

Le build se fait automatiquement sur GitHub Actions (Windows + Visual Studio 2022).

1. Forkez ce repo (ou créez un nouveau repo à partir de ce code)
2. Allez dans l'onglet **Actions** du repo
3. Cliquez sur **"Build VST3 + Standalone"** → **"Run workflow"** → **"Run workflow"**
4. Attendez ~10-15 minutes
5. Une fois le build terminé, cliquez sur le run → section **Artifacts** en bas
6. Téléchargez :
   - `TR808_909_VST3_Windows.zip` — contient le plugin VST3
   - `TR808_909_Standalone_Windows.zip` — contient le standalone .exe

## 📦 Installation du VST3 sur Windows

1. Dézippez `TR808_909_VST3_Windows.zip`
2. Copiez le dossier `TR808_909.vst3` dans :
   ```
   %LOCALAPPDATA%\Programs\Common\VST3\
   ```
   (Tapez cette adresse dans l'Explorateur Windows. Créez le dossier `VST3` s'il n'existe pas.)
3. Lancez votre DAW → le plugin "TR808_909" apparaîtra dans Instruments → Synth → Drum

## 🛠️ Build local (optionnel)

Si vous voulez compiler vous-même sur votre machine :

### Prérequis

- **Visual Studio 2022 Community** (gratuit) — https://visualstudio.microsoft.com/fr/downloads/
  - Cochez le workload **"Développement Desktop en C++"** pendant l'installation
- **CMake** — https://cmake.org/download/
- **Git** — https://git-scm.com/download/win

### Étapes

```bash
git clone https://github.com/juce-framework/JUCE.git extern/JUCE
cmake -B Build -G "Visual Studio 17 2022" -A x64
cmake --build Build --config Release --target TR808_909_VST3 TR808_909_Standalone
```

Le VST3 sera dans `Build/TR808_909_artefacts/Release/VST3/TR808_909.vst3`.

## 📜 Licence

- **Code du plugin** : MIT — © Z-Labs 2025
- **JUCE framework** : ISC — © Raw Material Software
- **VST3 SDK** : Steinberg VST3 SDK (intégré à JUCE)

Roland TR-808 et TR-909 sont des marques de Roland Corporation.
Ce projet est une synthèse indépendante et n'est pas affilié à Roland.
