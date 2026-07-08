#include "mainwindow.h"

bool keep_running = true;

int main(int argc, char *argv[]) {
    std::string development_enviroment;
    while(keep_running) {
        std::cout << "Do you want to work on the engine or GUI? [e] engine, [g] for GUI development and [exit] to close the program." << std::endl;
        std::cin >> development_enviroment;
        if(development_enviroment == "e") {
            while(std::cin.get() != '\n') {}
            std::string instruction;
            while(true) {
                std::cout << "Press '/' to enter a command or '.' for a directive" << std::endl;
                std::getline(std::cin,instruction);
            instruct(instruction);
            }
            return 0;
        } else if(development_enviroment == "g") {
            QApplication a(argc,argv);
            MainWindow w;
            w.show();
            return a.exec();
        } else if(development_enviroment == "exit") {
            keep_running = false;
        } else {
            std::cout << development_enviroment << " is invalid." << std::endl;
        }
    }
    return 0;
}