#include "Elements.h"

#pragma region Elements
Elements::Elements(std::string n, ElementType t, int pins) : name(n), type(t) {node_ids.resize(pins, -1);}
double Elements::getInductance() {return leach.equiv_series_inductance;}
std::tuple<double,double> Elements::find_g_eq_c() {
    double gamma = defaults.gamma;
    double h = context.dt;
    double C = leach.equiv_parallel_capacitance;
    double g_eq_c = 0.0;
    double i_eq_c = 0.0;
    if(context.stage == IntegrationStage::TR_Stage) {
        g_eq_c = (2*C)/(gamma*h);
        i_eq_c = g_eq_c*past.v_c_n+past.i_c_n;
    } else if(context.stage == IntegrationStage::BDF2_Stage) {
        g_eq_c = (2.0-gamma)/(h*(1.0-gamma))*C;
        i_eq_c = -C*(-past.v_c_gamma/(gamma*(1.0-gamma)*h)+((1.0-gamma)/(gamma*h))*past.v_c_n);
    }
    return {g_eq_c,i_eq_c};
}
std::tuple<double,double> Elements::find_thevenin_l() {
    double gamma = defaults.gamma;
    double h = context.dt;
    double L = getInductance();
    double r_eq_l = 0.0;
    double v_eq_l = 0.0;
    if(context.stage == IntegrationStage::TR_Stage) {
        r_eq_l = (2.0*L)/(gamma*h);
        v_eq_l = -r_eq_l*past.i_l_n-past.v_l_n;
    } else if(context.stage == IntegrationStage::BDF2_Stage) {
        r_eq_l = (2.0-gamma)/(h*(1.0-gamma))*L;
        v_eq_l = L*(-past.i_l_gamma/(gamma*(1.0-gamma)*h)+(1.0-gamma)/(gamma*h)*past.i_l_n);
    }
    return {r_eq_l,v_eq_l};
}
void Elements::printInfo() const {
    std::string_view element_type = elements[(int)type];
    int flush_indent = longest_element_name-element_type.size();
    std::string indent(flush_indent,' ');
    std::cout << element_type << indent << " | " << name << " ";
}
void Elements::getNodes(bool newline) const {
    std::string s;
    int node = 0;
    for(int id : node_ids) {
        if(usePinsInsteadOfNodes) {
            node++;
            s += "Pin " + std::to_string(node) + ": " + (id == -1 ? "floating " : (std::to_string(id) + " "));
        } else {
            s += node_ids_and_names.nodeName(id) + " ";
        }
    }
    std::cout << s;
    if(newline = true) std::cout << "\n";
}
void Elements::printParasiticInfo() const {
    auto permitted = permissions_lookup_table.find(type);
    const Permissions& allowed = permitted->second;
    std::ostringstream info_ss;
    if((allowed.series_resistance) && (leach.equiv_series_resistance > 0.0)) info_ss << " Rser=" << leach.equiv_series_resistance;
    if((allowed.series_inductance) && (leach.equiv_series_inductance > 0.0)) info_ss << " Lser=" << leach.equiv_series_inductance;
    if((allowed.parallel_resistance) && (leach.equiv_parallel_resistance > 0.0)) info_ss << " Rpar=" << leach.equiv_parallel_resistance;
    if((allowed.parallel_capacitance) && (leach.equiv_parallel_capacitance > 0.0)) info_ss << " Cpar=" << leach.equiv_parallel_capacitance;
    std::cout << info_ss.str();
}
void Elements::modifyName(std::string newName) {name = newName;}
void Elements::modifyRser(double newRser) {leach.equiv_series_resistance = newRser;}
void Elements::modifyLser(double newLser) {leach.equiv_series_inductance = newLser;}
void Elements::modifyRpar(double newRpar) {leach.equiv_parallel_resistance = newRpar;}
void Elements::modifyCpar(double newCpar) {leach.equiv_parallel_capacitance = newCpar;}
void Elements::applyMinConductance() {
    for(int n = 1; n <= context.nodes; n++) {
        if(context.mode != SimulationMode::ac_sweep) {
            timeDomain.A(n-1,n-1) += defaults.g_min;
        } else {
            freqDomain.A(n-1,n-1) += defaults.g_min;
        }
    }
}
bool Elements::convergenceCheck() {return true;}
void Elements::commitStep() {
    past.v_c_n = past.candidate_v_c_n;
    past.i_c_n = past.candidate_i_c_n;
    past.v_l_n = past.candidate_v_l_n;
    past.i_l_n = past.candidate_i_l_n;
}
void Elements::printResults() const {
    std::cout << name << " Current: " << solution.current << "A" << std::endl;
}

void Elements::saveCircuit(std::ofstream& circuit_netlist_file) const {
    std::string_view element_type = elements[(int)type];
    int flush_indent = longest_element_name - element_type.size();
    std::string indent(flush_indent,' ');
    circuit_netlist_file << element_type << indent << " | " << name << " ";
}
void Elements::saveParasitic(std::ofstream& circuit_netlist_file) const {
    auto permitted = permissions_lookup_table.find(type);
    const Permissions& allowed = permitted->second;
    std::ostringstream info_ss;
    if((allowed.series_resistance) && (leach.equiv_series_resistance > 0.0)) info_ss << " Rser=" << leach.equiv_series_resistance;
    if((allowed.series_inductance) && (leach.equiv_series_inductance > 0.0)) info_ss << " Lser=" << leach.equiv_series_inductance;
    if((allowed.parallel_resistance) && (leach.equiv_parallel_resistance > 0.0)) info_ss << "Rpar=" << leach.equiv_parallel_resistance;
    if((allowed.parallel_capacitance) && (leach.equiv_parallel_capacitance > 0.0)) info_ss << " Cpar=" << leach.equiv_parallel_capacitance;
    circuit_netlist_file << info_ss.str();
}
void Elements::saveNodes(std::ofstream& circuit_netlist_file) const {
    for(const auto& node : node_ids) {
        circuit_netlist_file << node_ids_and_names.nodeName(node) << " ";
    }
    circuit_netlist_file << '\n';
}
#pragma endregion

#pragma region VoltageSource
VoltageSource::VoltageSource(std::string n) : 
Elements(n, ElementType::voltage_source, 2) {}
bool VoltageSource::isValid(std::string& unitName) {
    unitName = "V";
    if(ac_wave != nullptr) return true;
    if(small_signal != nullptr) return true;
    return (voltage > 0) ? true : false;
}
void VoltageSource::printInfo() const {
    Elements::printInfo();
    if(ac_wave != nullptr) {
        std::string type = ac_wave->getType();
        if(type == "SineWave") {
            std::shared_ptr<SineWave> sine_wave = std::dynamic_pointer_cast<SineWave>(ac_wave);
            std::cout << sine_wave->getAttributes();
        } else if(type == "Pulse") {
            std::shared_ptr<Pulse> pulse = std::dynamic_pointer_cast<Pulse>(ac_wave);
            std::cout << pulse->getAttributes();
        }
    } else {
        std::cout << voltage << " ";
    }
    if(small_signal != nullptr) {
        std::shared_ptr<SmallSignal> smallSignal = std::dynamic_pointer_cast<SmallSignal>(small_signal);
        std::cout << smallSignal->getAttributes();
    }
    Elements::printParasiticInfo();
    bool newline = true;
    Elements::getNodes(newline);
}
double VoltageSource::getVoltage() {
    if((context.mode != SimulationMode::operating_point) && (ac_wave != nullptr)) {
        return ac_wave->getValue();
    }
    return voltage;
}
void VoltageSource::modifyVoltage(double newVoltage) {voltage = newVoltage;}
void VoltageSource::attachSmallSignal(std::shared_ptr<Waveforms> signal) {small_signal = signal;}
void VoltageSource::attachWave(std::shared_ptr<Waveforms> wave) {ac_wave = wave;}
void VoltageSource::stamp(SimulationContext& context) {
    int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;
    double current_voltage = getVoltage();
    if(i > 0) {
        timeDomain.A(i-1,k) += 1;
        timeDomain.A(k,i-1) += 1;
    }
    if(j > 0) {
        timeDomain.A(j-1,k) -= 1;
        timeDomain.A(k,j-1) -= 1;
    }
    timeDomain.A(k,k) -= (leach.equiv_series_resistance > 0) ? leach.equiv_series_resistance : defaults.r_min;
    if((leach.equiv_parallel_capacitance > 0) && (context.mode != SimulationMode::operating_point)) {
        auto [g_eq_c,i_eq_c] = find_g_eq_c();
        if(i > 0) {
            timeDomain.A(i-1,i-1) += g_eq_c;
            timeDomain.z(i-1) += i_eq_c;
        }
        if(j > 0) {
            timeDomain.A(j-1,j-1) += g_eq_c;
            timeDomain.z(j-1) -= i_eq_c;
        }
        if((i > 0) && (j > 0)) {
            timeDomain.A(i-1,j-1) -= g_eq_c;
            timeDomain.A(j-1,i-1) -= g_eq_c;
        }
    }
    timeDomain.z(k) += current_voltage;
}
void VoltageSource::postSolve(SimulationContext& context) {
    int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;
    double v_i = (i > 0) ? timeDomain.x(i-1) : 0.0;
    double v_j = (j > 0) ? timeDomain.x(j-1) : 0.0;
    double v = v_i-v_j;
    solution.voltage_drop = v;
    solution.current = timeDomain.x(k);
    if(leach.equiv_parallel_capacitance > 0.0) {
        auto [g_eq_c,i_eq_c] = find_g_eq_c();
        if((context.stage == IntegrationStage::DC_Steady_State) || (context.mode == SimulationMode::operating_point)) {
            past.v_c_n = v;
            past.i_c_n = 0.0;
        } else if(context.stage == IntegrationStage::TR_Stage) {
            past.v_c_gamma = v;
            past.i_c_gamma = g_eq_c*v-i_eq_c;
        } else if(context.stage == IntegrationStage::BDF2_Stage) {
            past.candidate_v_c_n = v;
            past.candidate_i_c_n = g_eq_c*v-i_eq_c;
        }
    }   
}
void VoltageSource::stampACSweep(SimulationContext& context,double omega) {
    int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;
    using namespace std::complex_literals;
    if(i > 0) {
        freqDomain.A(i-1,k) += 1;
        freqDomain.A(k,i-1) += 1;
    }
    if(j > 0) {
        freqDomain.A(j-1,k) -= 1;
        freqDomain.A(k,j-1) -= 1;
    }
    double R = (leach.equiv_series_resistance > 0.0) ? leach.equiv_series_resistance : defaults.r_min;
    freqDomain.A(k,k) -= R;
    if(leach.equiv_parallel_capacitance > 0.0) {
        std::complex<double> Y = omega*leach.equiv_parallel_capacitance*1.0i;
        if(i > 0) freqDomain.A(i-1,i-1) += Y;
        if(j > 0) freqDomain.A(j-1,j-1) += Y;
        if((i > 0) && (j > 0)) {
            freqDomain.A(i-1,j-1) -= Y;
            freqDomain.A(j-1,i-1) -= Y;
        }
    }
    double v_mag = small_signal->getValue();
    double v_phase = small_signal->getPhase();
    std::complex<double> v_complex = std::polar(v_mag,v_phase);
    freqDomain.z(k) += v_complex;
}
void VoltageSource::saveResults(double omega) {
    using namespace std::complex_literals;
    auto save = results.dataPoints.find(name);
    if (save != results.dataPoints.end()) {
        CircuitSignal& signal = save->second;        
        if (context.mode != SimulationMode::ac_sweep) {
            signal.magnitude.push_back(solution.current);
        } else {
            std::complex<double> i_total = freqDomain.x(branch_matrix_id);            
            signal.magnitude.push_back(std::abs(i_total));
            signal.phase.push_back(std::arg(i_total)*(180.0/std::numbers::pi));
        }
    }
}

void VoltageSource::saveCircuit(std::ofstream& circuit_netlist_file) const {
    Elements::saveCircuit(circuit_netlist_file);
    std::ostringstream info_ss;
    if(ac_wave != nullptr) {
        std::string type = ac_wave->getType();
        if(type == "SineWave") {
            std::shared_ptr<SineWave> sine_wave = std::dynamic_pointer_cast<SineWave>(ac_wave);
            info_ss << sine_wave->getAttributes();
        } else if(type == "Pulse") {
            std::shared_ptr<Pulse> pulse = std::dynamic_pointer_cast<Pulse>(ac_wave);
            info_ss << pulse->getAttributes();
        }
    } else {
        info_ss << voltage << " ";
    }
    if(small_signal != nullptr) {
        std::shared_ptr<SmallSignal> smallSignal = std::dynamic_pointer_cast<SmallSignal>(small_signal);
        info_ss << smallSignal->getAttributes();
    }
    circuit_netlist_file << info_ss.str();
    Elements::saveParasitic(circuit_netlist_file);
    Elements::saveNodes(circuit_netlist_file);
}
#pragma endregion

#pragma region CurrentSource
CurrentSource::CurrentSource(std::string n) :
Elements(n, ElementType::current_source, 2) {}
bool CurrentSource::isValid(std::string& unitName) {
    unitName = "A";
    if(ac_wave != nullptr) return true;
    if(small_signal != nullptr) return true;
    return (current > 0) ? true : false;
}
void CurrentSource::printInfo() const {
    Elements::printInfo();
    if(ac_wave != nullptr) {
        std::string type = ac_wave->getType();
        if(type == "SineWave") {
            std::shared_ptr<SineWave> sine_wave = std::dynamic_pointer_cast<SineWave>(ac_wave);
            std::cout << sine_wave->getAttributes();
        } else if(type == "Pulse") {
            std::shared_ptr<Pulse> pulse = std::dynamic_pointer_cast<Pulse>(ac_wave);
            std::cout << pulse->getAttributes();
        }
    } else {
        std::cout << current << " ";
    }
    if(small_signal != nullptr) {
        std::shared_ptr<SmallSignal> smallSignal = std::dynamic_pointer_cast<SmallSignal>(small_signal);
        std::cout << smallSignal->getAttributes();
    }
    Elements::printParasiticInfo();
    bool newline = true;
    Elements::getNodes(newline);
}
double CurrentSource::getCurrent() {
    if((context.mode != SimulationMode::operating_point) && (ac_wave != nullptr)) {
        return ac_wave->getValue();
    }
    return current;
}
void CurrentSource::modifyCurrent(double newCurrent) {current = newCurrent;}
void CurrentSource::attachSmallSignal(std::shared_ptr<Waveforms> signal) {small_signal = signal;}
void CurrentSource::attachWave(std::shared_ptr<Waveforms> wave) {ac_wave = wave;}
void CurrentSource::stamp(SimulationContext& context) {
    int i = node_ids[0], j = node_ids[1];
    double current_current = getCurrent();
    if(i > 0) timeDomain.z(i-1) -= current_current;
    if(j > 0) timeDomain.z(j-1) += current_current;
    if(leach.equiv_parallel_resistance > 0.0) {
        double g = 1.0/leach.equiv_parallel_resistance;
        if(i > 0) timeDomain.A(i-1,i-1) += g;
        if(j > 0) timeDomain.A(j-1,j-1) += g;
        if((i > 0) && (j > 0)) {
            timeDomain.A(i-1, j-1) -= g;
            timeDomain.A(j-1, i-1) -= g;
        }
    }
    if((leach.equiv_parallel_capacitance > 0.0) && (context.mode != SimulationMode::operating_point)) {
        auto [g_eq_c,i_eq_c] = find_g_eq_c();
        if(i > 0) {
            timeDomain.A(i-1,i-1) += g_eq_c;
            timeDomain.z(i-1) += i_eq_c;
        }
        if(j > 0) {
            timeDomain.A(j-1,j-1) += g_eq_c;
            timeDomain.z(j-1) -= i_eq_c;
        }
        if((i > 0) && (j > 0)) {
            timeDomain.A(i-1,j-1) -= g_eq_c;
            timeDomain.A(j-1,i-1) -= g_eq_c;
        }
    }
}
void CurrentSource::postSolve(SimulationContext& context) {
    int i = node_ids[0], j = node_ids[1];
    double v_i = (i > 0) ? timeDomain.x(i-1) : 0.0;
    double v_j = (j > 0) ? timeDomain.x(j-1) : 0.0;
    double v = v_i-v_j;
    solution.voltage_drop = v;
    solution.current = current;
    if(leach.equiv_parallel_capacitance > 0.0) {
        auto [g_eq_c,i_eq_c] = find_g_eq_c();
        if((context.stage == IntegrationStage::DC_Steady_State) || (context.mode == SimulationMode::operating_point)) {
            past.v_c_n = v;
            past.i_c_n = 0.0;
        } else if(context.stage == IntegrationStage::TR_Stage) {
            past.v_c_gamma = v;
            past.i_c_gamma = g_eq_c*v-i_eq_c;
        } else if(context.stage == IntegrationStage::BDF2_Stage) {
            past.candidate_v_c_n = v;
            past.candidate_i_c_n = g_eq_c*v-i_eq_c;
        }
    }
}
void CurrentSource::stampACSweep(SimulationContext& context,double omega) {
    int i = node_ids[0], j = node_ids[1];
    using namespace std::complex_literals;
    if(leach.equiv_parallel_resistance > 0.0) {
        std::complex<double> Y_r = 1.0/leach.equiv_parallel_resistance;
        if(i > 0) freqDomain.A(i-1, i-1) += Y_r;
        if(j > 0) freqDomain.A(j-1, j-1) += Y_r;
        if(i > 0 && j > 0) {
            freqDomain.A(i-1, j-1) -= Y_r;
            freqDomain.A(j-1, i-1) -= Y_r;
        }
    }
    if(leach.equiv_parallel_capacitance > 0.0) {
        std::complex<double> Y_c = (omega*leach.equiv_parallel_capacitance)*1.0i;
        if(i > 0) freqDomain.A(i-1,i-1) += Y_c;
        if(j > 0) freqDomain.A(j-1,j-1) += Y_c;
        if(i > 0 && j > 0) {
            freqDomain.A(i-1,j-1) -= Y_c;
            freqDomain.A(j-1,i-1) -= Y_c;
        }
    }
    double i_mag = small_signal->getValue();
    double i_phase = small_signal->getPhase();
    std::complex<double> i_complex = std::polar(i_mag,i_phase);
    if(i > 0) freqDomain.z(i-1) -= i_complex;
    if(j > 0) freqDomain.z(j-1) += i_complex;
}
void CurrentSource::saveResults(double omega) {
    using namespace std::complex_literals;
    auto save = results.dataPoints.find(name);
    if (save != results.dataPoints.end()) {
        CircuitSignal& signal = save->second;        
        if (context.mode != SimulationMode::ac_sweep) {
            signal.magnitude.push_back(solution.current);
        } else {
            int i = node_ids[0], j = node_ids[1];
            std::complex<double> v_i = (i > 0) ? freqDomain.x(i-1) : 0.0;
            std::complex<double> v_j = (j > 0) ? freqDomain.x(j-1) : 0.0;
            std::complex<double> v_drop = v_i-v_j;
            std::complex<double> i_total = getCurrent();
            std::complex<double> Y;
            if(leach.equiv_parallel_resistance > 0.0) Y += 1.0/leach.equiv_parallel_resistance;
            if(leach.equiv_parallel_capacitance > 0.0) Y += (omega*leach.equiv_parallel_capacitance*1.0i);
            if((leach.equiv_parallel_resistance > 0.0) || (leach.equiv_parallel_capacitance > 0.0)) i_total += v_drop*Y;
            signal.magnitude.push_back(std::abs(i_total));
            signal.phase.push_back(std::arg(i_total)*(180.0/std::numbers::pi));
        }
    }
}

void CurrentSource::saveCircuit(std::ofstream& circuit_netlist_file) const {
    Elements::saveCircuit(circuit_netlist_file);
    std::ostringstream info_ss;
    if(ac_wave != nullptr) {
        std::string type = ac_wave->getType();
        if(type == "SineWave") {
            std::shared_ptr<SineWave> sine_wave = std::dynamic_pointer_cast<SineWave>(ac_wave);
            info_ss << sine_wave->getAttributes();
        } else if(type == "Pulse") {
            std::shared_ptr<Pulse> pulse = std::dynamic_pointer_cast<Pulse>(ac_wave);
            info_ss << pulse->getAttributes();
        }
    } else {
        info_ss << current << " ";
    }
    if(small_signal != nullptr) {
        std::shared_ptr<SmallSignal> smallSignal = std::dynamic_pointer_cast<SmallSignal>(small_signal);
        info_ss << smallSignal->getAttributes();
    }
    circuit_netlist_file << info_ss.str();
    Elements::saveParasitic(circuit_netlist_file);
    Elements::saveNodes(circuit_netlist_file);
}
#pragma endregion

#pragma region Resistor
Resistor::Resistor(std::string n) :
Elements(n, ElementType::resistor, 2) {}
bool Resistor::isValid(std::string& unitName) {
    unitName = "ohms";
    return (values.resistance > 0);
}
void Resistor::printInfo() const {
    Elements::printInfo();
    std::ostringstream ss;
    ss << values.resistance << " ";
    if(values.tolerance > 0.0) ss << "tol=" << values.tolerance << " ";
    if(values.power_rating > 0.0) ss << "pwr=" << values.power_rating << " ";
    std::cout << ss.str();
    Elements::printParasiticInfo();
    bool newline = true;
    Elements::getNodes(newline);
}
void Resistor::modifyResistance(double newResistance) {values.resistance = newResistance;}
void Resistor::modifyTolerance(double newTolerance) {values.tolerance = newTolerance;}
void Resistor::modifyPowerRating(double newPowerRating) {values.power_rating = newPowerRating;}
void Resistor::stamp(SimulationContext& context) {
    int i = node_ids[0], j = node_ids[1];
    if(context.mode == SimulationMode::operating_point) {
        double g = 1.0/values.resistance;
        if(i > 0) timeDomain.A(i-1,i-1) += g;
        if(j > 0) timeDomain.A(j-1,j-1) += g;
        if((i > 0) && (j > 0)) {
            timeDomain.A(i-1,j-1) -= g;
            timeDomain.A(j-1,i-1) -= g;
        }
        return;
    }
    if(leach.equiv_parallel_capacitance > 0.0) {
        auto [g_eq_c,i_eq_c] = find_g_eq_c();
        if(i > 0) {
            timeDomain.A(i-1,i-1) += g_eq_c;
            timeDomain.z(i-1) += i_eq_c;
        }
        if(j > 0) {
            timeDomain.A(j-1,j-1) += g_eq_c;
            timeDomain.z(j-1) -= i_eq_c;
        }
        if((i > 0) && (j > 0)) {
            timeDomain.A(i-1,j-1) -= g_eq_c;
            timeDomain.A(j-1,i-1) -= g_eq_c;
        }
    }
    double G_branch = 1.0/values.resistance;
    double I_norton = 0.0;
    if(leach.equiv_series_inductance > 0.0) {
        auto [r_eq_l,v_eq_l] = find_thevenin_l();
        double r_total = values.resistance+r_eq_l;
        G_branch = 1.0/r_total;
        I_norton = -v_eq_l/r_total;
    }
    if(i > 0) timeDomain.A(i-1,i-1) += G_branch;
    if(j > 0) timeDomain.A(j-1,j-1) += G_branch;
    if((i > 0) && (j > 0)) {
        timeDomain.A(i-1,j-1) -= G_branch;
        timeDomain.A(j-1,i-1) -= G_branch;
    }
    if(I_norton != 0.0) {
        if(i > 0) timeDomain.z(i-1) -= I_norton;
        if(j > 0) timeDomain.z(j-1) += I_norton;
    }
}
void Resistor::postSolve(SimulationContext& context) {
    int i = node_ids[0], j = node_ids[1];
    double v_i = (i > 0) ? timeDomain.x(i-1) : 0.0;
    double v_j = (j > 0) ? timeDomain.x(j-1) : 0.0;
    double v = v_i-v_j;
    double i_cap = 0.0;
    if((leach.equiv_parallel_capacitance > 0.0) && (context.mode != SimulationMode::operating_point)) {
        auto [g_eq_c,i_eq_c] = find_g_eq_c();
        i_cap = g_eq_c*v-i_eq_c;
    }
    double i_branch = v/values.resistance;
    double v_l = 0.0;
    if((leach.equiv_series_inductance > 0.0) && (context.mode != SimulationMode::operating_point)) {
        auto [r_eq_l,v_eq_l] = find_thevenin_l();
        double r_total = values.resistance+r_eq_l;
        i_branch = (v-v_eq_l)/r_total;
        v_l = r_eq_l*i_branch+v_eq_l;
    }
    solution.voltage_drop = v;
    solution.current = i_branch+i_cap;
    if((context.stage == IntegrationStage::DC_Steady_State) || (context.mode == SimulationMode::operating_point)) {
        past.v_c_n = v;
        past.i_c_n = 0.0;
        past.v_l_n = 0.0;
        past.i_l_n = i_branch;
        past.candidate_v_c_n = v;
        past.candidate_i_c_n = 0.0;
        past.candidate_v_l_n = 0.0;
        past.candidate_i_l_n = i_branch;
    } else if(context.stage == IntegrationStage::TR_Stage) {
        past.v_c_gamma = v;
        past.i_c_gamma = i_cap;
        past.v_l_gamma = v_l;
        past.i_l_gamma = i_branch;
    } else if(context.stage == IntegrationStage::BDF2_Stage) {
        past.candidate_v_c_n = v;
        past.candidate_i_c_n = i_cap;
        past.candidate_v_l_n = v_l;
        past.candidate_i_l_n = i_branch;
    }
}
void Resistor::stampACSweep(SimulationContext& context,double omega) {
    using namespace std::complex_literals;
    int i = node_ids[0], j = node_ids[1];
    std::complex<double> Y_total = 0.0;
    if(leach.equiv_series_inductance > 0.0) {
        std::complex<double> Z_series = values.resistance+(omega*leach.equiv_series_inductance*1.0i);
        Y_total = 1.0/Z_series;
    } else {
        Y_total = 1.0/values.resistance;
    }
    if(leach.equiv_parallel_capacitance > 0.0) Y_total += (omega*leach.equiv_parallel_capacitance*1.0i);
    if(i > 0) freqDomain.A(i-1,i-1) += Y_total;
    if(j > 0) freqDomain.A(j-1,j-1) += Y_total;
    if((i > 0) && (j > 0)) {
        freqDomain.A(i-1,j-1) -= Y_total;
        freqDomain.A(j-1,i-1) -= Y_total;
    }
}
void Resistor::saveResults(double omega) {
    using namespace std::complex_literals;
    auto save = results.dataPoints.find(name);
    if (save != results.dataPoints.end()) {
        CircuitSignal& signal = save->second;        
        if (context.mode != SimulationMode::ac_sweep) {
            signal.magnitude.push_back(solution.current);
        } else {
            int i = node_ids[0], j = node_ids[1];
            std::complex<double> v_i = (i > 0) ? freqDomain.x(i-1) : 0.0;
            std::complex<double> v_j = (j > 0) ? freqDomain.x(j-1) : 0.0;
            std::complex<double> v_drop = v_i-v_j;
            std::complex<double> Z = values.resistance+(omega*leach.equiv_series_inductance*1.0i);
            std::complex<double> Y = 1.0/Z;
            if(leach.equiv_parallel_capacitance > 0.0) Y += (omega*leach.equiv_parallel_capacitance*1.0i);
            std::complex<double> i_total = v_drop*Y;            
            signal.magnitude.push_back(std::abs(i_total));
            signal.phase.push_back(std::arg(i_total)*(180.0/std::numbers::pi));
        }
    }
}

void Resistor::saveCircuit(std::ofstream& circuit_netlist_file) const {
    Elements::saveCircuit(circuit_netlist_file);
    std::ostringstream info_ss;
    info_ss << values.resistance << " ";
    if(values.tolerance > 0.0) info_ss << " tol=" << values.tolerance << " ";
    if(values.power_rating > 0.0) info_ss << " pwr=" << values.power_rating << " ";
    circuit_netlist_file << info_ss.str();
    Elements::saveParasitic(circuit_netlist_file);
    Elements::saveNodes(circuit_netlist_file);
}
#pragma endregion

#pragma region Capacitor
Capacitor::Capacitor(std::string n) :
Elements(n, ElementType::capacitor, 2) {}
double Capacitor::totalCapacitance() {return values.capacitance+leach.equiv_parallel_capacitance;}
std::tuple<double,double> Capacitor::find_thevenin_c() {
    double gamma = defaults.gamma;
    double h = context.dt;
    double C = totalCapacitance();
    double r_eq_c = 0.0;
    double v_eq_c = 0.0;
    if(context.stage == IntegrationStage::TR_Stage) {
        r_eq_c = (gamma*h)/(2.0*C);
        v_eq_c = r_eq_c*past.i_c_n+past.v_c_n;
    } else if(context.stage == IntegrationStage::BDF2_Stage) {
        r_eq_c = (h*(1.0-gamma))/((2.0-gamma)*C);
        v_eq_c = (1.0/(gamma*(2.0-gamma)))*past.v_c_gamma-((1.0-gamma)*(1.0-gamma)/(gamma*(2.0-gamma)))*past.v_c_n;
    }
    return {r_eq_c,v_eq_c};
}
std::tuple<double,double> Capacitor::find_thevenin_series() {
    double r_eq_branch = 0.0;
    double v_eq_branch = 0.0;
    auto [r_eq_c,v_eq_c] = find_thevenin_c();
    auto [r_eq_l,v_eq_l] = find_thevenin_l();
    r_eq_branch = leach.equiv_series_resistance+r_eq_c+r_eq_l;
    v_eq_branch = v_eq_c+v_eq_l;
    return {r_eq_branch,v_eq_branch};
}
bool Capacitor::isValid(std::string& unitName) {
    unitName = "F";
    return (values.capacitance > 0);
}
void Capacitor::printInfo() const {
    Elements::printInfo();
    std::ostringstream ss;
    ss << values.capacitance << " ";
    if(values.voltage_rating > 0.0) ss << "V=" << values.voltage_rating << " ";
    if(values.RMS_current_rating > 0.0) ss << "Irms=" << values.RMS_current_rating << " ";
    std::cout << ss.str();
    Elements::printParasiticInfo();
    bool newline = true;
    Elements::getNodes(newline);
}
void Capacitor::modifyCapacitance(double newCapacitance) {values.capacitance = newCapacitance;}
void Capacitor::modifyVoltageRating(double newVoltageRating) {values.voltage_rating = newVoltageRating;}
void Capacitor::modifyRMSCurrentRating(double newRMSCurrentRating) {values.RMS_current_rating = newRMSCurrentRating;}
void Capacitor::stamp(SimulationContext& context) {
    int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;
    if(leach.equiv_parallel_resistance > 0.0) {
        double g_eq = 1.0/leach.equiv_parallel_resistance;
        if(i > 0) timeDomain.A(i-1,i-1) += g_eq;
        if(j > 0) timeDomain.A(j-1,j-1) += g_eq;
        if((i > 0) && (j > 0)) {
            timeDomain.A(i-1,j-1) -= g_eq;
            timeDomain.A(j-1,i-1) -= g_eq;
        }
    }
    if(context.mode == SimulationMode::operating_point) {
        timeDomain.A(k,k) += 1.0;
        timeDomain.z(k) = 0.0;
        return;
    }
    if(i > 0) {
        timeDomain.A(i-1,k) += 1;
        timeDomain.A(k,i-1) += 1;
    }
    if(j > 0) {
        timeDomain.A(j-1,k) -= 1;
        timeDomain.A(k,j-1) -= 1;
    }
    auto [r_eq_branch,v_eq_branch] = find_thevenin_series();
    timeDomain.A(k,k) -= r_eq_branch;
    timeDomain.z(k) += v_eq_branch;
}
void Capacitor::postSolve(SimulationContext& context) {
    int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;
    double v_i = (i > 0) ? timeDomain.x(i-1) : 0.0;
    double v_j = (j > 0) ? timeDomain.x(j-1) : 0.0;
    double v = v_i-v_j;
    solution.voltage_drop = v;
    double i_branch = timeDomain.x(k);
    double i_epr = (leach.equiv_parallel_resistance > 0.0) ? (v / leach.equiv_parallel_resistance) : 0.0;
    solution.current = i_branch + i_epr;
    auto [r_eq_l,v_eq_l] = find_thevenin_l();
    double i_core = i_branch;
    double v_core_l = i_core*r_eq_l+v_eq_l;
    double v_core_c = v-(i_core*leach.equiv_series_resistance)-v_core_l;
    if((context.stage == IntegrationStage::DC_Steady_State) || (context.mode == SimulationMode::operating_point)) {
        past.v_c_n = v;
        past.i_c_n = 0.0;
        past.v_l_n = 0.0;
        past.i_l_n = 0.0;
        past.candidate_v_c_n = v;
        past.candidate_i_c_n = 0.0;
        past.candidate_v_l_n = 0.0;
        past.candidate_i_l_n = 0.0;
    } else if(context.stage == IntegrationStage::TR_Stage) {
        past.v_c_gamma = v_core_c;
        past.i_c_gamma = i_core;
        past.v_l_gamma = v_core_l;
        past.i_l_gamma = i_core;
    } else if(context.stage == IntegrationStage::BDF2_Stage) {
        past.candidate_v_c_n = v_core_c;
        past.candidate_i_c_n = i_core;
        past.candidate_v_l_n = v_core_l;
        past.candidate_i_l_n = i_core;
    }
}
void Capacitor::stampACSweep(SimulationContext& context,double omega) {
    using namespace std::complex_literals;
    int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;
    std::complex<double> Y_p = 0.0;
    if(leach.equiv_parallel_resistance > 0.0) Y_p += 1.0/leach.equiv_parallel_resistance;
    if(leach.equiv_parallel_capacitance > 0.0) Y_p += omega*leach.equiv_parallel_capacitance*1.0i;
    if(i > 0) freqDomain.A(i-1,i-1) += Y_p;
    if(j > 0) freqDomain.A(j-1,j-1) += Y_p;
    if((i > 0) && (j > 0)) {
        freqDomain.A(i-1,j-1) -= Y_p;
        freqDomain.A(j-1,i-1) -= Y_p;
    }
    std::complex<double> Z_s = leach.equiv_series_resistance+(omega*leach.equiv_series_inductance*1.0i)+1.0/(omega*values.capacitance*1.0i);
    std::complex<double> Y_s = 1.0/Z_s;
    if(i > 0) {
        freqDomain.A(i-1,k) += 1.0;
        freqDomain.A(k,i-1) += 1.0;
    }
    if(j > 0) {
        freqDomain.A(j-1,k) -= 1.0;
        freqDomain.A(k,j-1) -= 1.0;
    }
    freqDomain.A(k,k) -= Z_s;
}
void Capacitor::saveResults(double omega) {
    using namespace std::complex_literals;
    auto save = results.dataPoints.find(name);
    if (save != results.dataPoints.end()) {
        CircuitSignal& signal = save->second;        
        if (context.mode != SimulationMode::ac_sweep) {
            signal.magnitude.push_back(solution.current);
        } else {
            int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;
            std::complex<double> v_i = (i > 0) ? freqDomain.x(i-1) : 0.0;
            std::complex<double> v_j = (j > 0) ? freqDomain.x(j-1) : 0.0;
            std::complex<double> v_drop = v_i-v_j;
            std::complex<double> i_branch = freqDomain.x(k);
            std::complex<double> y_p = 0.0;
            if(leach.equiv_parallel_resistance > 0.0) y_p += 1.0/leach.equiv_parallel_resistance;
            if(leach.equiv_parallel_capacitance > 0.0) y_p += omega*leach.equiv_parallel_capacitance*1.0i;
            std::complex<double> i_total = i_branch+v_drop*y_p;            
            signal.magnitude.push_back(std::abs(i_total));
            signal.phase.push_back(std::arg(i_total)*(180.0/std::numbers::pi));
        }
    }
}

void Capacitor::saveCircuit(std::ofstream& circuit_netlist_file) const {
    Elements::saveCircuit(circuit_netlist_file);
    std::ostringstream info_ss;
    info_ss << values.capacitance << " ";
    if(values.voltage_rating > 0.0) info_ss << " V=" << values.voltage_rating << " ";
    if(values.RMS_current_rating > 0.0) info_ss << " Irms=" << values.RMS_current_rating << " ";
    circuit_netlist_file << info_ss.str();
    Elements::saveParasitic(circuit_netlist_file);
    Elements::saveNodes(circuit_netlist_file);
}
#pragma endregion

#pragma region Inductor
Inductor::Inductor(std::string n) :
Elements(n, ElementType::inductor, 2) {}
double Inductor::getInductance() {return values.inductance;}
bool Inductor::isValid(std::string& unitName) {
    unitName = "H";
    return (values.inductance > 0);
}
void Inductor::printInfo() const {
    Elements::printInfo();
    std::ostringstream ss;
    ss << values.inductance << " ";
    if(values.peak_current > 0.0) ss << "Ipk=" << values.peak_current;
    std::cout << ss.str();
    Elements::printParasiticInfo();
    bool newline = true;
    Elements::getNodes(newline);
}
void Inductor::modifyInductance(double newInductance) {values.inductance = newInductance;}
void Inductor::modifyPeakCurrent(double newPeakCurrent) {values.peak_current = newPeakCurrent;}
void Inductor::stamp(SimulationContext& context) {
    int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;
    if(leach.equiv_parallel_resistance > 0.0) {
        double g_eq = 1.0/leach.equiv_parallel_resistance;
        if(i > 0) timeDomain.A(i-1,i-1) += g_eq;
        if(j > 0) timeDomain.A(j-1,j-1) += g_eq;
        if((i > 0) && (j > 0)) {
            timeDomain.A(i-1,j-1) -= g_eq;
            timeDomain.A(j-1,i-1) -= g_eq;
        }
    }
    if(context.mode == SimulationMode::operating_point) {
        if(i > 0) {
            timeDomain.A(i-1,k) += 1.0;
            timeDomain.A(k,i-1) += 1.0;
        }
        if(j > 0) {
            timeDomain.A(j-1,k) -= 1.0;
            timeDomain.A(k,j-1) -= 1.0;
        }
        if(leach.equiv_series_resistance > 0.0) {
            timeDomain.A(k,k) -= leach.equiv_series_resistance;
        } else {
            timeDomain.A(k,k) -= defaults.r_min;
        }
        timeDomain.z(k) = 0.0;
        return;
    }
    auto [r_eq_l,v_eq_l] = find_thevenin_l();
    double r_total = leach.equiv_series_resistance+r_eq_l;
    if(r_total <= 0.0) r_total = defaults.r_min;
    if(leach.equiv_parallel_capacitance > 0.0) {
        auto [g_eq_c,i_eq_c] = find_g_eq_c();
        if(i > 0) {
            timeDomain.A(i-1,k) += 1.0;
            timeDomain.A(k,i-1) += (1.0+r_total*g_eq_c);
        }
        if(j > 0) {
            timeDomain.A(j-1,k) -= 1.0;
            timeDomain.A(k,j-1) -= (1.0+r_total*g_eq_c);
        }
        timeDomain.z(j-1) += v_eq_l-r_eq_l*i_eq_c;
    } else {
        if(i > 0) {
            timeDomain.A(i-1,k) += 1.0;
            timeDomain.A(k,i-1) += 1.0;
        }
        if(j > 0) {
            timeDomain.A(j-1,k) -= 1.0;
            timeDomain.A(k,j-1) -= 1.0;
        }
        timeDomain.z(k) += v_eq_l;
    }
    timeDomain.A(k, k) -= r_total;
}
void Inductor::postSolve(SimulationContext& context) {
    int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;
    double v_i = (i > 0) ? timeDomain.x(i-1) : 0.0;
    double v_j = (j > 0) ? timeDomain.x(j-1) : 0.0;
    double v = v_i-v_j;
    double i_branch = timeDomain.x(k);
    solution.voltage_drop = v;
    solution.current = i_branch;
    double i_cap = 0.0;
    if(leach.equiv_parallel_capacitance > 0) {
        auto [g_eq_c,i_eq_c] = find_g_eq_c();
        i_cap = g_eq_c*v-i_eq_c;
    }
    auto [r_eq_l,v_eq_l] = find_thevenin_l();
    double i_core = i_branch-i_cap;
    double v_core = v-(i_core*leach.equiv_series_resistance);
    if((context.stage == IntegrationStage::DC_Steady_State) || (context.mode == SimulationMode::operating_point)) {
        past.v_c_n = v;
        past.i_c_n = 0.0;
        past.v_l_n = v_core;
        past.i_l_n = i_branch;
        past.candidate_v_c_n = v;
        past.candidate_i_c_n = 0.0;
        past.candidate_v_l_n = v_core;
        past.candidate_i_l_n = i_branch;
    } else if(context.stage == IntegrationStage::TR_Stage) {
        past.v_c_gamma = v;
        past.i_c_gamma = i_cap;
        past.v_l_gamma = v_core;
        past.i_l_gamma = i_core;
    } else if(context.stage == IntegrationStage::BDF2_Stage) {
        past.candidate_v_c_n = v;
        past.candidate_i_c_n = i_cap;
        past.candidate_v_l_n = v_core;
        past.candidate_i_l_n = i_core;
    }
}
void Inductor::stampACSweep(SimulationContext& context,double omega) {
    using namespace std::complex_literals;
    int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;
    if(i > 0) {
        freqDomain.A(i-1,k) += 1.0;
        freqDomain.A(k,i-1) += 1.0;
    }
    if(j > 0) {
        freqDomain.A(j-1,k) -= 1.0;
        freqDomain.A(k,j-1) -= 1.0;
    }
    if(leach.equiv_parallel_resistance > 0.0) {
        double Y_eq = 1.0/leach.equiv_parallel_resistance;
        if(i > 0) freqDomain.A(i-1,i-1) += Y_eq;
        if(j > 0) freqDomain.A(j-1,j-1) += Y_eq;
        if((i > 0) && (j > 0)) {
            freqDomain.A(i-1,j-1) -= Y_eq;
            freqDomain.A(j-1,i-1) -= Y_eq;
        }
    }
    if(leach.equiv_parallel_capacitance> 0.0) {
        std::complex<double> Y_p = omega*leach.equiv_parallel_capacitance*1.0i;
        if(i > 0) freqDomain.A(i-1,i-1) += Y_p;
        if(j > 0) freqDomain.A(j-1,j-1) += Y_p;
        if((i > 0) && (j > 0)) {
            freqDomain.A(i-1,j-1) -= Y_p;
            freqDomain.A(j-1,i-1) -= Y_p;
        }
    }
    std::complex<double> Z = leach.equiv_series_resistance+(omega*values.inductance*1.0i);
    freqDomain.A(k,k) -= Z;
}
void Inductor::saveResults(double omega) {
    using namespace std::complex_literals;
    auto save = results.dataPoints.find(name);
    if (save != results.dataPoints.end()) {
        CircuitSignal& signal = save->second;
        if (context.mode != SimulationMode::ac_sweep) {
            signal.magnitude.push_back(solution.current);
        } else {
            int i = node_ids[0], j = node_ids[1], k = branch_matrix_id;                    
            std::complex<double> v_i = (i > 0) ? freqDomain.x(i-1) : 0.0;
            std::complex<double> v_j = (j > 0) ? freqDomain.x(j-1) : 0.0;
            std::complex<double> v_drop = v_i-v_j;                    
            std::complex<double> i_branch = freqDomain.x(k);                    
            std::complex<double> y_parallel = 0.0;
            if (leach.equiv_parallel_resistance > 0.0) y_parallel += 1.0/leach.equiv_parallel_resistance;
            if (leach.equiv_parallel_capacitance > 0.0) y_parallel += omega*leach.equiv_parallel_capacitance*1.0i;            
            std::complex<double> i_total = i_branch+(v_drop*y_parallel);
            signal.magnitude.push_back(std::abs(i_total));
            signal.phase.push_back(std::arg(i_total)*(180.0/std::numbers::pi));
        }
    }
}

void Inductor::saveCircuit(std::ofstream& circuit_netlist_file) const {
    Elements::saveCircuit(circuit_netlist_file);
    std::ostringstream info_ss;
    info_ss << values.inductance << " ";
    if(values.peak_current > 0.0) info_ss << " Ipk=" << values.peak_current << " ";
    circuit_netlist_file << info_ss.str();
    Elements::saveParasitic(circuit_netlist_file);
    Elements::saveNodes(circuit_netlist_file);
}
#pragma endregion

#pragma region Diode
Diode::Diode(std::string n) : Elements(n, ElementType::diode, 2) {}
template<typename D>
D Diode::pnjlim(D V_new,D V_old,D nV_T,D V_crit) {
    if constexpr (is_complex<D>::value) {
        return V_new;
    } else {
        if((V_new > V_crit) && (std::abs(V_new-V_old) > (2*nV_T))) {
            if(V_old > 0.0) {
                D arg = 1.0+(V_new-V_old)/nV_T;
                if(arg > 0.0) V_new = V_old+nV_T*std::log(arg);
                else V_new = V_crit;
            } else V_new = nV_T*std::log(V_new/nV_T);
        }
        return V_new;
    }
}
bool Diode::isValid(std::string& unitName) {return true;}
void Diode::printInfo() const {
    Elements::printInfo();
    std:: cout << model_name << " ";
    bool newline = true;
    Elements::getNodes(newline);
}
void Diode::stamp(SimulationContext& context) {
    int i  = node_ids[0], j = node_ids[1];
    double v_i = (i > 0) ? timeDomain.x(i-1) : 0.0;
    double v_j = (j > 0) ? timeDomain.x(j-1) : 0.0;
    double V_D_unlimited = v_i-v_j;
    double nV_T = values.n*context.V_T;
    double V_crit = nV_T*std::log(nV_T/(sqrt(2)*values.Is));
    double V_D = pnjlim(V_D_unlimited,past.V_D,nV_T,V_crit);
    past.V_D = V_D;
    double I_D = values.Is*(exp(V_D/(nV_T))-1.0);
    past.I_D = I_D;
    double g_d = (I_D+values.Is)/(nV_T);
    double I_eq = g_d*V_D-I_D;
    if(i > 0) {
        timeDomain.A(i-1,i-1) += g_d;
        timeDomain.z(i-1) += I_eq;
    }
    if(j > 0) {
        timeDomain.A(j-1,j-1) += g_d;
        timeDomain.z(j-1) -= I_eq;
    }
    if((i > 0) && (j > 0)) {
        timeDomain.A(i-1,j-1) -= g_d;
        timeDomain.A(j-1,i-1) -= g_d;
    }
}
bool Diode::convergenceCheck() {
    int i  = node_ids[0], j = node_ids[1];
    double relative_tolerance = defaults.relative_tolerance;
    double v_i = (i > 0) ? timeDomain.x(i-1) : 0.0;
    double v_j = (j > 0) ? timeDomain.x(j-1) : 0.0;
    double V_D = v_i-v_j;
    double DeltaV_D = std::abs(V_D-past.V_D);
    double max_allowed_v_error = defaults.absolute_tolerance_v+relative_tolerance*std::max(std::abs(V_D),std::abs(past.V_D));
    bool voltageConverge = DeltaV_D < max_allowed_v_error;
    double I_D = values.Is*(exp(V_D/(values.n*context.V_T))-1);
    double DeltaI_D = std::abs(I_D-past.I_D);
    double max_allowed_i_error = defaults.absolute_tolerance_i+relative_tolerance*std::max(std::abs(I_D),std::abs(past.I_D));
    bool currentConverge = DeltaI_D < max_allowed_i_error;
    return ((voltageConverge) && (currentConverge));
}
void Diode::postSolve(SimulationContext& context) {
    int i  = node_ids[0], j = node_ids[1];
    double v_i = (i > 0) ? timeDomain.x(i-1) : 0.0;
    double v_j = (j > 0) ? timeDomain.x(j-1) : 0.0;
    double V_D_unlimited = v_i-v_j;
    double nV_T = values.n*context.V_T;
    double V_crit = nV_T*std::log(nV_T/(sqrt(2)*values.Is));
    double V_D = pnjlim(V_D_unlimited,past.V_D,nV_T,V_crit);
    double I_D = values.Is*(exp(V_D/(nV_T))-1.0);
    solution.current = I_D;
}
void Diode::stampACSweep(SimulationContext& context,double omega) {
    int i  = node_ids[0], j = node_ids[1];
    double I_D = solution.current;
    double nV_T = values.n*context.V_T;
    double g_d = (I_D+values.Is)/nV_T;
    if(i > 0) freqDomain.A(i-1,i-1) += g_d;
    if(j > 0) freqDomain.A(j-1,j-1) += g_d;
    if((i > 0) && (j > 0)) {
        freqDomain.A(i-1,j-1) -= g_d;
        freqDomain.A(j-1,i-1) -= g_d;
    }
}
void Diode::saveResults(double omega) {
    using namespace std::complex_literals;
    auto save = results.dataPoints.find(name);
    if(save != results.dataPoints.end()) {
        CircuitSignal& signal = save->second;
        if(context.mode != SimulationMode::ac_sweep) {
            signal.magnitude.push_back(solution.current);
        } else {
            int i  = node_ids[0], j = node_ids[1];
            std::complex<double> v_i = (i > 0) ? timeDomain.x(i-1) : 0.0;
            std::complex<double> v_j = (j > 0) ? timeDomain.x(j-1) : 0.0;
            std::complex<double> V_D = v_i-v_j;
            double I_D_op = solution.current;
            double nV_T = values.n*context.V_T;
            double g_d = (I_D_op+values.Is)/nV_T;
            std::complex<double> I_D_ac = g_d*V_D;
            signal.magnitude.push_back(std::abs(I_D_ac));
            signal.phase.push_back(std::arg(I_D_ac)*(180.0/std::numbers::pi));
        }
    }
}
void Diode::saveCircuit(std::ofstream& circuit_netlist_file) const {
    Elements::saveCircuit(circuit_netlist_file);
    Elements::saveNodes(circuit_netlist_file);
}
#pragma endregion
// to be implemented later on after the diode is finished
#pragma region Transistor
Transistor::Transistor(std::string n) : Elements(n, ElementType::transistor, 3) {}
bool Transistor::isValid(std::string& unitName) {return true;}
void Transistor::printInfo() const {
    Elements::printInfo();
    std:: cout << model_name << " " << std::endl;
    bool newline = true;
    Elements::getNodes(newline);
}
void Transistor::stamp(SimulationContext& context) {

}
void Transistor::postSolve(SimulationContext& context) {

}
void Transistor::stampACSweep(SimulationContext& context,double omega) {

}
void Transistor::saveResults(double omega) {

}
void Transistor::saveCircuit(std::ofstream& circuit_netlist_file) const {
    Elements::saveCircuit(circuit_netlist_file);
    Elements::saveNodes(circuit_netlist_file);
}
#pragma endregion

#pragma region Ground
Ground::Ground(std::string n) : Elements(n, ElementType::gnd, 1) {}
bool Ground::isValid(std::string& unitName) {return true;}
void Ground::printInfo() const {
        Elements::printInfo();
        bool newline = true;
        Elements::getNodes(newline);
}
void Ground::stamp(SimulationContext& context) {}
void Ground::postSolve(SimulationContext& context) {}
void Ground::stampACSweep(SimulationContext& context,double omega) {}
void Ground::saveResults(double omega) {}

void Ground::saveCircuit(std::ofstream& circuit_netlist_file) const {
    Elements::saveCircuit(circuit_netlist_file);
    Elements::saveParasitic(circuit_netlist_file);
    Elements::saveNodes(circuit_netlist_file);
}
#pragma endregion