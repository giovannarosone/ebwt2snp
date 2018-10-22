# ebwt2snp

### Overview

The *ebwt2snp* suite can be used to discover SNPs/indels between two sets of reads (fasta/fastq) *without* aligning them to  a reference genome (alignment-free, reference-free) with just a scan of the extended Burrows Wheeler Transform (e-BWT) of the sets of reads and the LCP and gSA arrays. The output is a fasta file (in KisSNP2 format) where sequences are the contexts surrounding the identified SNPs.  The suite therefore finds its main use in applications where no reference genome is known (alignment-free, reference-free variation discovery). The following modules are available:

- **ebwt2clust** partitions the eBWT of a set of reads in clusters corresponding to the same nucleotide in the reference genome. Output: a ".clusters" file.
- **clust2snp** analyzes the clusters produced by ebwt2clust and detects SNPs and indels. Output: a ".snp" file (this is actually a fasta file in KisSNP++ format containing pairs of reads testifying the variations).

We call **ebwt2snp** the pipeline **ebwt2clust -> clust2snp**. Note: **ebwt2clust** and **clust2snp** require the Enhanced Generalized Suffix Array (EGSA) of the sets of reads (https://github.com/felipelouza/egsa or https://github.com/giovannarosone/BCR_LCP_GSA) to be built beforehand. Note also that the **ebwt2snp** pipeline finds many SNPs/indels twice: one time on the forward strand and one on the reverse complement strand.

If a ground-truth VCF file (of first against second individual) and the reference of the first individual are available, one can validate the .snp file generated by **clust2snp** using the executable **snp_vs_vcf** (this works on any file in KisSNP++ format).

If a reference (of the first individual) is available, one can extend the above pipeline to produce a vcf file. For this, one can use the tools

- **snp2fastq** converts the ".snp" file produced by the **ebwt2snp** pipeline into a ".fastq" file (with fake base qualities) ready to be aligned (e.g. using BWA-MEM) against the reference of the fist individual.
- **sam2vcf** converts the ".sam" file produced by aligning the above ".fastq" into a ".vcf" file containing the variations. 

We call **snp2vcf** the pipeline **snp2fastq -> bwa-mem -> sam2vcf**. Note that bwa-mem requires the reference of the first individual to be available. This reference can be computed, for example, using a standard bwa-mem+{sam,bcf,vcf}tools pipeline. 

To conclude, one can validate the VCF against a ground-truth VCF generated using a standard pipeline (for example, bwa-mem + {sam,bcf,vcf}tools) by using the tool **vcf_vs_vcf**. This is equivalent to validating the .snp file against a ground-truth VCF using **snp_vs_vcf**. The script **pipeline.sh** automates the whole pipeline (read below). 

The paper describing the theory behind the tool (eBWT positional clustering) has been published in:

---

*Nicola Prezza, Nadia Pisanti, Marinella Sciortino and Giovanna Rosone: Detecting mutations by eBWT. WABI 2018. Leibniz International Proceedings in Informatics, LIPIcs , 2018, 113, art. no. 3, Schloss Dagstuhl--Leibniz-Zentrum für Informatik.*

---

A pre-print version can be found here: https://arxiv.org/abs/1805.01876. 


### Install

~~~~
#download ebwt2snp, EGSA, and BCR
git clone https://github.com/nicolaprezza/ebwt2snp
git clone https://github.com/felipelouza/egsa
git clone https://github.com/giovannarosone/BCR\_LCP\_GSA

#build ebwt2snp
cd ebwt2snp
mkdir build
cd build
cmake ..
make

#build egsa
cd ../../egsa
make compile BWT=1
~~~~

### Run

The simplest option is to use the automated pipeline **pipeline.sh**, which takes as input two fastq files and a reference (any universal reference for the two individual), runs the  **ebwt2snp** and **snp2vcf** pipelines, computes the ground-truth SNPs using a standard pipeline (bwa-mem + {sam,bcf,vcf}tools), and computes the precision/sensitivity of **ebwt2snp** against the ground truth.  Read the file **pipeline.sh** for more information. 


Alternatively, if you wish to run our tools by your own, proceed as follows. Enter the folder with the two fasta files _reads1.fasta_  and _reads2.fasta_ (i.e. the reads of the two samples). We assume that executables 'egsa', 'ebwt2clust', and 'clust2snp' are global. 

~~~~
#Step 1: optional, but considerably increases sensitivity of the tool. Insert in reads1.fasta also the reverse-complement of the reads. Repeat with reads2.fasta

#Count reads in the first sample and concatenate fasta files
nreads1=`grep ">" reads1.fasta | wc -l`
cat reads1.fasta reads2.fasta > ALL.fasta

#Build the EGSA of the sets of reads
egsa ALL.fasta 0

#Build cluster file (do this in the same folder containing all other files)
ebwt2clust -i ALL.fasta -x 4 -y 4 -z 4

#Call SNPs (do this in the same folder containing all other files)
clust2snp -i ALL.fasta -n ${nreads1}  -x 4 -y 4 -z 4

#File ALL.snp.fasta now contains identified SNPs/indels. Note: the third field between "|" in the read-names of this file indicates the number of times the variant is observed (maximum value specified with option -c in clust2snp). You can further filter this file according to this field in order to improve accuracy.

~~~~
