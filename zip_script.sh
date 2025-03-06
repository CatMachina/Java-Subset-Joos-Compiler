#!/usr/bin/bash

git log --sparse --full-history > a3.log

zip -r a3.zip . -x ".gitignore" ".vscode/*" ".git/*"