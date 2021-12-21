<img src="https://raw.githubusercontent.com/QGCoder/qgcoder/master/doc/gcoder-001.png"/>

An interactive G-code editing GUI.

## Installation

* First install [https://github.com/QGCoder/libqgcodeeditor](https://github.com/QGCoder/libqgcodeeditor), a Qt5 designer widget plugin for editing G-code.

* Next: clone, build and run qgcoder as follows:
```bash
gh repo clone QGCoder/qgcoder
cd qgcoder
qmake
make
./qgcoder
```

Tested to run on Ubuntu 20.04. - [![CI](https://github.com/QGCoder/qgcoder/actions/workflows/main.yml/badge.svg)](https://github.com/QGCoder/qgcoder/actions/workflows/main.yml)

## Overview

To be written, see proof of concept [Video](https://www.youtube.com/watch?v=9D3hMXP5-QM).

To get a quick random test G-code file opened in qgcoder type into the Bash text field:
```bash
tests/ngc-urandom.sh
```

## Authors

* [@koppi](https://github.com/koppi)

