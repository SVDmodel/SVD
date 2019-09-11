# SVD outputs
## List of outputs
* [StateGrid](#StateGrid)
* [ResTimeGrid](#ResTimeGrid)
* [StateChange](#StateChange)
* [StateHist](#StateHist)
* [Fire](#Fire)

<a name="StateGrid"></a>
## StateGrid
writes ASCII grids with the stateId for each cell.

Grids are save to the location specified with the `path`property (`$year$` is replaced with the actual year).
### Parameters
* `interval`: output is written only every `interval` years (or every year if `interval=0`). For example, a value of 10 limits output to the simulation years 1, 11, 21, ...



<a name="ResTimeGrid"></a>
## ResTimeGrid
output of ASCII grids with the residence time (years) for each cell.
Grids are save to the location specified with the `path`property (`$year$` is replaced with the actual year).
### Parameters
* `interval`: output is written only every `interval` years (or every year if `interval=0`). For example, a value of 10 limits output to the simulation years 1, 11, 21, ...


<a name="StateChange"></a>
## StateChange
Details for individual state changes from DNN (potentially a lot of output data!)

The output contains for each cell the predicted states/probabilities (for `dnn.topKNClasses` classes), and the probabilities for the year of state change.

### Parameters
* `filter`: a filter expression; output is written if the expression is true; available variables are: `state`, `restime`, `x`, `y`, `year`
* `interval`: output is written only every `interval` years (or every year if `interval=0`). For example, a value of 10 limits output to the simulation years 1, 11, 21, ...


### Columns
Column|Description|Data type
------|-----------|---------
year | simulation year of the state change | Int
cellIndex | index of the affected cell (0-based) | Int
state | original stateId (before change) | Int
restime | residence time (yrs) (before change) | Int
nextState | new stateId (selected from s[i]) | Int
nextTime | year the state change (selected from t[i]) | Int
s[i] | candidate state Id *i* (with i 1..number of top-K classes) | Int
p[i] | probability for state *i* (0..1) | Double
t[i] | probability for a change in year *i* (with i 1..number of residence time classes) | Double


<a name="StateHist"></a>
## StateHist
Outputs a frequency distribution of states over the landscape.

### Parameters
 * not yet

### Columns
Column|Description|Data type
------|-----------|---------
year | simulation year | Int
state | stateId | Int
n | number of cells that are currently in the state `state` | Int


<a name="Fire"></a>
## Fire
Output on fire events (one event per line) and grids for the year of the last burn.

Grids are saved as ASCII grids to the location specified by the`lastFireGrid.path` property (`$year$` is replaced with the actual year). The value of the grid cells is the year of the last burn in a cell or 0 for unburned cells.

### Parameters
 * `lastFireGrid.filter`: a grid is written only if the expression evaluates to `true` (with `year` as variable). A value of 0 deactivates the grid output.

### Columns
Column|Description|Data type
------|-----------|---------
year | simulation year of the fire event | Int
id | unique identifier of a single fire | Int
x | x coordinate (m) of the ignition point | Double
y | y coordinate (m) of the ignition point | Double
planned_size | planned fire size (ha) | Double
realized_size | realized fire size (ha) | Double
share_high_severity | share of pixels burning with high severity (0..1) | Double



