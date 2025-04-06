#!/usr/bin/env bash

# It clones the repository, builds the packages and archives them.
[ ! -f "${HOME}/.dockerenv" ] && echo ".dockerenv not present, exiting now." && exit
REPO_DIR="${HOME}/${REPO_NAME}"

if [ $# -eq 1 ] && [ -f "${REPO_DIR}$1" ]; then
  CHECKSUM="${CHECKSUM}$(cat $1 | shasum -a 256)"
  echo "${CHECKSUM}" && exit 0
fi

if [ $# -eq 1 ] && [ -d "${REPO_DIR}$1" ]; then
  CHECKSUM=""
  DIRECTORY=$1
  for file in ${DIRECTORY}; do
      # shellcheck disable=SC2002
      CHECKSUM="${CHECKSUM}$(cat "$file" | shasum -a 256)"
  done
  echo "${CHECKSUM}" | shasum -a 256 && exit 0
fi

if [[ $1 = *"*"* ]]; then
    CHECKSUM=""
    PATTERN=$1
    for file in ${HOME}/${REPO_DIR}${PATTERN}; do
        # shellcheck disable=SC2002
        CHECKSUM=${CHECKSUM}$(cat "$file" | shasum -a 256)
    done
      echo "${CHECKSUM}" | shasum -a 256 && exit 0
fi
