#!/usr/bin/bash

git log --sparse --full-history > a4.log

zip -r a4.zip . -x ".gitignore" ".vscode/*" ".git/*"