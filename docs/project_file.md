# Project file

The project file is the central [configuration file](SVD_data_formats.md) of a SVD project.
When referring to input files, a relative path to the folder with the project file may be provided.

## General settings

#### `logging.file` (filepath)
Location of the SVD log file. The file is overwritten when the model is created. See TODO
#### `logging.setup.level` (string)
The logging level during the startup of the model 
(possible levels with decreasing level of detail: `trace`, `debug`, `info`, `error`, `off`)
#### `logging.model.level` (string)
The logging level during a simulation for processes in the main model (see also `logging.setup.level').
#### `logging.dnn.level` (string)
The logging level during a simulation for the DNN (see also `logging.setup.level').
#### `model.multithreading` (boolean)
Multithreading is disabled if `false` (mainly for debugging) (default true)
#### `model.threads` (numeric)
number of threads used by the SVD model (without threads specifically for the DNN) (default 4)


## DNN specific settings

#### `dnn.threads` (numeric)
The number of threads used for DNN processing (default 2)
#### `dnn.count` (numeric)
Number of (parallel) DNNs that are used. Each instance uses the same network (`dnn.file`) (default: 1)
#### `dnn.batchSize` (numeric)
The size of a single "batch". Multiple cells are processed simultaneously by the DNN, and the batch size
indicates how many. Bigger batch sizes are usually processed faster, if batches are too large memory problems
might occur. Typical values are between 512 and 4096 (powers of 2 are not required)
#### `dnn.maxBatchQueue` (numeric)
SVD maintains a queue of batches that wait for DNN processing. `maxBatchQueue` indicates the maximum number
of batches in the queue. Larger numbers might increase parallelism, but require more memory. Typical values 
are between 4 - 100.
#### `dnn.file` (filepath)
The path of the "frozen" Deep Neural Network. See TODO...
#### `dnn.metadata` (filepath)
Configuration file that describes the meta data of the DNN (input tensors). See the [configuration page](configuring_dnn_metadata.md) for details.

#### `dnn.topKNClasses` (numeric)
SVD select the `topKNClasses` most likely states from the probability distribution over all states (topK-algorithm). 
See also: TODO
#### `dnn.topKGPU` (boolean)
The topK-Algorithm for selecting candidate states from all states can either run on GPU (`true`) or on CPU (`false`).
Default: true

#### `dnn.state.name` (string)
The name of the output tensor in the trained network for the future state of a cell.
#### `dnn.state.N` (numeric)
The number of classes (number of different states) in the output tensor for the future state of a cell.

#### `dnn.restime.name` (string)
The name of the output tensor in the trained network for the remaining residence time.
#### `dnn.restime.N` (numeric)
The number of classes (number of different durations) in the output tensor for the remaining residence time.

#### `dnn.allowStateChangeAtMaxTime` (boolean)
The setting controls whether the selected residence time controls the subsequent state change. If the
value is `false`, then both are linked: a maximal time means no state change, and a lower residence time
forces a state change (i.e. disallows the current state as future state). If the value is `true`, then 
no interaction between residence time and state is simulated (default: `false`).

## Model components
### States
#### `states.file` (filepath)
The [data table](SVD_data_formats.md) containing the available states. The required columns in the
 file are: `stateId`, `composition`, `structure`, `fct`, `type`. See TODO for details on setting up states.
#### `states.extraFile` (filepath)
If provided, SVD loads additional properties for each state from a table. See also "cell variables" [TODO]

### Landscape
#### `landscape.grid` (filepath)
The [raster file](SVD_data_formats.md) that defines the spatial extent of the simulated landscape 
(and the environemnt). See [here](configuring_the_landscape.md) for details.
#### `landscape.file` (filepath)
The [data table](SVD_data_formats.md) that defines the climatic and environmental properties of the landscape.
See [here](configuring_the_landscape.md) for details.

#### `initialState.mode` (string)
The setting define how the initial state of the vegetation is set up.
Possible values are:
* `file`: initial states are extracted from the table in `landscape.file`. The expected column names are `stateId` for the initial state, 
and `residenceTime` for the initial value of residence time.
* `grid`: the initial state of the landscape is retrieved from raster file (settings `initialState.stateGrid`, `initialState.residenceTimeGrid`) 
* `random`: creates a random landscape (state: 1 - max-state, residence time: 0-10)

#### `initialState.stateGrid` (filepath)
[Raster](SVD_data_formats.md) file with the initial state Id for each cell. Missing values are not allowed.

#### `initialState.residenceTimeGrid` (filepath)
[Raster](SVD_data_formats.md) file with the initial value for residence time for each cell. Missing values are not allowed.

### Climate
#### `climate.file` (filepath)
A [data table](SVD_data_formats.md) with climate data. The required columns are:
* climateId: The climate region (see [landscape configuration](configuring_the_landscape.md) )
* year: the year (e.g. 2010)
* all other columns are the "payload" and describe climatic indicators for the given year

#### `climate.sequence.enabled` (boolean)
If `true` SVD uses a pre-defined sequence of year (see `climate.sequence`). The first simulation year uses
the first year of the sequence. If `false` SVD starts the first simulation year with the first
year in the climate data and runs until the end.

#### `climate.sequence` (string)
The `climate.sequence` is a list of years (separated by `,`) that is sequentially processed. For example, if
the sequence starts with `1950,1967,1954,...`, the year 1950 is used for the first simulation year, 1967 for
the second, and so forth. 

#### `model.species` (string)
List of species codes that are available (comma separated). See also [Neighbors](configuring_dnn_metadata.md).

## Outputs
Output-specific settings start with `output.`. The general scheme is:
`output.<outputname>.<setting>`. The `outputname` specifies the type of the output; 
see the [output page](outputs.md) for details on available outputs.

Settings that are generally available are:
* `enabled` (boolean): if `true` the output is active
* `file` or `path`: path to the output-file (`file`) or path that defines the path for output file*s* (`path`).
* additional settings are available for specific outputs

Outputs are data tables (`,` separated text files), see the [output page](outputs.md) for details.
Example:
```
output.StateGrid.enabled = true
output.StateGrid.path = output/state_$year$.asc
output.StateGrid.interval = 20
``` 


## Modules
Settings that are specific to a module start with `modules.`. The general scheme is:
`modules.<module-name>.<setting> = <value>`, where the `module-name` can be freely chosen.

See the [module page](modules.md) for details.

Settings that are generally available are:
* `enabled` (boolean): if `true` the module is active
* `type` (string): specifies the module `type` (See [module page](modules.md) for available types)

Additional module settings are `type` specific, and are described on the respective module pages. Note
that multiple modules of the same type (but with different a `module-name`) can be used.

