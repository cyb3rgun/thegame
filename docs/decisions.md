# Decisions

Numbered newest first.
## D-006 Commit authorship stays as is

Commit authorship stays as is. Existing history is never rewritten. The co-author trailer question is with Sascha; keep the trailer until told otherwise.

## D-005 Enforce LF via .gitattributes

Enforce LF via .gitattributes. The machine's core.autocrlf would otherwise create CRLF drift noise on every checkout and for every future contributor. "* text=auto" becomes the FIRST line of .gitattributes. The existing LFS binary patterns keep their explicit -text flags and win as the more specific rule. This lands now, before any Unreal files enter the repo.


## D-004 Game repo boundary

The game and its repository live exclusively in C:\Projects\CYB3RGUN\THEGAME. CC may create this folder and work inside it. Nothing else under C:\Projects\CYB3RGUN is created, modified or committed. Whether the parent folder is itself a git repo is checked read-only and only reported; any action on it is deferred to the architect.

## D-003 Docs trio

The docs trio mirrors the proven framework of the main project: game-concept.md as the living reference that only grows, decisions.md numbered newest first, seasons.md with the G-track table.

## D-002 Standard Unreal .gitignore

Standard Unreal .gitignore. Binaries, DerivedDataCache, Intermediate, Saved and IDE folders never enter version control.

## D-001 Git LFS from the very first commit

Git LFS from the very first commit. Unreal assets are binaries and GitHub hard-fails single files over 100 MB. Retrofitting LFS later rewrites history, so it starts now.
