// Copyright (c) 2018, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

#include <iostream>
#include <fstream>
#include <assert.h>
#include <vector>
#include <unistd.h>
#include <sstream>
#include <set>
#include <cstring>
#include "include.hpp"
#include <algorithm>

using namespace std;

int indel_window_def = 10;//if 2 indels are within this number of bases, keep only one of the two.
int indel_window = 0;

bool non_isolated = true;

void help(){

	cout << "compareVCF [OPTIONS]" << endl << endl <<
	"Converts the aligned calls (with bwa-mem) 'calls.sam' of clust2snp into a vcf file 'calls.sam.vcf'." << endl <<
	"Options:" << endl <<
		"-h          Print this help." << endl <<
		"-1 <arg>    Input VCF file to be validated. REQUIRED" << endl <<
		"-2 <arg>    Ground truth VCF. REQUIRED" << endl <<
		"-d <arg>    Indel window tolerance. Consider a true match if 2 indels are within <arg> bases (default = " <<  indel_window_def << ")" << endl;
	exit(0);
}

struct vcf_entry{

	string chr;
	uint64_t pos;

	string REF;
	string ALT;

	bool indel;

	bool operator<(const vcf_entry & a) const{

		if(chr.compare(a.chr) < 0 ) return true;
		else if(chr.compare(a.chr) == 0 ){

			return pos < a.pos;

		}else return false;

	}

	bool operator==(const vcf_entry & a) const{

		return (chr.compare(a.chr)== 0) and pos == a.pos and REF.compare(a.REF) == 0 and ALT.compare(a.ALT) == 0;

	}

};


set<vcf_entry> read_vcf(string path){


	set<vcf_entry> vcf;

	ifstream is(path);

	string line;

	while(getline(is, line)){

		if(line[0] != '#'){

			std::istringstream iss(line);

			string chr;
			string pos_s;
			string id;
			string ref;
			string alt;

			getline(iss, chr, '\t');
			getline(iss, pos_s, '\t');
			getline(iss, id, '\t');
			getline(iss, ref, '\t');
			getline(iss, alt, '\t');

			uint64_t pos = atoi(pos_s.c_str());

			vcf.insert(

				{

					chr,
					pos,
					ref,
					alt,
					(ref.length()>1 or alt.length()>1)

				}

			);

		}

	}

	is.close();

	return vcf;

}


int main(int argc, char** argv){

	if(argc < 2) help();

	string vcf1_path;
	string vcf2_path;

	int opt;
	while ((opt = getopt(argc, argv, "d:1:2:h")) != -1){
		switch (opt){
			case 'h':
				help();
			break;
			case 'd':
				indel_window = atoi(optarg);
			break;
			case '1':
				vcf1_path = string(optarg);
			break;
			case '2':
				vcf2_path = string(optarg);
			break;
			default:
				help();
			return -1;
		}
	}

	indel_window = indel_window==0 ? indel_window_def : indel_window;

	if(vcf1_path.compare("")==0) help();
	if(vcf2_path.compare("")==0) help();

	auto vcf1 = read_vcf(vcf1_path);
	auto vcf2 = read_vcf(vcf2_path);

	/*for(auto v:vcf2){

		cout << v.chr << "\t" << v.pos << "\t" << v.REF << "\t" << v.ALT << endl;

	}*/

	uint64_t TP_s = 0;
	uint64_t FP_s = 0;
	uint64_t FN_s = 0;

	uint64_t TP_i = 0;
	uint64_t FP_i = 0;
	uint64_t FN_i = 0;

	for(auto v:vcf1){

		if(v.indel){

			auto itL = vcf2.lower_bound(v);
			auto itU = vcf2.upper_bound(v);

			//find matching indel (before or after

			auto it = vcf2.end();

			if(	itL != vcf2.end() &&
				itL->indel and
				itL->chr.compare(v.chr)==0 and
				std::abs( int(itL->pos)-int(v.pos) ) <= indel_window ){

				it = itL;

			}

			if(	itU != vcf2.end() &&
				itU->indel and
				itU->chr.compare(v.chr)==0 and
				std::abs( int(itU->pos)-int(v.pos) )  <= indel_window  ){

				it = itU;

			}

			if(it != vcf2.end()){//found

				TP_i++;
				vcf2.erase(it);

			}else{//not found

				FP_i++;

			}

		}else{

			auto it = vcf2.find(v);

			if(it != vcf2.end()){//found

				TP_s++;
				vcf2.erase(it);

			}else{//not found

				FP_s++;

			}

		}

	}

	for(auto v:vcf2){

		FN_i += v.indel;
		FN_s += (not v.indel);

	}

	cout << "TP (SNP) = " << TP_s << endl;
	cout << "FP (SNP) = " << FP_s << endl;
	cout << "FN (SNP) = " << FN_s << endl << endl;

	cout << "TP (INDEL) = " << TP_i << endl;
	cout << "FP (INDEL) = " << FP_i << endl;
	cout << "FN (INDEL) = " << FN_i << endl << endl;

	cout << "TP (TOT) = " << TP_i+TP_s << endl;
	cout << "FP (TOT) = " << FP_i+FP_s << endl;
	cout << "FN (TOT) = " << FN_i+FN_s << endl << endl;


	cout << "sensitivity SNP = TP/(TP+FN) = " << 100*double(TP_s)/(double(TP_s)+double(FN_s)) << "%" << endl;
	cout << "precision   SNP = TP/(TP+FP) = " << 100*double(TP_s)/(double(TP_s)+double(FP_s)) << "%" << endl << endl;

	cout << "sensitivity INDEL = TP/(TP+FN) = " << 100*double(TP_i)/(double(TP_i)+double(FN_i)) << "%" << endl;
	cout << "precision   INDEL = TP/(TP+FP) = " << 100*double(TP_i)/(double(TP_i)+double(FP_i)) << "%" << endl << endl;

	cout << "sensitivity TOT = TP/(TP+FN) = " << 100*double(TP_s+TP_i)/(double(TP_s+TP_i)+double(FN_s+FN_i)) << "%" << endl;
	cout << "precision   TOT = TP/(TP+FP) = " << 100*double(TP_s+TP_i)/(double(TP_s+TP_i)+double(FP_s+FP_i)) << "%" << endl << endl;


}
