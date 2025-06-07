<img src="https://raw.githubusercontent.com/QGCoder/qgcoder/master/doc/qgcoder-001.png"/>

An interactive G-code editing GUI.

## Installation

* First install [https://github.com/QGCoder/libqgcodeeditor](https://github.com/QGCoder/libqgcodeeditor), a Qt5 designer widget plugin for editing G-code.

* Next: clone, build and run ```qgcoder``` as follows:
```bash
gh repo clone QGCoder/qgcoder && qgcoder
qmake && make -j$(nproc)
./qgcoder
```
or build and install a Ubuntu / Debian package as follows:
```bash
gh repo clone QGCoder/qgcoder && cd qgcoder
mk-build-deps -i -s sudo -t "apt --yes --no-install-recommends"
dpkg-buildpackage -b -rfakeroot -us -uc
sudo dpkg -i ../qgcoder*.deb
sudo apt -f install
```

– Tested with Ubuntu 24.04. - [![CI](https://github.com/QGCoder/qgcoder/actions/workflows/main.yml/badge.svg)](https://github.com/QGCoder/qgcoder/actions/workflows/main.yml)

## Overview

When started first, you have to provide ```qgcoder``` three filenames, as seen in the following screenshot:

<img src="https://raw.githubusercontent.com/QGCoder/qgcoder/master/doc/qgcoder-002.png"/>


A short [YouTube video](https://www.youtube.com/watch?v=9D3hMXP5-QM) shows, how you can interact inside ```qgcoder```.

## Author

* **Jakob Flierl** - [koppi](https://github.com/koppi)

## Contributors

* **ArcEye** - [ArcEye](https://github.com/ArcEye)
* **Mark Pictor**
* **Kazuyasu Hamada**
* **Anders Wallin** - [aewallin](https://github.com/aewallin)
