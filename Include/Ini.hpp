#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static std::string TrimString(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if(first == std::string::npos) { return ""; }
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

class IniFile {
  public:
    struct IniValue {
        void operator=(int iValue) {
            this->szValue = std::to_string(iValue);
            bIsValid = true;
        }

        void operator=(float fValue) {
            this->szValue = std::to_string(fValue);
            bIsValid = true;
        }

        void operator=(bool bValue) { this->szValue = bValue ? "true" : "false"; }

        void operator=(const std::string& szValue) { this->szValue = szValue; }

        void operator=(const S_vector& vVec) {
            this->szValue = "{" + std::to_string(vVec.x) + ", " + std::to_string(vVec.y) + ", " + std::to_string(vVec.z) + "}";
        }

        operator int() const { return std::stoi(szValue); }

        operator float() const { return std::stof(szValue); }

        operator bool() const {
            if(szValue == "true") { return true; }

            return false;
        }

        operator std::string() const { return szValue; }

        operator S_vector() const {
            S_vector vec;
            std::string str = szValue;

            if(str.front() == '{') { str.erase(str.begin()); }
            if(str.back() == '}') { str.pop_back(); }

            str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());

            std::istringstream iss(str);
            char delimiter;

            if(!((iss >> vec.x >> delimiter) && (delimiter == ',') && (iss >> vec.y >> delimiter) && (delimiter == ',') && (iss >> vec.z))) { vec = {}; }

            return vec;
        }

        void SetName(const std::string& szName) { this->szName = szName; }

        bool IsValid() const { return bIsValid; }

      private:
        std::string szName;
        std::string szValue;
        bool bIsValid = false;

        friend class IniFile;
    };

    struct IniSection {
        bool HasValue(const std::string& szValueName) {
            for(auto& value: vValues) {
                if(value.szName == szValueName) { return true; }
            }

            return false;
        }

        IniValue& operator[](const std::string& szName) {
            if(HasValue(szName)) return GetValue(szName);
            else
                return AddValue(szName);
        }

        IniValue& AddValue(const std::string& szValueName) {
            IniValue value;
            value.bIsValid = true;
            value.szName = szValueName;

            vValues.push_back(value);

            return vValues[vValues.size() - 1];
        }

        IniValue& GetValue(const std::string& szValueName) {
            for(auto& value: vValues) {
                if(value.szName == szValueName) { return value; }
            }

            static IniValue invalidValue;
            invalidValue.bIsValid = false;

            return invalidValue;
        }

      private:
        std::string szName;
        std::vector<IniValue> vValues;

        friend class IniFile;
    };

    IniFile() { m_bIsValid = true; }

    IniFile(const std::string& szFileName) {
        std::ifstream file(szFileName);
        if(!file.is_open()) { return; }

        std::string line;
        IniSection* currentSection = &m_sRootSection;

        while(std::getline(file, line)) {
            line = TrimString(line);
            if(line.empty() || line[0] == ';' || line[0] == '#') {
                continue; // Skip empty lines and comments
            }

            if(line[0] == '[' && line.back() == ']') {
                IniSection newSection;
                newSection.szName = line.substr(1, line.size() - 2);
                m_vSections.push_back(newSection);
                currentSection = &m_vSections.back();
            } else {
                size_t delimiterPos = line.find_first_of("=");
                if(delimiterPos != std::string::npos) {
                    IniValue value;
                    value.szName = TrimString(line.substr(0, delimiterPos));
                    value.szValue = TrimString(line.substr(delimiterPos + 1));
                    value.bIsValid = true;
                    if(currentSection != nullptr) { currentSection->vValues.push_back(value); }
                }
            }
        }

        file.close();
        m_bIsValid = true;
    }

    void Write(const std::string& szFileName) {
        std::ofstream file(szFileName);

        for(auto& value: m_sRootSection.vValues) {
            file << value.szName + " = " + value.szValue << std::endl;
        }
        file << std::endl;

        for(auto& section: m_vSections) {
            file << "[" + section.szName + "]" << std::endl;
            for(auto& value: section.vValues) {
                file << value.szName + " = " + value.szValue << std::endl;
            }
            file << std::endl;
        }
    }

    bool IsValid() const { return m_bIsValid; }

    bool HasSection(const std::string& szSectionName) {
        for(auto& section: m_vSections) {
            if(section.szName == szSectionName) { return true; }
        }

        return false;
    }

    IniSection& operator[](const std::string& szName) {
        if(HasSection(szName)) return GetSection(szName);
        else if(szName == "#root")
            return m_sRootSection;
        else
            return AddSection(szName);
    }

    IniSection& AddSection(const std::string& szSectionName) {
        IniSection section;
        section.szName = szSectionName;

        m_vSections.push_back(section);

        return m_vSections[m_vSections.size() - 1];
    }

    IniSection& GetSection(const std::string& szSectionName) {
        for(auto& section: m_vSections) {
            if(section.szName == szSectionName) { return section; }
        }

        return m_sRootSection;
    }

  private:
    IniSection m_sRootSection;
    std::vector<IniSection> m_vSections;
    bool m_bIsValid = false;
};