# Git Terminal Setup

This workspace is structured so you can initialize it with Git later and push it to GitHub from a terminal.

## Before You Start

- Install Git for Windows if `git` is not available in your terminal.
- Create an empty repository on GitHub first, or be ready to create one after `git init`.

## Recommended Terminal Flow

From the project root:

```powershell
git init
git add .
git commit -m "Initial ESP32-S3 rover project"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO.git
git push -u origin main
```

If you prefer SSH:

```powershell
git remote add origin git@github.com:YOUR_USERNAME/YOUR_REPO.git
git push -u origin main
```

## Useful Checks

```powershell
git status
git remote -v
git log --oneline
```

## What Is Already Prepared

- PlatformIO build artifacts are ignored
- VS Code generated files are ignored
- Project documentation is present
- Hardware BOM and integration notes are present
- Firmware source is in a publishable structure

## Important Note About The Board Profile

This project now uses a local custom PlatformIO board definition at `boards/esp32-s3-n16r8.json`.

That keeps the repository self-contained and aligned with your `ESP32-S3-N16R8` hardware instead of relying on a mismatched generic `8MB / no PSRAM` board profile.
