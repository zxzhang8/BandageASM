# <img src="http://rrwick.github.io/Bandage/images/logo.png" alt="Bandage" width="115" height="115" align="middle">BandageASM (fork)

## Overview
BandageASM is a GUI for viewing assembly graphs. It draws contigs as nodes with their connections, lets you label/colour/move nodes, and extract sequences directly from the graph. More info and binaries live upstream: https://github.com/rrwick/Bandage and http://rrwick.github.io/Bandage/.

## Fork additions
- **GAF path visualisation**: import `.gaf` files, list alignments in their own tab, inspect details, and highlight the corresponding paths on the drawn graph.
- **GAF paths performance**: the GAF tab now uses a paged table view with configurable page size and direct page jump, plus multi-node filtering with Any/All matching.
- **Selected-edge Gen Seq**: when edges are selected, a **Gen Seq** button appears to validate that they form one unambiguous path; errors report branching/disconnected nodes. For valid paths, a tab shows the ordered walk with exports:
  - **FASTA** if all nodes have sequence.
  - **GAF** always available to record the walk.
- **Selected-node path search**: from the Selection panel you can find paths that connect two chosen nodes within the selected-node set, inspect results in a tab, highlight paths on the graph, and export a single path to FASTA.
- **Node context menu**: right-click a node to show its name, open its sequence in a tab, or set it as the Start/End for selected-node path search.
- **Selection mode**: a toggle in "Find paths in selection" keeps current selections when clicking empty space, so you can inspect without accidentally clearing nodes/paths.

## Why it helps genome assembly work
- Map multiple assemblies or reference genome onto the assembly graph to guide the consensus generation.
- Turn highlighted graph walks into reusable sequences/paths for polishing, validation, or downstream comparison.

For the full original documentation and releases, please see the original Bandage project.

## Windows deployment
If you build with Qt 6.10.1 MinGW 64-bit, do not distribute only `Bandage.exe`. The executable depends on Qt DLLs, platform plugins, and MinGW runtime libraries that must be packaged together.

After building the Release target on Windows, run:

```bat
build_scripts\deploy_windows_qt6_mingw64.bat
```

The script:
- copies `build\Desktop_Qt_6_10_1_MinGW_64_bit-Release\release\Bandage.exe` into a clean distribution folder
- runs `windeployqt --release --compiler-runtime`
- copies common MinGW runtime DLLs if needed
- creates `Bandage_Windows_Qt6_MinGW64.zip`

Distribute the generated zip and ask users to extract the full archive before running `Bandage.exe`.
