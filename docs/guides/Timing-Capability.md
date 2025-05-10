VDMS has the ability to output a variety of timing information for use in tuning and optimization.

There are 3 sets of timers that can be enabled via the server side configuration:

* VCL Timing: this outputs time spent in individual VCL operations
* High Level Timing: outputs coarse grained timing spent in end-to-end and to time to send the response back to the client
* Query Timing: This outputs the amount of time spent in the PMGD handler (neo4J support coming later), such as preprocessing (construct protobuf) response construction, and PMGD query time.

Each of these timers can respectively be enabled as follows in the server configuration JSON.

```json
"print_vcl_timing": true,
"print_high_level_timing": true,
"print_query_timing": true
```

There are many different timerIDs. To find what sections that are encompassing, you can simply do a string search within the source code and look for the code block wrapped by add_timestamp calls.