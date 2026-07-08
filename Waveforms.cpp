#include "Waveforms.h"

#pragma region SmallSignal
SmallSignal::SmallSignal(double amp, double phi_deg = 0.0) : 
    amplitude(amp) {phi_rad = phi_deg*(std::numbers::pi/180.0);}
std::string SmallSignal::getAttributes() {
    std::ostringstream ss;
    ss << "AC(" << amplitude << " " << phi_rad*(180.0/std::numbers::pi) << ") ";
    return ss.str();
}
double SmallSignal::getValue() const {return amplitude;}
double SmallSignal::getPhase() const {return phi_rad;}
double SmallSignal::getFrequency() {return 0.0;}
std::vector<double> SmallSignal::getBreakpoints() const {return {};}
void SmallSignal::modifyAmplitude(double newAmplitude) {amplitude = newAmplitude;}
void SmallSignal::modifyPhase(double newPhi_deg) {phi_rad = newPhi_deg*(std::numbers::pi/180.0);}
#pragma endregion

#pragma region SineWave
SineWave::SineWave(double off, double amp, double freq, double t_d, double th, double phi_deg, double n = 0.0) :
    offset(off), amplitude(amp), frequency(freq), time_delay(t_d), theta(th), Ncycles(n) {phi_rad = phi_deg*(std::numbers::pi/180.0);}
std::string SineWave::getAttributes() {
    std::ostringstream ss;
    ss << "SINE(" << offset << " " << amplitude << " " << frequency << " " << time_delay << " " << theta << " " << phi_rad*(180.0/std::numbers::pi) << " " << Ncycles << ") ";
    return ss.str();
}
double SineWave::getValue() const {
//    if(context.mode == SimulationMode::ac_sweep) return offset;
    if(context.mode == SimulationMode::ac_sweep) return amplitude;
    double time = context.time;
    if(time < time_delay) return offset;
    double relative_time = time-time_delay;
    if(Ncycles > 0.0) {
        double duration = Ncycles/frequency;
        if(relative_time > duration) return offset;
    }
    double omega = 2*std::numbers::pi*frequency;
    double dampening = std::exp(-theta*relative_time);
    return amplitude*dampening*std::sin(omega*relative_time+phi_rad)+offset;
}
double SineWave::getPhase() const {return phi_rad;}
double SineWave::getFrequency() {return frequency;}
std::vector<double> SineWave::getBreakpoints() const {
    std::vector<double> breakpoints;
    breakpoints.push_back(time_delay);
    if(Ncycles > 0.0) {
        double end_of_cycles = time_delay+Ncycles/frequency;
        if(end_of_cycles > (time_delay+defaults.step_min)) breakpoints.push_back(end_of_cycles);
    }
    return breakpoints;
}
void SineWave::modifyOffset(double newOffset) {offset = newOffset;}
void SineWave::modifyAmplitude(double newAmplitude) {amplitude = newAmplitude;}
void SineWave::modifyFrequency(double newFrequency) {frequency = newFrequency;}
void SineWave::modifyTimeDelay(double newTimeDelay) {time_delay = newTimeDelay;}
void SineWave::modifyTheta(double newTheta) {theta = newTheta;}
void SineWave::modifyPhase(double newPhi_deg) {phi_rad = newPhi_deg*(std::numbers::pi/180.0);}
void SineWave::modifyNcycles(double newNcycles) {Ncycles = newNcycles;}
#pragma endregion

#pragma region Pulse
Pulse::Pulse(double v1, double v2, double t_d, double t_r, double t_f, double ton, double p, double n = 0.0) :
    V1(v1), V2(v2), time_delay(t_d), rise_time(t_r), fall_time(t_f), on_time(ton), period(p), Ncycles(n) {}
std::string Pulse::getAttributes() {
    std::ostringstream ss;
    ss << "PULSE(" << V1 << " " << V2 << " " << time_delay << " " << rise_time << " " << fall_time << " " << on_time << " " << period << " " << Ncycles << ") ";
    return ss.str();
}
double Pulse::getValue() const {
    double time = context.time;
    if(time < time_delay) return V1;
    if(Ncycles > 0.0) {
        double time_elapsed = time-time_delay;
        if(time_elapsed >= Ncycles*period) return V1;
    }
    double time_cycle = std::fmod((time-time_delay),period);
    double volt_diff = V2-V1;
    if(time_cycle < rise_time) {
        if(rise_time > 0.0) return V1+(time_cycle/rise_time)*(volt_diff);
        return V2;
    }
    double start_of_fall = rise_time+on_time;
    if(time_cycle <= start_of_fall) return V2;
    double time_active = start_of_fall+fall_time;
    if(time_cycle < time_active) {
        if(fall_time > 0.0) return V2-((time_cycle-rise_time-on_time)/fall_time)*(volt_diff);
        return V1;
    }
    return V1;
}
double Pulse::getPhase() const {return 0.0;}
double Pulse::getFrequency() {return 0.0;}
std::vector<double> Pulse::getBreakpoints() const {
    std::vector<double> breakpoints;
    double pulse_on = time_delay+rise_time;
    double pulse_falling = pulse_on+on_time;
    double pulse_off = pulse_falling+fall_time;
    double max_cycles = (Ncycles > 0.0) ? Ncycles : 20;
    for(int i = 0; i < max_cycles; i++) {
        double offset = i*period;
        breakpoints.push_back(time_delay+offset);
        breakpoints.push_back(pulse_on+offset);
        breakpoints.push_back(pulse_falling+offset);
        breakpoints.push_back(pulse_off+offset);
    }
    std::sort(breakpoints.begin(),breakpoints.end());
    auto new_end = std::unique(breakpoints.begin(),breakpoints.end());
    breakpoints.erase(new_end,breakpoints.end());
    return breakpoints;
}
void Pulse::modifyV1(double newV1) {V1 = newV1;}
void Pulse::modifyV2(double newV2) {V2 = newV2;}
void Pulse::modifyTimeDelay(double newTimeDelay) {time_delay = newTimeDelay;}
void Pulse::modifyRiseTime(double newRiseTime) {rise_time = newRiseTime;}
void Pulse::modifyFallTime(double newFallTime) {fall_time = newFallTime;}
void Pulse::modifyOnTime(double newOnTime) {on_time = newOnTime;}
void Pulse::modifyPeriod(double newPeriod) {period = newPeriod;}
void Pulse::modifyNcycles(double newNcycles) {Ncycles = newNcycles;}
#pragma endregion