# Expressions and variables

## Cell variables
Every single cell on the landscape is described by a specific state and a given environment. Expressions
(see below) that are evaluated in the context of a cell can access state- and environment-specific
variables.

### Standard variables
Variable|Description
-----|----
`index` | index of the cell on the landscape (0..N cells)
`environmentId` | Id of the environment (see [landscape setup](configuring_the_landscape.md)
`climateId` | Id of the climate zone (see [landscape setup](configuring_the_landscape.md)
`stateId` | Id of the state (1..N)
`residenceTime` | number of years a cell is already in the state `stateId`

### Additional variables
In addition to the standard variables, further environment- and state-specific variables are available. 
* Environment variables: additional columns in the `landscape.file` table (see [landscape setup](configuring_the_landscape.md)
* State variables: content of the `states.extraFile` table, and module specific variables (see [states](states.md) for details)

A list of all available variables can be found in the log file.

## Expression
An "expression" is a mathematical formula (usually a function) which is provided in text form (e.g. `3*x+2`). 

The expression engine in SVD is also used in [iLand](iland.boku.ac.at) and was originally developed 
for the [Picus model](http://www.wabo.boku.ac.at/picus.html). Expressions are parsed only once 
and converted to an internal "representation" which can be executed with very little overhead. 
When using more complex mathematical functions (e.g. exp()), the overall performance is 
comparable to hard coded C++.

### Types of variables
Variable names within an expression depends on the context. There are three main cases:
* Expressions are bound to an "object" which provides access to (internal) values. The available variable names
depend on the object. A typical case are variables of a `cell` (see above). 
* unbound variables: The name of the variable is not pre-defined and the first encountered variable name 
in the expression is used: the expressions `3*x+1` and `3*year+1` behave the same. 

### Basic operations
Expressions can combine basic arithmetic operators, variables, and functions. The basic 
operators are `+`,`-`,`*` and `/`. Additionally, 
the caret `^` can be used for power functions (e.g. `x*x` can be written as `x^2`). 
Floating point numbers must use the dot (`.`) for constants (e.g. 0.001). 
The comma (`,`) is used to separate arguments in function calls.

### Boolean expressions
Expressions can be used to evaluate *boolean expressions*, i.e. expressions with a result 
value of either "true" or "false". Logic operators are `and` and `or`, `=`, and `<>`. 
Boolean expressions are typically used to filter or select from a set of objects based on a criterion. 
E.g.: using the expression `soilDepth>100 and pctSand>50` as a filter, would result 
in a list of deep and sandy soils.

Logic and "mathematical" operators can be used jointly: 
every non-zero value is evaluated as `true`, zero (0) as `false`. 
The literals `true` and `false` can be used and are internally converted to 1 and 0, respectively. 
Hence, a logical `not` can be expressed as `expression <> true`.


### Functions
The general form of function is:

`functionname(list of arguments)`

#### Mathematical functions
Mathematical functions are executed without checking for the validity of the arguments 
(e.g. division by 0, or tan(pi/2)).

Name, Arguments|Description|Example
------|--------|-------
sin(x)|the sin of x, x as radians.|`sin(x)`
cos(x)|the cosine of x, x as radians|`cos(x)`
tan(x)|the tangens of x, x as radians|`tan(x)`
exp(x)|exponential function, `exp(1)=2.7182...`|`exp(-k*LAI)`
ln(x)|the logarithm of base *e*, `ln(2.7182...)=1`|`ln(x)`
sqrt(x)|the square root of x (equivalent to `x^0.5`)|sqrt(x)
mod(x,y)|return the modulo (remainder) of x/y. e.g. mod(13,10)=3|`if(mod(id,2)=0, 1, 0)`
round(x)|Returns the integral value that is nearest to x, with halfway cases rounded away from zero.|`round(x)`||

#### Logical functions

Name, Arguments|Description|Example
------|--------|-------
min(x1,x2,...,xn)|returns the minimum value of the arguments. Argument count must be >1 and <10 |`min(x,0)`
max(x1,x2,...,xn)|returns the maximum value of the arguments. Argument count must be >1 and <10 |`max(min(x,1),0)`
if(condition, true, false)|logical if-then-else construct. if "condition" is true, then "true" is returned, "false" otherwise. E.g.: a abs()-function: `if(x<0;-x;x)`. Note that both clauses are calculated in every case!|`if(x<0;-x;x)`
in(value, arg1, arg2, ... , argn)|returns true if *value* is in the list or arguments, false otherwise.|in(year,100,200,300)

#### More functions

Name, Arguments|Description|Example
------|--------|-------
polygon(value, x1,y1, x2,y2, x3,y3, ..., xn,yn)|return is: *y1* if value<x1, *yn* if value>xn, or the lineraly interpolated numeric y-value.|`polygon(x, 0,0, 1,0.5)`
sigmoid(x, type, param1, param2)|The value of "sigmoid" curve at *x*. The type of curve is designated by *type* with the two parameters *param1* and *param2*. Type is one of: 0: [logistic](http://en.wikipedia.org/wiki/Logistic_function), 1: [Hill function](http://en.wikipedia.org/wiki/Hill_function), 2: 1-logistic, 3: 1-hill|`sigmoid(x, 0, 10, 100)`
rnd(from, to)|returns a uniformly distributed random function between *from* and *to*.|`rnd(0,1)`
rndg(mean, stddev)|returns a random number drawn from a Gaussian normal distribution with *mean* as the mean value *stddev* as the standard deviation.|`rndg(0,1)`

