#include "CircuitManager.h"

CircuitSolver solver;

#pragma region private
std::tuple<std::string,bool> CircuitManager::name_check(std::string name_to_check) {
    if(name_to_check.empty()) {
        std::cout << "No new name for the element was given." << std::endl;
        return {" ",false};
    }
    auto is_unique = [this](const std::string& name_to_check) -> bool {
        return element_map.find(name_to_check) == element_map.end();
    };
    if(!(is_unique(name_to_check))) {
        std::cout << "Component name is already in use." << std::endl;
        return {" ",false};
    }
    return {name_to_check,true};
}
bool CircuitManager::isNumber(std::string input) {
    if(input.empty()) return false;
    try {
        size_t non_numeric_char;
        double temp = std::stod(input,&non_numeric_char);
        if(non_numeric_char == input.length()) return true;
    } catch(const std::exception& error) {}
    try {
        bool silent = true;
        double temp = EnginUnits(input,silent);
        if(temp != 0.0) return true;
    } catch(const std::exception& error) {}
    return false;
}
std::stringstream CircuitManager::extractWaveValues(std::stringstream& ss) {
    std::string buffer;
    std::getline(ss,buffer,')');
    return std::stringstream(buffer);
}
bool CircuitManager::effectivelyEmpty(std::stringstream& ss) {
    ss >> std::ws;
    return (ss.peek() == std::stringstream::traits_type::eof());
}

void CircuitManager::register_element(std::unique_ptr<Elements> e) {
    element_map[e->name] = e.get();
    all_elements.push_back(std::move(e));
    Logger::info("Registered " + all_elements.back()->name);
    info.changesMade = true;
}
void CircuitManager::merge_nets(const int merged_node_id,std::vector<int>& merge_net_list) {
    std::unordered_set<int> nets_to_change(merge_net_list.begin(),merge_net_list.end());
    for(auto& element : all_elements) {
        for(auto& node : element->node_ids) {
            if(nets_to_change.count(node)) node = merged_node_id;
        }
    }
/*
    Iterating through every element every time you add a wire can get slow in huge circuits. 
    If your simulations start lagging, look into a data structure algorithm called 
    Disjoint-Set (Union-Find). It is perfectly designed for tracking and merging grouped 
    networks efficiently.
*/
}

std::string CircuitManager::defaultName(ElementType type) {
    std::string name;
    name.push_back(defaultLetter[(int)type]);
    int *item = number_of[(int)type]; 
    while(true) {
        name += std::to_string(*item);
        auto search = element_map.find(name);
        if(search == element_map.end()) break;
        (*item)++;
        name.erase(1);
    }
    return name;
}
void CircuitManager::addVoltageSource(std::string name) {
    auto v = std::make_unique<VoltageSource>(name);
    register_element(std::move(v));
    info.v_sources++;
}
void CircuitManager::addCurrentSource(std::string name) {
    auto i = std::make_unique<CurrentSource>(name);
    register_element(std::move(i));
    info.i_sources++;
}
void CircuitManager::addResistor(std::string name) {
    auto r = std::make_unique<Resistor>(name);
    register_element(std::move(r));
}
void CircuitManager::addCapacitor(std::string name) {
    auto c = std::make_unique<Capacitor>(name);
    register_element(std::move(c));
    info.c_sources++;
}
void CircuitManager::addInductor(std::string name) {
    auto l = std::make_unique<Inductor>(name);
    register_element(std::move(l));
    info.l_sources++;
}
void CircuitManager::addDiode(std::string name) {
    auto d = std::make_unique<Diode>(name);
    register_element(std::move(d));
}
void CircuitManager::addTranistor(std::string name) {
    auto q = std::make_unique<Transistor>(name);
    register_element(std::move(q));
}
void CircuitManager::addGround(std::string name) {
    auto gnd = std::make_unique<Ground>(name);
    gnd->node_ids[0] = 0;
    node_ids_and_names.addToBimap(0,'N' + std::to_string(0));
    register_element(std::move(gnd));
}

void CircuitManager::changeNodeName(std::stringstream& ss) {
    std::string name, newName;
    if(!(ss >> name)) {
        std::cout << "No node's name to modify was specified." << std::endl;
        return;
    }
    if(!(ss >> newName)) {
        std::cout << "No new name was entered for the node " << name << "." << std::endl;
        return;
    }
    node_ids_and_names.changeName(name,newName);
}
#pragma endregion

#pragma region public
void CircuitManager::addElement(std::stringstream& ss) {
    std::string element_type, name_to_check;
    if(!(ss >> element_type)) {
        std::cout << "No element type specified." << std::endl;
        return;
    }
    if(!(ss >> name_to_check)) {
        auto search = element_type_lookup_table.find(element_type);
        name_to_check = defaultName(search->second);
    }
    auto [name,isUnique] = name_check(name_to_check);
    if(!isUnique) return;
    auto lookup_element = element_lookup_table.find(element_type);
    if(lookup_element == element_lookup_table.end()) {
        std::cout << element_type << "is not a valid type of element." << std::endl;
        return;
    }
    lookup_element->second(name);
    if(effectivelyEmpty(ss)) return;
    editElement(ss,name);
}
void CircuitManager::editElement(std::stringstream& ss,std::string name) {
    if(name.empty()) {
        if(!(ss >> name)) {
            std::cout << "The element you wish to set values for my be declared." << std::endl;
            return;
        }
    }
    if(effectivelyEmpty(ss)) {
        std::cout << "Nothing was specified to change regarding the element " << name << std::endl;
        return;
    }
    auto element = element_map.find(name);
    if(element == element_map.end()) {
        std::cout << "No element called " << name << " exists." << std::endl;
        return;
    }
    ElementType type = element->second->type;
    auto permitted = permissions_lookup_table.find(type);
    if(permitted == permissions_lookup_table.end()) {
        std::cout << "Element type " << elements[(int)type] << " is not included in the permissions look up table." << std::endl;
        return;
    }
    const Permissions& allowed = permitted->second;
    Elements* e = static_cast<Elements*>(element->second);
    std::string token;
    while(ss >> token) {
        if(token.contains("name")) {
            size_t end_of_key = token.find("=");
            if(end_of_key == std::string::npos) {
                std::cout << "Error occured when trying to change the name of " << e->name << "." << std::endl;
                std::cout << "To change the name of " << e->name << " use name=newName." << std::endl;
            }
            token.erase(0,end_of_key+1);
            auto [newName,isUnique] = name_check(token);
            if(!isUnique) return;
            auto search = element_map.find(name);
            auto element = std::find_if(all_elements.begin(),all_elements.end(),
                [&name](const std::unique_ptr<Elements> &e) {
                return ((e) && (e->name == name));
            });
            if((search == element_map.end()) || (element == all_elements.end())) {
                if(!((search == element_map.end()) && (element == all_elements.end()))) {
                    std::cout << "WARNING: " << name << " should exist in both but does not." << std::endl;
                }
                std::cout << "No element called " << name << " exists." << std::endl;
                return;
            }
            search->second->modifyName(newName);
            element->get()->modifyName(newName);
            auto node_buffer = element_map.extract(search);
            if(node_buffer.empty()) {
                std::cout << "ERROR: " << name << " could not be found" << std::endl;
                element_map.insert(std::move(node_buffer));
                return;
            }
            node_buffer.key() = newName;
            element_map.insert(std::move(node_buffer));
        }
        size_t num_pos = token.find_first_of("0123456789.-");
//        if(num_pos == std::string::npos) return;
        if(isNumber(token)) {
            if(type == ElementType::voltage_source) {
                VoltageSource* v = static_cast<VoltageSource*>(e);
                v->modifyVoltage(EnginUnits(token));
                v->ac_wave = nullptr;
            }
            else if(type == ElementType::current_source) {
                CurrentSource* i = static_cast<CurrentSource*>(e);
                i->modifyCurrent(EnginUnits(token));
                i->ac_wave = nullptr;
            }
            else if(type == ElementType::resistor) static_cast<Resistor*>(e)->modifyResistance(EnginUnits(token));
            else if(type == ElementType::capacitor) static_cast<Capacitor*>(e)->modifyCapacitance(EnginUnits(token));
            else if(type == ElementType::inductor) static_cast<Inductor*>(e)->modifyInductance(EnginUnits(token));
            continue;
        }
        if((type == ElementType::voltage_source) || (type == ElementType::current_source)) {
            if(token.contains("SINE")) {
                double offset, amplitude, frequency, time_delay, theta, phase, Ncycles;
                std::string off, amp, freq, delay, th, phi_deg, n;
                std::stringstream wave_ss = extractWaveValues(ss);
                if((e->ac_wave != nullptr) && (e->ac_wave->getType() == "SineWave") && (token.contains('='))) {
                    if(token.back() == ')') token.pop_back();
                    short num_of_sine_parameters = 7;
                    auto wave = dynamic_cast<SineWave*>(e->ac_wave.get());
                    bool first_run = true;
                    for(short i = 1; i <= num_of_sine_parameters; i++) {
                        num_pos = token.find_first_of("=")+1;
                        if(token.contains("off")) wave->modifyOffset(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("amp")) wave->modifyAmplitude(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("freq")) wave->modifyFrequency(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("delay")) wave->modifyTimeDelay(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("theta")) wave->modifyTheta(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("phase")) wave->modifyPhase(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("cycles")) wave->modifyNcycles(EnginUnits(token.substr(num_pos)));
                        else if(first_run) {
                            std::cout << "Must specificy the parameter(s) to change and to what." << std::endl;
                            std::cout << "Parameter keys are as follows: off= amp= freq= delay= theta= phase= cycles=, which are then followed by the new value." << std::endl;
                            return;
                        }
                        first_run = false;
                        if(!(wave_ss >> token)) break;
                    }
                    continue;
                }
                if(!(wave_ss >> amp >> freq >> delay >> th >> phi_deg >> n)) {
                    std::cout << "ERROR: incorrect syntax used for creating a sine wave for " << e->name << "." << std::endl;
                    std::cout << "Correct syntax: SINE(offset amplitude frequency time_delay theta phi_deg Ncycles)" << std::endl;
                    return;
                }
                offset = EnginUnits(token.substr(num_pos));
                amplitude = EnginUnits(amp);
                frequency = EnginUnits(freq);
                time_delay = EnginUnits(delay);
                theta = EnginUnits(th);
                phase = EnginUnits(phi_deg);
                Ncycles = EnginUnits(n);
                std::shared_ptr<Waveforms> sine = std::make_shared<SineWave>(offset,amplitude,frequency,time_delay,theta,phase,Ncycles);
                e->attachWave(sine);
                continue;
            } else if(token.contains("PULSE")) {
                double V1, V2, time_delay, rise_time, fall_time, on_time, period, Ncylces;
                std::string v1, v2, delay, rise, t_f, t_on, p, n;
                std::stringstream wave_ss = extractWaveValues(ss);
                if((e->ac_wave != nullptr) && (e->ac_wave->getType() == "Pulse") && (token.contains('='))) {
                    if(token.back() == ')') token.pop_back();
                    short num_of_pulse_parameters = 8;
                    auto wave = dynamic_cast<Pulse*>(e->ac_wave.get());
                    bool first_run = true;
                    for(short i = 1; i <= num_of_pulse_parameters; i++) {
                        num_pos = token.find_first_of("=")+1;
                        if(token.contains("V1")) wave->modifyV1(EnginUnits(token.substr(num_pos))); // gives invalid suffix error
                        else if(token.contains("V2")) wave->modifyV2(EnginUnits(token.substr(num_pos))); // gives invalid suffix error and returns wrong values (6->2)
                        else if(token.contains("delay")) wave->modifyTimeDelay(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("rise")) wave->modifyRiseTime(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("fall")) wave->modifyFallTime(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("on")) wave->modifyOnTime(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("period")) wave->modifyPeriod(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("cycles")) wave->modifyNcycles(EnginUnits(token.substr(num_pos)));
                        else if(first_run) {
                            std::cout << "Must specificy the parameter(s) to change and to what." << std::endl;
                            std::cout << "Parameter keys are as follows: V1= V2= delay= rise= fall= on= period= cycles=, which are then followed by the new value." << std::endl;
                            return;
                        }
                        first_run = false;
                        if(!(wave_ss >> token)) break;
                    }
                    continue;
                }
                if(!(wave_ss >> v2 >> delay >> rise >> t_f >> t_on >> p >> n)) {
                    std::cout << "ERROR: incorrect syntax used for creating a pulse for " << e->name << "." << std::endl;
                    std::cout << "Correct syntax: PULSE(V1 V2 time_delay rise_time fall_time on_time period Ncycles)" << std::endl;
                    return;
                }
                V1 = EnginUnits(token.substr(num_pos));
                V2 = EnginUnits(v2);
                time_delay = EnginUnits(delay);
                rise_time = EnginUnits(rise);
                fall_time = EnginUnits(t_f);
                on_time = EnginUnits(t_on);
                period = EnginUnits(p);
                Ncylces = EnginUnits(n);
                std::shared_ptr<Waveforms> pulse = std::make_shared<Pulse>(V1,V2,time_delay,rise_time,fall_time,on_time,period,Ncylces);
                e->attachWave(pulse);
                continue;
            } else if(token.contains("AC")) {
                double amplitude, phase;
                std::string amp, phi_deg;
                std::stringstream wave_ss = extractWaveValues(ss);
                if((e->small_signal != nullptr) && (e->small_signal->getType() == "SmallSignal") && (token.contains('='))) {
                    if(token.back() == ')') token.pop_back();
                    short num_of_pulse_parameters = 2;
                    auto wave = dynamic_cast<SmallSignal*>(e->small_signal.get());
                    bool first_run = true;
                    for(short i = 1; i <= num_of_pulse_parameters; i++) {
                        num_pos = token.find_first_of("=")+1;
                        if(token.contains("amp")) wave->modifyAmplitude(EnginUnits(token.substr(num_pos)));
                        else if(token.contains("phase")) wave->modifyPhase(EnginUnits(token.substr(num_pos)));
                        else if(first_run) {
                            std::cout << "Must specificy the parameter(s) to change and to what." << std::endl;
                            std::cout << "Parameter keys are as follows: amp= phase=, which are then followed by the new value." << std::endl;
                            return;
                        }
                        first_run = false;
                        if(!(wave_ss >> token)) break;
                    }
                    continue;
                }
                if(!(wave_ss >> phi_deg)) {
                    std::cout << "ERROR: incorrect syntax used for applying a small signal to " << e->name << "." << std::endl;
                    std::cout << "Correct syntax: AC(amplitude phi_deg)" << std::endl;
                    return;
                }
                amplitude = EnginUnits(token.substr(num_pos));
                phase = EnginUnits(phi_deg);
                std::shared_ptr<Waveforms> small_signal = std::make_shared<SmallSignal>(amplitude,phase);
                e->attachSmallSignal(small_signal);
                continue;
            }
        }        
        if(type == ElementType::resistor) {
            Resistor* r = static_cast<Resistor*>(e);
            if(token.contains("tol")) r->modifyTolerance(EnginUnits(token.substr(num_pos)));
            else if(token.contains("pwr")) r->modifyPowerRating(EnginUnits(token.substr(num_pos)));
            continue;
        }
        if(type == ElementType::capacitor) {
            Capacitor* c = static_cast<Capacitor*>(e);
            if(token.contains("V=")) {c->modifyVoltageRating(EnginUnits(token.substr(num_pos)));}
            else if(token.contains("Irms")) {c->modifyRMSCurrentRating(EnginUnits(token.substr(num_pos)));}
            continue;
        }
        if((type == ElementType::inductor) && (token.contains("Ipk"))) {
            static_cast<Inductor*>(e)->modifyPeakCurrent(EnginUnits(token.substr(num_pos)));
            continue;
        }
        if(num_pos != std::string::npos) {
            std::string numeric_value = token.substr(num_pos);
            if((allowed.series_resistance) && (token.contains("Rser"))) e->modifyRser(EnginUnits(numeric_value));
            else if((allowed.series_inductance) && (token.contains("Lser"))) e->modifyLser(EnginUnits(numeric_value));
            else if((allowed.parallel_resistance) && (token.contains("Rpar"))) e->modifyRpar(EnginUnits(numeric_value));
            else if ((allowed.parallel_capacitance) && (token.contains("Cpar"))) e->modifyCpar(EnginUnits(numeric_value));
            continue;
        }
    }
    info.changesMade = true;
}
void CircuitManager::deleteElement(std::stringstream& ss) {
    std::string key;
    ss >> key;
    if(key.empty()) std::cout << "No element chosen, please enter the name of the element you wish to delete." << std::endl;
    auto search = element_map.find(key);
    if(search == element_map.end()) {
        std::cout << "No element called " << key << " could be found." << std::endl;
        return;
    }
    Elements* toDelete = search->second;
    if(toDelete->type == ElementType::gnd)
    std::erase_if(all_elements,[toDelete](const std::unique_ptr<Elements>& item){return item.get() == toDelete;});
    element_map.erase(search);
    std::cout << "Element " << key << " was successfully deleted." << std::endl;
    info.changesMade = true;
}
void CircuitManager::wire(std::stringstream& ss) {
    std::vector<int*> element_pin_ptr;
    std::vector<int> nets;
    bool allFloating = true;
    {
        std::vector<std::string> name_list, pins;
        {
            std::stringstream temp(ss.str());
            std::string buffer;
            int count = 0;
            while(temp >> buffer) {
                if(buffer == "wire") continue;
                count++;
                (count%2 == 1) ? name_list.push_back(buffer) : pins.push_back(buffer);
            }
            if((count%2 != 0) || (count/2 < 2)) {
                std::cout << "ERROR: not enough elements and respective pins were given to wire together." << std::endl;
                return;
            }
        }
        for(int index = 0; index < name_list.size(); index++) {
            std::string name = name_list[index];
            auto search = element_map.find(name);
            if(search == element_map.end()) {
                std::cout << name << " does not exist." << std::endl;
                return;
            }
            int pin_number;
            try {
                pin_number = std::stoi(pins[index]);
            } catch(const std::exception& error) {
                std::cout << "ERROR: " << name << " must be followed by the pin, as a number, you wish to connect." << std::endl;
                return;
            }
            if((pin_number < 1) || (pin_number > search->second->node_ids.size())) { // lool into this and add an actual message
                std::cout << name << "" << std::endl;
                return;
            }
            if(search->second->type == ElementType::gnd) isGrounded = true;
            if(search->second->node_ids[pin_number-1] != -1) allFloating = false;
            element_pin_ptr.push_back(&(search->second->node_ids[pin_number-1]));
            nets.push_back(search->second->node_ids[pin_number-1]);
        }
    }
    if((allFloating) || (isGrounded)) {
        int ground_node = 0;
        for (auto& pin : element_pin_ptr) {
            *pin = (allFloating) ? info.next_node_id : ground_node;
        }
        if(allFloating) {
            std::string node_name = std::to_string(info.next_node_id);
            double zero_padding = defaults.padding-node_name.length();
            if(zero_padding < 0) zero_padding = 0;
            node_ids_and_names.addToBimap(info.next_node_id,'N' + std::string(zero_padding,'0') + node_name);
            info.next_node_id++;
            return;
        }
    }
    std::sort(nets.begin(),nets.end());
    auto duplicates = std::unique(nets.begin(),nets.end());
    nets.erase(duplicates,nets.end());
    int floating = -1;
    if(nets.front() == floating) nets.erase(nets.begin());
    merge_nets(nets.front(),nets);
    info.changesMade = true;
}
void CircuitManager::circuit(std::stringstream& ss) { // work in progress
    if(all_elements.empty()) {
        std::cout << "The circuit is currently empty." << std::endl;
        return;
    }
    std::cout << "------ Current Circuit Elements ------" << std::endl;
    for(const auto& e : all_elements) {
        e->printInfo();
    }
    std::cout << "Number of nodes: " << info.next_node_id-1 << std::endl;
    std::cout << "Number of voltage sources: " << info.v_sources << std::endl;
    std::cout << "Number of current sources: " << info.i_sources << std::endl;
    std::cout << "Simulation type: " << info.simulation_directive << std::endl;
    std::cout << "Circuit name: " << info.circuit_file_name << std::endl;
}

void CircuitManager::simulate(std::stringstream& ss) {
    if(element_map.empty()) {
        std::cout << "The circuit is currently empty." << std::endl;
        return;
    }
    context.nodes = info.next_node_id-1;
    if(!info.setConfigs) {
        std::cout << "Simulations configs have not been set, use the command (/analysisConfig) to set them up." << std::endl; // need to update this message
        return;
    }
    solver.runSimulator(element_map);
}
void CircuitManager::operatingPoint(std::stringstream& ss) {
    context.mode = SimulationMode::operating_point;
    info.setConfigs = true;
    info.simulation_directive = ".op";
    info.changesMade = true;
}
void CircuitManager::transient(std::stringstream& ss) {
    context.mode = SimulationMode::transient;
    std::string start, max_step, stop;
    if(!(ss >> start >> stop >> max_step)) {
        std::cout << "ERROR: incorrect syntax used." << std::endl;
        std::cout << "Correct syntax: start_time stop_time max_time_step" << std::endl;
        return;
    }
    context.start = EnginUnits(start);
    context.stop = EnginUnits(stop);
    context.max_dt = EnginUnits(max_step);
    if(context.start < 0.0) {
        std::cout << "Start time cannot be less than 0." << std::endl;
        return;
    }
    if(context.max_dt <= 0.0) {
        std::cout << "Max time step must be greater than 0." << std::endl;
        return;
    }
    if(context.start >= context.stop) {
        std::cout << "Stop time must be greater than the start time." << std::endl;
        return;
    }
    info.setConfigs = true;
    info.simulation_directive = ".tran " + std::to_string(context.start) + " " + std::to_string(context.stop) + " " + std::to_string(context.max_dt);
    info.changesMade = true;
}
void CircuitManager::smallSignal(std::stringstream& ss) {
    context.mode = SimulationMode::ac_sweep;
    std::string sweep_type;
    if(!(ss >> sweep_type)) {
        std::cout << "No sweep type was specified." << std::endl;
        return;
    }
    if(sweep_type == "list") {
        std::string buffer;
        bool areFrequencies = false;
        while(ss >> buffer) {
            results.time_or_freq.push_back(EnginUnits(buffer));
            areFrequencies = true;
        }
        if(!areFrequencies) {
            std::cout << "Must specify the frequencies to sweep after list." << std::endl;
            std::cout << "Example: .ac list freq_1 freq_2 freq_3 ... freq_n" << std::endl;
            return;
        }
        for(auto& freq : results.time_or_freq) {
            freq = std::max(freq,defaults.f_min);
        }
        std::sort(results.time_or_freq.begin(),results.time_or_freq.end());
        auto duplicates = std::unique(results.time_or_freq.begin(),results.time_or_freq.end());
        results.time_or_freq.erase(duplicates,results.time_or_freq.end());
        context.sweep = SweepType::list;
        info.setConfigs = true;
        info.simulation_directive = ".ac list";
        std::string temp;
        for(const auto& freq : results.time_or_freq) {
            info.simulation_directive.append(' ' + std::to_string(freq));
        }
    } else if((sweep_type == "lin") || (sweep_type == "oct") || (sweep_type == "dec")) {
        std::vector<double> freq_range;
        std::string buffer;
        for(int index = 0; index < 3; index++) {
            if(!(ss >> buffer)) {
                std::cout << "ERROR: missing value(s)." << std::endl;
                std::cout << "Correct syntax: .ac <lin oct dec> start_frequency stop_frequency points_per_div" << std::endl;
                return;
            }
            freq_range.push_back(EnginUnits(buffer));
            if(freq_range.back() <= 0.0) freq_range.back() = defaults.f_min;
        }
        if(freq_range[0] >= freq_range[1]) {
            std::cout << "The stop frequency must be greater than the start frequency." << std::endl;
            return;
        }
        freq_range[2] = std::ceil(freq_range[2]);
        if(freq_range[2] < 1) {
            std::string div;
            std::cout << "The number of points ";
            div = (sweep_type == "oct") ? "per octave " : (sweep_type == "dec") ? "per decade " : "";
            std::cout << div << "must be at least 1." << std::endl;
            return;
        }
        if(sweep_type == "lin") context.sweep = SweepType::linear;
        if(sweep_type == "oct") context.sweep = SweepType::octave;
        if(sweep_type == "dec") context.sweep = SweepType::decade;
        solver.determineFrequencies(freq_range); // need to pass freq_range
        info.setConfigs = true;
        info.simulation_directive = ".ac " + sweep_type;
        for(const auto& value : freq_range) {
            info.simulation_directive.append(' ' + std::to_string(value));
        }
    } else {
        std::cout << sweep_type << " is invalid." << std::endl;
        std::cout << "Valid sweep types are as follows: list lin oct dec" << std::endl;
        return;
    }
    info.changesMade = true;
}

void CircuitManager::set(std::stringstream& ss) {
    std::string subfunction;
    if(!(ss >> subfunction)) {
        std::cout << "Need to specify the field to change." << std::endl;
        return;
    }
    if(subfunction.contains("node")) {changeNodeName(ss);}
    info.changesMade = true;
}
void CircuitManager::model(std::stringstream& ss) {
    info.changesMade = true;
}

void CircuitManager::saveCircuitAs(std::stringstream& ss) {
    std::string circuit_netlist;
    std::string name;
    if(!(ss >> name)) {
        std::cout << "No name was specified for saving the circuit." << std::endl;
        return;
    }
    circuit_netlist = name + extension::netlist;
    int iteration = 0;
    while(true) {
        if(filenameCheck(circuit_netlist)) {
            iteration++;
            circuit_netlist = name + std::to_string(iteration)+ extension::netlist;
            continue;
        }
        break;
    }
    info.circuit_file_name = circuit_netlist;
    saveCircuit();
}
void CircuitManager::saveCircuit() {
    std::ofstream circuit_netlist_file(info.circuit_file_name);
    if(!(circuit_netlist_file.is_open())) {
        std::cout << "For unknown reasons, file called " << info.circuit_file_name << " could not be opened." << std::endl;
        return;
    }
    circuit_netlist_file << "~ " << info.circuit_file_name << '\n';
    for(const auto& element : all_elements) {
        element->saveCircuit(circuit_netlist_file);
    }
    circuit_netlist_file << info.simulation_directive << '\n';
    circuit_netlist_file.close();
    std::cout << "Circuit was successfully saved." << std::endl;
    info.changesMade = false;
}
void CircuitManager::loadCircuit(std::stringstream& ss) {
    std::string filename;
    if(!(ss >> filename)) {
        std::cout << "No filename was given." << std::endl;
        return;
    }
    filename += extension::netlist;
    if(!(std::filesystem::exists(filename))) {
        std::cout << "The file " << filename << " does not exist." << std::endl;
        return;
    }
    std::ifstream circuit_netlist(filename);
    if(!(circuit_netlist.is_open())) {
        std::cout << "Circuit " << filename << " could not be opened." << std::endl;
        return;
    }
    std::string line;
    std::stringstream netlist_ss;
    std::getline(circuit_netlist,line);
    netlist_ss.str(line);
    std::string type;
    netlist_ss >> type >> info.circuit_file_name;
    if((!(type.contains("~"))) || (filename != info.circuit_file_name)) {
        std::cout << "Formating error in " << filename << "." << std::endl;
        return;
    }
    while(std::getline(circuit_netlist,line)) {
        netlist_ss.clear();
        netlist_ss.str(line);
        netlist_ss >> type;
        if(type.contains('.')) {
            if(type.contains("op")) operatingPoint(netlist_ss);
            if(type.contains("tran")) transient(netlist_ss);
            if(type.contains("ac")) smallSignal(netlist_ss);
            std::cout << info.simulation_directive << std::endl;
            continue;
        } else {
            std::string name;
            {    
                std::string trash;
                netlist_ss >> trash >> name;
            }
            std::stringstream element_ss;
            element_ss << type << " " << name << netlist_ss.rdbuf();
            addElement(element_ss);
            auto search = element_map.find(name);
            if(search == element_map.end()) {
                std::cout << name << " does not exist." << std::endl;
                return;
            }
            netlist_ss.seekg(0,std::ios::beg);
            int pins = search->second->node_ids.size();
            std::vector<std::string> node_list;
            {
                std::vector<std::string> tokens;
                std::string token;
                while(netlist_ss >> token) {
                    tokens.push_back(token);
                }
                int items = tokens.size();
                if(items < pins) {
                    std::cout << "ERROR: " << name << " has more pins than it's element type allows." << std::endl;
                    return;
                }
                for(int pin = pins; pin > 0; pin--) {
                    node_list.push_back(tokens[items-pin]);
                }
            }
            std::vector<std::string> custom_named_nodes;
            std::vector<int> element_pin;
            if(search->second->type == ElementType::gnd) {
                int ground = 0;
                node_ids_and_names.addToBimap(ground,node_list.back());
                continue;
            }
            int id = -1;
            for(int node = 0; node < node_list.size(); node++) {
                id = node_ids_and_names.nodeID(node_list[node]);
                if(id != -1) {
                    search->second->node_ids[node] = id;
                } else if((node_list[node].contains('N') && (isNumber(node_list[node].substr(1))))) {
                    int node_id = std::stoi(node_list[node].substr(1));
                    node_ids_and_names.addToBimap(node_id,node_list[node]);
                    if(node_id > 0) info.next_node_id++;
                    search->second->node_ids[node] = node_id;
                } else {
                    custom_named_nodes.push_back(node_list[node]);
                    element_pin.push_back(node);
                }
            }
            for(int index = 0; index < custom_named_nodes.size(); index++) {
                node_ids_and_names.addToBimap(info.next_node_id,custom_named_nodes[index]);
                search->second->node_ids[element_pin[index]] = info.next_node_id;
                info.next_node_id++;
            }
            continue;
        }
    }
}
void CircuitManager::closeCircuit() {
    if(info.changesMade) {
        // need to add logic to confirm if the user wants to close without saving in a way that works in engine mode and GUI mode
    }
    all_elements.clear();
    element_map.clear();
    info.circuit_file_name = defaults.filename;
    info.simulation_directive = "";
    info.next_node_id = 1;
    info.setConfigs = false;
    info.v_sources = 0;
    info.i_sources = 0;
    info.l_sources = 0;
    info.c_sources = 0;
    node_ids_and_names.clear();
}
#pragma endregion