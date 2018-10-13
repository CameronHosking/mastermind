// Mastermind.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "PRNG\PRNG.h"

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

#include <math.h>

uint64_t getCountsRepresentation(uint32_t code, int length);
std::string intToString(uint32_t intRepresentation, int length);
uint32_t stringToInt(const std::string &s);

struct Code {
	Code(uint32_t code = 0, uint64_t characterCounts = 0)
		:code(code), counts(characterCounts)
	{}
	Code(const std::string &s,int length)
		:code(stringToInt(s)), 
		counts(getCountsRepresentation(code, length))
	{}
	uint32_t code;
	uint64_t counts;
};

bool stopGuessing = false;
int bitsPerLetter;
uint32_t blackMask;
uint32_t charMask;
Code bestGuess;
std::string characterSet;
std::vector<Code> possibleCodes;
std::vector<uint64_t> possiblePermutatinsCharCounts;
fastPRNG::PRNG rng;

struct TinyTimer {
	void reset() {
		t = std::chrono::high_resolution_clock::now();
	}

	TinyTimer()
	{
		reset();
	}

	void printElapsedTime() {
		auto current = std::chrono::high_resolution_clock::now();
		std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(current - t).count()/double(std::nano::den) << std::endl;
	}

	void printElapsedTimeAndReset()
	{
		printElapsedTime();
		reset();
	}
	std::chrono::steady_clock::time_point t;
};

struct Result {
	Result() :
		b(0),
		w(0) {}
	int b;
	int w;
	bool operator==(const Result &other)
	{
		return other.b == b&&other.w == w;
	}
	bool operator!=(const Result &other)
	{
		return !operator==(other);
	}
};

//helper method for unique first guesses method
//returns a set of all the unique guesses of length L
//with access up to K letters and a minimum value of M
std::vector<std::vector<int>> f(int K, int L, int M)
{
	std::vector<std::vector<int>> t;
	//base case
	if (K == 1)
	{
		t.push_back(std::vector<int>());
		t.at(0).push_back(L);
		return t;
	}

	for (int i = M; i <= L/K; ++i)
	{
		std::vector<std::vector<int>> subset = f(K-1, L-i, i);
		for (std::vector<int> s : subset)
		{
			t.push_back(s);
			t.back().push_back(i);
		}
	}
	return t;
}
//Calculates the unique set of all the possible first guesses
//keeping in mind that only numbers of different character matters
//I can't explain the maths used here it's based on several pages of scribblings
std::vector<std::vector<int>> uniqueFirstGuesses(int K, int L)
{
	std::vector<std::vector<int>> t;
	for (int i = 1; i <= K; ++i)
	{
		std::vector<std::vector<int>> subset = f(i, L, 1);
		for (std::vector<int> s : subset)
		{
			t.push_back(s);
		}
	}
	return t;
}

std::string getCodeWithGivenLetterDistribution(const std::vector<int> &dist)
{
	std::string validChars = characterSet;
	std::string code;
	for (int i : dist)
	{
		int charIndex = rng.next() % validChars.size();

		code.append(i, validChars[charIndex]);
		validChars.erase(charIndex,1);
	}
	std::random_shuffle(code.begin(), code.end());
	return code;
}

std::vector<Code> getFirstGuessCodes(int K, int L)
{
	std::vector<std::vector<int>> u = uniqueFirstGuesses(K, L);
	std::vector<Code> codes;
	for (std::vector<int> dist : u)
	{
		std::string s = getCodeWithGivenLetterDistribution(dist);
		codes.push_back(Code(s,s.size()));
	}
	return codes;
}

std::string intToString(uint32_t intRepresentation, int length)
{
	std::string s = std::string(length, ' ');
	for (int i = length - 1; i >= 0; --i)
	{
		s[i] = characterSet[intRepresentation&charMask];
		intRepresentation >>= bitsPerLetter;
	}
	return s;
}

uint32_t stringToInt(const std::string &s)
{
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



std::string generateCode(int length, std::string characterSet)
{
	std::string code;
	for (int i = 0; i < length; ++i)
	{
		code.append(1,characterSet.at(rng.getUint32() % characterSet.size()));
	}
	return code;
}

//get all possible permutations of l characters from a set of characters of size k 
std::vector<uint32_t> getAllPermutations(int l, int k)
{
	if (l == 0)
	{
		std::vector<uint32_t> base;
		base.push_back(0);
		return base;
	}
	std::vector<uint32_t> allPermutations;
	allPermutations.reserve(powI(k, l));
	std::vector<uint32_t> substrings = getAllPermutations(l - 1, k);
	uint32_t location = 1 << bitsPerLetter*(l-1);
	for (int i = 0; i < k; ++i)
	{
		uint32_t thisCharacter = i*location;
		for (uint32_t s : substrings)
		{
			allPermutations.push_back(s | thisCharacter);
		}
	}
	return allPermutations;
}

uint64_t getCountsRepresentation(uint32_t code, int length)
{
	uint64_t lengthMask = (1ULL << length) - 1;//length 1s
	uint64_t counts = 0;
	for (int i = 0; i < length; ++i)
	{
		uint32_t num = charMask&(code >> (i*bitsPerLetter));
		counts |= (counts << 1)&(lengthMask << (length*num));
		counts |= 1ULL << (length*num);
	}
	return counts;
}

//use if (log2(length-1) + 1)*setSize < 32
Result getGuessResult(uint32_t guess, uint64_t guessCharacterCounts, uint32_t code, uint64_t codeCharacterCounts)
{
	uint32_t m = blackMask;
	//count blacks
	uint32_t leftover = ~(guess^code);//leftover will contain blocks of 1's for each match
	m &= leftover;
	for (int i = 1; i < bitsPerLetter;++i)
	{
		m = (m << 1) & leftover;
	}
	Result r;
	r.b = __popcnt(m);

	r.w = __popcnt64(guessCharacterCounts&codeCharacterCounts);

	r.w -= r.b;
	//count whites
	
	return r;
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

void updatePossiblecodes(Result guessResult,const Code &guessedCode)
{
	std::vector<Code> newPossibleCodes;
	for (Code c : possibleCodes)
	{
		if (guessResult== getGuessResult(guessedCode.code,guessedCode.counts,c.code,c.counts))
			newPossibleCodes.push_back(c);
	}
	possibleCodes = newPossibleCodes;
}

double getGuessScore(Code s,int length)
{
	int possibleResults = powI(2, length);
	int *counts = new int[possibleResults];
	for (int i = 0; i < possibleResults; ++i) { counts[i] = 0; }
	for (Code c : possibleCodes)
	{
		Result res = getGuessResult(c.code, c.counts, s.code, s.counts);
		counts[res.b*length + res.w]++;
	}
	return calcScore1(counts, possibleResults);
}

void calculateBestGuess(int length, std::string characterSet)
{
	
	if (possibleCodes.empty())
	{
		TinyTimer t;
		std::vector<uint32_t> codes = getAllPermutations(length, characterSet.size());
		t.printElapsedTimeAndReset();
		int size = sizeof(Code);
		possibleCodes.reserve(sizeof(Code)*codes.size());
		for (uint32_t code : codes)
		{
			possibleCodes.push_back(Code(code, getCountsRepresentation(code, length)));
		}
		t.printElapsedTime();
		std::vector<Code> firstGuesses = getFirstGuessCodes(characterSet.size(), length);
		double minScore = INFINITY;
		for (Code c : firstGuesses)
		{
			double score = getGuessScore(c, length);
			if (score < minScore)
			{
				minScore = score;
				bestGuess = c;
				std::cout << intToString(bestGuess.code, length) << "\t" << minScore << std::endl;
			}
		}
	}
	else
	{
		double minScore = INFINITY;
		for (int i = 0; i < 10000 && !stopGuessing; ++i)
		{
			Code s = possibleCodes[rng.next() % possibleCodes.size()];
			double score = getGuessScore(s, length);
			if (score < minScore)
			{
				minScore = score;
				bestGuess = s;
				std::cout << intToString(bestGuess.code, length) << "\t" << minScore << std::endl;
			}
		}
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

void calculateCodeMetrics(int length)
{
	bitsPerLetter = int(log2(characterSet.size() - 1)) + 1;
	charMask = (1 << bitsPerLetter) - 1;
	blackMask = 1;
	for (int i = 1; i < length; ++i)
	{
		blackMask = (blackMask << bitsPerLetter) + 1;
	}
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
		std::cout << intToString(bestGuess.code,length) << std::endl;
		guesses++;
		int black;
		while (true)
		{
			std::cout << "black:";
			if (readFromCin(black) && black <= length && black >= 0);
			break;
		}
		if (black == length) {
			break;
		}
		int white;
		while (true)
		{
			std::cout << "white:";
			if (readFromCin(white) && black + white <= length && white >= 0);
			break;
		}
		std::cout << "OK... thinking ..." << std::endl;
		Result r;
		r.b = black;
		r.w = white;
		updatePossiblecodes(r, bestGuess);
		if (possibleCodes.size() == 1)
		{
			bestGuess = possibleCodes.at(0);
		}
		else if (possibleCodes.empty()) 
		{
			std::cout << "You've lied to me." << std::endl;
			break;
		}
		else
		{
			calculateBestGuess(length, set);
		}
	}
	std::cout << "I took " << guesses << " round" << ((guesses == 1) ? "." : "s.") << std::endl;

	readFromCin(guesses);

}

bool validateGuess(std::string guess, int length)
{
	if (guess.size() != length)
		return false;
	for (char c : guess)
	{
		if (characterSet.find(c) == std::string::npos)
			return false;
	}
	return true;
}

void humanGuessCode(int length, std::string set)
{
	Code code(generateCode(length, set),length);
	Result r;
	int guesses = 0;
	std::cout << "What is your first guess?" << std::endl;
	do {
		std::string guessString;
		while (true)
		{
			readFromCin(guessString);
			if (validateGuess(guessString, length))
			{
				break;
			}
			else
			{
				std::cout << "Your guess must consist of " << length << " character"<<((length>1)?"s":"")<<" from the set {" << characterSet << "}." << std::endl;
			}	
		}
		Code guess(guessString,length);
		r = getGuessResult(code.code, code.counts, guess.code, guess.counts);
		guesses++;
		std::cout << guessString <<"\tblack:" << r.b << "\twhite:" << r.w << std::endl;
	} while (r.b < length);
	std::cout << "congratualtions that is the code!" << std::endl;
	std::cout << "It took you " << guesses << " round" << ((guesses == 1) ? "." : "s.") << std::endl;
	std::cin >> set;
}

int main()
{
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
	calculateCodeMetrics(codeLength);
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

