#include"Commands.h"

using command = std::function<void(std::stringstream&)>;

CircuitManager manager;

CircuitSolver engine;

QCustomPlot *plot;

using command_lookup = std::function<void(std::stringstream&)>;

using directive_lookup = std::function<void(std::stringstream&)>;

using script_tool_lookup = std::function<void(std::stringstream&)>;

void clearScreen() {std::cout << "\033[2J\033[1;1H";}

void close() {exit(0);}
// want to make it so if you have not simulated the circuit it informs you
void plotable() {
    std::vector<std::string> result_names;
    for(const auto& [name,_] : results.dataPoints) {
        result_names.push_back(name);
    }
    std::sort(result_names.begin(), result_names.end(), [](const std::string& a, const std::string& b) {
        if (a == "I(GND)") return false;
        if (b == "I(GND)") return true;
        char prefixA = a[0];
        char prefixB = b[0];
        if (prefixA != prefixB) {return prefixA == 'V';}
        return a < b;
    });
    for(const auto& name : result_names) {
        std::cout << name << std::endl;
    }
}

std::tuple<QVector<double>,QVector<double>> findDataPoints(std::string name) {
    auto values = results.dataPoints.find(name);
    if(values == results.dataPoints.end()) {
        std::cout << name << " does not exist." << std::endl;
        return {{},{}};
    }
    QVector<double> dependentMag(values->second.magnitude.begin(),values->second.magnitude.end());
    QVector<double> dependentPhase(values->second.phase.begin(),values->second.phase.end());
    if(context.mode == SimulationMode::ac_sweep) {
        std::transform(dependentMag.begin(),dependentMag.end(),dependentMag.begin(),[](double dB) {
            return (dB <= defaults.min_voltage_cutoff) ? 20*std::log10(defaults.min_voltage_cutoff) : 20*std::log10(dB);
        });
    }
    std::cout << "\nSuccessful data point retrieval.\n" << std::endl;
    return {dependentMag,dependentPhase};
}

int graph(std::stringstream& ss) {
    if(!simulationRan) {
        std::cout << "Must have simulated the circuit before results can be plotted." << std::endl;
        return -1;
    }
    if(results.time_or_freq.empty()) {
        std::string type = (context.mode != SimulationMode::ac_sweep) ? "time" : "frequency";
        std::cout << "No " << type << " vector created, make sure to simulate the circuit first." << std::endl;
        return -1;
    }
    std::string name;
    if(!(ss >> name)) {
        std::cout << "No name was given." << std::endl;
        return -1;
    }
    auto [dependentMag,dependentPhase] = findDataPoints(name);
    if(dependentMag.empty()) {
        std::cout << "ERROR: no magnitude values found in results set" << std::endl;
        return -1;
    }
    QVector<double> independentValues(results.time_or_freq.begin(),results.time_or_freq.end());
    int argc = 1;
    static char program_name[] = "CircuitTree Figure";
    char* argv[] = {program_name};
    QApplication figure(argc,argv);
    plot = new QCustomPlot();
    plot->setWindowTitle(QString::fromStdString(name));
    plot->resize(800,600);
    plot->addGraph();
    plot->graph(0)->setPen(QPen(Qt::black));
    plot->graph(0)->setData(independentValues,dependentMag);
    plot->graph(0)->rescaleValueAxis();
    plot->addGraph();
    plot->graph(1)->setPen(QPen(Qt::blue));
    plot->graph(1)->setKeyAxis(plot->xAxis);
    plot->graph(1)->setValueAxis(plot->yAxis2);
    plot->graph(1)->setData(independentValues,dependentPhase);
    bool foundRange = false;
    QCPRange magRange = plot->graph(0)->getValueRange(foundRange,QCP::sdBoth);
    QSharedPointer<QCPAxisTickerFixed> magTicker(new QCPAxisTickerFixed);
    if (magRange.size() < 0.1) {
        plot->yAxis->setRange(-0.1, 0.1);
        magTicker->setTickStep(0.01);
    } 
    else if (magRange.size() < 1.0) {
        plot->yAxis->setRange(-1.0, 1.0);
        magTicker->setTickStep(0.1);
    } 
    else {
        plot->yAxis->setRange(magRange.lower - magRange.size()*0.1, magRange.upper + magRange.size()*0.1);        
        if (magRange.size() < 20.0) {
            magTicker->setTickStep(2.0);
        } else {
            magTicker->setTickStep(10.0);
        }
    }
    plot->yAxis->setTicker(magTicker);
    plot->yAxis->setVisible(true);
    plot->yAxis->setLabel("Magnitude");
    plot->yAxis2->setLabel("Phase");
    QSharedPointer<QCPAxisTickerFixed> phaseTicker(new QCPAxisTickerFixed);
    phaseTicker->setTickStep(15.0);
    plot->yAxis2->setTicker(phaseTicker);
    plot->yAxis2->setSubTicks(true);
    QCPRange phaseRange = plot->graph(1)->getValueRange(foundRange,QCP::sdBoth);
    if(phaseRange.size() < 30.0) {
        double center  = std::round(phaseRange.center()/15.0)*15.0;
        plot->yAxis2->setRange(center-45.0,center+45.0);
    } else {
        double lower = std::floor(phaseRange.lower/15.0)*15.0-15.0;
        double upper = std::ceil(phaseRange.upper/15.0)*15.0+15.0;
        plot->yAxis2->setRange(lower,upper);
    }
    QString xlabel = (context.mode != SimulationMode::ac_sweep) ? "Time (s)" : "Frequency (Hz)";
    plot->xAxis->setLabel(xlabel);
    if(context.mode == SimulationMode::ac_sweep) {
        plot->graph(0)->setName("Magnitude");
        plot->graph(1)->setName("Phase");
        plot->legend->setVisible(true);
        plot->yAxis2->setVisible(true);
        plot->xAxis->setScaleType(QCPAxis::stLogarithmic);  
        QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
        plot->xAxis->setTicker(logTicker); 
        double lowerBound = (independentValues.front() <= 0.0) ? 0.1 : independentValues.front();
        plot->xAxis->setRange(lowerBound,independentValues.back());
    } else {
        plot->yAxis2->setVisible(false);
        plot->xAxis->setScaleType(QCPAxis::stLinear);
        QSharedPointer<QCPAxisTicker> linTicker(new QCPAxisTicker);
        plot->xAxis->setTicker(linTicker);
        plot->xAxis->setRange(independentValues.front(),independentValues.back());
    }
    plot->yAxis->grid()->setVisible(true);
    plot->yAxis->grid()->setSubGridVisible(true);
    plot->xAxis->grid()->setVisible(true);
    plot->xAxis->grid()->setSubGridVisible(true);
    plot->replot();
    plot->show();
    return figure.exec();
}

enum class Initiator {
    command,
    directive,
    script,
    text,
    unknown
};

struct InitiatorInfo {
    Initiator type;
    std::string name;
};

const std::unordered_map<char,InitiatorInfo> initiator_list {
    {'/', {Initiator::command, "command"}},
    {'.', {Initiator::directive, "directive"}},
    {'$', {Initiator::script, "script"}},
    {'~', {Initiator::text, "text"}}
};
// may want to further sort and separate between circuit funtions and app/console functions
const std::unordered_map<std::string,command_lookup> command_lookup_table {
    {"clear", [](std::stringstream&) {clearScreen();}},
    {"save_as", [](std::stringstream& ss) {manager.saveCircuitAs(ss);}},
    {"save", [](std::stringstream&) {manager.saveCircuit();}},
    {"load", [](std::stringstream& ss) {manager.loadCircuit(ss);}},
    {"close", [](std::stringstream&) {manager.closeCircuit();}},
    {"exit", [](std::stringstream&) {close();}},
    {"addElement", [](std::stringstream& ss) {manager.addElement(ss);}},
    {"editElement", [](std::stringstream& ss) {manager.editElement(ss);}},
    {"deleteElement", [](std::stringstream& ss) {manager.deleteElement(ss);}},
    {"wire", [](std::stringstream& ss) {manager.wire(ss);}},
    {"circuit", [](std::stringstream& ss) {manager.circuit(ss);}},
    {"validCircuit", [](std::stringstream& ss) {engine.validCircuit(manager.element_map);}},
    {"plotable", [](std::stringstream&) {plotable();}},
    {"plot", [](std::stringstream& ss) {graph(ss);}}
};

const std::unordered_map<std::string,directive_lookup> directive_lookup_table {
    {"set", [](std::stringstream& ss) {manager.set(ss);}},
    {"model", [](std::stringstream& ss) {manager.model(ss);}},
    {"op", [](std::stringstream& ss) {manager.operatingPoint(ss);}},
    {"tran", [](std::stringstream& ss) {manager.transient(ss);}},
    {"ac", [](std::stringstream& ss) {manager.smallSignal(ss);}},
    {"simulate", [](std::stringstream& ss) {manager.simulate(ss);}}
};
// to be implemented
const std::unordered_map<std::string,script_tool_lookup> script_tool_lookup_table {

};

std::tuple<Initiator,std::string> initiator_check(char initiator) {
    auto lookup_initiator = initiator_list.find(initiator);
    if(lookup_initiator != initiator_list.end()) return {lookup_initiator->second.type,lookup_initiator->second.name};
    return {Initiator::unknown," "};
}

void instruct(std::string instruction) {
    if(instruction.empty()) {
        std::cout << "No instruction was given." << std::endl;
        return;
    }
    char initiator = instruction[0];
    auto [instruction_type, name] = initiator_check(initiator);
    if(instruction_type == Initiator::unknown) {
        std::cout << "Unknown instruction, use /help to see all possible commands, directives and scripting." << std::endl;
        return;
    }
    if((instruction_type == Initiator::command) || (instruction_type == Initiator::directive)) instruction.erase(0,1);
    if(instruction_type == Initiator::text) { // to be implemented later on during the GUI stage, specifically for schematic design
        std::cout << "To be implemented later on for GUI schematic usage" << std::endl;
        return;
    }
    std::stringstream ss(instruction);
    std::string search;
    if(!(ss >> search)) {
        std::cout << "A " << name << " must be declared." << std::endl;
        return;
    }
    switch(instruction_type) {
        case(Initiator::command):
        {
            auto lookup_command = command_lookup_table.find(search);
            if(lookup_command == command_lookup_table.end()) {
                std::cout << "Unknown command: " << search << std::endl;
                return;
            }
            lookup_command->second(ss);
            break;
        }
        case(Initiator::directive):
        {
            auto lookup_directive = directive_lookup_table.find(search);
            if(lookup_directive == directive_lookup_table.end()) {
                std::cout << "Unknown directive: " << search << std::endl;
                return;
            }
            lookup_directive->second(ss);
            break;
        }
        case(Initiator::script): // to be implemented later on
        {
            std::cout << "For to be implemented new scripting system, if actually included" << std::endl;
            break;
        }
    }
}