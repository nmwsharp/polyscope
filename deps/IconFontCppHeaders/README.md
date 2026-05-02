Support the development and maintenance of _IconFontCppHeaders_ through [GitHub Sponsors](https://github.com/sponsors/dougbinks) or [Patreon](https://www.patreon.com/enkisoftware).

# IconFontCppHeaders

[https://github.com/juliettef/IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders)

C and C++ headers, C# and Python classes, Rust files and Go packages for icon fonts Font Awesome, Fork Awesome, Google Material Design, Pictogrammers Material Design icons, Kenney game icons, Fontaudio, Codicons and Lucide.

A set of language files for using icon fonts in C, C++, C#, Python, Rust and Go, along with the python script we used to create the files.

Each language file contains defines for one font, with each icon code point defined as `ICON_*`, along with the *min*, *max* and *max 16 bit* code points for font loading purposes. The min excludes the ASCII characters code points. The max 16 bit is for use with libraries that only support 16 bit code points, for example Dear ImGui.

In addition you can use the python script to convert ttf font files to C and C++ headers. 
Each ttf icon font file is converted to a C and C++ header file containing a single array of bytes. 
To enable conversion, run the `GenerateIconFontCppHeaders.py` script with `ttf2headerC = True`. 

## Language Files
The following table lists all the language files for each icon font included in the _IconFontCppHeaders_ repository.
| Abbr | Icon Font | C, C++ | C# | Python | Go (*) | Rust
| - | - | - | - | - | - | -
| FA | **[ Font&nbsp;Awesome 4 ](#font-awesome-fa)** | [IconsFontAwesome4.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome4.h) | [IconsFontAwesome4.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome4.cs) | [IconsFontAwesome4.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome4.py) | [IconsFontAwesome4.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome4.go) | [IconsFontAwesome4.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome4.rs)
| FA | **[ Font&nbsp;Awesome 5&nbsp;free ](#font-awesome-fa)** | [IconsFontAwesome5.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5.h) [IconsFontAwesome5Brands.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5Brands.h) | [IconsFontAwesome5.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5.cs) [IconsFontAwesome5Brands.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5Brands.cs) | [IconsFontAwesome5.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5.py) [IconsFontAwesome5Brands.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5Brands.py) | [IconsFontAwesome5.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5.go) [IconsFontAwesome5Brands.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5Brands.go) | [IconsFontAwesome5.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5.rs) [IconsFontAwesome5Brands.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5Brands.rs)
| FA | **[ Font&nbsp;Awesome 5&nbsp;pro ](#font-awesome-fa)** | [IconsFontAwesome5Pro.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5Pro.h) [IconsFontAwesome5ProBrands.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5ProBrands.h) | [IconsFontAwesome5Pro.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5Pro.cs) [IconsFontAwesome5ProBrands.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5ProBrands.cs) | [IconsFontAwesome5Pro.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5Pro.py) [IconsFontAwesome5ProBrands.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5ProBrands.py) | [IconsFontAwesome5Pro.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5Pro.go) [IconsFontAwesome5ProBrands.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5ProBrands.go) | [IconsFontAwesome5Pro.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5Pro.rs) [IconsFontAwesome5ProBrands.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome5ProBrands.rs)
| FA | **[ Font&nbsp;Awesome 6&nbsp;free ](#font-awesome-fa)** | [IconsFontAwesome6.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome6.h) [IconsFontAwesome6Brands.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome6Brands.h) | [IconsFontAwesome6.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome6.cs) [IconsFontAwesome6Brands.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome6Brands.cs) | [IconsFontAwesome6.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome6.py) [IconsFontAwesome6Brands.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome6Brands.py) | [IconsFontAwesome6.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome6.go) [IconsFontAwesome6Brands.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome6Brands.go) | [IconsFontAwesome6.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome6.rs) [IconsFontAwesome6Brands.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome6Brands.rs)
| FA | **[ Font&nbsp;Awesome 7&nbsp;free ](#font-awesome-fa)** | [IconsFontAwesome7.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome7.h) [IconsFontAwesome7Brands.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome7Brands.h) | [IconsFontAwesome7.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome7.cs) [IconsFontAwesome7Brands.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome7Brands.cs) | [IconsFontAwesome7.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome7.py) [IconsFontAwesome7Brands.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome7Brands.py) | [IconsFontAwesome7.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome7.go) [IconsFontAwesome7Brands.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome7Brands.go) | [IconsFontAwesome7.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome7.rs) [IconsFontAwesome7Brands.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontAwesome7Brands.rs)
| FK | **[ Fork&nbsp;Awesome ](#fork-awesome-fk)** | [IconsForkAwesome.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsForkAwesome.h) | [IconsForkAwesome.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsForkAwesome.cs) | [IconsForkAwesome.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsForkAwesome.py) | [IconsForkAwesome.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsForkAwesome.go) | [IconsForkAwesome.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsForkAwesome.rs)
| MD | **[ Google Material&nbsp;Design ](#google-material-design-icons-md-and-material-symbols-ms)** | [IconsMaterialDesign.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialDesign.h) | [IconsMaterialDesign.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialDesign.cs) | [IconsMaterialDesign.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialDesign.py) | [IconsMaterialDesign.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialDesign.go) | [IconsMaterialDesign.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialDesign.rs)
| MS | **[ Google Material&nbsp;Symbols ](#google-material-design-icons-md-and-material-symbols-ms)** | [IconsMaterialSymbols.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialSymbols.h) | [IconsMaterialSymbols.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialSymbols.cs) | [IconsMaterialSymbols.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialSymbols.py) | [IconsMaterialSymbols.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialSymbols.go) | [IconsMaterialSymbols.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialSymbols.rs)
| MDI | **[ Pictogrammers Material&nbsp;Design ](#pictogrammers-material-design-icons-mdi)** | [IconsMaterialDesignIcons.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialDesignIcons.h) | [IconsMaterialDesignIcons.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialDesignIcons.cs) | [IconsMaterialDesignIcons.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialDesignIcons.py) | [IconsMaterialDesignIcons.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialDesignIcons.go) | [IconsMaterialDesignIcons.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsMaterialDesignIcons.rs)
| KI | **[ Kenney&nbsp;Game ](#kenney-game-icons-and-expansion-ki)** | [IconsKenney.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsKenney.h) | [IconsKenney.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsKenney.cs) | [IconsKenney.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsKenney.py) | [IconsKenney.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsKenney.go) | [IconsKenney.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsKenney.rs)
| FAD | **[ Fontaudio ](#fontaudio-fad)** | [IconsFontaudio.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontaudio.h) | [IconsFontaudio.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontaudio.cs) | [IconsFontaudio.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontaudio.py) | [IconsFontaudio.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontaudio.go) | [IconsFontaudio.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsFontaudio.rs)
| CI | **[ Codicons ](#codicons-ci)** | [IconsCodicons.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsCodicons.h) | [IconsCodicons.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsCodicons.cs) | [IconsCodicons.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsCodicons.py) | [IconsCodicons.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsCodicons.go) | [IconsCodicons.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsCodicons.rs)
| LC | **[ Lucide ](#lucide-lc)** | [IconsLucide.h](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsLucide.h) | [IconsLucide.cs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsLucide.cs) | [IconsLucide.py](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsLucide.py) | [IconsLucide.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsLucide.go) | [IconsLucide.rs](https://github.com/juliettef/IconFontCppHeaders/blob/main/IconsLucide.rs)

(* For Go see also [font.go](https://github.com/juliettef/IconFontCppHeaders/blob/main/font.go) and [go.mod](https://github.com/juliettef/IconFontCppHeaders/blob/main/go.mod))

## Icon Fonts

### Font Awesome FA
* [fontawesome.com](https://fontawesome.com)
* [github.com/FortAwesome/Font-Awesome](https://github.com/FortAwesome/Font-Awesome)

#### Font Awesome 4
* [github.com/FortAwesome/Font-Awesome/tree/4.x](https://github.com/FortAwesome/Font-Awesome/tree/4.x)
* [icons.yml](https://github.com/FortAwesome/Font-Awesome/blob/4.x/src/icons.yml)
* [fontawesome-webfont.ttf](https://github.com/FortAwesome/Font-Awesome/blob/4.x/fonts/fontawesome-webfont.ttf)

#### Font Awesome 5+
Font Awesome versions 5 and above split the different styles of icons into different font files with identical codepoints for *regular*, *solid* and *light* (pro only) styles, and a different set of codepoints for *brands*. We have put the *brands* into separate language files.
##### 5 Free
* [github.com/FortAwesome/Font-Awesome/tree/5.x](https://github.com/FortAwesome/Font-Awesome/tree/5.x)
* [icons.yml](https://github.com/FortAwesome/Font-Awesome/blob/5.x/metadata/icons.yml)
* [fa-brands-400.ttf](https://github.com/FortAwesome/Font-Awesome/blob/5.x/webfonts/fa-brands-400.ttf)
* [fa-regular-400.ttf](https://github.com/FortAwesome/Font-Awesome/blob/5.x/webfonts/fa-regular-400.ttf)
* [fa-solid-900.ttf](https://github.com/FortAwesome/Font-Awesome/blob/5.x/webfonts/fa-solid-900.ttf)

##### 6 Free
* [github.com/FortAwesome/Font-Awesome/tree/6.x](https://github.com/FortAwesome/Font-Awesome/tree/6.x)
* [icons.yml](https://github.com/FortAwesome/Font-Awesome/blob/6.x/metadata/icons.yml)
* [fa-brands-400.ttf](https://github.com/FortAwesome/Font-Awesome/blob/6.x/webfonts/fa-brands-400.ttf)
* [fa-regular-400.ttf](https://github.com/FortAwesome/Font-Awesome/blob/6.x/webfonts/fa-regular-400.ttf)
* [fa-solid-900.ttf](https://github.com/FortAwesome/Font-Awesome/blob/6.x/webfonts/fa-solid-900.ttf)

##### 7 Free
* [github.com/FortAwesome/Font-Awesome/tree/7.x](https://github.com/FortAwesome/Font-Awesome/tree/7.x)
* [icons.yml](https://github.com/FortAwesome/Font-Awesome/blob/7.x/metadata/icons.yml)
* You may need to convert the `.woff2` files to `.ttf` depending upon your loader
  * [fa-brands-400.woff2](https://github.com/FortAwesome/Font-Awesome/blob/7.x/webfonts/fa-brands-400.woff2)
  * [fa-regular-400.woff2](https://github.com/FortAwesome/Font-Awesome/blob/7.x/webfonts/fa-regular-400.woff2)
  * [fa-solid-900.woff2](https://github.com/FortAwesome/Font-Awesome/blob/7.x/webfonts/fa-solid-900.woff2)

##### Pro
* Refer to the [section on Font Awesome pro language files](#generating-font-awesome-pro-language-files)

### Fork Awesome FK
* [forkaweso.me/Fork-Awesome](https://forkaweso.me/Fork-Awesome)
* [github.com/ForkAwesome/Fork-Awesome](https://github.com/ForkAwesome/Fork-Awesome)
* [icons.yml](https://github.com/ForkAwesome/Fork-Awesome/blob/master/src/icons/icons.yml)
* [forkawesome-webfont.ttf](https://github.com/ForkAwesome/Fork-Awesome/blob/master/fonts/forkawesome-webfont.ttf)

### Google Material Design Icons MD and Material Symbols MS
* [fonts.google.com/icons](https://fonts.google.com/icons)
* [github.com/google/material-design-icons](https://github.com/google/material-design-icons)
#### Material Design Icons MD
* [fonts.google.com/icons?icon.set=Material+Icons](https://fonts.google.com/icons?icon.set=Material+Icons)
* [Codepoints](https://github.com/google/material-design-icons/blob/master/font/MaterialIcons-Regular.codepoints)
* [MaterialIcons-Regular.ttf](https://github.com/google/material-design-icons/blob/master/font/MaterialIcons-Regular.ttf)
#### Material Symbols MS
* [fonts.google.com/icons?icon.set=Material+Symbols](https://fonts.google.com/icons?icon.set=Material+Symbols)
* [Codepoints](https://github.com/google/material-design-icons/blob/master/variablefont/MaterialSymbolsOutlined%5BFILL%2CGRAD%2Copsz%2Cwght%5D.codepoints)
* [MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf](https://github.com/google/material-design-icons/blob/master/variablefont/MaterialSymbolsOutlined%5BFILL,GRAD,opsz,wght%5D.ttf)
* [MaterialSymbolsRounded[FILL,GRAD,opsz,wght].ttf](https://github.com/google/material-design-icons/blob/master/variablefont/MaterialSymbolsRounded[FILL,GRAD,opsz,wght].ttf)
* [MaterialSymbolsSharp[FILL,GRAD,opsz,wght].ttf](https://github.com/google/material-design-icons/blob/master/variablefont/MaterialSymbolsSharp[FILL,GRAD,opsz,wght].ttf)

### Pictogrammers Material Design Icons MDI
* [pictogrammers.com/library/mdi](https://pictogrammers.com/library/mdi/)
* [github.com/Templarian/MaterialDesign-Webfont](https://github.com/Templarian/MaterialDesign-Webfont)
* [materialdesignicons.css](https://github.com/Templarian/MaterialDesign-Webfont/blob/master/css/materialdesignicons.css)
* [materialdesignicons-webfont.ttf](https://github.com/Templarian/MaterialDesign-Webfont/blob/master/fonts/materialdesignicons-webfont.ttf)  

### Kenney Game Icons and Expansion KI
* [kenney.nl/assets/game-icons](http://kenney.nl/assets/game-icons) and [kenney.nl/assets/game-icons-expansion](http://kenney.nl/assets/game-icons-expansion) 
* [github.com/nicodinh/kenney-icon-font](https://github.com/nicodinh/kenney-icon-font)
* [kenney-icons.css](https://github.com/nicodinh/kenney-icon-font/blob/master/css/kenney-icons.css)
* [kenney-icon-font.ttf](https://github.com/nicodinh/kenney-icon-font/blob/master/fonts/kenney-icon-font.ttf)

### Fontaudio FAD
* [github.com/fefanto/fontaudio](https://github.com/fefanto/fontaudio)
* [fontaudio.css](https://github.com/fefanto/fontaudio/blob/master/font/fontaudio.css)
* [fontaudio.ttf](https://github.com/fefanto/fontaudio/blob/master/font/fontaudio.ttf)

### Codicons CI
* [microsoft.github.io/vscode-codicons/dist/codicon](https://microsoft.github.io/vscode-codicons/dist/codicon.html)
* [github.com/microsoft/vscode-codicons](https://github.com/microsoft/vscode-codicons)
* [codicon.css](https://microsoft.github.io/vscode-codicons/dist/codicon.css)
* [codicon.ttf](https://microsoft.github.io/vscode-codicons/dist/codicon.ttf)

### Lucide LC
* [lucide.dev](https://lucide.dev)
* [github.com/lucide-icons/lucide](https://github.com/lucide-icons/lucide)
* [lucide.css](https://unpkg.com/lucide-static@latest/font/lucide.css)
* [lucide.ttf](https://unpkg.com/lucide-static@latest/font/lucide.ttf)

### Ionicons
* Unsupported as of 29 Apr 2020. See [Issue #16](https://github.com/juliettef/IconFontCppHeaders/issues/16).

## Generating Font Awesome Pro Language Files

Font Awesome Pro is a commercial product. We only support version 5. The following steps show how we generated the language files:  
  1. Download the Font Awesome Pro 5 Web package from [fontawesome.com](https://fontawesome.com). 
  2. To generate the language files, drop `icons.yml` in the same directory as `GenerateIconFontCppHeaders.py` and run the script. (You'll find `icons.yml` under `..\fontawesome-pro-n.n.n-web\metadata\icons.yml` where `n.n.n` is the version number.)
  3. Use the language files with the following icon files:  
    * `..\fontawesome-pro-n.n.n-web\webfonts\fa-brands-400.ttf`  
    * `..\fontawesome-pro-n.n.n-web\webfonts\fa-light-300.ttf`  
    * `..\fontawesome-pro-n.n.n-web\webfonts\fa-regular-400.ttf`  
    * `..\fontawesome-pro-n.n.n-web\webfonts\fa-solid-900.ttf`

For versions 6 and above the process should be similar. You'll find an example for [Font Awesome 6 Pro on @jakerieger's fork](https://github.com/jakerieger/IconFontCppHeaders).

## Example Code

Using [Dear ImGui](https://github.com/ocornut/imgui) as an example UI library:

```Cpp

#include "IconsFontAwesome5.h"

ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontDefault();
float baseFontSize = 13.0f; // 13.0f is the size of the default font. Change to the font size you use.
float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

// merge in icons from Font Awesome
static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
ImFontConfig icons_config; 
icons_config.MergeMode = true; 
icons_config.PixelSnapH = true; 
icons_config.GlyphMinAdvanceX = iconFontSize;
io.Fonts->AddFontFromFileTTF( FONT_ICON_FILE_NAME_FAS, iconFontSize, &icons_config, icons_ranges );
// use FONT_ICON_FILE_NAME_FAR if you want regular instead of solid

// in an imgui window somewhere...
ImGui::Text( ICON_FA_PAINT_BRUSH "  Paint" ); // use string literal concatenation
// outputs a paint brush icon and 'Paint' as a string.
```

## Projects Using the Font Icon Language Files

### Avoyd
The Voxel Editor user interface uses Dear ImGui with Font Awesome icon fonts.  
[www.avoyd.com](https://www.avoyd.com)

![Screenshot of Avoyd Voxel Editor UI using an IconFontCppHeaders C++ header file for Font Awesome with Dear ImGui](https://github.com/juliettef/Media/blob/main/IconFontCppHeaders_Avoyd_voxel_editor.png?raw=true)

### bgfx
Cross-platform rendering library  
[bkaradzic.github.io/bgfx/overview](https://bkaradzic.github.io/bgfx/overview.html)  
[github.com/bkaradzic/bgfx](https://github.com/bkaradzic/bgfx)

### glChAoS.P
Real time 3D strange attractors scout  
[www.michelemorrone.eu/glchaosp](https://www.michelemorrone.eu/glchaosp)  
[github.com/BrutPitt/glChAoS.P](https://github.com/BrutPitt/glChAoS.P)

![Screenshot of glChAoS.P UI using an IconFontCppHeaders language file for Font Awesome with Dear ImGui](https://raw.githubusercontent.com/BrutPitt/glChAoS.P/master/imgsCapture/ssWGL_half.jpg)

### iPlug2
Cross platform C++ audio plug-in framework  
[iplug2.github.io](https://iplug2.github.io)  
[github.com/iplug2/iplug2](https://github.com/iplug2/iplug2)

### Lumix Engine
3D C++ open source game engine  
[github.com/nem0/LumixEngine](https://github.com/nem0/LumixEngine)  

![Screenshot of Lumix Engine editor using an IconFontCppHeaders language file for Font Awesome with Dear ImGui](https://raw.githubusercontent.com/wiki/nem0/LumixEngine/files/features/editor.jpg)

### Tracy Profiler
Real time, nanosecond resolution, remote telemetry frame profiler for games and other applications.  
[bitbucket.org/wolfpld/tracy](https://bitbucket.org/wolfpld/tracy)  

[![New features in Tracy Profiler v0.6](https://img.youtube.com/vi/uJkrFgriuOo/0.jpg)](https://www.youtube.com/watch?v=uJkrFgriuOo)

### Visual 6502 Remix
Transistor level 6502 Hardware Simulation  
[floooh.github.io/visual6502remix](https://floooh.github.io/visual6502remix)  
[github.com/floooh/v6502r](https://github.com/floooh/v6502r)  

### Games
- [Avatar: Frontiers of Pandora](https://www.mobygames.com/game/213552/avatar-frontiers-of-pandora/)
- [Avoyd](https://www.avoyd.com)
- [Wonder Boy: The Dragon's Trap](https://www.mobygames.com/game/87084/wonder-boy-the-dragons-trap/)

## Related Tools

### ImGuiFontStudio
Create font subsets  
[github.com/aiekick/ImGuiFontStudio](https://github.com/aiekick/ImGuiFontStudio)  

## Contributing

Thanks to everyone who has contributed to IconFontCppHeaders so far. In order to make things easier please bear in mind the following:  
- I regularly update the repository. If you urgently need an update and can't run the generator yourself, raise an issue specifying the icon set(s) you need updated.  
  - Don't submit PRs for updating the defines (the output of the generator).  
- Before submitting a PR, please raise an issue describing the problem or the enhancement you suggest. If you're able, propose an implementation.  
- If you use IconFontCppHeaders and would like your project to be featured on this page, raise an issue or email me at juliette@enkisoftware.com.  

## Credits

Development - [Juliette Foucaut](http://www.enkisoftware.com/about.html#juliette) - [@juliettef](https://github.com/juliettef)  
Requirements - [Doug Binks](http://www.enkisoftware.com/about.html#doug) - [@dougbinks](https://github.com/dougbinks)  
None language implementation and [refactoring](https://gist.github.com/paniq/4a734e9d8e86a2373b5bc4ca719855ec) - [Leonard Ritter](http://www.leonard-ritter.com/) - [@paniq](https://github.com/paniq)  
Suggestion to add a define for the ttf file name - [Sean Barrett](https://nothings.org/) - [@nothings](https://github.com/nothings)  
Initial Font Awesome 5 implementation - [Codecat](https://codecat.nl/) - [@codecat](https://github.com/codecat)  
Suggestion to add Fork Awesome - [Julien Deswaef](http://xuv.be/) - [@xuv](https://github.com/xuv)  
Suggestion to add Ionicons - [Omar Cornut](http://www.miracleworld.net/) - [@ocornut](https://github.com/ocornut)  
C# language implementation - Rokas Kupstys - [@rokups](https://github.com/rokups)  
Suggestion to add Material Design Icons - Gustav Madeso - [@madeso](https://github.com/madeso)  
Fontaudio implementation - [Oli Larkin](https://www.olilarkin.co.uk/) - [@olilarkin](https://github.com/olilarkin)  
Initial ttf to C and C++ headers conversion implementation - Charles Mailly - [@Caerind](https://github.com/Caerind)  
Python language implementation - Hang Yu - [@yhyu13](https://github.com/yhyu13)  
Go language implementation - Matt Pharr - [@mpp](https://github.com/mmp)  
Codicons implementation - Robert Ryan - [@rtryan98](https://github.com/rtryan98)  
Rust language implementation - Gaeel Bradshaw-Rodriguez - [@Bradshaw](https://github.com/Bradshaw)  
Pictogrammers Material Design icons implementation - Bobby Anguelov - [@BobbyAnguelov](https://github.com/BobbyAnguelov)  
Lucide icons implementation - Lucide Contributors - [@lucide-icons](https://github.com/lucide-icons/lucide#credits)