# Fire module

The SVD fire module simulates wildfire on the base SVD resolution (100m). Fire spreads dynamically with a cellular automaton approach
and takes into account wind conditions, terrain, and vegetation state dependent burn probabilities. A fire keeps spreading until it
stops intrinsically (e.g., when the combustible biomass is exhausted), or a pre-defined maximum fire size is reached.

The fire module requires a digital elevation model (DEM). The DEM is configured in the via  the `visualization.dem` setting in the 
[configuration file](project_file.md).

Conceptually, the implementation closely follows the [iLand fire module](http://iland.boku.ac.at/wildfire).

### Vegetation states

State specific fire characteristics are defined in a text file given by `stateFile`. The table has the following columns
and requires one entry for each defined state:

Column | Description
------ | -----------
stateId    | unique (state Id)[states.md]
pSeverity  | probability that the fire on a cell is high severity
pBurn      | probability that a cell in this state will burn if reached by fire


## Ignitions

Ignitions for the SVD fire module are pre-defined and loaded at startup from a table (see `ignitionFile`). 
A fire is determined by its starting coordinates, wind conditions (speed and direction), and the maximum size (max_size).


The ignition file contains the following columns:


Column | Description
------ | -----------
year   | Year of the fire (simulation year, first year=1)
x      | metric x-coordiante of the ignition. If the ignition point (x/y)
y      | metric y-coordiante of the ignition
max_size | maximum fire size in ha
windspeed | wind speed (ms-1) during the wind event. The wind speed is assumed to be constant during one fire event.
winddirection | the direction of wind during the fire event. The value is given in degrees, 0=north, 90=east, ...

## Fire spread

The implementation of fire spread is round-based. In each round, first the probability of spreading to neighboring cells
is calculated for each currently burning cell (Moore-Neighborhood) and accumulated for potential target cells (i.e., 
the probability is higher if several neighboring cells are currently burning). Secondly, a random number decides
for each potential target cell (i.e. spread probability >0) whether the cell should burn. This also takes into account
a vegetation state specific burn probability. After burning, the fire on a cell extinguishes with `extinguishProb` and
spreads no further. The process stops when the burned area reaches the pre-defined maximum fire size, or when within 
one round no new cells are ignited. In order to avoid stopping fires too soon, the state specific burn probabilitiy is 
assumed being 1 and no fire extinguishing is simulated (i.e. extinguishProb assumed as 0) during the first five rounds.


### Spread probability 
The probability of fire spread from a burning cell to one of its neighboring cells is based on the approach of 
FireBGC v2 ([Keane et al 2001](http://www.treesearch.fs.fed.us/pubs/37464)). Fire spread distance is influenced 
by wind speed and wind direction and the slope on the DEM. Fire spreads further upslope and in wind direction.

See http://iland.boku.ac.at/wildfire for details.

The spread distance _m_ is consequently transformed into the probability of reaching the focus cell . 

p= `spreadToDistProb` ^ (1 / m_px )

with _m_px_ the spread distance _m_ divided by the distance per pixel (a distance of 100m for immediate neighbors, 
and 141.42m for diagnonal neighbors), and `spreadToDistProb` the probability that the fire reaches _m_ when it would 
have to spread over multiple cells.

The following table summarizes the different steps:

Step | Description
-----| ------------
Potentially spread into cell? | Probability that fire from adjacent cells spread into focal cell. A random number decides if it does.
Focal cell burns? | If yes, a state specific burn probability decides if the cell actually burns (a random number decides)
Further spread? | If yes, a burned cell spreads to its neighbors (in the next round) with a probability of 1 - `extinguishProb`, or stops spreading otherwise.


### Spread across non vegetated area
Fire cannot spread into or spot over areas that are not at least potentially vegetated (i.e. "out of the project areas"). 
Consequently, out-of-project areas form effective fire breaks.

Fires can spread across simulated areas, that are not currently vegetated. To allow this behavior:

* set non zero burn probabilities in the `stateFile` for these states
* set a "loop" for the fire induces transitions in the `transitionFile` (i.e. the same _stateId_ and _targetId_)

Note that such cells count as burned (for outputs and for determining if the fire already reached the maximum fire size). 

## Fire effects
The vegetation state of cells affected by fire transitions to a new state according to the matrix given in `transitionFile`.
Here the transition can be deterministic (i.e. providing a transition probability of 1), or probabilistic (i.e. when
multiple target states are provided in the table).



## Configuration

The module is configured in the [project file](project_file.md).  
In addition to the `enabled` and `type` setting, the fire module has the following settings:

* ### `transitionFile` (filepath)
The file containing a transition matrix (see [matrix module](module_matrix.md)) for post fire states (see above).

* ### `ignitionFile` (filepath)
The data table with the list of ignition points and fire sizes.

* ### `stateFile` (filepath)
Data file with additional state-specific variables that specify per-state burn probabilities.

* ### `extinguishProb` (numeric)

* ### `spreadToDistProb` (numeric)
The probability 

## Output
The module can provide tabular and gridded [output](outputs.md#Fire).