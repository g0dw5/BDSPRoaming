# BDSPRoaming

This is a high performance program for searching certain pokemons(SID/TID, IVs, shiny) in SWSH overworld and BDSP roaming.

## Usage
I assume that you already have an environment for compiling C++ code under c++14. Sorry for the inconvenience usage.

- git clone git@github.com:g0dw5/BDSPRoaming.git
- cd PATH_TO_SOURCE_CODE
- mkdir cmake_build
- cd cmake_build
- cmake -DCMAKE_BUILD_TYPE=Release ../
- make
- ./RoamingID --mode=swsh --sid=1234 --tid=567890 --ivs=31,0,31,31,31,31 --shiny=square --flawless=0
