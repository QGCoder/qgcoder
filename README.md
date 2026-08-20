<img src="https://raw.githubusercontent.com/QGCoder/qgcoder/master/doc/qgcoder-001.png"/>

An interactive G-code editing GUI.

## Installation

* First install [https://github.com/QGCoder/libqgcodeeditor](https://github.com/QGCoder/libqgcodeeditor), a Qt5 / Qt6 designer widget plugin for editing G-code.

* Next: clone, build and run ```qgcoder``` as follows:
```bash
gh repo clone QGCoder/qgcoder && cd qgcoder
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
./build/qgcoder
```
or build and install a Ubuntu / Debian package as follows:
```bash
gh repo clone QGCoder/qgcoder && cd qgcoder
mk-build-deps -i -s sudo -t "apt --yes --no-install-recommends"
dpkg-buildpackage -b -rfakeroot -us -uc
sudo dpkg -i ../qgcoder*.deb
sudo apt -f install
```

– Tested with Ubuntu 24.04 LTS and Ubuntu 26.04 LTS - [![CI](https://github.com/QGCoder/qgcoder/actions/workflows/main.yml/badge.svg)](https://github.com/QGCoder/qgcoder/actions/workflows/main.yml)

## Overview

When started first, you have to provide ```qgcoder``` three filenames, as seen in the following screenshot:

<img src="https://raw.githubusercontent.com/QGCoder/qgcoder/master/doc/qgcoder-002.png"/>


A short [YouTube video](https://www.youtube.com/watch?v=9D3hMXP5-QM) shows, how you can interact inside ```qgcoder```.

## Author

* **Jakob Flierl** - [koppi](https://github.com/koppi) - main application (`main.cpp`, `mainwin.*`, `settings_dlg.*`, `view.*`, packaging) - [GPL-2.0](LICENSE)

## Contributors

* **ArcEye** (Mick) - [ArcEye](https://github.com/ArcEye) - large file handling, settings dialog and menu wiring in `mainwin.*` / `settings_dlg.*` (2016) - [GPL-2.0](LICENSE)
* **Mark Pictor** - `g2m` G-code-to-mesh interpreter core (`canonLine`, `canonMotion`, `canonMotionless`, `helicalMotion`, `linearMotion`, `machineStatus`, `nanotimer`, `lex_analyzer`, 2010) - [GPL-2.0-or-later](g2m/canonLine.hpp)
* **Anders Wallin** - [aewallin](https://github.com/aewallin) - modifications to `g2m/g2m.hpp` / `g2m/g2m.cpp` (2011) - [GPL-2.0-or-later](g2m/g2m.hpp)
* **Kazuyasu Hamada** - modifications to `g2m/g2m.hpp` and the `g2m/gplayer.*` G-code player (2015) - [GPL-2.0-or-later](g2m/g2m.hpp)
