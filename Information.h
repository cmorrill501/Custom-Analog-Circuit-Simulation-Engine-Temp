#ifndef Information_H
#define Information_H

#include <cmath>
#include <complex>

#include "Eigen/Dense"

#include "EnginUnits.h"
#include "CustomExtensions.h"

struct Environment {
    double temp = 300.15;
};

inline Environment envr;

enum class SimulationMode {
    operating_point,
    transient,
    ac_sweep,
    unknown
};

enum class IntegrationStage {
    DC_Steady_State,
    TR_Stage,
    BDF2_Stage
};

enum class SweepType {
    list,
    linear,
    octave,
    decade
};

struct SimulationContext {
    int nodes;
    SimulationMode mode;
    IntegrationStage stage;
    double time;
    double start;
    double max_dt;
    double stop;
    double dt;
    SweepType sweep;
    double V_T = k_B*envr.temp/q_charge;
};

inline SimulationContext context;

struct Defaults {
    // should probably move to separate struct centered around simulation characteristics
    double padding = 3;
    double iterations = 100;
    std::string filename = "test" + extension::netlist;
    // should probably move to separate struct centered around default simulation values
    double g_min = 1e-12;
    double r_min = 1e-12;
    double f_min = 1e-15;
    double min_step_per_period = 20;
    double step_min = 1e-15;
    // should probably move to separate struct centered around error calculation
    double gamma = 2.0-sqrt(2.0);
    double min_max_error_ratio = 1e-6;
    double relative_tolerance = 0.001;
    double absolute_tolerance_v = 1e-6;
    double absolute_tolerance_i = 1e-12;
    double safety = 0.85;
    // to be moved to separate struct for graphing defaults
    double y_axis_buffer_flat = 1.0;
    double y_axis_buffer_perc = 0.1;
    double min_voltage_cutoff = 1e-12;
};

inline Defaults defaults;

template<typename T>
struct LinearAlgebraContext {
    Eigen::MatrixX<T> A;
    Eigen::VectorX<T> z;
    Eigen::VectorX<T> x;
};

inline LinearAlgebraContext<double> timeDomain;
inline LinearAlgebraContext<std::complex<double>> freqDomain;

#endif