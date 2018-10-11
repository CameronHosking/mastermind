// Mastermind.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

#include <math.h>

bool stopGuessing = false;
std::string bestGuess;
std::string characterSet;
std::vector<std::string> possiblePermutions;

std::string intToString(uint32_t intRepresentation, int length)
{
	int bitsPerLetter = int(log2(characterSet.size()-1))+1;
	uint32_t mask = 1;
	for (int i = 1; i < bitsPerLetter; ++i)
	{
		mask = (mask << 1) + 1;
	}
	std::string s = std::string(length, ' ');
	for (int i = length - 1; i >= 0; --i)
	{
		s[i] = characterSet[intRepresentation&mask];
		intRepresentation >>= bitsPerLetter;
	}
	return s;
}

uint32_t stringToInt(const std::string &s)
{
	int bitsPerLetter = int(log2(characterSet.size() - 1)) + 1;
	uint32_t result = 0;
	for (char c : s)
	{
		result <<= bitsPerLetter;
		result += characterSet.find_first_of(c);
	}
	return result;
}

int64_t powI(int64_t x, int64_t p)
{
	if (p == 0) return 1;
	if (p == 1) return x;

	int64_t tmp = powI(x, p / 2);
	if (p % 2 == 0) return tmp * tmp;
	else return x * tmp * tmp;
}

struct Result {
	Result() :
		rightColourRightLocation(0),
		rightColourWrongLocation(0) {}
	int rightColourRightLocation;
	int rightColourWrongLocation;
};

std::string generateCode(int length, std::string characterSet)
{
	std::string code;
	for (int i = 0; i < length; ++i)
	{
		code.append(1,characterSet.at(rand() % characterSet.size()));
	}
	return code;
}
std::vector<std::string> getAllPermutations(int length, std::string characterSet)
{
	if (length == 0)
	{
		std::vector<std::string> base;
		base.push_back("");
		return base;
	}
	std::vector<std::string> allPermutations;
	allPermutations.reserve(powI(characterSet.size(),length));
	std::vector<std::string> substrings = getAllPermutations(length - 1, characterSet);
	for(char c : characterSet)
	{
		for (std::string substring : substrings)
		{
			allPermutations.push_back(substring + c);
		}
	}
	return allPermutations;
}

Result getGuessResult(const std::string &guess,const std::string &code,const std::string &characterSet)
{
	Result r;
	for (int i = 0; i < code.size(); ++i)
	{
		if (guess.at(i) == code.at(i))
		{
			r.rightColourRightLocation++;
			r.rightColourWrongLocation--;//since we will count these again in the next step;
		}
	}

	for (char c : characterSet)
	{
		r.rightColourWrongLocation += std::min(std::count(guess.begin(), guess.end(), c), std::count(code.begin(), code.end(), c));
	}
	return r;
}

bool resultMatch(Result &guessResult, const std::string &guess, const std::string &code, const std::string &characterSet)
{
	int rightColourRightLocation = 0;
	int rightColourWrongLocation = 0;
	for (int i = 0; i < code.size(); ++i)
	{
		if (guess.at(i) == code.at(i))
		{
			rightColourRightLocation++;
			rightColourWrongLocation--;//since we will count these again in the next step;
		}
	}
	if (rightColourRightLocation != guessResult.rightColourRightLocation)
		return false;
	for (char c : characterSet)
	{
		rightColourWrongLocation += std::min(std::count(guess.begin(), guess.end(), c), std::count(code.begin(), code.end(), c));
	}
	if (rightColourWrongLocation != guessResult.rightColourWrongLocation)
		return false;

	return true;
}
double calcScore1(int* counts, int num)
{
	double max = 0;
	for (int i = 0; i < num; ++i)
	{
		if(double(counts[i]) > max)
			max = double(counts[i]);
	}
	return max;
}
double calcScore(int* counts, int num)
{
	double squaredSum = 0;
	for (int i = 0; i < num; ++i)
	{
		squaredSum += double(counts[i]) * double(counts[i]);
	}
	return squaredSum;
}

void updatePossiblecodes(Result guessResult,const std::string &guess,const std::string &characterSet)
{
	std::vector<std::string> newPossibleCodes;
	for (std::string s : possiblePermutions)
	{
		if (resultMatch(guessResult,guess, s, characterSet))
			newPossibleCodes.push_back(s);
	}
	possiblePermutions = newPossibleCodes;
}

void calculateBestGuess(int length, std::string characterSet)
{
	if (possiblePermutions.empty())
	{
		possiblePermutions = getAllPermutations(length, characterSet);
	}
	int possibleResults = powI(2, length);
	int *counts = new int[possibleResults];
	bestGuess = "";
	double minScore = INFINITY;
	for (int i = 0; i < 1000 && !stopGuessing; ++i)
	{
		std::string s = generateCode(length, characterSet);
		for (int i = 0; i < possibleResults; ++i) { counts[i] = 0; }
		for (std::string g : possiblePermutions)
		{
			Result res = getGuessResult(g, s, characterSet);
			counts[res.rightColourRightLocation*length + res.rightColourWrongLocation]++;
		}
		double score = calcScore(counts, possibleResults);
		if (score < minScore)
		{
			minScore = score;
			bestGuess = s;
		}
		//std::cout << i << std::endl;
	}
}

bool readFromCin(int &x)
{
	std::string in;
	std::getline(std::cin, in);
	try {
		x = std::stoi(in);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

int readFromCin(std::string &s)
{
	std::getline(std::cin, s);
	return s.size();
}

void computerGuessCode(int length, std::string set, std::thread &computer)
{
	while (true)
	{
		int option;
		std::cout << "type 1 once you have decided on your code:";
		if (readFromCin(option) && option == 1 )
			break;
	}
	std::cout << "OK... thinking ..." << std::endl;
	stopGuessing = true;
	computer.join();
	stopGuessing = false;
	int guesses = 0;
	while (true)
	{
		std::cout << bestGuess << std::endl;
		guesses++;
		int rCrL;
		while (true)
		{
			std::cout << "rCrL:";
			if (readFromCin(rCrL) && rCrL <= length && rCrL >= 0);
			break;
		}
		if (rCrL == length) {
			break;
		}
		int rCwL;
		while (true)
		{
			std::cout << "rCwL:";
			if (readFromCin(rCwL) && rCrL + rCwL <= length && rCwL >= 0);
			break;
		}
		std::cout << "OK... thinking ..." << std::endl;
		Result r;
		r.rightColourRightLocation = rCrL;
		r.rightColourWrongLocation = rCwL;
		updatePossiblecodes(r, bestGuess, set);
		if (possiblePermutions.size() == 1)
		{
			bestGuess = possiblePermutions.at(0);
		}
		else{
			calculateBestGuess(length, set);
		}
	}
	std::cout << "I took " << guesses << " round" << ((guesses == 1) ? "." : "s.") << std::endl;

	readFromCin(guesses);

}

void humanGuessCode(int length, std::string set)
{
	std::string code = generateCode(length, set);
	Result r;
	int guesses = 0;
	do {
		std::string guess;
		std::cin >> guess;
		uint32_t guessAsInt = stringToInt(guess);
		std::cout << guessAsInt << std::endl;
		std::cout << intToString(guessAsInt,length) << std::endl;

		r = getGuessResult(guess, code, set);
		guesses++;
		std::cout << "rCrL:" << r.rightColourRightLocation << "\trCwL:" << r.rightColourWrongLocation << std::endl;
	} while (r.rightColourRightLocation < length);
	std::cout << "congratualtions that is the code!" << std::endl;
	std::cout << "It took you " << guesses << " round" << ((guesses == 1) ? "." : "s.") << std::endl;
	std::cin >> set;
}

int main()
{
	srand(std::chrono::system_clock::now().time_since_epoch().count());
	while (characterSet.empty())
	{
		std::cout << "Input character set:";
		readFromCin(characterSet);
	}
	std::sort(characterSet.begin(), characterSet.end());
	characterSet.erase(std::unique(characterSet.begin(), characterSet.end()), characterSet.end());
	std::cout << std::endl << "Character set is:" << characterSet << std::endl;
	
	int codeLength;
	while (true)
	{
		std::cout << "Input code length:";
		if (readFromCin(codeLength))
		{
			if (codeLength < 1)
			{
				std::cout << "Code length must be greater than 0." << std::endl;
			}
			else if (pow(2, 32) < pow(characterSet.size(), codeLength))
			{
				std::cout << "Please, this problem is NP hard. Be reasonable." << std::endl;
			}else{
				break;
			}
		}
	}

	std::cout << std::endl << "Code length is:" << codeLength << std::endl;
	std::thread computer(calculateBestGuess,codeLength, characterSet);
	int option;
	while (true)
	{
		std::cout << "Type 1 for you to guess the computers code or 2 for the computer to guess your code:";
		if (readFromCin(option) && (option == 1 || option == 2))
			break;
	}
	if (option == 1)
	{
		stopGuessing = true;
		humanGuessCode(codeLength, characterSet);
		computer.join();
	}
	else
	{
		computerGuessCode(codeLength, characterSet, computer);
	}
    return 0;
}

