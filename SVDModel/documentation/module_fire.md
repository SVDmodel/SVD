# Fire module
TODO: desribe....
## Ignitions

## Fire effects

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

## Output
The module can provide tabular and gridded [output](outputs.md#Fire).