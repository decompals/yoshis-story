# Yoshi's Story Decompilation

This is a WIP decomp of Yoshi's Story (US).

## Requirements
* Python 3.8 or greater is required to build this project. To install the requirements, please run the following command:

`python3 -m pip install -r tools/splat/requirements.txt --upgrade`


## Building

1. Place the US Yoshi's Story rom into the root of the repository as `baserom.z64`.
2. Set up tools and extract assets and code from the rom: `make setup`
3. Rebuild the rom: `make`
