/**
 * @yaml_parser.cc
 * @brief Class to parse yaml files
 *
 * This contains the function definitions for parsing a yaml file into different sections.
 *
 */

#include <edge-ml-accelerator/utils/yaml_parser.h>

namespace edgeml
{
    namespace utils
    {

        /**
        Creates the class constructor
        */
        yamlParser::yamlParser()
        {
        }

        /**
        Creates the class destructor
        */
        yamlParser::~yamlParser()
        {
        }

        /**
        Routine to convert YAML to JSON
        @param filename input YAML file
        @return jsonParser::jValue as the JSON output
        */
        jsonParser::jValue yamlParser::yaml2json(const char* filename)
        {
            std::string mainStr;
            std::string jsonParamsStr1 = "{}", jsonParamsStr2 = "[]";
            jsonParser::jValue jsonBlockParams, jsonSubBlockParams;
            jsonParser::jValue jsonMainBlockParams = jsonParser::parser::parse(jsonParamsStr1);

            std::string line;
            std::ifstream in(filename);
            std::string str = "";
            std::vector<std::string> allStrsVec, mainStrVec, subStrsVec;
            std::vector<int> mainStrPos, depthPos;
            std::string tmp;

            int iter = 0;
            while (std::getline(in, tmp))
            {
                if (tmp.compare(beg_delimiter) == 0)
                    continue;

                if (std::isalpha(tmp[0]))
                {
                    if ((tmp.find(colon_space_delimiter) != std::string::npos))
                    {
                        depthPos.push_back(-99);
                        std::string currStr = std::regex_replace(tmp, std::regex{space_delimiter}, std::string{""});
                        std::string token;
                        if (currStr.find(colon_delimiter.c_str()) != std::string::npos)
                        {
                            size_t pos = 0;
                            while ((pos = currStr.find(colon_delimiter)) != std::string::npos)
                            {
                                token = currStr.substr(0, pos);
                                currStr.erase(0, pos + colon_delimiter.length());
                            }
                            jsonSubBlockParams.add_property(token, jsonParser::parser::parse(currStr));
                        }
                        mainStrVec.push_back(token);
                    }
                    else
                    {
                        depthPos.push_back(0);
                        std::string currStr = std::regex_replace(tmp, std::regex{colon_delimiter}, std::string{""});
                        mainStrVec.push_back(currStr);
                    }
                    mainStrPos.push_back(iter);
                }
                else
                {
                    subStrsVec.push_back(tmp);
                    int n = tmp.length(), spaces_counter = 0;
                    for (int j=0; j<n; ++j)
                    {
                        if (std::isalpha(tmp[j]))
                        {
                            break;
                        }
                        spaces_counter++;
                    }
                    depthPos.push_back(spaces_counter/2);
                }
                allStrsVec.push_back(tmp);
                iter++;
            }

            for (unsigned int i=0; i<mainStrPos.size(); i++)
            {
                jsonBlockParams = yaml2jsonParser(allStrsVec, depthPos, mainStrPos[i], mainStrPos[i+1]);
                std::string jsonBlockParamsStr = jsonBlockParams[0].to_string();
                jsonMainBlockParams.add_property(mainStrVec[i], jsonBlockParams[0]);
            }

            return jsonMainBlockParams;
        }

        /**
        Routine to parse YAML to JSON
        @param allStrsVec vector of strings of all the lines of the YAML file
        @param depthPos depth of the elements in the YAML file
        @param start_index starting index of the allStrsVec
        @param end_index ending index of the allStrsVec
        @return jsonParser::jValue as the JSON output
        */
        jsonParser::jValue yamlParser::yaml2jsonParser(std::vector<std::string> allStrsVec, std::vector<int> depthPos, int start_index, int end_index)
        {
            std::string mainStr, currStr;
            std::string jsonParamsStr1 = "{}", jsonParamsStr2 = "[]";
            std::vector<std::string> strArr;
            jsonParser::jValue jsonBlockParams, jsonSubBlockParams;
            jsonParser::jValue jsonMainBlockParams = jsonParser::parser::parse(jsonParamsStr1);
            bool isMulti = false;
            int curr_depth = depthPos[start_index];

            if (curr_depth==-99)
            {
                currStr = allStrsVec[start_index];
                currStr = std::regex_replace(currStr, std::regex{space_delimiter}, std::string{""});
                if (currStr.find(colon_delimiter.c_str()) != std::string::npos)
                {
                    size_t pos = 0;
                    std::string token;
                    while ((pos = currStr.find(colon_delimiter)) != std::string::npos)
                    {
                        token = currStr.substr(0, pos);
                        currStr.erase(0, pos + colon_delimiter.length());
                    }
                    jsonMainBlockParams.add_property(token, jsonParser::parser::parse(currStr));
                }
                return jsonMainBlockParams;
            }

            if ((allStrsVec[start_index].find(colon_delimiter.c_str()) != std::string::npos))
            {
                mainStr = allStrsVec[start_index];
                mainStr = std::regex_replace(mainStr, std::regex{colon_delimiter}, std::string{""});
                mainStr = std::regex_replace(mainStr, std::regex{space_delimiter}, std::string{""});
                if ((allStrsVec[start_index+1].find(hyphen_space_delimiter.c_str()) != std::string::npos))
                {
                    jsonBlockParams = jsonParser::parser::parse(jsonParamsStr2);
                    isMulti = true;
                }
                else
                {
                    jsonBlockParams = jsonParser::parser::parse(jsonParamsStr1);
                    isMulti = false;
                }
            }

            jsonSubBlockParams = jsonParser::parser::parse(jsonParamsStr1);

            for (int i=start_index+1; i<end_index; i++)
            {
                if (depthPos[i]==curr_depth+1)
                {
                    if ((allStrsVec[i].find(hyphen_space_delimiter.c_str()) != std::string::npos))
                    {
                        if (jsonSubBlockParams.size()>0)
                        {
                            jsonBlockParams.add_element(jsonSubBlockParams);
                            jsonSubBlockParams = jsonParser::parser::parse(jsonParamsStr1);
                        }
                        currStr = allStrsVec[i];
                        currStr = std::regex_replace(currStr, std::regex{hyphen_space_delimiter}, std::string{""});
                        currStr = std::regex_replace(currStr, std::regex{space_delimiter}, std::string{""});
                        if (currStr.find(colon_delimiter.c_str()) != std::string::npos)
                        {
                            size_t pos = 0;
                            std::string token;
                            while ((pos = currStr.find(colon_delimiter)) != std::string::npos)
                            {
                                token = currStr.substr(0, pos);
                                currStr.erase(0, pos + colon_delimiter.length());
                                break;
                            }
                            jsonSubBlockParams.add_property(token, jsonParser::parser::parse(currStr));
                        }
                        else
                        {
                            strArr.push_back(currStr);
                        }
                    }
                    else if ((allStrsVec[i].find(colon_space_delimiter.c_str()) != std::string::npos))
                    {
                        currStr = allStrsVec[i];
                        currStr = std::regex_replace(currStr, std::regex{hyphen_space_delimiter}, std::string{""});
                        currStr = std::regex_replace(currStr, std::regex{space_delimiter}, std::string{""});
                        if (currStr.find(colon_delimiter.c_str()) != std::string::npos)
                        {
                            size_t pos = 0;
                            std::string token;
                            while ((pos = currStr.find(colon_delimiter)) != std::string::npos)
                            {
                                token = currStr.substr(0, pos);
                                currStr.erase(0, pos + colon_delimiter.length());
                                break;
                            }
                            jsonSubBlockParams.add_property(token, jsonParser::parser::parse(currStr));
                        }
                    }
                    else
                    {
                        int sub_start_index = i;
                        int sub_end_index = i+1;
                        int sub_curr_depth = depthPos[sub_end_index];
                        while (1)
                        {
                            sub_end_index++;
                            if (depthPos[sub_end_index]<sub_curr_depth)
                                break;
                        }
                        jsonParser::jValue tmpJsonParams_ = yaml2jsonParser(allStrsVec, depthPos, sub_start_index, sub_end_index);
                        currStr = allStrsVec[sub_start_index];
                        currStr = std::regex_replace(currStr, std::regex{hyphen_space_delimiter}, std::string{""});
                        currStr = std::regex_replace(currStr, std::regex{colon_delimiter}, std::string{""});
                        currStr = std::regex_replace(currStr, std::regex{space_delimiter}, std::string{""});
                        jsonSubBlockParams.add_property(currStr, tmpJsonParams_[0]);
                    }
                }
            }
            if (isMulti)
            {
                if (strArr.size()>0)
                {
                    std::string combineStr = "";
                    for (int ii=0; ii<strArr.size(); ii++)
                    {
                        combineStr += strArr[ii] + ", ";
                        jsonSubBlockParams = jsonParser::parser::parse(strArr[ii]);
                        jsonBlockParams.add_element(jsonSubBlockParams);
                    }
                }
                else
                {
                    jsonBlockParams.add_element(jsonSubBlockParams);
                }
                jsonMainBlockParams.add_property(mainStr, jsonBlockParams);
            }
            else
            {
                jsonMainBlockParams.add_property(mainStr, jsonSubBlockParams);
            }

            return jsonMainBlockParams;
        }

    }
}