/**
 * @yaml_parser.h
 * @brief Creating a Yaml Parser class
 *
 * This contains the prototypes of setting and getting parameters for yaml parser.
 *
 */

#ifndef __YAML_PARSER_H__
#define __YAML_PARSER_H__

#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <stdio.h>
#include <thread>
#include <sstream>
#include <regex>
#include <algorithm>

#include <edge-ml-accelerator/utils/json_parser.h>
#include <edge-ml-accelerator/utils/yaml_parser.h>

namespace edgeml
{
    namespace utils
    {

        class yamlParser
        {
            public:
                yamlParser();
                ~yamlParser();
                jsonParser::jValue yaml2json(const char* filename);

            private:
                std::string beg_delimiter = "---",
                        space_delimiter = " ",
                        colon_delimiter = ":",
                        hyphen_delimiter = "-",
                        colon_space_delimiter = ": ",
                        hyphen_space_delimiter = "- ";
                jsonParser::jValue yaml2jsonParser(std::vector<std::string> allStrsVec, std::vector<int> depthPos, int start_index, int end_index);
        };

    }
}

#endif