# random_numbers_samples — Projektbeschreibung

## Ziel

`random_numbers_samples` ist ein interaktives Visualisierungswerkzeug für Zufallsstichproben aus
Wahrscheinlichkeitsverteilungen. Es dient als Lern- und Forschungsumgebung, um das Verhalten von
Stichprobenfunktionen (z.B. Mittelwert, Varianz) empirisch zu untersuchen und grafisch darzustellen.

Die im Quellcode dokumentierten Forschungsfragen sind:

- Wie ist die Verteilung der Längen von Konfidenzintervallen, die anhand der t-Verteilung berechnet
  werden, im Vergleich zu Konfidenzintervallen, die bei bekannter Varianz über die Normalverteilung
  berechnet werden?
- Wie verteilen sich die wahren Parameterwerte auf Konfidenzintervallen?
- Wie unterscheiden sich die Mittelwert-Verteilungen der geometrischen und der Bernoulli-Verteilung?

## Funktionsweise

### Zufallszahlenerzeugung

Zufallszahlen werden mit dem Hardware-RDRAND-Befehl (`_rdrand32_step`) erzeugt — nicht mit einem
Software-PRNG. Damit sind die Ergebnisse nicht reproduzierbar und hardware-nah zufällig.

### Stichprobenerhebung

Der Benutzer konfiguriert:

- **Verteilung** (siehe unten) und deren Parameter
- **Anzahl der Stichproben** (`number_samples`): wie viele Stichproben erzeugt werden
- **Stichprobengrösse** (`sample_size`): wie viele Zufallszahlen pro Stichprobe gezogen werden

Die Generierung erfolgt multithreaded (12 Threads) über `std::thread`.

Für jede Stichprobe werden folgende Kennzahlen berechnet:

| Name        | Bedeutung                                         |
| ----------- | ------------------------------------------------- |
| `sum`       | Summe aller Stichprobenwerte                      |
| `mean`      | Arithmetisches Mittel                             |
| `tts`       | Total Sum of Squares (Gesamtquadratsumme)         |
| `variance1` | Populationsvarianz (geteilt durch n)              |
| `variance2` | Stichprobenvarianz / unbiased (geteilt durch n-1) |

### Visualisierung

Das Fenster ist zweigeteilt:

- **Linke Hälfte (Plot):** OpenGL-gerendertes Koordinatensystem mit einem Histogramm der gewählten
  Stichprobenfunktion und einer überlagerten Normalverteilungs-PDF als Referenzkurve. Der Plot
  unterstützt Scrolling/Panning.
- **Rechte Hälfte (User Input):** Dear-ImGui-Steuerungsbereich mit folgenden Abschnitten:
  - *Random Samples*: Verteilungsauswahl, Parametereinstellung, Stichprobenkonfiguration,
    Auswahl der anzuzeigenden Stichprobenfunktion
  - *Histogram*: Anzahl der Bins
  - *Axis Limits*: X- und Y-Achsengrenzen sowie Gitterabstand
  - *Colors*: Farben für Kurve und Histogramm
  - *Licenses*: Lizenztexte der eingebetteten Bibliotheken

## Unterstützte Wahrscheinlichkeitsverteilungen (17)

| Kategorie         | Verteilungen                                                 |
| ----------------- | ------------------------------------------------------------ |
| Gleichförmig      | uniform int, uniform real                                    |
| Bernoulli-Familie | bernoulli, binomial, negative binomial, geometric            |
| Poisson-Familie   | poisson, exponential, gamma, weibull, extreme value          |
| Normal-Familie    | normal, log normal, chi squared, cauchy, fisher f, student t |

## Technologie-Stack

| Komponente      | Zweck                                     |
| --------------- | ----------------------------------------- |
| C++26           | Sprache                                   |
| OpenGL 4.5 Core | Rendering                                 |
| GLFW            | Fensterverwaltung und OpenGL-Kontext      |
| libepoxy        | OpenGL-Funktionslader                     |
| Dear ImGui      | Immediate-Mode-GUI                        |
| GLM             | 2D-Vektormathematik (glm::vec2)           |
| Boost.Math      | Wahrscheinlichkeitsdichtefunktionen (PDF) |
| embed-resource  | Einbetten von Lizenztexten als Ressourcen |

## Build

CMake 3.16+, C++26-Compiler (GCC 15+). Auf Linux: `libepoxy` muss systemseitig installiert sein.

```sh
cmake -B build -S .
cmake --build build
```

Das Binary liegt danach unter `build/random_numbers_samples`.

### Bekannte Build-Voraussetzungen (Linux/Arch)

- `libepoxy` (OpenGL-Lader, ersetzt GLAD): `sudo pacman -S libepoxy`
- Boost-Lizenzdatei ist lokal unter `licenses/LICENSE_1_0.txt` abgelegt (Arch legt sie nicht
  systemseitig ab)
- `imgui_tables.cpp` muss explizit mit gebaut werden (in CMakeLists bereits eingetragen)
- `src/` muss als Include-Pfad gesetzt sein (für `custom_imconfig.h` via `IMGUI_USER_CONFIG`)
- `IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<epoxy/gl.h>` wird als Compile-Definition gesetzt, damit imgui
  keinen eigenen GL-Loader verwendet
- `<epoxy/gl.h>` wird in `custom_imconfig.h` eingebunden, damit alle imgui-Compilation-Units die
  OpenGL-Typen kennen

## Laufzeit-Debugging

### Starten

```sh
./build/random_numbers_samples
```

Die Applikation öffnet ein GLFW-Fenster mit OpenGL 4.5 Core Profile. Ein aktiver Display-Server
(X11 oder Wayland) ist erforderlich.

### Bekannte Laufzeit-Eigenheiten

| Meldung / Verhalten                                                                    | Ursache                                                                                                                    | Bewertung                                   |
| -------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------- |
| `Glfw Error 65548: Wayland: The platform does not support setting the window position` | `glfwSetWindowPos` ist auf Wayland nicht erlaubt; der Aufruf wird nur auf X11 ausgeführt                                   | Harmlos, kein Fehler                        |
| Font-Lade-Pfad                                                                         | Auf Linux wird `/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf` geladen; schlägt das fehl, greift der imgui-Default-Font | Kein Absturz                                |
| `io.Fonts->Build()` darf **nicht** manuell aufgerufen werden                           | Neuere imgui-Versionen (`RendererHasTextures`) verwalten den Font-Atlas intern                                             | Manueller Aufruf führt zu Assertion-Absturz |

### Häufige Absturzursachen

- **Font-Assertion**: `Could not load font file!` — der Font-Pfad existiert nicht. Entweder den
  Pfad in `src/script.cpp` anpassen oder sicherstellen, dass ein Fallback auf
  `io.Fonts->AddFontDefault()` vorhanden ist.
- **`Build()` vor Backend-Init**: `Called ImFontAtlas::Build() before RendererHasTextures got set`
  — `io.Fonts->Build()` entfernen; das Backend baut den Atlas selbst.
- **OpenGL-Typen unbekannt in imgui**: `GLuint`/`GLint` nicht deklariert — `#include <epoxy/gl.h>`
  muss in `custom_imconfig.h` stehen, nicht nur in `glfw_include.h`.

## Screenshots erstellen und anzeigen

Screenshots werden mit `hyprshot` erstellt. Der Speicherort wird durch die Umgebungsvariable `HYPRSHOT_DIR` definiert (z.B. `$HOME/Bilder/Bildschirmfotos`).

### Screenshot des aktuellen Monitors (ohne Mausinteraktion)
```bash
hyprshot -m output -m eDP-1 -d
```
- `-m output -m eDP-1` — nimmt den Monitor `eDP-1` auf (Monitor-Namen via `hyprctl monitors` ermitteln)
- `-m output -m active` — nimmt den aktiven Monitor auf (generisch)
- `-d` — zeigt Debug-Ausgabe mit dem gespeicherten Dateipfad im Terminal

### Screenshot lesen/anzeigen lassen
Den ausgegebenen Pfad (z.B. `/home/titan99/Bilder/Bildschirmfotos/2026-03-16-130101_hyprshot.png`) an Claude Code übergeben — dieser kann Bilddateien lesen und den Inhalt beschreiben.

### Bereich ausschneiden (Crop) für bessere Lesbarkeit
Da der volle Screenshot bei kleinen Elementen (z.B. Waybar-Uhr) schwer lesbar ist, kann ein Ausschnitt erstellt werden:
```bash
magick <screenshot.png> -crop <breite>x<höhe>+<x>+<y> /tmp/crop.png
```
Beispiel — obere linke Ecke (Waybar-Uhr, 200x35 Pixel):
```bash
magick /home/titan99/Bilder/Bildschirmfotos/2026-03-16-130707_hyprshot.png -crop 200x35+0+0 /tmp/clock_crop.png
```
Hinweis: `convert` ist in ImageMagick v7 veraltet — stattdessen `magick` verwenden.

### Screenshot nur vom Applikationsfenster

`hyprshot -m window` erfordert Mausinteraktion und ist nicht automatisierbar. Die zuverlässige
Alternative: App in einen leeren Workspace verschieben, dort den vollen Monitor-Screenshot aufnehmen
und anschliessend die Waybar wegcroppen.

**Ablauf:**

```bash
# 1. Alte Instanzen beenden
pkill -f random_numbers_samples

# 2. Neue Instanz starten (landet auf Workspace 3)
./build/random_numbers_samples &
sleep 3

# 3. Fenster in leeren Workspace verschieben (z.B. 9)
hyprctl dispatch movetoworkspacesilent "9,title:random_samples"

# 4. Zu Workspace 9 wechseln
hyprctl dispatch workspace 9
sleep 1

# 5. Screenshot aufnehmen
hyprshot -m output -m eDP-1

# 6. Waybar (30 px logisch = 36 px physisch bei Scale 1.2) wegcroppen
magick <screenshot.png> -crop 1913x1037+4+40 /tmp/app_window.png

# 7. App beenden
pkill -f random_numbers_samples
```

**Bekannte Einschränkungen:**
- `hyprshot` läuft manchmal im Hintergrund weiter, auch wenn der Screenshot gespeichert wurde —
  den gespeicherten Pfad aus der Ausgabe (`Saving in: ...`) direkt verwenden, nicht auf den
  Prozess warten.
- `pkill random_numbers_samples` schlägt fehl (Name > 15 Zeichen) — stattdessen `pkill -f random_numbers_samples` verwenden.
- Der `[workspace:N]`-Dispatch in `hyprctl dispatch exec` platziert das Fenster nicht zuverlässig
  auf dem gewünschten Workspace; `movetoworkspacesilent` nach dem Start ist stabiler.
