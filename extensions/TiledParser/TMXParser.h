
#include "pch.h"
#include <fstream>
#include <sstream>
#include <strstream>
#include <string>
#include <map>
#include <chrono>

using namespace olc;

struct XMLTag{
    std::string tag;
    std::map<std::string,std::string> data;

    const std::string FormatTagData(std::map<std::string,std::string>tiles) {
        std::string displayStr="";
        for (std::map<std::string,std::string>::iterator it=data.begin();it!=data.end();it++) {
            displayStr+= "  " + it->first + ": " + it->second + "\n";
        }
        return displayStr;
    }

    friend std::ostream& operator << (std::ostream& os, XMLTag& rhs) { 
        os << rhs.tag <<"\n"<<  rhs.FormatTagData(rhs.data) <<"\n";
        return os; 
    }
    
    int GetInteger(std::string dataTag) {
        return std::stoi(data[dataTag]);
    }
    double GetDouble(std::string dataTag) {
        return std::stod(data[dataTag]);
    }
    bool GetBool(std::string dataTag) {
        if (data[dataTag]=="0") {
            return false;
        } else {
            return true;
        }
        
    }
};

struct LayerTag{
    XMLTag tag;
    std::vector<std::vector<int>> tiles;

    std::string str() {
        std::string displayStr=tag.tag+"\n"+tag.FormatTagData(tag.data);
        displayStr+="  DATA ("+std::to_string(tiles[0].size())+"x"+std::to_string(tiles.size())+")\n";
        return displayStr;
    }
};

struct Map_TMX{
    XMLTag MapData;
    XMLTag TilesetData;
    std::vector<LayerTag> LayerData;

    std::string FormatLayerData(std::ostream& os, std::vector<LayerTag>tiles) {
        std::string displayStr;
        for (int i=0;i<LayerData.size();i++) {
            displayStr+=LayerData[i].str();
        }
        return displayStr;
    }

    friend std::ostream& operator << (std::ostream& os, Map_TMX& rhs) { 
        os << rhs.MapData <<"\n"<< rhs.TilesetData <<"\n"<< rhs.FormatLayerData(os,rhs.LayerData) <<"\n";
        return os; 
    }
};

class TMXParser{

    public:

    Map_TMX GetData() {
        return parsedMapInfo;
    }

	private:

    Map_TMX parsedMapInfo;

#if NDEBUG
    bool bInDebugMode = false; // Set to false to disable debug logs
#else
    bool bInDebugMode = false; // Set to true to enable debug logs
#endif


	void ParseTag(std::string tag) {
		XMLTag newTag;
		
        //First character is a '<' so we discard it.
		tag.erase(0,1); 

        //last characters in the tag '>'. 
        tag.erase(tag.length() - 1, 1);

       
        // We have an edge case, where the last characters can be > or /> 
        // We need to manage this with a conditional statement
        char lastChar = tag.at(tag.length() - 1);
        if (lastChar == '/')
        {
            //last characters in the tag '/'. 
            tag.erase(tag.length() - 1, 1);
        }


        // Now parse by spaces.
		std::stringstream s(tag); //Turn it into a string stream to now parse into individual whitespaces.
		std::string data;

		while (s.good()) {
            data.clear();
			s>>data;
			if (newTag.tag.length()==0) { //Tag's empty, so first line is the tag.
				newTag.tag=data;
				if (bInDebugMode)
					std::cout<<"Tag: "<<newTag.tag<<"\n";

			} else {
				std::string key = data.substr(0,data.find("="));
				std::string value = data.substr(data.find("=")+1,std::string::npos);

                //Strip Quotation marks.
                value = value.substr(1,std::string::npos);
                value = value.substr(0,value.length()-1);

				newTag.data[key]=value;

				if (bInDebugMode)
					std::cout<<"  "<<key<<":"<<newTag.data[key]<<"\n";
			}
		}

        if (newTag.tag=="map") {
            parsedMapInfo.MapData=newTag;
        } else if (newTag.tag=="tileset") {
            parsedMapInfo.TilesetData=newTag;

        } else if (newTag.tag=="layer") {
            LayerTag l = {newTag};
            parsedMapInfo.LayerData.push_back(l);

        } else {
            if (bInDebugMode)
                std::cout<<"Unsupported tag format! Ignoring."<<"\n";
        }
		if (bInDebugMode)
			std::cout<<"\n"<<"=============\n";
	}

	public:

	TMXParser(std::string file)
    {
		/*std::cout << "Parsing TMX file: " << file << "\n";
        auto start = std::chrono::high_resolution_clock::now();*/
        // Lets load the entire file into a string to make things a little faster
		std::ifstream f(file,std::ios::in | std::ios::binary);
		if (!f.is_open()) {
			if (bInDebugMode)
				std::cout << "Failed to open file: " << file << "\n";
			return;
        };
		f.seekg(0, std::ios::end);
		std::streampos fileSize = f.tellg();
		f.seekg(0, std::ios::beg);

		std::string fileContent;
		fileContent.resize(fileSize);
		f.read(&fileContent[0], fileSize);
        f.close();

		// Now we have the entire file in a string, we can parse it.
		size_t currentPosition = 0;
        std::stringstream s(fileContent);
		std::string accumulator = "";
        std::string data;

		while (s.good()) {
            data.clear();
			s>>data;
            if (data.empty()) continue;

			if (accumulator.length()>0) {
				accumulator+=" "+data;
				//Check if it ends with '>'
				if (data[data.length()-1]=='>') {
					ParseTag(accumulator);
					accumulator.clear();
				}
			} else
			if (data[0]=='<') {
				//Beginning of XML tag.
				accumulator=data;
			} else {
                //Start reading in data for this layer.
                std::vector<int>rowData;
                while (data.find(",")!=std::string::npos) {
                    std::string datapiece = data.substr(0,data.find(","));
                    data = data.substr(data.find(",")+1,std::string::npos);
                    rowData.push_back(stoi(datapiece));
                }
                if (data.length()) {
                    rowData.push_back(stoi(data));
                }
                parsedMapInfo.LayerData[parsedMapInfo.LayerData.size()-1].tiles.push_back(rowData);
            }
		}

        if (bInDebugMode)
            std::cout<<"Parsed Map Data:\n"<<parsedMapInfo<<"\n";

		//auto end = std::chrono::high_resolution_clock::now();
  //      // Calculate duration in milliseconds
  //      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  //      // Output results
  //      std::cout << "Start Time TMX:    " << start.time_since_epoch().count() << " ns\n";
  //      std::cout << "End Time TMX:      " << end.time_since_epoch().count() << " ns\n";
  //      std::cout << "Duration TMX:      " << duration.count() << " ms\n";

	}

    virtual ~TMXParser(){}

};