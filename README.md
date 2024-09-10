# Tiny alternative for MRE SDK  

For now only packing tools

## What are the CMake projects in this repo about?

|Folder|Description|
|-|-|
|PackApp|App for pack .vxp app from .axf (or .dll), .res and tags|
|PackRes|App for pack .res file from files. Now mre-2.0 format not support|

## How to build?
### Linux

```
mkdir build
cd build
cmake ..
make
```

### Wimdows (Visual Studio)

```
mkdir vs
cd vs
cmake -G "Visual Studio 17 2022" ..
TinyMRESDK.sl
```

Also you can use cmake gui for config.

## How to use

Run with `-h` to get help