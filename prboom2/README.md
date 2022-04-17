# PrBoom: Ray Traced

Real-time path tracing support for the [PrBoom sourceport](https://github.com/coelckers/prboom-plus).



## Build

1. Build RTGL1 library
    * https://github.com/sultim-t/RayTracedGL1

1. Clone this repository
    * `git clone https://github.com/sultim-t/prboom-plus-rt.git`

1. Add an environment variable `RTGL1_SDK_PATH`, it must point to the RTGL1 root folder

1. Make sure the external development libraries required by PrBoom are available. For PrBoom-RT, SDL2/Ogg/Vorbis/VorbisFile are a requirement.
    
    * Windows: 
        * extract `prboom-rt-windows-dependencies.zip` (can be found [here](https://github.com/sultim-t/prboom-plus-rt/releases/tag/v2.6.1-rt1.0.2)) to the `prboom2` folder
        * it's a subset of [all PrBoom dependencies](https://github.com/coelckers/prboom-plus/releases/tag/windows_dependencies)
 
    * Linux: install using a package manager

1. Configure and build
    ```
    mkdir build
    cd build
    cmake ..
    cmake --build .
    ```
    
1. Copy `ovrd` folder to the compiled `prboom-plus` executable
    * it's better to create a hard link to `ovrd` to have it shared between different configs (e.g. Debug/Release/etc) 

1. Run `prboom-plus`



## Modding

### Creating custom textures

`ovrd/mat` folder contains new textures to use with the new lighting system. 
Each texture is compressed and wrapped into `.ktx2` format.

Assume that `CRATELIT` texture needs to be modified.

1. Setup environment:
    * Install [CompressonatorCLI](https://gpuopen.com/compressonator/) 
        * and append a path to its executable to `$PATH` environment variable (so `compressonatorcli.exe` can be called from anywhere)
    * Install Python3
    * Download `CreateKTX2.py` script (can be found [here](https://github.com/sultim-t/prboom-plus-rt/releases/tag/v2.6.1-rt1.0.2)) and place to `ovrd` folder
        * this tool is a part of [RTGL1](https://github.com/sultim-t/RayTracedGL1#textures)

1. Create files (each of them is ):
    * `ovrd/mat_raw/gfx/CRATELIT.png` -- albedo map
        * alpha channel (transparency) is used, if Doom uses transparency for that texture 
    * `ovrd/mat_raw/gfx/CRATELIT_rme.png`:
        * roughness in `R` channel
        * metallicity in `G` channel
        * emission in `B` channel, color is controlled by albedo map at the same pixel
    * `ovrd/mat_raw/gfx/CRATELIT_n.png` -- normal map, `G` channel is ignored (it's assumed to be always 255) 
1. Run `CreateKTX2.py` script with Python3
    * it will pack textures from `ovrd/mat_raw` to `ovrd/mat`

1. Changes should be visible, when the game reloads textures (usually, on game re-enter)

*Note: [flats](https://doom.fandom.com/wiki/Flat) are placed to `ovrd/mat/flat`, everything else is in `ovrd/mat/gfx`*



### Special effects for the textures
`ovrd/textures_metainfo.txt` contains configuration for textures. Each section has its own set of parameters, the first parameter is always a texture name:
* `@WATER`
    * if it's a water / mirror texture 

* `@RASTERIZED_WITH_LIGHT`
    * draw without ray tracing (no shadows/reflections/etc), but add a light source to it
    * format: `<texture name> <HEX RGB color> <light range multiplier, default=1.0>`

* `@VERTICAL_CONELIGHT`
    * if a light source that points up should be added  
    * format: `<texture name> <HEX RGB color> <intensity>`

* `@EMISSIVE`
    * make texture emissive (e.g. lava)
    * format: `<texture name> <emission intensity>`
    

* `@EMISSIVE_WITHOUT_INDIRECT_ILLUMINATION` -- same as `@EMISSIVE`

* `@MONOCHROME_FOR_COLORMAPS` -- make a texture grayscale



### Custom map light sources

*The system currently is very limited: `map_metainfo_doom1.txt` can be used with only one specific game, as it doesn't contain info about WAD.*

1. Pass `-rtdevmode` argument to `prboom-plus`
1. Use `Numpad +` and `Numpad -` to increase/decrease intensity of a light source at the center of a map sector.
1. To save light data to `map_metainfo_doom1.txt`, use `Numpad *`



