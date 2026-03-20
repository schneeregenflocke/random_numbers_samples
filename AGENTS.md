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
| C++17           | Sprache                                   |
| OpenGL 4.5 Core | Rendering                                 |
| GLFW            | Fensterverwaltung und OpenGL-Kontext      |
| libepoxy        | OpenGL-Funktionslader                     |
| Dear ImGui      | Immediate-Mode-GUI                        |
| GLM             | 2D-Vektormathematik (glm::vec2)           |
| Boost.Math      | Wahrscheinlichkeitsdichtefunktionen (PDF) |
| embed-resource  | Einbetten von Lizenztexten als Ressourcen |

## Build

CMake 3.16+, C++17-Compiler. Auf Linux: `libepoxy` muss systemseitig installiert sein.

```sh
cmake -B build -S .
cmake --build build
```
