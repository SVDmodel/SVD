# Configuring DNN meta data

## Introduction
In order to define the meta data that describes the bindings to the DNN SVD uses a single [configuration file](input_files.md)
which is set by the `dnn.metadata` setting in the main configuration file.

Each binding is described by a number of settings, which follow this naming scheme:

`input.<tensorname>.<key> = <value>`

## Common settings
### `enabled`
Boolean value, if `true` the binding is active, if `false` the binding is not active
### `dim`
The number of dimensions of the input tensor. Possible values are 0, 1, and 2. 0 dimensions can be used for
constants (e.g. `training_phase` = false)
### `sizeX`
number of elements along the first axis of the tensor. For 1-dimensional tensors this is the total number of elements,
for 2-dimensional tensors it is the number of rows
### `sizeY`
number of elements along the second axis of the tensor. For 1-dimensional tensors this is 0,
for 2-dimensional tensors it is the number of columns
### `dtype`
The data type of the tensor. Currently, the following data types are supported:

dtype | description
------ | ----------
float | 32-bit floating point number; this is the preferred floating point type used for DNN (since calculations are faster compared 64-bit floating points (double))
int16  | 16-bit signed integer (value range: -32,768 to +32,767)
int64 | 64-bit signed integer (value range: -9,223,372,036,854,775,808 to +9,223,372,036,854,775,807)
uint16 | 16-bit unsigned integer (value range: 0 - 65,535)
float16 | 16-bit floating point number (half precision)
bool | 1 bit boolean value

Note, that not all bindings support all possible data types.

### `type`
The `type` tells SVD what kind of data is expected by the binding. The following types are possible, detailed
description are below.

type | description
-----| ------------
State | The current state of the cell (single integer value of `stateId - 1`). Data type is `uint16`.
ResidenceTime | the residence time of the cell in years divided by 10. Data type is `float`, single value.
Climate | climate time series for the next 10 years for the given cell. The tensor is 2-dimensional and the data type is `float`.
Neighbors | a 1-dimensional vector (data type `float`) with species share for the local and intermediate neighborhood.
Scalar | a constant (with a fixed value set during the setup process)
Var | a 1-dimensional vector (data type `float`) with variable size that is populated with user-defined values.
Function | a single `float` value that is calculated on the fly by built in functions.

Deprecated types (used in earlier versions of SVD):

type | description
-----| ------------
DistanceOutside | single `float` value populated with the environment variable `distanceOutside` 
SiteNPKA | two `float` values, populated with "(availableNitrogen-58.5)/41.436" and "(soilDepth-58.5)/41.436"


## Available bindings

### Var
Defines a 1d vector (with size `sizeX`) and populates the vector with user-defined "expressions". The expressions
are defined by the `transformations` setting (e.g., to specify the transformations for a tensor with the name
`site`, use `input.site.transformations`).

The syntax is as follows:
```
input.<name>.transformations = { expression1 }, { expressione2 }, ...,  { expressionN }
```
`expression[1-N]` corresponds to the 1st - Nth value in the vector, i.e. N equals `sizeX`.

The expression can make use of all available context variables for a cell (e.g. state variables and environment variables).

Example:
```
# environment variables for available nitrogen and percentage of sand, divided by 100
input.site.transformations={availableNitrogen/100}, {pctSand/100}
```

### Function
The `Function` binding is a gateway to data items that are provided by (hard-coded) functions in SVD. 
The `function` key defines which function to execute for a cell.

function | description
---------|------------
DistToSeedSource | calculates the minimum distance to a cell that acts as seed source for the current cell

Example:
```
# specifies to use the "DistToSeedSource" function for the "distance" tensor
input.distance.function=DistToSeedSource
```

### Neighbors
SVD calculates the shares for the local and intermediate neighborhood for all available species (i.e. potential
seed input). The species shares are calculated based on state information (mainly the compositional type). A list
of available species is defined with the `model.species` setting in the SVD config file. For each species
in `model.species` the function produces two values (local, and intermediate neighborhood). The size of
the tensor has to be number of species * 2.

TODO: more details

### Climate
TODO: more details
