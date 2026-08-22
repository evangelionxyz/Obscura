## Obscura Engine

A modular C++ game engine with runtime plugin support and a Qt 6 QML-based Editor.

### Commands

#### Full Build (Engine + Qt Editor)
```powershell
.\Scripts\BuildAll.ps1 -Target All -Config Debug
```

#### Engine Only (Premake)
Generate:
```powershell
premake5 vs2026 --file=Scripts/Generate.lua
```

Build:
```powershell
msbuild Obscura.slnx /p:Configuration=Debug /p:Platform=x64
```

Run Console Host:
```powershell
.\Build\x86_64\Debug\Obscura.Editor.exe
```

#### Qt Editor (CMake + Qt 6)
Configure & Build:
```powershell
cmake -S Obscura/QtEditor -B Build/QtEditor -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\msvc2022_64"
cmake --build Build/QtEditor --config Debug
```

Run Qt Editor:
```powershell
$env:PATH = "C:\Qt\6.11.1\msvc2022_64\bin;" + $env:PATH
.\Build\QtEditor\Debug\ObscuraQtEditor.exe
```

