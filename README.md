# p4c Code Metrics Extension

This repository contains the implementation of an extension to the frontend of the [p4c](https://github.com/p4lang/p4c) compiler. This extension consists from a set of compiler passes that collect detailed code metrics from P4-16 programs. Implementation of this extension was a part of my [bachelor's thesis](https://www.vut.cz/studenti/zav-prace/detail/161792). The majority of the implementation is located in [frontends/p4/metrics](frontends/p4/metrics). The final version of this implementation was merged into the production version of the p4c compiler.

---

## Program Structure


The p4c compiler consists of three stages:

- **Frontend**: Parses and type-checks the input P4 code and builds the Intermediate Representation (IR).
- **Midend**: Performs target-independent optimizations and transformations.
- **Backend**: Generates code for a specific target.

Each stage uses a sequence of passes that operate over the IR.

### Metrics Extension Overview

The metrics collection module is implemented as a set of standalone passes in the frontend stage. Each metric is implemented in its own `.cpp` file inside a new `frontends/metrics/` directory.

Passes are based on the `Inspector` class and follow the visitor pattern to traverse the IR without modifying it. Metric results are stored in a shared `Metrics` structure.

Metric collection is controlled via a new command-line argument: `--metrics`. Users can choose specific metrics or use the keyword `all`. Based on these arguments, the appropriate passes are conditionally executed.

At the end of the frontend stage, the selected metrics are exported into a JSON and a TXT file.

---

## Implementation Details

The passes which make up this extension and collect individual code metrics are:

#### General Metrics

- **LinesOfCodeMetricPass**: Counts unique source code lines.
- **CyclomaticComplexityPass**: Calculates cyclomatic complexity per parser/control/action.
- **HalsteadMetricsPass**: Computes vocabulary, length, effort, difficulty, and bug estimates.
- **UnusedCodeMetricPass**: Tracks unused declarations removed by the frontend.
- **NestingDepthMetricPass**: Measures nesting depth of control-flow constructs.

#### P4-Specific Metrics

- **InlinedActionsMetricPass**: Counts actions inlined by the compiler.
- **MatchActionTableMetricsPass**: Measures size and structure of match-action tables.
- **ParserMetricsPass**: Counts parser states and evaluates their complexity.
- **ExternalObjectsMetricPass**: Tracks usage of extern functions and structures.
- **HeaderMetricsPass**: Analyzes headers and field sizes.
- **HeaderPacketMetricsPass**: Collects per-packet header modifications and manipulations.

#### Export Pass

- **ExportMetricsPass**: Saves selected metrics to `*_metrics.txt` and `*_metrics.json`.

All passes are registered via the `MetricsPassManager` and inserted conditionally based on the user's command-line options.

---

## Usage

> For installation instructions and prerequisites, refer to the official [p4c GitHub repository](https://github.com/p4lang/p4c).

### Run the Compiler with Metrics

```p4c --metrics halstead,cyclomatic,header-general your_program.p4```

Collecting all metrics is also possible with the keyword "all"

```p4c --metrics all my_program.p4```

## Output

After compilation, the following files will be generated:

    program_name_metrics.txt – Human-readable summary

    program_name_metrics.json – Structured JSON for tools/scripts


## Testing

Ten automatic tests, which utilize the Google Test framework, were incorporated into the p4c unit testing framework. The tests are located in [test/gtest](test/gtest). To ensure the stability of the extension, additional batch testing was performed by compiling over 400 programs from the official test suite data.