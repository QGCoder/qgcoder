<img src="https://raw.githubusercontent.com/QGCoder/gcoder/master/doc/gcoder-001.png"/>

An interactive G-code editing GUI.

## Installation

* First install the [https://github.com/QGCoder/libqgcodeeditor](https://github.com/QGCoder/libqgcodeeditor), a Qt5 designer widget plugin for editing G-code.

* Next: clone, build and run gcoder as follows:
```bash
gh repo clone QGCoder/gcoder
cd gcoder
qmake
make
./gcoder
```

Tested on Ubuntu 20.04. -- ![CI](https://github.com/QGCoder/gcoder/actions/workflows/main.yml/badge.svg)](https://github.com/QGCoder/gcoder/actions/workflows/main.yml)

## Overview

To be written, see proof of concept [Video](https://www.youtube.com/watch?v=9D3hMXP5-QM).

To get a quick random test G-Code file opened in GCoder type into the Bash text field:
```bash
tests/ngc-urandom.sh
```

## License

See [LICENSE](https://github.com/QGCoder/gcoder/blob/master/LICENSE).

