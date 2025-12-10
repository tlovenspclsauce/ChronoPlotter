# ChronoPlotter Build Instructions

## Prerequisites
- Qt 5.15.2 (msvc2019_64) installed at `C:\Qt\5.15.2\msvc2019_64`
- Visual Studio 2022 or Visual Studio 18 (preview) with x64 tools
- QXlsx library (included in QXlsx/ subdirectory)

## Building with Qt/qmake

### Using Command Line

1. Open a command prompt or PowerShell in the ChronoPlotter directory
2. Set up the environment:
   ```powershell
   $env:PATH = "C:\Qt\5.15.2\msvc2019_64\bin;$env:PATH"
   ```

3. Generate Makefiles:
   ```powershell
   qmake ChronoPlotter.pro
   ```

4. Build (choose one method):
   
   **Option A: Using jom (faster, parallel builds)**
   ```powershell
   C:\Qt\Tools\QtCreator\bin\jom\jom.exe
   ```
   
   **Option B: Using nmake**
   ```cmd
   nmake release
   ```

### Using Qt Creator

1. Open `ChronoPlotter.pro` in Qt Creator
2. Select the MSVC 2019 64-bit kit
3. Build (Ctrl+B)

### Using Visual Studio

1. Generate Visual Studio project:
   ```powershell
   qmake -tp vc ChronoPlotter.pro
   ```

2. Open `ChronoPlotter.vcxproj` in Visual Studio
3. Select Release configuration
4. Build Solution (F7)

## Important Notes

### x64 Build Configuration
The project is configured to build as x64. The `.pro` file includes:
- Forced x64 architecture settings
- `/MACHINE:X64` linker flag for MSVC

### Qt DLL Deployment
The build system automatically deploys required Qt DLLs using `windeployqt` after a successful build. The executable and all dependencies will be in:
- `release/ChronoPlotter.exe` (Release build)
- `debug/ChronoPlotter.exe` (Debug build)

### Manual DLL Deployment
If automatic deployment fails, you can manually deploy Qt DLLs:
```powershell
C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe release\ChronoPlotter.exe --release --no-translations
```

## Creating a Single Executable with Embedded DLLs (Optional)

If you want to distribute ChronoPlotter as a single .exe file without separate DLL files, you have several options:

### Option 1: Static Linking (Recommended for licensing compliance)

**Note:** This requires Qt to be built with static linking, which is not included in the standard Qt binaries.

1. Install Qt with static linking support or build Qt from source with static configuration
2. Modify `ChronoPlotter.pro` to use static linking:
   ```qmake
   CONFIG += static
   QTPLUGIN += qwindows qwindowsvistastyle
   DEFINES += QT_STATICPLUGIN
   ```
3. Rebuild the project - this will produce a single .exe file

**Limitations:**
- Requires Qt Commercial license or compliance with LGPL static linking requirements
- Much larger executable size (~20-50 MB vs ~1 MB with DLLs)
- Longer compile times

### Option 2: DLL Bundling with Enigma Virtual Box (Easiest)

**Enigma Virtual Box** is a free tool that packages an application and its DLLs into a single executable.

1. Download Enigma Virtual Box from: https://enigmaprotector.com/en/downloads.html
2. Build ChronoPlotter normally (with DLLs)
3. Run Enigma Virtual Box
4. Configure:
   - **Input File Name:** `<your_build_directory>\release\ChronoPlotter.exe`
   - **Output File Name:** `<desired_output_path>\ChronoPlotter_Portable.exe`
5. Add files to virtualize:
   - Click "Add" ? "Add Folder" and select the entire `release\` directory
   - Or manually add required DLLs and plugin folders
6. Click "Process" to create the single executable

**Files to include:**
- All .dll files in `release\` folder
- `platforms\` folder (required for Qt application startup)
- `imageformats\` folder (for image loading)
- `printsupport\` folder (for printing functionality)
- `styles\` folder (for Windows visual styles)
- `iconengines\` folder (for SVG icon support)

### Option 3: Using windeployqt with --dir and manual packaging

1. Build the application
2. Deploy to a specific directory:
   ```powershell
   C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe release\ChronoPlotter.exe --release --no-translations --dir deploy
   ```
3. Use a tool like **BoxedApp Packer** or **Inno Setup** with compression to create a single installer or executable

### Comparison of Methods

| Method | File Size | Ease | License Concerns | Startup Speed |
|--------|-----------|------|------------------|---------------|
| Dynamic DLLs (default) | Small exe + DLLs | Easy | None | Fast |
| Static Linking | Large (~30MB) | Hard | LGPL/Commercial | Fastest |
| Enigma Virtual Box | Medium (~25MB) | Very Easy | None | Slightly slower |
| Installer (Inno Setup) | Variable | Medium | None | Fast |

### Recommended Approach

For most users, **Enigma Virtual Box** is recommended because:
- ? Free and easy to use
- ? No license concerns
- ? Creates truly portable single .exe
- ? No installation required
- ? Works with existing build system

For commercial or advanced users who need maximum performance, consider static linking with a commercial Qt license.

## Troubleshooting

### "LNK1112: module machine type 'x86' conflicts with target machine type 'x64'"
This error indicates mixed x86/x64 object files. Solution:
1. Clean the build directory: `Remove-Item release,debug -Recurse -Force`
2. Rebuild from scratch

### "Cannot find Qt DLLs"
Ensure Qt bin directory is in PATH before building:
```powershell
$env:PATH = "C:\Qt\5.15.2\msvc2019_64\bin;$env:PATH"
```

### Compiler not found
If using nmake/jom directly, set up the Visual Studio environment:
```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

Or for VS 2019:
```cmd
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
```

### Single exe won't start (Enigma Virtual Box)
Make sure you included the `platforms\` folder - Qt applications require the platform plugin to start.

## Output
After a successful build, the following will be in the `release/` directory:
- ChronoPlotter.exe (main executable)
- Qt5*.dll (Qt libraries)
- Various plugin directories (platforms/, imageformats/, etc.)

All files in the `release/` directory are required for the application to run.
