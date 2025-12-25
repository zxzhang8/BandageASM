# <img src="http://rrwick.github.io/Bandage/images/logo.png" alt="Bandage" width="115" height="115" align="middle">Bandage (fork)

## Overview
Bandage is a GUI for viewing assembly graphs from *de novo* assemblers (e.g. Velvet, SPAdes, MEGAHIT). It draws contigs as nodes with their connections, lets you label/colour/move nodes, and extract sequences directly from the graph. More info and binaries live upstream: https://github.com/rrwick/Bandage and http://rrwick.github.io/Bandage/.

## Fork additions
- **GAF path visualisation**: import `.gaf` files, list alignments in their own tab, inspect details, and highlight the corresponding paths on the drawn graph.
- **GAF paths performance**: the GAF tab now uses a paged table view with configurable page size and direct page jump, plus multi-node filtering with Any/All matching.
- **Selected-edge Gen Seq**: when edges are selected, a **Gen Seq** button appears to validate that they form one unambiguous path; errors report branching/disconnected nodes. For valid paths, a tab shows the ordered walk with exports:
  - **FASTA** if all nodes have sequence.
  - **GAF** always available to record the walk.
- **Selected-node path search**: from the Selection panel you can find paths that connect two chosen nodes within the selected-node set, inspect results in a tab, highlight paths on the graph, and export a single path to FASTA.
- **Node context menu**: right-click a node to show its name, open its sequence in a tab, or set it as the Start/End for selected-node path search.

## Why it helps genome assembly work
- Map multiple assemblies or reference genome onto the assembly graph to guide the consensus generation.
- Turn highlighted graph walks into reusable sequences/paths for polishing, validation, or downstream comparison.

For the full original documentation and releases, please see the original Bandage project.
