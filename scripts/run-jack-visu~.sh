#!/bin/sh -e
exec "`dirname "$0"`/visu~-jack" -t "`basename "$0"`" "$@"
