#ifndef CircuitManager_H
#define CircuitManager_H

#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <set>

#include "CircuitSolver.h"

using create_element = std::function<void(std::string)>;

class CircuitManager {
    private:
        bool isGrounded = false;

        std::tuple<std::string,bool> name_check(std::string name_to_check);
        bool isNumber(std::string input);
        std::stringstream extractWaveValues(std::stringstream& ss);
        bool effectivelyEmpty(std::stringstream& ss);

        void register_element(std::unique_ptr<Elements> e);
        // may want to improve for larger circuits rather than looping through
        void merge_nets(const int merged_node_id,std::vector<int>& merge_nodes);

        char defaultLetter[8] = {'V','I','R','C','L','D','Q','G'};
        int V = 1, I = 1, R = 1, C = 1, L = 1, D = 1, Q = 1, G = 1;
        int *number_of[8] = {&V,&I,&R,&C,&L,&D,&Q,&G};
        std::string defaultName(ElementType type);
        void addVoltageSource(std::string name = "");
        void addCurrentSource(std::string name = "");
        void addResistor(std::string name = "");
        void addCapacitor(std::string name = "");
        void addInductor(std::string name = "");
        void addDiode(std::string name = "");
        void addTranistor(std::string name = "");
        void addGround(std::string name = "");

        void changeNodeName(std::stringstream& ss);
    public:
        struct CircuitInformation {
            std::string circuit_file_name = defaults.filename;
            std::string simulation_directive;
            int next_node_id = 1;
            int v_sources = 0;
            int i_sources = 0;
            int l_sources = 0;
            int c_sources = 0;
            bool setConfigs = false;
            bool changesMade = false;
        }info;
        std::vector<std::unique_ptr<Elements>> all_elements;
        std::unordered_map<std::string,Elements*> element_map;
        std::unordered_map<std::string,create_element> element_lookup_table = {
            {"voltage_source", [this](std::string name) {addVoltageSource(name);}},
            {"current_source", [this](std::string name) {addCurrentSource(name);}},
            {"resistor", [this](std::string name) {addResistor(name);}},
                // would eventually have potentiometers thay can be changed mid simulation
                // would eventually like to have different kinds of resistors such as thermistors, photoresistors
            {"capacitor", [this](std::string name) {addCapacitor(name);}},
                // would need to distinguish between polarized and non polarized
            {"inductor", [this](std::string name) {addInductor(name);}},
            {"diode", [this](std::string name) {addDiode(name);}},
                // would need to distguish between types such as zener, LED, etc.
                // polzarization could also become problematic as well
//            {"transistor", [this](std::string name) {addTransistor(name);}},
                // would need to distinguish type and various properties
                // making sure the right terminal is connected to the correct net could be problematic
            {"gnd", [this](std::string name) {addGround(name);}}
        };
        void addElement(std::stringstream& ss);
        void editElement(Elements* element);
        void editElement(std::stringstream& ss,std::string name = "");
        void deleteElement(std::stringstream& ss);
        void wire(std::stringstream& ss);
        void sortAllElements(); // currently unused
        void circuit(std::stringstream& ss);
        
        void set(std::stringstream& ss);
        void model(std::stringstream& ss);

        void operatingPoint(std::stringstream& ss);
        void transient(std::stringstream& ss);
        void smallSignal(std::stringstream& ss);
        void simulate(std::stringstream& ss);

        void saveCircuitAs(std::stringstream& ss);
        void saveCircuit();
        void loadCircuit(std::stringstream& ss);
        void closeCircuit();
};

#endif