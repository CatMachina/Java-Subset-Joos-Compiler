#!/usr/bin/bash

git log --sparse --full-history > a1.log

zip -r a1.zip . -x ".gitignore" ".vscode/*" ".git/*"