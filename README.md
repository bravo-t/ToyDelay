# ToyDelay: An educational delay calculation engine for static timing analysis tools

## Supported devices
Voltage: `Vname N+ N- value/pwl(t v t v ...)`

Current: `Iname N+ N- value/pwl(t v t v ...)`

Resistor: `Rname N+ N- value`

Capacitor: `Cname N+ N- value`

Inductor: `Lname N+ N- value`

VCVS: `Ename N+ N- NC+ NC- Value`

VCCS: `Gname N+ N- NC+ NC- Value`

CCVS: `Hname N+ N- NC+ NC- Value`

CCCS: `Fname N+ N- NC+ NC- Value`

Gate cell instances: `Xinst LibCellName pinA nodeA pinB nodeB ...` (Only supported in `.delay` mode) 

## Supported commands and options

`.lib lib_file`: Specifies the path of the library file.

`Xinst LibCellName pinA nodeA pinB nodeB ...`: Instantiates the standard cell. `LibCellName` shoule match the one in library file. `pinX` specifies the pin name of the gate cell, and `nodeX` specifies the node connected to `pinX`. 

`.option [name] driver={rampvoltage|current}`: Specifies the driver model of cell timing arcs. `rampvoltage` means a ramp voltage source, in series to a resistor connected to the voltage source, will be used to model the driver pin behavior. The details are described in "Performance computation for precharacterized CMOS gates with RC loads". `current` means a current source will be used to model the driver bahavior, and composite current source (CCS) data will be used to calculate the delay.

`.option [name] loader={fixed|varied}`: Specifies the behavior of the load capacitor of the loader pin. `fixed` means a fixed value will be used for the capacitor, whereas `varied` means the capacitor value will change, and the values come from receiver cap LUT.

`.option [name] net={tran|awe}`: Specifies how the RC network will be handled in delay calculation. `tran` means transient simulation will be used to calculate net delay, and `awe` means pole-zero analysis will be used. Right now only `tran` is supported.

`.delay Xinst/output`: Sets the analysis mode to full stage delay calculation. For specifed `Xinst/output` pin, all delay and transition values of the cell arc that connected to the output pin, as well as the net arcs connected from the output pin, are calculated. Internally the `X` devices, or standard cells, will be elaborated with basic devices, thus new devices and nodes will be created, based on the specified driver model and loader model. Specifically:

  `driver=rampvoltage` creates new devices `inst/driverPin/Vd` as the ramp voltage source, `inst/driverPin/Rd` as the resistor connected to the ramp voltage source, and new node `inst/driverPin/VPOS` as the positive terminal of the ramp voltage source. The internal structure of cell instances (include both driver model and loader model) is shown as below:

```
    +---------------------------------------------------------------+  
    |                                                               |  
    |                    Instance/driverPin/VPOS                    |  
    | loaderPin                 |                         driverPin |  
  +---+                         v          +--------+             +---+
  |   +---------+           +--------------|        +-------------+   |
  +---+         |           |              +----+---+             +---+
    |            |           |                  ^                   |  
    |            |           |                  |                   |  
    |            |           |          Instance/driverPin/Rd       |  
    |         ---+---     +-----+                                   |  
    |    +-->             |  ^  |<--Instance/driverPin/Vd           |  
    |    |    ---+---     |  |  |                                   |  
    |    |       |        +--+--+                                   |  
    |    |       |           |                                      |  
    |    |       |           |                                      |  
    |    |       |           |                                      |  
    |    |    ---+---      --+--                                    |  
    |    |     -----        ---                                     |  
    |    |      ---          -                                      |  
    |  Instance/loaderPin/Cl                                        |  
    |                                        StdCell Instance       |  
    +---------------------------------------------------------------+  
```

  `driver=current` creates new devices `inst/driverPin/Id` as the current source.

  Loader models are the same on circuit structures, that create new capacitor `inst/loadPin/Cl`. The difference between `fixed` and `varied` are the values of the capacitor.

### Global commands

`.debug [module] 1`: Enable debug output. This command now supports enable debug information for specified modules only, if `module` is omitted, debug information for all modules are enabled. Valid module names are `all` for enabling all modules, `root` for root solver, `sim` for transient simulation, `circuit` for circuit building, `pz` for pole-zero analysis, `nldm` for NLDM delay calculation, and `ccs` for CCS delay calculation.

`.plot tran [width=xx height=xx canvas=xxx] [name.]V(NodeName) [name.]I(DeviceName)`: Generate a simple ASCII plot in terminal for easier debugging. If `width` and `height` directives are not given, the tool will use current terminal size for plot width and height. Multiple simulation results can be plotted in a single chart by specifying a canvas name. Currently at most 4 plots can be drawn in one canvas. Now the command can plot data from different analysis data into one canvas, specified with `name.` prefix. (This command is not supported in PZ analysis.)

`.measure tran[.name] variable_name trig V(node)/I(device)=trigger_value TD=xx targ V(node)/I(device)=target_value`: Measure the event time between trigger value happend and target value happend. (This command is not supported in PZ analysis.)

## Compile and run
`git clone --recurse-submodules` and `make` should be sufficient. The executable is generated under current code directory and named "delay".

To run, just give the executable the spice deck you want to simulate. 

## Examples

`./delay examples/nldm_calc.cir` gives an example of NLDM delay calculation.

`./delay examples/ccs_calc.cir` gives an example of CCS delay calculation. The expected output can be found in [examples/ccs_calc.log](examples/ccs_calc.log).


