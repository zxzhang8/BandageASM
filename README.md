# <img src="http://rrwick.github.io/Bandage/images/logo.png" alt="Bandage" width="115" height="115" align="middle">BandageASM (fork)

## Overview
BandageASM is a fork of original Bandage repo It draws contigs as nodes with their connections, lets you label/colour/move nodes, and extract sequences directly from the graph. More info and binaries live upstream: https://github.com/rrwick/Bandage.

## Fork additions
- **GAF path visualisation**: import `.gaf` files, list alignments in their own tab, inspect details, and select the corresponding paths on the drawn graph using the standard graph-selection style. Selecting another path or graph node replaces the previous selection, and **Clear selection** removes it. One GAF tab is kept at a time; loading another valid file asks before replacement and defaults to keeping the current file. Invalid imports leave the current GAF state unchanged.
- **GAF paths performance**: the GAF tab now uses a paged table view with configurable page size and direct page jump, plus multi-node filtering with Any/All/Contained matching. **Any** accepts a path containing at least one filter node, **All** requires every filter node to occur in the path, and **Contained** requires the path to contain at least one filter node while every node in that path belongs to the filter set. Filtered results can be saved as GAF across all pages, not just the visible page.
- **GAF support heatmap**: visualise every alignment in the complete current GAF filter result on the main graph. Nodes and traversed edges are coloured by support using Log or Linear scaling, counted either by alignment record or unique query. Hovering shows exact support; filtering marks an active heatmap as out of date until it is explicitly refreshed.
- **Selected-edge Gen Seq**: when edges are selected, a **Gen Seq** button appears to validate that they form one unambiguous path; errors report branching/disconnected nodes. For valid paths, a tab shows the ordered walk with exports:
  - **FASTA** if all nodes have sequence.
  - **GAF** always available to record the walk.
- **Selected-node path search**: from the Selection panel you can find paths that connect two chosen nodes within the selected-node set, inspect results in a tab, select paths on the graph using the standard graph-selection style, and export a single path to FASTA. In addition to the original search, a coverage-only **Beam search** accepts any numeric GFA segment tag (defaulting to `rd` when available), while read- and coverage-aware path optimization (**RCAP**) uses either all loaded GAF alignments or the complete current filtered result set. Both run in the background and can be cancelled. RCAP also provides an **Advanced configure** tab for evidence filtering, coverage fitting, base objective weights, context lengths, and the solver seed.
- **Node context menu**: right-click a node to show its name, open its sequence in a tab, or set it as the Start/End for selected-node path search.
- **GFA tag node labels**: optional attributes from GFA segment records can be selected from the Node labels controls and displayed on graph nodes.
- **Selection mode**: a toggle in "Find paths in selection" keeps current selections when clicking empty space, so you can inspect without accidentally clearing nodes/paths.
- **Layout import/export**: **File → Save layout** stores the complete visible-node set, node polylines, and single/double display mode in a compact BandageASM v2 `.layout` file. **File → Load layout** restores that snapshot after the matching graph is loaded. A SHA-256 fingerprint covering graph topology and sequence content prevents applying a layout to a different graph. Version 1 layout files are intentionally rejected; viewport, colours, labels, and GAF tabs are not included.
- **BED annotations**: load BED3-BED12 records from **Graph controls → BED annotations** and draw node-local feature intervals, thick regions, BED12 blocks, and optional names directly on graph nodes. The BED chromosome field is matched to a graph node name, coordinates are zero-based and half-open within that node, and the strand field selects the node orientation. A successful load replaces the previous BED file; malformed, unmatched, and out-of-range records are skipped and summarized, while a failed load preserves the current annotations. This feature was designed with explicit reference to the BED annotation implementation in **BandageNG**, then implemented independently in BandageASM with stricter validation, transactional replacement, separate display controls, and diagnostic summaries.

## BED annotations

Load a graph, optionally draw it, then click **Load BED** under **BED annotations**. Use the **Interval**, **Thick**, **Blocks**, and **Text** checkboxes to independently control the displayed layers. BED item RGB colours are used when present; otherwise a blue default is used. **Clear** removes the loaded annotations without changing the graph.

The node-local BED interpretation and the three graphical layers—whole interval, thick region, and BED12 blocks—are based on the corresponding feature in **BandageNG**. BandageASM does not depend on BandageNG at build time or runtime: its parser, storage, controls, rendering integration, validation, and error handling are implemented in the main BandageASM source tree. Unlike the referenced BandageNG loader, the BandageASM loader also accepts comment lines beginning with `#` and UCSC-style `track` and `browser` lines.

An example for `tests/test_plasmids.gfa` is provided at [`tests/bed_annotations_example.bed`](tests/bed_annotations_example.bed). Load the GFA first, draw it, and then load the BED file.

## Finding paths through local tangles

Select the nodes that define the local tangle, choose distinct **Start** and **End** nodes in **Find paths in selection**, and then choose an algorithm:

- **Standard** performs the original selected-subgraph path search.
- **Beam search** uses graph connectivity and a numeric per-node coverage tag. It does not require a GAF file. `rd` is selected automatically when it is available on every selected node.
- **RCAP** combines graph connectivity, node coverage, and read-to-graph alignments and solves the resulting model with OR-Tools CP-SAT. A GAF file must be loaded, and **GAF evidence** can use either all loaded alignments or all alignments remaining after the current GAF filters. Evidence extraction is not limited to the currently displayed GAF page.

For RCAP, click **Advanced configure** below the algorithm selector to open a separate configuration tab. The formula shown at the top describes the implemented objective and identifies where the configurable parameters are used. Click the blue information icon beside a parameter for a description of its role and the effect of changing it.

The advanced settings include coverage dispersion and Huber loss, the RCAP minimum coverage ratio, full-thread and context evidence fractions, minimum and maximum context length, alignment-score retention, base coverage/read objective weights, and the CP-SAT solver seed. RCAP dynamically adjusts the two base objective weights from the mean retained-read confidence while preserving their total. Read evidence is weighted using alignment score, identity, mapping coverage, and mapping ambiguity; repeated context matches are capped by their expected count. **Restore defaults** fills the form with built-in defaults. **Confirm** applies the values, closes the configuration tab, and returns to **Graph & controls**; **Cancel** closes the tab without applying edits. A yellow exclamation mark beside **Advanced configure** indicates that confirmed values differ from the defaults.

Beam search and RCAP run in a cancellable progress dialog. Successful candidates are opened in **Selected node paths**, where one or more rows can be highlighted without permanently changing the underlying node style. A single selected result can be exported to FASTA.

## Why it helps genome assembly work
- Map multiple assemblies or reference genome onto the assembly graph to guide the consensus generation.
- Turn highlighted graph walks into reusable sequences/paths for polishing, validation, or downstream comparison.

For the full original documentation and releases, please see the original Bandage project.

## Build dependency

The main application requires the OR-Tools C++ distribution for the RCAP path finder's CP-SAT solver. Set `ORTOOLS_ROOT` to a matching installation before running qmake; the directory must contain `include/` and `lib/`:

```sh
qmake Bandage.pro ORTOOLS_ROOT=/path/to/or-tools
```

# Contact

If you have any questions or wish to add new features, feel free to contact: zexinzhang@stu.xjtu.edu.cn .
