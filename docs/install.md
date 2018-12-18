# Installing and compiling SVD

You can either use the precompiled version of SVD, or build SVD for yourself. 

## Instructions for compiling SVD 

The SVD model is a stand alone modelling software written in C++ and available under a 
GPL license. SVD builds on the [Qt](https://qt.io) framework, and is best compiled
and modified with the tools provided by Qt (e.g. the QtCreator IDE).

Currently, building SVD (and TensorFlow) is only available for Windows.

SVD consists of a number of sub-projects:

* `Predictor`: the link to TensorFlow; this part includes TensorFlow-headers and contains
 the logic for communicating with TensorFlow
* `SVDCore`: The main part of the model (representation of the simulated area, data, ...)
* `SVDUI`: The Qt-based user interface

To build SVD:
 
* open the `SVDModel.pro` file in QtCreator
* Build all sub-projects
* Run the `SVDUI.exe` 

The tricky part is the compilation of the `Predictor` sub-project as this requires a local
TensorFlow installation (for include files) and a compiled version of TensorFlow in a DLL. 
Check the `Predictor.pro` file which includes some links for further information.

### Compiling TensorFlow

Compiling TensorFlow with CMake on Windows is hard. I did go through this (see [here](https://github.com/tensorflow/tensorflow/issues/15254)
and will likely (have to) touch the issue again. At this point I hope to add a proper how-to.

A compiled and GPU-enabled version of tensorflow.dll can be found in the `executable` folder.

