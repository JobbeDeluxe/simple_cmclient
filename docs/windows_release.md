# CityMania Windows-Builds für OpenTTD 14.1

Dieser Leitfaden erklärt zwei Wege, wie Sie lauffähige Windows-Pakete des CityMania-Clients erzeugen können:

1. **Automatischer Build über GitHub Actions** – erzeugt geprüfte ZIP-Pakete (mit `openttd.exe`) für x86, x64 und ARM64 und eignet sich für Releases oder Test-Builds.
2. **Manueller Build auf einem lokalen Windows-System** – falls Sie ohne CI arbeiten oder kurzfristig ein individuelles Paket benötigen.

Beide Varianten erzeugen eine Datei `citymania-client-<version>-windows-<arch>.zip`, die Sie direkt als Release-Download bereitstellen können.

---

## 1. Automatischer Build über GitHub Actions

Voraussetzungen:

- Zugriff auf das CityMania-GitHub-Repository mit Berechtigung, Workflows zu starten.
- Ein Commit oder Tag, das den gewünschten Stand des Quellcodes enthält.

### 1.1 Workflow starten

1. Pushen Sie den gewünschten Stand ins GitHub-Repository (z. B. Branch `citymania-14.1`).
2. Öffnen Sie auf GitHub die Registerkarte **Actions** und wählen Sie den Workflow **Release** aus.
3. Klicken Sie auf **Run workflow** und geben Sie im Feld `ref` den zu bauenden Ref an, z. B. `refs/heads/citymania-14.1` (für einen Branch) oder `refs/tags/v14.1-citymania` (für einen Tag).
4. Bestätigen Sie mit **Run workflow**. Der Build startet und benötigt – abhängig von der Auslastung – etwa 20–30 Minuten.

> **Hinweis:** Legen Sie stattdessen ein Git-Tag an und veröffentlichen Sie es, startet der Workflow automatisch, sobald das Release auf GitHub veröffentlicht wurde.

### 1.2 Artefakte herunterladen

1. Öffnen Sie nach Abschluss des Workflows den entsprechenden Lauf (Run).
2. Am Ende der Seite finden Sie für jede Architektur Artefakte namens `citymania-client-windows-x86`, `citymania-client-windows-x64` bzw. `citymania-client-windows-arm64`.
3. Laden Sie das gewünschte Artefakt herunter. Der Download enthält eine ZIP-Datei `citymania-client-<version>-windows-<arch>.zip` mit `openttd.exe`, Sprachdateien, Basesets sowie den Inhalten aus `release_files/`.
4. Verwenden Sie diese ZIP-Datei direkt als Release-Download oder hängen Sie sie manuell an eine GitHub-Release an.

### 1.3 Symbols & Debugging (optional)

Zusätzlich zum Client-Artefakt erzeugt der Workflow `symbols-windows-<arch>`. Dieses Archiv enthält PDB- und Breakpad-Symboldateien und kann zur Crash-Analyse aufbewahrt werden.

---

## 2. Manueller Build unter Windows

Voraussetzungen:

- Windows 10/11 64 Bit.
- [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) mit den Komponenten „Desktopentwicklung mit C++“ sowie optional ATL/MFC.
- [vcpkg](https://github.com/microsoft/vcpkg) (im Modus `x64-windows-static` installiert).
- [Ninja](https://ninja-build.org/) (entweder separat installiert oder über `Visual Studio Installer` hinzufügen).
- Git und CMake (werden mit Visual Studio und vcpkg geliefert).

> **Tipp:** Stellen Sie sicher, dass Sie alle Befehle in der passenden „x64 Native Tools Command Prompt für VS 2022“ (oder der entsprechenden Architektur) ausführen, damit die Compiler-Umgebung korrekt initialisiert ist.

### 2.1 Repository klonen

```powershell
cd C:\work
git clone https://github.com/CityMania-org/simple_cmclient.git
cd simple_cmclient
```

### 2.2 Build-Werkzeuge erzeugen

Einige Build-Schritte benötigen Host-Tools (z. B. `strgen`). Diese werden einmal separat erstellt:

```powershell
cmake -S . -B build-host -G "Ninja" -DOPTION_TOOLS_ONLY=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-host --target tools
```

### 2.3 Windows-Client (x64) bauen

```powershell
cmake -S . -B build -G "Ninja" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static `
  -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DHOST_BINARY_DIR="${PWD}/build-host" `
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --target openttd
```

Ersetzen Sie `C:/path/to/vcpkg` durch Ihr tatsächliches vcpkg-Verzeichnis. Für 32-Bit-Builds ändern Sie den Triplet-Wert auf `x86-windows-static`; für ARM64 verwenden Sie `arm64-windows-static`.

Der fertige Client liegt anschließend als `build/openttd.exe` (bzw. `openttd64.exe` bei 32-Bit-Builds) vor.

### 2.4 Release-Paket erstellen

```powershell
cpack --config build/CPackConfig.cmake
```

CPack legt die erzeugten Archive im Ordner `build/bundles/` ab. Standardmäßig heißen sie `openttd-<version>-windows-<arch>.zip`.

Um das Paket als CityMania-Release zu veröffentlichen, benennen Sie die ZIP-Datei um:

```powershell
cd build/bundles
ren openttd-*.zip citymania-client-<version>-windows-<arch>.zip
```

Ersetzen Sie `<version>` durch die Versionsnummer aus `build/.version` oder aus der ausgegebenen CMake-Statusmeldung. Diese ZIP-Datei enthält `openttd.exe`, alle notwendigen Daten (Sprachen, Basesets, `release_files/`) sowie die Standarddokumentation.

### 2.5 Upload als GitHub-Release

1. Erstellen Sie auf GitHub ein neues Release (oder bearbeiten Sie ein bestehendes).
2. Laden Sie die umbenannte ZIP-Datei hoch.
3. (Optional) Fügen Sie die Symbol-Dateien aus `build/symbols/` als separaten Anhang hinzu, falls Crashdump-Analysen benötigt werden.

---

## 3. Häufige Fragen

**Welche Version wird gebaut?**

Die Version wird automatisch aus `src/rev.cpp.in` und den Git-Tags bestimmt. Stellen Sie sicher, dass `FindVersion.cmake` beim Konfigurieren keine Warnungen ausgibt.

**Warum benötigt der Build zwei Konfigurationsordner (`build-host` und `build`)?**

Windows-Builds verwenden einige Hilfsprogramme, die auf der Host-Plattform laufen. Damit Sie auch für ARM64 oder x86 bauen können, werden diese Tools zunächst einmal für die Host-Architektur erstellt.

**Kann ich die CI-Artefakte länger aufbewahren?**

Die standardmäßige Aufbewahrungszeit beträgt fünf Tage. Laden Sie wichtige Artefakte herunter und speichern Sie sie extern, wenn Sie sie langfristig benötigen.

**Wie aktualisiere ich die Release-Dateien (`release_files/`)?**

Legen Sie neue oder geänderte Inhalte direkt im Verzeichnis `release_files/` ab. Sowohl der CI-Workflow als auch CPack übernehmen diese automatisch in jedes erzeugte ZIP-Paket.
