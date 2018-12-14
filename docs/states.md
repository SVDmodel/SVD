# Vegetation states 
Vegetation states are a central concept in SVD. Every simulated cell is - at every timestep - in one of the
possible states, and transitions between states are the mechanism how vegetation dynamics are expressed.

## Definition of states
The potential states are defined by the setting [`states.file`](project_file.md) in the project file.

The table has the following columns:

Column | Description
-------| -----------
stateId | a unique numeric (integer) Id of the state. Ids should start with 1 (0 is reserved as an invalid state)
composition | a string describing the main species (see below)
structure | an integer class describing the structure (top canopy height class) (0..N)
fct | functioning, an integer class describing the vegetation density class (0..N)
name | a string with a label for the state (if omitted a label is constructed from composition, structure, and function)
type | a string describing the handling module (see below)

### Composition, structure, and functioning
From those three properties of a vegetation state, SVD currently only uses the `composition` internally 
and requires thus a specific format (the composition is used for calculating species shares in the neighborhood of a cell):

* A list of all allowed species is defined in `model.species`. The preferred species coding scheme is a four
character combination of genus and species (e.g., "*Pi*cea *ab*ies" -> "piab")
* the composition is a space-delimited string of up to four species (e.g. "PIAB fasy pisy")
* uppercase species codes (e.g. "PIAB") indicate the dominate species, lowercase species codes ("piab") specify
admixed species
* "unforested" is a special composition reserved for, well, a state without forest cover

Please note, that "species" could be defined more broadly, e.g. as mixture types or vegetation types.

### Extra variables
Additional state specific variables can be added by using the `states.extraFile` table. Also modules can
add state-specific variables (e.g. the [fire module](module_fire.md)).

### The *type* of a state
The `type` defines for each state the responsible "party", i.e. the process / module that is the default
"handler" for a particular state. The task of a handler is to determine future states of all cells
that are currently in one of the handled states. See the [modules](modules.md) page for more details.

Possible values for `type` are:
* blank: the handling process is the main DNN
* the name of a module: cells in this state are handled by the given module

