# Configuration file
SVD uses a text-based configuration file to control the main settings of the model (default-extension: `.conf`). The settings are provides as values in key-value pairs with '=' acting as the separator. By default, all text after the '=' is part of the value. The keys are grouped logically into 'sections', e.g. the key `logging.file` could be read as the `file` setting in the group `logging`. The order of keys in the configuration file is random.
SVD requires values for some settings (and throws an error if no value is provided), and uses default values for other settings (instances are logged when log-level 'trace' is used).

Lines starting with '#' are treated as comments and ignored.

Internally, the following data types are used:
* numeric (double): a floating point value
* integer: an integer value
* boolean: a logical value, use `true` / `True` to indicate true, and `false` / `False` otherwise
* string: text (e.g. file names), provide no quotation marks
* list: comma-separated list of values (e.g. `1,2,3,4`)

### Section 'model'


| Setting | Type | Description |
| -------- | -------- | -------- |
| model.multithreading     | boolean     | Enabled/disable the use of mulitple threads for the main model in SVD (not related to DNN inference). |
| model.threads | integer | Number of threads to use if `model.multithreading` is enabled. The 'ideal' thread count (number of CPU cores) is used if missing or if the value is `-1`. |



### Section 'logging'
The user can control the level of detail for which log messages are generated. The available log levels are (decreasing level of details):
* "trace"
* "debug"
* "info"
* "warning"
* "error"
* "critical"
* "off"

Note that the log level can be adjusted for the startup and the execution phase separately, i.e. high level of details (e.g. "debug") during startup and low level of details (e.g. "error") are feasible.

A typical log entry includes a timestamp, the source (setup, dnn, main) and the log level (debug, error, ...):
```
[2018-01-23 11:17:25.551] [setup] [info] Enabled multithreading for the model (# threads=2).
```


| Setting | Type | Description |
| -------- | -------- | -------- |
| logging.file     | string     | Path to the log file (relative to the project root) |
| logging.setup.level | string | Debug level during model setup. |
| logging.model.level | string | Debug level for the main model (during execution). |
| logging.dnn.level | string | Debug level for the DNN sub module (during execution). |



### Section 'dnn'

| Setting | Type | Description |
| -------- | -------- | -------- |
| dnn.file     | string     | Path to the stored DNN (relative to the project root) |
| dnn.maxBatchQueue | integer | The maximum queue size for batches |
| dnn.topKNClasses | integer | the number of classes used by the top K algorithm for selecting candidate states (default: 10)|
| dnn.topKGPU | boolean | if `true` (the default), the selection of the top K classes after DNN inference is done on GPU (using TensorFlow ops), and on CPU otherwise (not using TensorFlow). |

### Section 'states'

### Section 'climate'

### Section 'landscape'

### Section 'initialState'

### Section 'externalSeeds'

### Section 'output'