#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <stack>
#include <sstream>

class datafile
{
public:
    inline void SetString(const std::string& sString, const size_t nItem = 0) // 1,2,3,"hello world", abc
    { 
        if (nItem >= m_vContent.size())
            m_vContent.resize(nItem + 1);
        m_vContent[nItem] = sString;
    }

    inline const std::string GetString(const size_t nItem = 0) const 
    {
        if (nItem >= m_vContent.size())
            return "";
        else
            return m_vContent[nItem];
    }

    inline void SetReal(const double d, const size_t nItem = 0) 
    {
        SetString(std::to_string(d), nItem);
    }

    inline const double GetReal(const size_t nItem = 0) const 
    {
        return std::atof(GetString(nItem).c_str());
    }

    inline void SetInt(const int32_t n, const size_t nItem = 0) 
    {
        SetString(std::to_string(n), nItem);
    }

    inline const int GetInt(const size_t nItem = 0) const 
    {
        return std::atoi(GetString(nItem).c_str());
    }

    inline size_t GetValueCount() const
    {
        return m_vContent.size();
    }

    inline datafile& operator[](const std::string& name) 
    {
        if (m_mapObjects.count(name) == 0) {
            m_mapObjects[name] = m_vecObjects.size();
            m_vecObjects.push_back({ name,datafile() });
        }
        return m_vecObjects[m_mapObjects[name]].second;
    }

    inline static bool Write(const datafile& n, const  std::string& sFileName,
        const std::string& sIndent = "    ", const char sListSep = ',') 
    {
        std::string sSeparator = std::string(1, sListSep) + " ";
        size_t nIndentCount = 0;

        std::function<void(const datafile&, std::ofstream&)> write = [&](const datafile& n, std::ofstream& file)
        {
            auto indent = [&](const std::string& sString, const size_t nCount)
            {
                std::string sOut;
                for (size_t n = 0; n < nCount; n++)sOut += sString;
                return sOut;
            };

            for (auto const& property : n.m_vecObjects)
            {
                if (property.second.m_vecObjects.empty()) {
                    file << indent(sIndent, nIndentCount) << property.first << " = ";
                    size_t nItems = property.second.GetValueCount();
                    for (size_t i = 0; i < property.second.GetValueCount(); i++) 
                    {
                        size_t x = property.second.GetString(i).find_first_of(sListSep);
                        if (x != std::string::npos) 
                        {
                            file << "\"" << property.second.GetString(i) << "\"" << ((nItems > 1) ? sSeparator : "");
                        }
                        else 
                        {
                            file << property.second.GetString(i) << ((nItems > 1) ? sSeparator : "");
                        }
                        nItems--;
                    }
                    file << "\n";
                }
                else
                {
                    file <<  indent(sIndent, nIndentCount) << property.first << "\n";
                    file << indent(sIndent, nIndentCount) << "{\n";
                    nIndentCount++;
                    write(property.second, file);
                    file << indent(sIndent, nIndentCount) << "}\n";
                }
            }
            if (nIndentCount > 0)nIndentCount--;
        };

        std::ofstream file(sFileName);
        if (file.is_open()) {
            write(n, file);
            return true;
        }
        return false;
    }

    inline static bool Read(datafile& n, const std::string& sFileName, const char sListSep = ',')
    {
        std::ifstream file(sFileName);
        if (file.is_open()) 
        {
            std::string sPropName = "";
            std::string sPropValue = "";

            std::stack<std::reference_wrapper<datafile>> stkPath;
            stkPath.push(n);

            while (!file.eof()) 
            {
                std::string line;
                std::getline(file, line);

                auto trim = [](std::string& s)
                {
                    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
                    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
                };
                trim(line);
                if (!line.empty())
                {
                    size_t x = line.find_first_of('=');
                    if (x != std::string::npos) 
                    {
                        sPropName = line.substr(0, x);
                        trim(sPropName);
                        sPropValue = line.substr(x + 1, line.size());
                        trim(sPropValue);

                        bool bInQuotes = false;
                        std::string sToken;
                        size_t nTokenCount = 0;
                        for (const auto c : sPropValue)
                        {
                            if (c == '\"') 
                            {
                                bInQuotes = !bInQuotes;
                            }
                            else
                            {
                                if (bInQuotes) {
                                    sToken.append(1, c);
                                }
                                else 
                                {
                                    if (c == sListSep) 
                                    {
                                        trim(sToken);
                                        stkPath.top().get()[sPropName].SetString(sToken, nTokenCount);
                                        sToken.clear();
                                        nTokenCount++;
                                    }
                                    else 
                                    {
                                        sToken.append(1, c);
                                    }
                                }
                            }
                        }
                        if (!sToken.empty()) {
                            trim(sToken);
                            stkPath.top().get()[sPropName].SetString(sToken, nTokenCount);
                        }
                    }
                    else
                    {
                        if (line[0] == '{')
                        {
                            stkPath.push(stkPath.top().get()[sPropName]);
                        }
                        else 
                        {
                            if (line[0] == '}')
                            {
                                stkPath.pop();
                            }
                            else
                            {
                                sPropName = line;
                            }
                        }
                    }
                }
            }

            file.close();
            return true;
        }
        return false;
    }

private:
    // The "list of strings" that make up a property value
    std::vector<std::string> m_vContent;
    std::vector<std::pair<std::string, datafile>>m_vecObjects;
    std::unordered_map<std::string, size_t> m_mapObjects;
};


int main()
{
    datafile df;
    auto& node = df["data"];
    node["name"].SetString("World");
    node["age"].SetInt(50);
    node["height"].SetReal(175.5);

    node["code"].SetString("C++");
    node["code"].SetInt(17,1);
    node["code"].SetString("Abcc",2);
    
    auto& x = node["projects"];
    x["Number"].SetInt(100);
    x["Project 1"].SetString("Hello WOrld");
    auto& b = node["books"];
    int n = 10;
    b["Count"].SetInt(n);
    for (int i = 0; i < n; i++) {
        b["Book " + std::to_string(i + 1)].SetInt(rand());
    }

    datafile::Write(df, "datafile.txt");
    datafile xx;
    datafile::Read(xx, "datafile2.txt");
    datafile::Write(xx, "datafile4.txt");
}
