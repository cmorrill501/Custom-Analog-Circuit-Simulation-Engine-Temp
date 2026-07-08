### [0.17.0] - [0.17.4]
    - Added: Standard diode element type.
    - Changed: Flipped the default diode orientation so forward bias correctly points toward Pin 2.
    - Changed: Minor backend optimizations and architectural changes to the circuit solver engine.
    - Fixed: Convergence failures occurring during diode simulation.
    - Fixed: Error preventing small-signal analysis results from being saved or graphed when a diode was in the circuit.
### [0.16.0]
    - Added: Automatic default naming conventions for newly created elements.
### [0.15.0]
    - Added: Ability to customize node names.
    - Changed: Integrated custom node names into the data-saving and plotting modules for better visualization.
### [0.14.0] & [0.14.1]
    - Added: Support for saving and loading netlists, alongside the ability to close active circuits.
    - Fixed: Bug affecting the default file name assignment when saving circuits.
### [0.13.0]
    - Changed: Rebuilt and refactored multiple internal functions responsible for circuit construction.
### [0.12.0] & [0.12.1]
    - Added: Visual plotting functionality for simulation results.
    - Fixed: Data-saving bug that disrupted proper rendering in the plotting engine.
### [0.11.0]
    - Changed: Completely rebuilt the backend architecture for how simulation results are saved.
### [0.10.0] & [0.10.1]
    - Added: Support for parasitic element values on sources, resistors, capacitors, and inductors.
    - Changed: Extended the /editElement command to allow modification of component parasitic values.
### [0.9.0] - [0.9.2]
    - Added: Small-signal AC simulation capability and AC stimulus elements.
    - Changed: Integrated AC stimulus editing into the /editElement command.
    - Fixed: A crash caused by running a small-signal simulation without an active AC stimulus.
    - Fixed: Formatting and parsing issues when editing small-signal stimulants.
### [0.8.0]
    - Added: Transient simulation solver.
### [0.7.0] & [0.7.1]
    - Added: Ability to delete elements or edit their names, values, and attached waveforms.
    - Fixed: Bug where the /editElement [element] command would fail silently without throwing an error message.
### [0.6.0] & [0.6.1]
    - Added: Support for sine wave and pulse waveforms.
    - Fixed: An issue preventing a single source from simultaneously holding a DC value and an attached waveform (e.g., a sine wave).
### [0.5.0]
    - Added: Inductor and capacitor components.
### [0.4.0]
    - Added: Operating point (DC) circuit solver.
### [0.3.0]
    - Added: Component connectivity, allowing elements to be linked together into a netlist.
### [0.2.0]
    - Added: Core SPICE components: voltage/current sources, resistors, and ground nodes.
### [0.1.0]
    - Added: Initial CLI framework with basic exit and clear commands.
