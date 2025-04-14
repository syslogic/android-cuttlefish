#!/usr/bin/env bash

# It will run DNF install.
[ ! -f "/root/.dockerenv" ] && echo ".dockerenv not present, exiting now." && exit 1
[ ! -d "/root/.rpms" ] && echo "/root/.rpms not found, exiting now." && exit 1
cd "/root/.rpms" || exit 1

for package in ./cuttlefish-$1-*.rpm; do
  dnf -y --skip-broken install "${package}"
done
