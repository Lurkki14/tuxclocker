#!/usr/bin/env sh

cd "$(dirname "$0")"

cabal install . --overwrite-policy=always "$@"
