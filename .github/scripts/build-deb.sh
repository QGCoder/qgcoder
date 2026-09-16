#!/bin/bash
# Little build loop for the .deb based release jobs. It runs inside the target
# distribution's own container image (debian:*, ubuntu:*), where the workspace
# is mounted at /build; the resulting packages land in /build/artifacts so the
# GitHub host can upload them.
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive
export DEBEMAIL="${DEBEMAIL:?DEBEMAIL must be set}"
export DEBFULLNAME="${DEBFULLNAME:?DEBFULLNAME must be set}"

# The image's codename doubles as both the changelog distribution and the
# ~<codename>1 version suffix, exactly like the Debian/Ubuntu release train
# uses "$(. /etc/lsb-release && echo $DISTRIB_CODENAME)".
TARGET="$(. /etc/os-release && echo "${VERSION_CODENAME}")"

# The base images ship with the deb-src entries commented out.
if ls /etc/apt/sources.list.d/*.sources >/dev/null 2>&1; then
    sed -Ei 's/^Types: deb$/Types: deb deb-src/' /etc/apt/sources.list.d/*.sources
else
    sed -Ei 's/^# deb-src/deb-src/' /etc/apt/sources.list
fi
apt-get update -qq
apt-get -qq -y install --no-install-recommends \
    ca-certificates devscripts equivs fakeroot git lintian

rm -rf /work
mkdir -p /work /build/artifacts

# libqgcodeeditor is not packaged anywhere but Debian/Ubuntu, and only those
# flavours that still carry it, so build a fresh .deb of it from upstream.
git clone --depth 1 https://github.com/QGCoder/libqgcodeeditor /work/libqgcodeeditor
cd /work/libqgcodeeditor
git fetch --tags --quiet
# its debian/rules runs `dh_makeshlibs --add-udeb shlibs:Depends`
# unconditionally, and the bookworm-era debhelper refuses to when there is no
# udeb to write Depends for; the flag is harmless everywhere else.
sed -i 's/dh_makeshlibs --add-udeb shlibs:Depends/dh_makeshlibs/' debian/rules
VERSION="$(git tag -l | tail -n1 | sed -e "s/^v//" -e "s/-/+git/")"
DEBEMAIL="${DEBEMAIL}" dch --create \
    --distribution "${TARGET}" \
    --package libqgcodeeditor \
    --newversion "${VERSION}~${TARGET}1" \
    "Automatic release build"
mk-build-deps --install --tool "apt-get --yes --no-install-recommends"
dpkg-buildpackage -b -rfakeroot -us -uc
cd /work
# one of the libqgcodeeditor packages (the -doc one) depends on a doc helper
# (libjs-jquery) that is not installed yet; let apt resolve and configure
# rather than letting dpkg -i abort the build midway.
dpkg -i libqgcodeeditor*.deb || true
apt-get -qq -y -f install

# qgcoder itself: copy the checked-out tree so dpkg-buildpackage can write its
# outputs to /work (the container root, not the /build checkout), ours and
# libqgcodeeditor's .debs both land there.
cp -a /build /work/qgcoder-src
cd /work/qgcoder-src
# the copy keeps the host's file ownership, so root's git would decline to
# touch the repository it does not own
git config --global --add safe.directory /work/qgcoder-src
git fetch --tags --quiet
VERSION="$(git tag -l | tail -n1 | sed -e "s/^v//" -e "s/-/+git/")"
DEBEMAIL="${DEBEMAIL}" dch --create \
    --distribution "${TARGET}" \
    --package qgcoder \
    --newversion "${VERSION}~${TARGET}1" \
    "Automatic release build"
dpkg --configure -a || true
apt-get -qq -y -f install
mk-build-deps --install --tool "apt-get --yes --no-install-recommends"
# Debian ships the Qt 6 OpenGL dev files in their own package that qt6-base-dev
# merely Recommends; mk-build-deps --no-install-recommends does not pull those
# in, and qgcoder's CMake needs Qt6::OpenGL/OpenGLWidgets. Ubuntu vendors the
# files inside qt6-base-dev itself, so this only triggers on Debian.
dpkg -S Qt6OpenGLConfig.cmake >/dev/null 2>&1 || apt-get -qq -y install libqt6opengl6-dev
dpkg-buildpackage -b -rfakeroot -us -uc

# only the build outputs - the *-build-deps metapackages stay in their source
# directories inside /work and never make it here.
cp /work/*.deb /work/*.buildinfo /work/*.changes /build/artifacts/ 2>/dev/null || true