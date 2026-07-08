#ifndef Waveforms_H
#define WaveForms_H

#include <string>
#include <vector>
#include <numbers>

#include "Information.h"

class Waveforms {
    public:
        virtual ~Waveforms() = default;
        virtual std::string getType() = 0;
        virtual std::string getAttributes() = 0;
        virtual double getValue() const = 0;
        virtual double getPhase() const = 0;
        virtual double getFrequency() = 0;
        virtual std::vector<double> getBreakpoints() const = 0;
};

class SmallSignal : public Waveforms {
    public:
        double amplitude;
        double phi_rad;
        SmallSignal(double amp, double phi_de);
        std::string getType() override {return "SmallSignal";}
        std::string getAttributes() override;
        double getValue() const override;
        double getPhase() const override;
        double getFrequency() override;
        std::vector<double> getBreakpoints() const override;
        void modifyAmplitude(double newAmplitude);
        void modifyPhase(double newPhi_deg);
};

class SineWave : public Waveforms {
    public:
        double offset;
        double amplitude;
        double frequency;
        double time_delay;
        double theta;
        double phi_rad;
        double Ncycles;
        SineWave(double off, double amp, double freq, double delay, double th, double phi_deg, double cycles);
        std::string getType() override {return "SineWave";}
        std::string getAttributes() override;
        double getValue() const override;
        double getPhase() const override;
        double getFrequency() override;
        std::vector<double> getBreakpoints() const override;
        void modifyOffset(double newOffset);
        void modifyAmplitude(double newAmplitude);
        void modifyFrequency(double newFrequency);
        void modifyTimeDelay(double newTimeDelay);
        void modifyTheta(double newTheta);
        void modifyPhase(double newPhi_deg);
        void modifyNcycles(double newNcycles);
};

class Pulse : public Waveforms {
    public:
        double V1;
        double V2;
        double time_delay;
        double rise_time;
        double fall_time;
        double on_time;
        double period;
        double Ncycles;
        Pulse(double v1, double v2, double delay, double rise, double fall, double on, double p, double cycles);
        std::string getType() override {return "Pulse";}
        std::string getAttributes() override;
        double getValue() const override;
        double getPhase() const override;
        double getFrequency() override;
        std::vector<double> getBreakpoints() const override;
        void modifyV1(double newV1);
        void modifyV2(double newV2);
        void modifyTimeDelay(double newTimeDelay);
        void modifyRiseTime(double newRiseTime);
        void modifyFallTime(double newFallTime);
        void modifyOnTime(double newOnTime);
        void modifyPeriod(double newPeriod);
        void modifyNcycles(double newNcycles);
};

#endif