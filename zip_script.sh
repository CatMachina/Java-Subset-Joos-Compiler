#!/usr/bin/bash

git log --sparse --full-history > a2.log

zip -r a2.zip . -x ".gitignore" ".vscode/*" ".git/*"