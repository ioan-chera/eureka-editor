#!/bin/bash
set -e

function process() {
    INFILE="${3}"
    OUTFILE="pm/${2}"
    export PM_BASE="${1}"
    export PM_FILE="${INFILE}"
    echo ==== ${OUTFILE} ====
    cat ${INFILE} |
        awk '{ gsub(/:kbd:/, ":sub:") } { print }' |
        awk '{ gsub(/:download:/, ":sup:") } { print }' |
        pandoc -f RST -t pmwiki.lua > ${OUTFILE}
}

process "index"          "User.Index"        "source/index.rst"
process "management"     "User.Projects"     "source/project-management/index.rst"
process "concepts"       "User.Concepts"     "source/mapping-concepts/index.rst"
process "installation"   "User.Installation" "source/installation.rst"
process "basics"         "User.Basics"       "source/basics/index.rst"
process "user_interface" "User.UI"           "source/user-interface/index.rst"
process "intro"          "User.Intro"        "source/introduction.rst"

process "ck_prefabs"   "Cookbook.Prefabs"      "source/cookbook/prefabs/index.rst"
process "ck_traps"     "Cookbook.Traps"        "source/cookbook/traps/index.rst"
process "ck_stairs"    "Cookbook.Stairs"       "source/cookbook/stairs/index.rst"
process "ck_altar"     "Cookbook.Altar"        "source/cookbook/altar/index.rst"
process "ck_sky"       "Cookbook.Sky"          "source/cookbook/sky/index.rst"
process "ck_teleport"  "Cookbook.Teleporters"  "source/cookbook/teleporters/index.rst"
process "ck_doors"     "Cookbook.Doors"        "source/cookbook/doors/index.rst"
process "ck_lifts"     "Cookbook.Lifts"        "source/cookbook/lifts/index.rst"
process "ck_pool"      "Cookbook.ToxicPool"    "source/cookbook/toxic-pool/index.rst"
process "ck_curve"     "Cookbook.CurvedStairs" "source/cookbook/curved-stairs/index.rst"

