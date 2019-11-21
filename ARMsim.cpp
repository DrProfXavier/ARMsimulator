#include <iostream>
#include <istream>
#include <fstream>
#include <string>
#include <array>
#include <queue>

void printRegisters(std::array<int, 32> regies) //will print 32 registers, formatted
{
	std::cout << "\n\nRegisters\n";
	std::cout << "X00:";
	for (int r = 0; r < 8; r++)
	{
		std::cout << "\t" << regies.at(r);
	}
	std::cout << "\nX08:";
	for (int r = 8; r < 16; r++)
	{
		std::cout << "\t" << regies.at(r);
	}
	std::cout << "\nX16:";
	for (int r = 16; r < 24; r++)
	{
		std::cout << "\t" << regies.at(r);
	}
	std::cout << "\nX24:";
	for (int r = 24; r < 32; r++)
	{
		std::cout << "\t" << regies.at(r);
	}
	std::cout << "\n\n";
}

void printDataTable(std::vector<int> datum, int start, int last)
{
	int startAddr = start;
	int lastAddr = last;
	int elements = (last - start) / 4;
	std::cout << "Data";
	for (int block = 0; block <= elements; block++)
	{
		if ((block == 0) || (block) % 8 == 0)
		{
			std::cout << "\n" << start + (block * 4) << ":";
		}
		std::cout << "\t" << datum[block];
	}
	std::cout << std::endl;
}

int twosComp(std::string binary)
{
	int i = -1;
	if (binary.at(0) == '0')
	{
		i = std::stoi(binary, nullptr, 2);
		return i;
		//std::cout << " Data: " << i;
	}
	else //It was a leading 1, so flip em and add 1
	{
		std::string complement = "";
		for (int b = 0; b < binary.size(); b++)
		{
			if (binary.at(b) == '1') //1->0
			{
				binary.replace(b, 1, "0");
			}
			else //0->1
			{
				binary.replace(b, 1, "1");
			}
		}
		i = std::stoi(binary, nullptr, 2);
		i = -1 * (i + 1);
	}
	return i;
}

struct instruction
	{
		//Hopefully no harm in zero, empty strings cause errors in ::compile's declarations
		int address = 0;
		std::string cat = "0";
		std::string opCode = "0";
		std::string dest = "0";
		std::string src1 = "0";
		std::string src2 = "0";
		std::string srcDest = "0";
		std::string imdtVal = "0";
		std::string offSet = "0";

		//Classify opcodes at construction
		instruction(std::string instr, int addr)
		{
			this->cat = instr.substr(0, 3);
			this->address = addr;

			if (this->cat == "001") //If category 1 (CBZ/CBNZ): opCode = 5 bits, Src1 = 5 bits, Branch Offset = 19 bits
			{
				this->opCode = instr.substr(3, 5);
				this->src1 = instr.substr(8, 5);
				this->offSet = instr.substr(13, 19);
			}
			else if (this->cat == "010") //If cat 2 (ADDI, SUBI,ANDI,ORRI,EORI): op=7b, dest=5b, src1=5b, immVal = 12b
			{
				this->opCode = instr.substr(3, 7);
				this->dest = instr.substr(10, 5);
				this->src1 = instr.substr(15, 5);
				this->imdtVal = instr.substr(20, 12);
			}
			else if (this->cat == "011") //If cat 3 (EOR, ADD, SUB, AND, ORR, LSR, LSL): op=8b, dest=5b, src1=5b, src2=5b,000000
			{
				this->opCode = instr.substr(3, 8);
				this->dest = instr.substr(11, 5);
				this->src1 = instr.substr(16, 5);
				this->src2 = instr.substr(21, 5);
				//Last 6 bits are 000000/Don't care
			}
			else if (this->cat == "100") //If cat 4 (LDUR, STUR): op=8bits, srcDest=5b, src1=5b, immVal=11b
			{
				this->opCode = instr.substr(3, 8);
				this->srcDest = instr.substr(11, 5);
				this->src1 = instr.substr(16, 5);
				this->imdtVal = instr.substr(21, 11);
			}
			else if (this->cat == "101") //It's dummy
			{
				this->imdtVal = "DUMMY";
			}
			//set imdt_Data to 2's complement in each cataegory so we don't do it on dummy

		}
		
	};

//Function for actual computation
void compile(std::vector<instruction>& instVect, std::array<int, 32> regArray, std::vector<int>& datattable, int dataStart, int lastAddr)
{
	int cycle = 1;
	int cat = 0;

	std::vector<instruction>::iterator vIter;
	for (vIter = instVect.begin(); vIter != instVect.end(); ++vIter)
	{
		std::string opCode = vIter->opCode;
		std::string tag = "test";

		int address = vIter->address;
		int cat = std::stoi(vIter->cat, nullptr, 2);
		int dest = std::stoi(vIter->dest, nullptr, 2);
		int src1 = std::stoi(vIter->src1, nullptr, 2);
		int src2 = std::stoi(vIter->src2, nullptr, 2);
		int srcDest = std::stoi(vIter->srcDest, nullptr, 2);

		int imdtVal = -1;
		if (cat != 5)
		{
			imdtVal = std::stoi(vIter->imdtVal, nullptr, 2); //need to convert if neg 2's comp
		}
		int offSet = twosComp(vIter->offSet);	// \->Done here

		int destIndex = (dest - dataStart) / 4;
		int src1Index = (src1 - dataStart) / 4;
		int src2Index = (src2 - dataStart) / 4;
		int src1Index_offset = ((regArray[src1] + imdtVal) - dataStart) / 4; //Consider  compVal instead of imdtVal for operaions that do this
		int sturIndex_offset = ((regArray[src1] + imdtVal) - dataStart) / 4;
		int bitsToShift = src2;

		//Branch index, most complicated
		int branchIndex = (((address + (offSet * 4)) - 64) / 4) - 1 ;//(address - 64) / 4; //64 is given starting constant, index in instVect

		std::cout << "--------------------\nCycle " << cycle << ":\t" << address;
		switch (cat)
		{
		case 1: //Cat 1...CBZ,CBNZ
			if (opCode == "10000") //CBZ, is branch (reg[src1Index] == 0?)
			{
				//tag = "CBZ X" << ;
				if (regArray[src1] == 0) //If == 0 Changed from [src1Index]
				{   
					vIter = instVect.begin() + branchIndex; //Hmm, specific vect access				
				} //If not == 0 do nothing
				std::cout << "\t CBZ X" << src1 << ", #" << offSet;
			}
			else // "10001", CBNZ
			{
				//tag = "CBNZ";
				std::cout << "\t CBNZ X" << src1 << ", #" << offSet;
				if (regArray[src1] != 0) //If == 0 //Changed from [src1Index]
				{
					//branch index = addr 
					vIter = instVect.begin() + branchIndex; //Hmm, specific vect access				
				}
			}//If == 0, do nothing
			break;

		case 2: //CAT 2 ORRI,EOR, ADDI, SUBI, AND1
			if (opCode == "1000000") //ORRI
			{

				regArray[destIndex] = regArray[src1Index] | imdtVal;
				std::cout << "\t ORRI X" << dest << ", X" << src1 << ", #" << imdtVal;
			}
			if (opCode == "1000001") //EORI
			{
				tag = "EORI";
				regArray[destIndex] = /*Item in register[] OR */regArray[src1Index] ^ imdtVal;
				std::cout << "\t EORI X" << dest << ", X" << src1 << ", #" << imdtVal;
			}
			if (opCode == "1000010") //ADDI
			{
				tag = "ADDI";
				regArray[dest] = /*Item in register[] OR */regArray[src1] + imdtVal;
				std::cout << "\t ADDI X" << dest << ", X" << src1 << ", #" << imdtVal;
			}
			if (opCode == "1000011") //SUBI
			{
				tag = "SUBI";
				regArray[dest] = /*Item in register[] OR */regArray[src1] - imdtVal;
				std::cout << "\t SUBI X" << dest << ", X" << src1 << ", #" << imdtVal;
			}
			if (opCode == "1000100") //ANDI
			{
				tag = "ANDI";
				regArray[dest] = /*Item in register[] OR */regArray[src1] & imdtVal;
				std::cout << "\t ANDI X" << dest << ", X" << src1 << ", #" << imdtVal;
			}
			break;

		case 3: //Cat 3
			
			if (opCode == "10100000") //EOR
			{
				tag = "EOR";
				regArray[dest] = regArray[src1] ^ regArray[src2];
				std::cout << "\t EOR X" << dest << ", X" << src1 << ", X" << src2;
			}
			if (opCode == "10100010") //ADD
			{
				tag = "ADD";
				regArray[dest] = regArray[src1] + regArray[src2];
				std::cout << "\t ADD X" << dest << ", X" << src1 << ", X" << src2;
			}
			if (opCode == "10100011") //SUB
			{
				tag = "SUB";
				regArray[dest] = regArray[src1] - regArray[src2];
				std::cout << "\t SUB X" << dest << ", X" << src1 << ", X" << src2;
			}
			if (opCode == "10100100") //AND
			{
				tag = "AND";
				regArray[dest] = regArray[src1] & regArray[src2];
				std::cout << "\t AND X" << dest << ", X" << src1 << ", X" << src2;
			}
			if (opCode == "10100101") //ORR
			{
				tag = "ORR";
				regArray[dest] = regArray[src1] | regArray[src2];
				std::cout << "\t ORR X" << dest << ", X" << src1 << ", X" << src2;
			}
			if (opCode == "10100110") //LSR, regArray[src2] determines shift amount=> which is found in src2 reg
			{
				tag = "LSR";
				//regArray[destIndex];
				//regArray[destIndex] = regArray[src1Index] << regArray[src2Index]; 
				regArray[dest] = regArray[src1] >> regArray[src2];
				std::cout << "\t LSR X" << dest << ", X" << src1 << ", X" << src2;
			}
			if (opCode == "10100111") //LSL
			{
				tag = "LSL";
				//regArray[destIndex];
				//regArray[destIndex] = regArray[src1Index] >> regArray[src2Index];
				regArray[dest] = regArray[src1] << regArray[src2];
				std::cout << "\t LSL X" << dest << ", X" << src1 << ", X" << src2;
			}
			break;

		case 4: //Cat 4 LDUR and STUR

			if (opCode == "10101010") //LDUR
			{
				tag = "LDUR";
				//regArray[srcDest] = regArray[src1Index_offset];
				regArray[srcDest] = datattable.at(src1Index_offset);
				std::cout << "\t LDUR X" << srcDest << ", [X" << src1 << ", #" << imdtVal << "]";
			}
			if (opCode == "10101011") //STUR
			{
				tag = "STUR";
				//regArray[sturIndex_offset] = regArray[srcDest];
				datattable.at(sturIndex_offset) = regArray[srcDest];
				std::cout << "\t STUR X" << srcDest << ", [X" << src1 << ", #" << imdtVal << "]";
			}
			break;

		case 5: //Cat 5
			std::cout << "\tDUMMY";
			break;

		default:
			std::cout << " Ya wanked up the switch";
			break;
		}
		printRegisters(regArray);
		printDataTable(datattable, dataStart, lastAddr);
		cycle++;
	}
}

void arm_sim(const std::string& file_name) //takes in file, reads lines, should create inst structures and push_back to vector
{
	//Workflow Vars.
	bool dummyFound = false;
	int line = 0;
	int addr = 64;
	int dataStart = 0;
	int finalAddr = -1;
	int cycle = 1;

	//Data Structs
	std::vector<instruction> instVect;
	std::vector<int> dataTable;
	std::array<int, 32> regArray = { 0 };


	//Reads file
	std::ifstream inFile;
	inFile.open(file_name);
	std::string inStruct;

	//Read file, creat instructions, place in approp. vectors
	if (inFile)
	{
		while (std::getline(inFile, inStruct))
		{
			//Lines each appear and exist as opCode
			//std::cout << "\nline" << line << ": " << inStruct << " Cat: " << inStruct.substr(0, 3); //Debug

			if (!dummyFound && inStruct.substr(0, 3) != "101")//Operation
			{
				//Try making INstruciton object, and push_back to vector
				instVect.push_back(instruction(inStruct, addr)); //uses constructor to make and pushback object in one step
				//std::cout << "\tOpCode: " << instVect[line].opCode << "\tAddr$: " << instVect[line].address; //Debug
			}

			if (dummyFound) //manipulate dataTable here, start pushing to dataTable vect.
			{

				int i = -1;

				if (inStruct.at(0) == '0')
				{
					i = std::stoi(inStruct, nullptr, 2);
					//std::cout << " Data: " << i;
				}
				else //It was a leading 1, so flip em and add 1
				{
					std::string complement = "";
					for (int b = 0; b < 32; b++)
					{
						if (inStruct.at(b) == '1') //1->0
						{
							inStruct.replace(b, 1, "0");
						}
						else //0->1
						{
							inStruct.replace(b, 1, "1");
						}
					}
					i = std::stoi(inStruct, nullptr, 2);
					i = -1 * (i + 1);
				}
				//std::cout << " Data# at addr: " << addr << " = " << i;
				dataTable.push_back(i);
			}

			if (!dummyFound && inStruct.substr(0, 3) == "101")//Dummy
			{
				instVect.push_back(instruction(inStruct, addr));
				dummyFound = true;
				dataStart = addr + 4;
				//std::cout << " Dummy?" << dummyFound;

			}

			line++;
			addr += 4;
		}
		finalAddr = addr - 4;
		//Debug
		//std::cout << "\nData start: " << dataStart << "\tFinal addr$: " << finalAddr;
	}

	compile(instVect, regArray, dataTable, dataStart, finalAddr);
}


int main(int argc, char** argv)
{
	std::string binFile = argv[1];
	arm_sim(binFile);
	//arm_sim("sample.txt");
}