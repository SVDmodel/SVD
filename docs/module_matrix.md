# Matrix module

## Transition matrix

The transition matrix used by the module is loaded from `transitionFile` and has the following columns:

Column | Desription
-------| ----------
stateId | The [Id](states.md) of the current state
key | an additional qualifier (see text)
targetId | The [Id](states.md) of the target state
p | The probability (0..1) of a transition from `stateId` to `targetId` 

The probabilities `p` need to sum up to 1 for every unique combination of `stateId` and `key`. The `key`
allows the definition of (many) alternative transition probabilities for the same state. 

Consider the following (constructed) example, where the transition probabilities depend on 
an environment variable "soilDepth" and are different for shallow soils (soilDepth < 30cm).
The table below shows a transition matrix for shallow, and deep soils (note that for state 2 and 
shallow soils the state does not change):

State | p(State 1) | p(State 2) | p(State 3) | Soildepth
------| ----------- | ---------| ----------- | ---------
1 | 0% | 20% | 80% | <30cm
1 | 0%  | 40% | 60% | >=30cm
2 | 0% | 100% | 0% | <30cm
2 | 10% | 80% | 10% | >=30cm
...|...|...|...|... 

Converted to the "long" format used by the module, the table looks like:

stateId | key | targetId | p
--------| ----|-----|-----
1 | 0 | 2 | 0.2
1 | 0 | 3 | 0.8
1 | 1 | 2 | 0.4
1 | 1 | 3 | 0.6
2 | 0 | 2 | 1
2 | 1 | 1 | 0.1
2 | 1 | 2 | 0.8
2 | 1 | 3 | 0.1
...|...|...|...

The information whether a cell has shallow or deep soil, can be derived from the environment 
(assuming there is a variable `soilDepth`) and can be expressed as an 
[expression](variables.md) (`keyFormula`): 

`if(soilDepth<30, 0, 1)`

### Deterministic transitions
If for a given state several `targetIds` are provided, then the target state is chosen based on the
provided probabilities. Deterministic transition based on some criterion (e.g. change from state
1 to state 2 after 20 years) can be simulated by setting the probabilities `p` to 1 and by expressing 
the criterion as an expression for different `keys` (as long as the required variables are available). 
For example, the expression `min(residenceTime/10, 10)` yields the decade (0: 0-9yrs, 1: 10-19yrs, ...), 
which can be used as `key` in the transition matrix:

stateId | key | targetId | p | comment
--------| ----|-----|----- | --------
1 | 0 | 1 | 1 | 0-9 years, no change
1 | 1 | 1 | 1 | 10-19 years, no change
1 | 2 | 2 | 1 | >=20 years, change to state 2

## Handled states
To set the module as handler for specific states, use the module name as the `type` property in the
[state](states.md) definition (see [modules](modules.md)).

## Configuration

The module is configured in the [project file](project_file.md).  
In addition to the `enabled` and `type` setting, the matrix module has the following settings:

* ### `transitionFile` (filepath)
The file containing the data for the transition matrix (see above).

* ### `keyFormula` (expression)
The `keyFormula` is an expression that can access all cell variables. The result of the expression
is used as the `key` in the transition matrix (see above). The value of the expression is converted
to an integer (the floating point fraction is truncated, i.e. 1.9 -> 1). To disable, set to "" or to a
constant ("0").
