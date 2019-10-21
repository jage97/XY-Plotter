/*
 * Parser.cpp
 *
 *  Created on: 3.10.2019
 *      Author: jaakk
 */

#include "Parser.h"

vector<string> Parser::parse(string line) {
	vector<string> lineParts;
	int i = 0;
	while (line.size() > 0) {
		lineParts.push_back(line.substr(0, line.find(' ')));
		line.erase(0, (lineParts[i].size() + 1));
		i++;
	}
	return lineParts;
}
