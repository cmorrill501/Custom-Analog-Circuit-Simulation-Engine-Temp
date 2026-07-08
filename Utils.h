#ifndef Utils_H
#define Utils_H

#include <iostream>
#include <fstream>

template<typename T>
T clamp(T value,T min,T max) {
    if(min > max) {
        T temp = min;
        min = max;
        max = temp;
    }
    if(value > max) return max;
    if(value < min) return min;
    return value;
}

template<typename T>
T map(T input,T iMin,T iMax, T oMin,T oMax) {
    double normalize = static_cast<double>(input - iMin) / (iMax - iMin);
    normalize = clamp(normalize,T(0),T(1)); 
    T output = normalize * (oMax - oMin) + oMin;
    return std::abs(output);
}

inline bool filenameCheck(const std::string& filename) {
    std::ifstream file(filename.c_str());
    return file.good();
}

class BidirectionMap {
    private:
        std::unordered_map<int,std::string> id_to_name;
        std::unordered_map<std::string,int> name_to_id;
        bool verifyExists(int id) {
            auto search = id_to_name.find(id);
            if(search == id_to_name.end()) {
                std::cout << id << " does not exist." << std::endl;
                return false;
            }
            return true;
        }
        bool verifyExists(std::string name) {
            auto search = name_to_id.find(name);
            if(search == name_to_id.end()) {
                std::cout << name << " does4 not exist." << std::endl;
                return false;
            }
            return true;
        }
    public:
        void addToBimap(int id, std::string name = "") {
            if(name.empty()) name = std::to_string(id);
            id_to_name[id] = name;
            name_to_id[name] = id;
        }
        void removeFromBimap(int id) {
            if(!(verifyExists(id))) return;
            auto search_id = id_to_name.find(id);
            auto search_name = name_to_id.find(search_id->second);
            id_to_name.erase(search_id);
            name_to_id.erase(search_name);
            std::cout << id << " has be deleted successfully." << std::endl;
        }
        void clear() {
            id_to_name.clear();
            name_to_id.clear();
        }
        void changeName(std::string name,std::string newName) {
            if(name.empty()) {
                std::cout << "No name to modify was specified." << std::endl;
                return;
            }
            if(newName.empty()) {
                std::cout << "No new name was entered to change " << name << "to." << std::endl;
                return;
            }
            if(!(verifyExists(name))) return;
            auto search = name_to_id.find(name);
            auto nodeName = name_to_id.extract(search);
            if(nodeName.empty()) {
                std::cout << "ERROR: " << name << " could not be found" << std::endl;
                name_to_id.insert(std::move(nodeName));
                return;
            }
            nodeName.key() = newName;
            name_to_id.insert(std::move(nodeName));
            search = name_to_id.find(newName);
            id_to_name[search->second] = newName;
        }
        int nodeID (std::string name) {
            if(!(verifyExists(name))) return -1;
            auto search = name_to_id.find(name);
            return search->second;
        }
        std::string nodeName(int id) {
            if(id == -1) return "N-1";
            if(!(verifyExists(id))) return "DNE";
            auto search = id_to_name.find(id);
            return search->second;
        }
        std::vector<int> savedIDs() {
            std::vector<int> IDs;
            for(const auto& id : name_to_id) {
                IDs.push_back(id.second);
            }
            return IDs;
        }
        std::vector<std::string> savedNames() {
            std::vector<std::string> names;
            for(const auto& name : id_to_name) {
                names.push_back(name.second);
            }
            return names;
        }
};

#endif