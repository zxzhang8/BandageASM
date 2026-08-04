# <img src="http://rrwick.github.io/Bandage/images/logo.png" alt="Bandage" width="115" height="115" align="middle">BandageASM (fork)

## Overview
BandageASM is a fork of original Bandage repo It draws contigs as nodes with their connections, lets you label/colour/move nodes, and extract sequences directly from the graph. More info and binaries live upstream: https://github.com/rrwick/Bandage.

## Fork additions
- **GAF path visualisation**: import `.gaf` files, list alignments in their own tab, inspect details, and select the corresponding paths on the drawn graph using the standard graph-selection style. Selecting another path or graph node replaces the previous selection, and **Clear selection** removes it. One GAF tab is kept at a time; loading another valid file asks before replacement and defaults to keeping the current file. Invalid imports leave the current GAF state unchanged.
- **GAF paths performance**: the GAF tab now uses a paged table view with configurable page size and direct page jump, plus multi-node filtering with Any/All/Contained matching. **Any** accepts a path containing at least one filter node, **All** requires every filter node to occur in the path, and **Contained** requires the path to contain at least one filter node while every node in that path belongs to the filter set. Filtered results can be saved as GAF across all pages, not just the visible page.
- **Selected-edge Gen Seq**: when edges are selected, a **Gen Seq** button appears to validate that they form one unambiguous path; errors report branching/disconnected nodes. For valid paths, a tab shows the ordered walk with exports:
  - **FASTA** if all nodes have sequence.
  - **GAF** always available to record the walk.
- **Selected-node path search**: from the Selection panel you can find paths that connect two chosen nodes within the selected-node set, inspect results in a tab, select paths on the graph using the standard graph-selection style, and export a single path to FASTA. In addition to the original search, a coverage-only **Beam search** accepts any numeric GFA segment tag (defaulting to `rd` when available), while read-aware **CP-SAT** uses either all loaded GAF alignments or the complete current filtered result set. Both run in the background and can be cancelled. CP-SAT also provides an **Advanced configure** tab for evidence filtering, coverage fitting, objective weights, context lengths, and the solver seed.
- **Node context menu**: right-click a node to show its name, open its sequence in a tab, or set it as the Start/End for selected-node path search.
- **GFA tag node labels**: optional attributes from GFA segment records can be selected from the Node labels controls and displayed on graph nodes.
- **Selection mode**: a toggle in "Find paths in selection" keeps current selections when clicking empty space, so you can inspect without accidentally clearing nodes/paths.
- **Layout import/export**: **File → Save layout** stores the complete visible-node set, node polylines, and single/double display mode in a compact BandageASM v2 `.layout` file. **File → Load layout** restores that snapshot after the matching graph is loaded. A SHA-256 fingerprint covering graph topology and sequence content prevents applying a layout to a different graph. Version 1 layout files are intentionally rejected; viewport, colours, labels, and GAF tabs are not included.

## Finding paths through local tangles

Select the nodes that define the local tangle, choose distinct **Start** and **End** nodes in **Find paths in selection**, and then choose an algorithm:

- **Standard** performs the original selected-subgraph path search.
- **Beam search** uses graph connectivity and a numeric per-node coverage tag. It does not require a GAF file. `rd` is selected automatically when it is available on every selected node.
- **CP-SAT** combines graph connectivity, node coverage, and read-to-graph alignments. A GAF file must be loaded, and **GAF evidence** can use either all loaded alignments or all alignments remaining after the current GAF filters. Evidence extraction is not limited to the currently displayed GAF page.

For CP-SAT, click **Advanced configure** below the algorithm selector to open a separate configuration tab. The formula shown at the top describes the implemented objective and identifies where the configurable parameters are used. Click the blue information icon beside a parameter for a description of its role and the effect of changing it.

The advanced settings include coverage dispersion and Huber loss, full-thread and context evidence fractions, minimum and maximum context length, alignment-score retention, coverage/read objective weights, and the CP-SAT random seed. **Restore defaults** fills the form with built-in defaults. **Confirm** applies the values, closes the configuration tab, and returns to **Graph & controls**; **Cancel** closes the tab without applying edits. A yellow exclamation mark beside **Advanced configure** indicates that confirmed values differ from the defaults.

Beam search and CP-SAT run in a cancellable progress dialog. Successful candidates are opened in **Selected node paths**, where one or more rows can be highlighted without permanently changing the underlying node style. A single selected result can be exported to FASTA.

## Why it helps genome assembly work
- Map multiple assemblies or reference genome onto the assembly graph to guide the consensus generation.
- Turn highlighted graph walks into reusable sequences/paths for polishing, validation, or downstream comparison.

For the full original documentation and releases, please see the original Bandage project.

## Build dependency

The main application requires the OR-Tools C++ distribution for the CP-SAT path finder. Set `ORTOOLS_ROOT` to a matching installation before running qmake; the directory must contain `include/` and `lib/`:

```sh
qmake Bandage.pro ORTOOLS_ROOT=/path/to/or-tools
```
