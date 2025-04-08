#!/usr/bin/env bash

# It clones the repository.
[ ! -f "${HOME}/.dockerenv" ] && echo ".dockerenv not present, exiting now." && exit 1
[ $# -lt 2 ] && echo "REPO_USER and/or REPO_NAME missing, exiting now." && exit 1
[ $# -eq 2 ] && REPO_USER=$1 && REPO_NAME=$2

cd "${HOME}" && git clone "https://github.com/${REPO_USER}/${REPO_NAME}.git"
cd "${REPO_NAME}" || exit 1

# TODO: Line to be removed upon merging to `main` branch; the PR is `rpm-build`.
[ "${REPO_USER}" == "syslogic" ] && git switch rpmbuild
