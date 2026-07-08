## Current Roadmap

### Phase 1: Core Framework & Netlist Architecture (Completed)
### Goal: Build the user interface and the underlying data structures to hold the circuit in memory.
    - Versions Covered: 0.1.0, 0.2.0, 0.3.0
    - Key Tasks:
        - Set up the CLI loop with basic exit and clear commands.
        - Define the base Element class and subclasses for basic components (VSource, ISource, Resistor, Ground).
        - Implement the netlist structure to connect these elements together.
    - Milestone: You can type commands to build a resistive circuit in memory, though it can't simulate anything yet.

### Phase 2: DC Foundations & Reactive Components (Completed)
### Goal: Implement the first mathematical circuit solver and introduce time-varying components and waveforms.
    - Versions Covered: 0.4.0, 0.5.0, 0.6.0, 0.6.1
    - Key Tasks:
        - Develop an operating point (DC) circuit solver to calculate steady-state node voltages.
        - Expand the component library to include energy-storage elements (capacitors and inductors).
        - Implement math and data structures for sine wave and pulse waveforms.
        - Enable voltage/current sources to simultaneously support both a static DC offset and a time-varying waveform.
    - Milestone: The simulator can solve basic DC circuits and support complex, time-varying component definitions in memory.

### Phase 3: Interactive Editing & Advanced Simulation Solvers (Completed)
### Goal: Give users dynamic control over the netlist and introduce time-domain (Transient) and frequency-domain (AC) simulation engines.
    - Versions Covered: 0.7.0 - 0.9.2
    - Key Tasks:
        - Introduce the /editElement and deletion commands to allow live manipulation of component properties and waveforms.
        - Implement a Transient simulation solver to calculate circuit behavior over a specified time interval.
        - Develop a small-signal AC simulation capability along with dedicated AC stimulus elements.
        - Add robust error handling to prevent application crashes when simulations are run with missing or malformed stimuli.
    - Milestone: Users can interactively modify an active circuit and run advanced transient and frequency-sweep analyses.

### Phase 4: Non-Ideal Modeling & Visual Plotting Engine (Completed)
### Goal: Enhance simulation realism with non-ideal component behavior and transition from text-based outputs to visual graphics.
    - Versions Covered: 0.10.0 - 0.12.1
    - Key Tasks:
        - Add support for parasitic element values (e.g., ESR, ESL) across core components and expand the /editElement command to modify them.
        - Completely rebuild the underlying backend architecture to handle and save massive sets of simulation data efficiently.
        - Build a frontend visual plotting engine to render simulation results as graphs.
        - Debug data-pipeline serialization issues to ensure smooth data transfer between the simulation backend and the plotting engine.
    - Milestone: The tool functions as a highly capable, realistic SPICE simulator that outputs visual waveforms rather than raw data matrices.

### Phase 5: Verification & Visualization (The "Sanity Check" Phase) (Completed)
### Goal: Implement visual plotting of simulation results and verify solver accuracy against industry-standard LTSpice.
    - Versions Covered: Post-0.12.1 (Verification)
    - Key Tasks:
        - Build functionality to plot simulation results visually rather than relying on raw text or data matrices.
        - Benchmark the TR-BDF2 transient solver by simulating an RLC circuit with a step input and validating the plotted ringing.
        - Validate the AC sweep matrix by simulating an RC filter and verifying the resulting magnitude and phase plots against LTSpice.
    - Milestone: Simulation outputs are visually verifiable, proving the mathematical accuracy of your transient and AC solvers against industry standards.

### Phase 6: The Input System & SPICE Compatibility (Completed)
### Goal: Transition from a pure CLI to a professional file-based input workflow by writing a standard SPICE netlist parser.
    - Versions Covered: Input Subsystem Update
    - Key Tasks:
        - Streamline the command system, separating runtime execution commands from circuit/simulation directives.
        - Refactor the element-creation and command workflows to cleanly separate element parameter editing from simulation type declarations.
        - Add the ability to save active circuit netlists directly to text files.
        - Develop a text-based netlist parser to dynamically read and interpret standard linear SPICE netlists ($R, L, C, V, I$).
        - Implement dynamic loading to automatically instantiate components and populate the internal runtime map and nodes.
    - Milestone: The simulator can dynamically read standard, linear SPICE netlist text files, build the circuit in memory, and execute specified directives.

### Phase 7: Non-Linear Components & Iterative Solvers (In Progress)
### Goal: Tackle non-linear circuit physics by implementing iterative mathematical solvers and semiconductor models.
    - Versions Covered: Non-Linear Engine Update
    - Key Tasks:
        - Implement a Newton-Raphson iterative loop to linearize and solve exponential semiconductor equations step-by-step until convergence.
        - Model and integrate the Shockley diode equation, calculating and stamping equivalent linear conductances (G_eq) and current sources (I_eq) into the matrix.
        - Integrate active transistor models by implementing the Ebers-Moll model for BJTs and the Shichman-Hodges model for MOSFETs.
        - Upgrade the AC Sweep engine to run a preliminary DC Operating Point, freeze semiconductor conductances at their operating bias, and execute the complex frequency sweep.
    - Milestone: The simulation engine successfully handles active, non-linear semiconductor circuits using an iterative matrix solver.

### Phase 8: Scalability & Performance (Matrix Optimization)
### Goal: Optimize the mathematical backend to handle to be added complex circuits like op-amps and logic gates efficiently by switching to sparse matrices.
    - Versions Covered: Core Optimization Update
    - Key Tasks:
        - Transition the primary circuit matrices (dense representations) over to efficient sparse matrix data structures (Eigen::SparseMatrix).
        - Replace the dense matrix solver (colPivHouseholderQr) with a high-performance sparse matrix solver (Eigen::SparseLU).
        - Optimize memory and CPU utilization by eliminating calculations and storage for empty, zero-valued matrix cells.
    - Milestone: The simulation engine can process complex circuits containing thousands of nodes in milliseconds, eliminating dense matrix CPU bottlenecks.

### Phase 9: Advanced EDA Features
### Goal: Build out an industry-standard Electronic Design Automation (EDA) feature set to handle advanced simulation modes, modularity, and statistical analysis.
    - Versions Covered: Advanced Features / Production Release
    - Key Tasks:
        - Implement a DC Sweep (.DC) engine that wraps the operating point solver in an outer loop to sweep a source and plot IV curves.
        - Expand the netlist parser to handle modular subcircuits (.SUBCKT), dynamically mapping local subcircuit nodes to the global matrix.
        - Introduce component model cards (.MODEL) to allow centralized definitions of physical semiconductor parameters.
        - Develop a Monte Carlo / Tolerance Analysis loop that randomly perturbs component values within their tolerance variables to yield statistical error distributions.
    - Milestone: The engine operates as a fully fleshed-out EDA platform capable of modular netlisting, advanced parameter sweeps, and yield analysis.
