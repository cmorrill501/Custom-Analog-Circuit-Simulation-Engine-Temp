#ifndef CircuitSolver_H
#define CircuitSolver_H

#include <set>

#include "Elements.h"

inline bool simulationRan = false;

class CircuitSolver {
    private:
        void invalidValue(const std::string& name,const std::string& unitName);
        std::vector<double> getAllFrequencies(const std::unordered_map<std::string,Elements*>& element_map);

        void print_operating_point_results(const std::unordered_map<std::string,Elements*>& element_map);

        double determineFirstStep(const std::unordered_map<std::string, Elements*>& element_map);
        double determineMaxFrequency(const std::unordered_map<std::string, Elements*>& element_map);
        double findCircuitFrequency(const std::unordered_map<std::string,Elements*>& element_map);
        
        bool executeStep(const std::unordered_map<std::string,Elements*>& element_map);
        double determineMaxErrorRatio(const std::unordered_map<std::string,Elements*>& element_map);

        void resultsStorageInitialization(const std::unordered_map<std::string, Elements*>& element_map);
        void recordNodeResults(const std::unordered_map<std::string,Elements*>& element_map);
        
        void operating_point(const std::unordered_map<std::string,Elements*>& element_map);
        void transient(const std::unordered_map<std::string,Elements*>& element_map);
        void ac_sweep(const std::unordered_map<std::string,Elements*>& element_map);
    public:
        void determineFrequencies(std::vector<double>& freq_range);
        
        int validCircuit(const std::unordered_map<std::string, Elements*>& element_map);
        
        void runSimulator(const std::unordered_map<std::string,Elements*>& element_map);
};

#endif