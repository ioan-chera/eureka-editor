#!/bin/bash
set -e

function process() {
    INFILE="${2}"
    OUTFILE="${1}.pm"
    export PM_BASE="${1}"
    echo ==== ${OUTFILE} ====
    cat ${INFILE} |
        awk '{ gsub(/:kbd:/, ":sub:") } { print }' |
        awk '{ gsub(/:download:/, ":sup:") } { print }' |
        pandoc -f RST -t pmwiki.lua > ${OUTFILE}
}

process "management" "source/project-management/index.rst"
process "concepts" "source/mapping-concepts/index.rst"
process "index" "source/index.rst"
process "installation" "source/installation.rst"
process "basics" "source/basics/index.rst"
process "user_interface" "source/user-interface/index.rst"
process "intro" "source/introduction.rst"

process "ck_prefabs" "source/cookbook/prefabs/index.rst"
process "ck_index" "source/cookbook/index.rst"
process "ck_traps" "source/cookbook/traps/index.rst"
process "ck_stairs" "source/cookbook/stairs/index.rst"
process "ck_altar" "source/cookbook/altar/index.rst"
process "ck_sky" "source/cookbook/sky/index.rst"
process "ck_teleport" "source/cookbook/teleporters/index.rst"
process "ck_doors" "source/cookbook/doors/index.rst"
process "ck_lifts" "source/cookbook/lifts/index.rst"
process "ck_pool" "source/cookbook/toxic-pool/index.rst"
process "ck_curve" "source/cookbook/curved-stairs/index.rst"

