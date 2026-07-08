#ifndef Commands_H
#define Commands_H

#include <QApplication>
#include <QVector>
#include "qcustomplot.h"

#include <cstdlib>

#include "CircuitManager.h"

bool filenameCheck(const std::string& filename);

void clearScreen();
void close();
std::tuple<QVector<double>,QVector<double>> findDataPoints(std::string name);
void plotable();
int createPlot(std::stringstream& ss);
void function_call(std::string command);

void instruct(std::string instruction);
void directives(std::stringstream& ss);
void scripting(std::stringstream& ss);

#endif