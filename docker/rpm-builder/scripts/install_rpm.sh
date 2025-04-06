#!/usr/bin/env bash

# It will run DNF install.
[ ! -f "${HOME}/.dockerfile" ] && echo ".dockerfile not present, exiting now." && exit
cd "${HOME}/.rpms" || echo "${HOME}/.rpms not found, exiting now." && exit

PACKAGES=""
for file in ./*.rpm; do
    PACKAGES="${PACKAGES} $file"
done
dnf -y install "${PACKAGES}"
