#!/usr/bin/bash
ASSIGNMENT_ARG=$1
ASSIGNMENT=${ASSIGNMENT_ARG:="a6"}

rm $ASSIGNMENT.log $ASSIGNMENT.zip

git log --sparse --full-history > $ASSIGNMENT.log

zip -r $ASSIGNMENT.zip . -x ".gitignore" ".vscode/*" ".git/*"

/u8/cs_build/bin/marmoset_submit "cs444/644" "${ASSIGNMENT^^} code" $ASSIGNMENT.zip -u=-e32xu-dy5zhang-v57gupta-a259zhan-