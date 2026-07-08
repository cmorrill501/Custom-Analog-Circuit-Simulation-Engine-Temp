#include "CircuitSolver.h"

#pragma region private
void CircuitSolver::invalidValue(const std::string& name,const std::string& unitName) {
    std::cout << "Circuit is invalid: " << name << " must have a value greater than 0.0 " << unitName << "." << std::endl; 
}
std::vector<double> CircuitSolver::getAllFrequencies(const std::unordered_map<std::string,Elements*>& element_map) {
    std::vector<double> frequencies;
    Waveforms* signal = nullptr;
    for(const auto& [_,element_ptr] : element_map) {
        if(element_ptr->type == ElementType::voltage_source) {
            signal = dynamic_cast<VoltageSource*>(element_ptr)->ac_wave.get();
        } else if(element_ptr->type == ElementType::current_source) {
            signal = dynamic_cast<CurrentSource*>(element_ptr)->ac_wave.get();
        }
        if(signal != nullptr) frequencies.push_back(signal->getFrequency());
    }
    return frequencies;
}

void CircuitSolver::print_operating_point_results(const std::unordered_map<std::string,Elements*>& element_map) {
    std::cout << "\n---- Operation Point ----" << std::endl;
    for(int i = 0; i < context.nodes; i++) {
        std::cout << "V(n" << i+1 << "): " << timeDomain.x(i) << std::endl;
    }
    std::cout << "\n---- Component Currents ----" << std::endl;
    for(const auto& [name,element_ptr] : element_map) {
        if(element_ptr->type != ElementType::gnd) {
            element_ptr->postSolve(context);
            element_ptr->printResults();
        }
    }
    std::cout << "\n";
}
// can be better optimized
double CircuitSolver::determineFirstStep(const std::unordered_map<std::string, Elements*>& element_map) {
    double max_derivative = 0.0;
    for(const auto& [name,element_ptr] : element_map) {
        if(element_ptr->type == ElementType::capacitor) {
            auto* cap = static_cast<Capacitor*>(element_ptr);
            double dv_dt = std::abs(cap->solution.current/cap->values.capacitance);
            double relative_rate = dv_dt/(defaults.relative_tolerance*std::abs(cap->solution.voltage_drop)+defaults.absolute_tolerance_v);
            max_derivative = std::max(max_derivative, relative_rate);
        } else if(element_ptr->type == ElementType::inductor) {
            auto* ind = static_cast<Inductor*>(element_ptr);
            double di_dt = std::abs(ind->solution.voltage_drop/ind->values.inductance);
            double relative_rate = di_dt/(defaults.relative_tolerance*std::abs(ind->solution.current)+defaults.absolute_tolerance_i);
            max_derivative = std::max(max_derivative, relative_rate);
        }
    }
    if(max_derivative <= 0.0) return 1e-9;
    return clamp(1.0/max_derivative,1e-9,context.max_dt);
}
double CircuitSolver::determineMaxFrequency(const std::unordered_map<std::string, Elements*>& element_map) {
    std::vector<double> freqs = getAllFrequencies(element_map);
    if(freqs.empty()) return 0.0;
    return *std::max_element(freqs.begin(),freqs.end());
}
double CircuitSolver::findCircuitFrequency(const std::unordered_map<std::string,Elements*>& element_map) {
    std::vector<double> freqs = getAllFrequencies(element_map);
    if(freqs.empty()) return defaults.f_min;
    return *std::min_element(freqs.begin(),freqs.end());
}

bool CircuitSolver::executeStep(const std::unordered_map<std::string,Elements*>& element_map) {
    bool doesConvergence = true;
    for(int trial = 0; trial < defaults.iterations; trial++) {
        timeDomain.A.setZero();
        timeDomain.z.setZero();
        for(const auto& [_,element_ptr] : element_map) {
            element_ptr->stamp(context);
        }
        Elements::applyMinConductance();
        timeDomain.x = timeDomain.A.colPivHouseholderQr().solve(timeDomain.z);
        doesConvergence = true;
        for(const auto& [_,element_ptr] : element_map) {
            element_ptr->postSolve(context);
            if((element_ptr->type == ElementType::diode) && (!(element_ptr->convergenceCheck()))) doesConvergence = false;
        }
        if(doesConvergence) break;
    }
    if(!doesConvergence) std::cout << "ERROR: could not converge" << std::endl;
    return doesConvergence;
}
double CircuitSolver::determineMaxErrorRatio(const std::unordered_map<std::string,Elements*>& element_map) {
    double max_error_ratio = 0.0;
    double gamma = defaults.gamma;
    double c_lte = std::abs((3.0*gamma*gamma-4.0*gamma+2.0)/(6.0*(2.0-gamma)));
    for(const auto& [_,element_ptr] : element_map) {
        ElementType type = element_ptr->type;
        if(type == ElementType::capacitor) {
            auto* cap = static_cast<Capacitor*>(element_ptr);
            double capacitance = cap->values.capacitance;
            double dv_dt_n = cap->past.i_c_n/capacitance;
            double dv_dt_gamma = cap->past.i_c_gamma/capacitance;
            double dv_dt_n1 = cap->past.candidate_i_c_n/capacitance;
            double div_diff = dv_dt_n1/(1.0-gamma)-dv_dt_gamma/(gamma*(1.0-gamma))+dv_dt_n/gamma;
            double lte = c_lte*context.dt*std::abs(div_diff);
            double core_v = cap->past.candidate_v_c_n;
            double allowed = defaults.relative_tolerance*std::max(std::abs(core_v),std::abs(cap->past.v_c_n))+defaults.absolute_tolerance_v;
            max_error_ratio = std::max(max_error_ratio,lte/allowed);
        } else if(type == ElementType::inductor) {
            auto* ind = static_cast<Inductor*>(element_ptr);
            double inductance = ind->values.inductance;
            double di_dt_n = ind->past.v_l_n/inductance;
            double di_dt_gamma = ind->past.v_l_gamma/inductance;
            double di_dt_n1 = ind->past.candidate_v_l_n/inductance;
            double div_diff = di_dt_n1/(1.0-gamma)-di_dt_gamma/(gamma*(1.0-gamma))+di_dt_n/gamma;
            double lte = c_lte*context.dt*std::abs(div_diff);
            double core_i = ind->past.candidate_i_l_n;
            double allowed = defaults.relative_tolerance*std::max(std::abs(core_i),std::abs(ind->past.i_l_n))+defaults.absolute_tolerance_i;
            max_error_ratio = std::max(max_error_ratio,lte/allowed);
        }
    }
    return std::max(defaults.min_max_error_ratio,max_error_ratio);
}

void CircuitSolver::resultsStorageInitialization(const std::unordered_map<std::string, Elements*>& element_map) {
    size_t estimatedSteps = static_cast<size_t>((context.stop-context.start)/context.max_dt);
    int frequencies = results.time_or_freq.size();
    std::vector<std::string> keys;
    if(context.mode != SimulationMode::ac_sweep) {
        results.time_or_freq.clear();
        if(context.mode == SimulationMode::transient) results.time_or_freq.reserve(estimatedSteps);
    }
    results.dataPoints.clear();
    keys.reserve(context.nodes+element_map.size());
    for(int node = 1; node <= context.nodes; node++) {
        keys.push_back("V("+node_ids_and_names.nodeName(node)+")");
    }
    for(const auto& [name,_] : element_map) {
        keys.push_back("I("+name+")");
    }
    for(const auto& key : keys) {
        CircuitSignal& sig = results.dataPoints[key];
        switch(context.mode) {
            case(SimulationMode::operating_point):
            {
                sig.magnitude.resize(1,0.0);
                break;
            }
            case(SimulationMode::transient):
            {
                sig.magnitude.reserve(estimatedSteps);
                break;
            }
            case(SimulationMode::ac_sweep):
            {
                sig.magnitude.reserve(frequencies);
                sig.phase.reserve(frequencies);
                break;
            }
        }
    }
}
void CircuitSolver::recordNodeResults(const std::unordered_map<std::string,Elements*>& element_map) {
    for(int node = 0; node < context.nodes; node++) {
        std::string key = "V("+node_ids_and_names.nodeName(node+1)+")";
        auto save = results.dataPoints.find(key);
        if(save == results.dataPoints.end()) {
            std::cout << "ERROR: problem with saving voltage result to " << key << std::endl;
            return;
        }
        CircuitSignal& signal = save->second;
        if(context.mode != SimulationMode::ac_sweep) {
            signal.magnitude.push_back(timeDomain.x(node));
        } else {
            auto complex_value = freqDomain.x(node);
            signal.magnitude.push_back(std::abs(complex_value));
            signal.phase.push_back(std::arg(complex_value)*(180.0/std::numbers::pi));
        }
    }
}

void CircuitSolver::operating_point(const std::unordered_map<std::string,Elements*>& element_map) {
    resultsStorageInitialization(element_map);
    if(context.mode != SimulationMode::ac_sweep) results.time_or_freq.push_back(0.0);
    if(!(executeStep(element_map))) return;
    if(context.mode == SimulationMode::ac_sweep) return;
    double omega = 0;
    for(const auto& [_,element_ptr] : element_map) {
        element_ptr->saveResults(omega);
    }
    if(context.mode == SimulationMode::operating_point) print_operating_point_results(element_map);
}
void CircuitSolver::transient(const std::unordered_map<std::string,Elements*>& element_map) {
    resultsStorageInitialization(element_map);
    std::set<double> breakpoints;
    for(const auto& [_,element_ptr] : element_map) {
        if(element_ptr->type == ElementType::voltage_source) {
            auto wave = static_cast<VoltageSource*>(element_ptr)->ac_wave;
            if(wave) {
                for(double points : wave->getBreakpoints()) {
                    breakpoints.insert(points);
                }
            }
        } else if(element_ptr->type == ElementType::current_source) {
            auto wave = static_cast<CurrentSource*>(element_ptr)->ac_wave;
            if(wave) {
                for(double points : wave->getBreakpoints()) {
                    breakpoints.insert(points);
                }
            }
        }
    }
    context.stage = IntegrationStage::DC_Steady_State;
    double omega = 0.0;
    context.time = 0.0;
    // operating_point(element_map);
    recordNodeResults(element_map);
    for(const auto& [name,element_ptr] : element_map) {
    //    element_ptr->postSolve(context);
        element_ptr->commitStep();
        element_ptr->saveResults(omega);
    }
    results.time_or_freq.push_back(context.time);
    context.dt = determineFirstStep(element_map);
    double t = 0.0;
    double max_step = 0.0;
    double gamma = defaults.gamma;
    {
        double temp = determineMaxFrequency(element_map);
        max_step = (temp > 0.0) ? std::min(context.max_dt, 1.0 / (defaults.min_step_per_period * temp)) : context.max_dt;
    //    std::cout << "max frequency: " << temp << std::endl; 
    //    std::cout << "max step: " << max_step << std::endl;
    }
    context.dt = (context.dt > max_step) ? max_step : context.dt;
    int run = 0;
    omega = 2*std::numbers::pi*findCircuitFrequency(element_map);
    Eigen::VectorXd x_n;
    bool successful_step = true;
    while(t < context.stop) {
        run++;
    //    std::cout << "Trial: " << run << std::endl;
        if (context.stop-t < 1e-14) {
            t = context.stop;
            break; 
        }
        if(t+context.dt > context.stop) context.dt = context.stop-t;
        bool hit_breakpoint = false;
        auto point_it = breakpoints.upper_bound(t+1e-14);
        if((point_it != breakpoints.end()) && ((t+context.dt) >= *point_it)) {
            context.dt = *point_it-t;
            hit_breakpoint = true;
        }
        x_n = timeDomain.x;
        context.stage = IntegrationStage::TR_Stage;
        context.time = t+gamma*context.dt;
        double max_error_ratio = 0.0;
        successful_step = executeStep(element_map);
        if(successful_step) {
            context.stage = IntegrationStage::BDF2_Stage;
            context.time = t+context.dt;
            successful_step = executeStep(element_map);
            if(successful_step) max_error_ratio = determineMaxErrorRatio(element_map);
        }
        if((max_error_ratio < 1.0) && (successful_step)) {
//            std::cout << "Time: " << t << std::endl; 
//            std::cout << "dt: " << context.dt << std::endl;
            results.time_or_freq.push_back(t);
            t += context.dt;
            recordNodeResults(element_map);
            for(const auto& [name,element_ptr] : element_map) {
                element_ptr->commitStep();
                element_ptr->saveResults(omega);
            }
            double growth_factor = defaults.safety*std::pow(1.0/max_error_ratio,1.0/3.0);
            if(hit_breakpoint) {
                context.dt = std::max(1e-9,max_step*0.01);
            } else {
                context.dt *= clamp(growth_factor,0.5,2.0);
            }
            if(context.dt > max_step) context.dt = max_step;
        } else {
            timeDomain.x = x_n;
            double shrink_factor = defaults.safety*std::pow(1.0/max_error_ratio,1.0/3.0);
            context.dt *= clamp(shrink_factor,0.1,0.9);
        }         
        double completion = t/context.stop;
        std::cout << "\rProgress: " << completion*100 << "%\033[K";
        if(completion == 1) break;
        if(context.dt < defaults.step_min) {
            std::cout << "Simulation aborted: Timestep too small (underflow at t = " << t << " s)." << std::endl;
            break;
        }
    }
    std::cout << "\nSimulation Finished." << std::endl;
    std::cout << "Time steps: " << results.time_or_freq.size() << std::endl;
    // uses TR-BDF2
}
void CircuitSolver::ac_sweep(const std::unordered_map<std::string,Elements*>& element_map) {
    resultsStorageInitialization(element_map);
    operating_point(element_map);
    double omega = 0.0;
    for(const auto& [name,element_ptr] : element_map) {
        ElementType type = element_ptr->type;
        if((type == ElementType::voltage_source) || (type == ElementType::current_source)) {
            if(static_cast<Elements*>(element_ptr)->small_signal == nullptr) {
                std::cout << "No AC stimulus found for the source " << name << std::endl;
                return;
            }
        }
    }
    int numberOfFreqs = results.time_or_freq.size();
    for(int i = 0; i < numberOfFreqs; i++) {
        freqDomain.A.setZero();
        freqDomain.z.setZero();
        omega = 2*std::numbers::pi*results.time_or_freq[i];
        for(const auto& [_,element_ptr] : element_map) {
            element_ptr->stampACSweep(context,omega);
        }
        Elements::applyMinConductance();
        freqDomain.x = freqDomain.A.colPivHouseholderQr().solve(freqDomain.z);
        recordNodeResults(element_map);
        for(const auto& [_,element_ptr] : element_map) {
            element_ptr->saveResults(omega);
        }
        double completion = (i+1.0)/numberOfFreqs;
        std::cout << "\rProgress: " << completion*100 << "%\033[K";
        if(completion == 1) break;
    }
    std::cout << "\nSimulation Finished." << std::endl;
}
#pragma endregion

#pragma region public
void CircuitSolver::determineFrequencies(std::vector<double>& freq_range) {
    double min_frequency = (freq_range.front() > 0.0) ? freq_range.front() : defaults.f_min;
    double max_frequency = freq_range[1];
    double points = freq_range.back();
    results.time_or_freq.push_back(min_frequency);
    if(context.sweep == SweepType::linear) {
        double delta_f = (max_frequency-min_frequency)/(points-1);
        while(results.time_or_freq.back() <= max_frequency) {
            results.time_or_freq.push_back(results.time_or_freq.back()+delta_f);
        }
    } else {
        double k = std::pow(((context.sweep == SweepType::octave) ? 2.0 : 10.0),1.0/points);
        while(results.time_or_freq.back() <= max_frequency) {
            results.time_or_freq.push_back(results.time_or_freq.back()*k);
        }
    }
    if(results.time_or_freq.back() < max_frequency) results.time_or_freq.push_back(max_frequency);
}

int CircuitSolver::validCircuit(const std::unordered_map<std::string, Elements*>& element_map) {
    if(context.nodes == 0) {
        std::cout << "Circuit invalid: No active nodes exist." << std::endl;
        return -1;
    }
    bool grounded = false;
    bool groundExists = false;
    bool invalidElementValue = false;
    std::string unitName;
    for(const auto& e : element_map) {
        for(int pinCheck : e.second->node_ids) {
            if(pinCheck == -1) {
                std::cout << "Circuit invalid: Pin " << pinCheck << " of " << e.second->name << " is floating" << std::endl;
                return -1;
            } else if((pinCheck == 0) && (e.second->type != ElementType::gnd)) {
                grounded = true;
            }
            if(e.second->type == ElementType::gnd) groundExists = true;
        }
    }
    if(!grounded) {
        std::cout << "Circuit invalid: The circuit is not grounded." << std::endl;
        return -1;
    }
    if(!groundExists) {
        std::cout << "Ground is not present in the circuit." << std::endl;
        return -1;
    }
    for(const auto& [name,element_ptr] : element_map) {
        if(!(element_ptr->isValid(unitName))) {
            invalidValue(element_ptr->name,unitName);
            return -1;
        }
    }
    std::cout << "Circuit is valid." << std::endl;
    return 0;
}

void CircuitSolver::runSimulator(const std::unordered_map<std::string,Elements*>& element_map) {
    // may need to make it use all_elements in the future because of how hash maps store and look up stored data
    // consider making it such that it determines if there is a unique solution and double checks the results
    if(validCircuit(element_map) != 0) return;
    int current_branch_row = context.nodes;
    for(const auto& [name,element_ptr] : element_map) {
        auto& type = element_ptr->type;
        if((type == ElementType::voltage_source) || (type == ElementType::inductor) || (type == ElementType::capacitor)) {
            element_ptr->branch_matrix_id = current_branch_row++;
        }
    }
    {
        int size = current_branch_row;
        if(context.mode != SimulationMode::ac_sweep) {
            timeDomain.A = Eigen::MatrixXd::Zero(size,size);
            timeDomain.z = Eigen::VectorXd::Zero(size);
            timeDomain.x = Eigen::VectorXd::Zero(size);
        } else {
            freqDomain.A = Eigen::MatrixXcd::Zero(size,size);
            freqDomain.z = Eigen::VectorXcd::Zero(size);
            freqDomain.x = Eigen::VectorXcd::Zero(size);
        }
    }
    if(context.mode == SimulationMode::operating_point) {operating_point(element_map);}
    else if(context.mode == SimulationMode::transient) {transient(element_map);}
    else if(context.mode == SimulationMode::ac_sweep) {ac_sweep(element_map);}
    simulationRan = true;
}
#pragma endregion