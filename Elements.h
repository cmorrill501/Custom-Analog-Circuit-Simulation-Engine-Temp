#ifndef Elements_H
#define Elements_H

#include <iostream>
#include <memory>
#include <vector>
#include <tuple>
#include <string_view>
#include <fstream>
#include <type_traits>

#include "Waveforms.h"
#include "Logger.h"

enum class ElementType {
    voltage_source,
    current_source,
    resistor,
    capacitor,
    inductor,
    diode,
    transistor,
    gnd
};

const std::unordered_map<std::string,ElementType> element_type_lookup_table {
    {"voltage_source",ElementType::voltage_source},
    {"current_source",ElementType::current_source},
    {"resistor",ElementType::resistor},
    {"capacitor",ElementType::capacitor},
    {"inductor",ElementType::inductor},
    {"diode",ElementType::diode},
    {"transistor",ElementType::transistor},
    {"gnd",ElementType::gnd}
};

struct CircuitSignal {
    QVector<double> magnitude;
    QVector<double> phase;
};

struct SimulationResults {
    QVector<double> time_or_freq;
    std::unordered_map<std::string,CircuitSignal> dataPoints;
};

inline SimulationResults results;

struct Permissions {
    bool series_resistance;
    bool series_inductance;
    bool parallel_resistance;
    bool parallel_capacitance;
};

const std::unordered_map<ElementType,Permissions> permissions_lookup_table {
    {ElementType::voltage_source,{true,false,false,true}},
    {ElementType::current_source,{true,false,false,true}},
    {ElementType::resistor,{false,true,false,true}},
    {ElementType::capacitor,{true,true,true,true}},
    {ElementType::inductor,{true,false,true,true}},
    {ElementType::diode,{false,false,false,false}},
    {ElementType::gnd,{false,false,false,false}}
};

inline std::vector<std::string_view> elements = {
    "voltage_source",
    "current_source",
    "resistor",
    "capacitor",
    "inductor", 
    "diode",
    "transistor",
    "gnd"
};

using namespace std::string_literals;

inline int longest_element_name = "Voltage Source"s.length();

inline BidirectionMap node_ids_and_names;

inline bool usePinsInsteadOfNodes = false;

class Elements {
    private:
        virtual double getInductance();
    protected:
        std::tuple<double,double> find_g_eq_c();
        std::tuple<double,double> find_thevenin_l();
    public:
        std::string name;
        ElementType type;
        std::vector<int> node_ids;
        struct Parasitic {
            double equiv_series_resistance = 0.0;
            double equiv_series_inductance = 0.0;
            double equiv_parallel_resistance = 0.0;
            double equiv_parallel_capacitance = 0.0;
        };
        Parasitic leach;
        std::shared_ptr<Waveforms> ac_wave = nullptr;
        std::shared_ptr<Waveforms> small_signal = nullptr;
        int branch_matrix_id = -1;
        struct SimulationResults {
            double voltage_drop;
            double current;
        }solution;
        struct PastValues {
            double v_c_n = 0.0;
            double i_c_n = 0.0;
            double v_c_gamma = 0.0;
            double i_c_gamma = 0.0;
            double candidate_v_c_n = 0.0;
            double candidate_i_c_n = 0.0;
            double v_l_n = 0.0;
            double i_l_n = 0.0;
            double v_l_gamma = 0.0;
            double i_l_gamma = 0.0;
            double candidate_v_l_n = 0.0;
            double candidate_i_l_n = 0.0;
            double V_D = 0.0;
            double I_D = 0.0;
        };
        PastValues past;
        Elements(std::string n, ElementType t, int pins);
        virtual ~Elements() = default;
        virtual bool isValid(std::string& unitName) = 0;
        virtual void printInfo() const = 0;
        virtual void getNodes(bool newline = false) const;
        virtual void printParasiticInfo() const;
        virtual void modifyName(std::string newName);
        virtual void modifyRser(double newRser);
        virtual void modifyLser(double newLser);
        virtual void modifyRpar(double newRpar);
        virtual void modifyCpar(double newCpar);
        virtual void attachSmallSignal(std::shared_ptr<Waveforms> signal) {}
        virtual void attachWave(std::shared_ptr<Waveforms> wave) {}
        virtual void stamp(SimulationContext& context) = 0;
        static void applyMinConductance();
        virtual bool convergenceCheck();
        virtual void postSolve(SimulationContext& context) = 0;
        virtual void commitStep();
        virtual void printResults() const;
        virtual void stampACSweep(SimulationContext& context,double omega) = 0;
        virtual void saveResults(double omega) = 0;

        virtual void saveCircuit(std::ofstream& circuit_netlist_file) const;
        virtual void saveParasitic(std::ofstream& circuit_netlist_file) const;
        virtual void saveNodes(std::ofstream& circuit_netlist_file) const;
};

class VoltageSource : public Elements {
    private:
        
    public:
        double voltage = 0.0;
        VoltageSource(std::string n);
        bool isValid(std::string& unitName) override;
        void printInfo() const override;
        double getVoltage();
        void modifyVoltage(double newVoltage);
        void attachSmallSignal(std::shared_ptr<Waveforms> signal) override;
        void attachWave(std::shared_ptr<Waveforms> wave) override;
        void stamp(SimulationContext& context) override;
        void postSolve(SimulationContext& context) override;
        void stampACSweep(SimulationContext& context,double omega) override;
        void saveResults(double omega) override;

        void saveCircuit(std::ofstream& circuit_netlist_file) const;;
};

class CurrentSource : public Elements {
    private:
        
    public:
        double current = 0.0;
        CurrentSource(std::string n);
        bool isValid(std::string& unitName) override;
        void printInfo() const override;
        double getCurrent();
        void modifyCurrent(double newCurrent);
        void attachSmallSignal(std::shared_ptr<Waveforms> signal);
        void attachWave(std::shared_ptr<Waveforms> wave);
        void stamp(SimulationContext& context) override;
        void postSolve(SimulationContext& context) override;
        void stampACSweep(SimulationContext& context,double omega) override;
        void saveResults(double omega) override;

        void saveCircuit(std::ofstream& circuit_netlist_file) const;;
};
// need to decide how the tolerance is taken into effect
// power rating to be implemented later on
class Resistor : public Elements {
    private:
        
    public:
        struct ElementValues {
            double resistance = 0.0;
            double tolerance = 0.0;
            double power_rating = 0.0;
        };
        ElementValues values;
        Resistor(std::string n);
        bool isValid(std::string& unitName) override;
        void printInfo() const override;
        void modifyResistance(double newResistance);
        void modifyTolerance(double newTolerance);
        void modifyPowerRating(double newPowerRating);
        void stamp(SimulationContext& context) override;
        void postSolve(SimulationContext& context) override;
        void stampACSweep(SimulationContext& context,double omega) override;
        void saveResults(double omega) override;

        void saveCircuit(std::ofstream& circuit_netlist_file) const;;
};
// voltage rating and RMS current to be implemented later on
class Capacitor : public Elements {
    private:
        double totalCapacitance();
        std::tuple<double,double> find_thevenin_c();
        std::tuple<double,double> find_thevenin_series();
    public:
        struct ElementValues {
            double capacitance = 0.0;
            double voltage_rating = 0.0;
            double RMS_current_rating = 0.0;
        };
        ElementValues values;
        Capacitor(std::string n);
        bool isValid(std::string& unitName) override;
        void printInfo() const override;
        void modifyCapacitance(double newCapacitance);
        void modifyVoltageRating(double newVoltageRating);
        void modifyRMSCurrentRating(double newRMSCurrentRating);
        void stamp(SimulationContext& context) override;
        void postSolve(SimulationContext& context) override;
        void stampACSweep(SimulationContext& context,double omega) override;
        void saveResults(double omega) override;

        void saveCircuit(std::ofstream& circuit_netlist_file) const;;
};
// peak current to be implemented later on
class Inductor : public Elements {
    private:
        double getInductance() override;
    protected:
        
    public:
        struct ElementValues {
            double inductance = 0.0;
            double peak_current = 0.0; // to be implemented later on
        };
        ElementValues values;
        Inductor(std::string n);
        bool isValid(std::string& unitName) override;
        void printInfo() const override;
        void modifyInductance(double newInductance);
        void modifyPeakCurrent(double newPeakCurrent);
        void stamp(SimulationContext& context) override;
        void postSolve(SimulationContext& context) override;
        void stampACSweep(SimulationContext& context,double omega) override;
        void saveResults(double omega) override;

        void saveCircuit(std::ofstream& circuit_netlist_file) const;;
};

class Diode : public Elements {
    private:
        template<typename T>
        struct is_complex : std::false_type {};
        template<typename T>
        struct is_complex<std::complex<T>> : std::true_type {};
        template<typename D>
        D pnjlim(D V_new,D V_old,D nV_T,D V_crit);
        std::complex<double> pastV_D = 0.0;
    public:
        struct ElementValues {
            double Is = EnginUnits("90p");
            double n = 1.4;
        };
        ElementValues values;
        Diode(std::string n);
        std::string model_name;
        bool isValid(std::string& unitName);
        void printInfo() const override;
        void stamp(SimulationContext& context);
        bool convergenceCheck() override;
        void postSolve(SimulationContext& context) override;
        void stampACSweep(SimulationContext& context,double omega) override;
        void saveResults(double omega);

        void saveCircuit(std::ofstream& circuit_netlist_file) const;
};
// to be implemented later on after the diode is finished
class Transistor : public Elements {
    public:
        Transistor(std::string n);
        std::string model_name;
        bool isValid(std::string& unitName);
        void printInfo() const override;
        void stamp(SimulationContext& context);
        void postSolve(SimulationContext& context) override;
        void stampACSweep(SimulationContext& context,double omega) override;
        void saveResults(double omega);

        void saveCircuit(std::ofstream& circuit_netlist_file) const;
};

class Ground : public Elements {
    public:
        Ground(std::string n);
        bool isValid(std::string& unitName);
        void printInfo() const override;
        void stamp(SimulationContext& context);
        void postSolve(SimulationContext& context) override;
        void stampACSweep(SimulationContext& context,double omega) override;
        void saveResults(double omega);

        void saveCircuit(std::ofstream& circuit_netlist_file) const;
};

#endif