#!/usr/bin/env bash

# It will run DNF install.
[ ! -f "${HOME}/.dockerenv" ] && echo ".dockerenv not present, exiting now." && exit 1
cd "${HOME}/.rpms" || echo "${HOME}/.rpms not found, exiting now." && exit 1

PACKAGES="nano"
for package in ${HOME}/.rpms; do PACKAGES="${PACKAGES} $package"; done
echo "Packages to install: ${PACKAGES}"
dnf -y install "${PACKAGES}"
