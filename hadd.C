/*
> hadd.C(TString detector)
- Used to combine the evaluated root files while running jobs in batches
- Arguments
  # detector - CEMC, EEMC, FEMC, FHCAL, HCALIN, HCALOUT
- Output file - merged_Eval_<detector>.root
*/

/*
Authors - 	Sagar Joshi (ee190002054@iiti.ac.in)
		Siddhant Rathi (me190003061@iiti.ac.in)

v2.0
*/

#include <string.h>
#include "TChain.h"
#include "TFile.h"
#include "TH1.h"
#include "TTree.h"
#include "TKey.h"
#include "Riostream.h"
#include <iostream> 
#include <dirent.h>
#include <errno.h>

TList *FileList;
TFile *Target;
 
void MergeRootfile( TDirectory *target, TList *sourcelist );
 
 
void hadd(TString detector) {
  // Prepare the files to me merged

  //std::cout<<"Please enter the name of the detector (all caps): ";
  //std::cin>>detector;

  int nCopies = successfulJobs; 

  if(gSystem->AccessPathName("EvalFiles/Eval_" + detector + "_0.root")) {
    for(int i = 1; i < nCopies; i++){
      gSystem->CopyFile("hsimple.root", "EvalFiles/Eval_" + detector + "_" + TString::Itoa(i,10) + ".root");
    }
  }
 
  // in an interactive ROOT session, edit the file names
  // Target and FileList, then
  // root > .L hadd.C
  // root > hadd()
 
  Target = TFile::Open("merged_Eval_" + detector + ".root", "RECREATE" ); 
  FileList = new TList();
  
  for(int i = 0; i < nCopies; i++){
    FileList->Add( TFile::Open("EvalFiles/Eval_" + detector + "_" + TString::Itoa(i,10) + ".root"));
  }
      

  MergeRootfile( Target, FileList );
 
}
 
void MergeRootfile( TDirectory *target, TList *sourcelist ) {
 
  //  cout << "Target path: " << target->GetPath() << endl;
  TString path( (char*)strstr( target->GetPath(), ":" ) );
  path.Remove( 0, 2 );
 
  TFile *first_source = (TFile*)sourcelist->First();
  first_source->cd( path );
  TDirectory *current_sourcedir = gDirectory;
  //gain time, do not add the objects in the list in memory
  Bool_t status = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
 
  // loop over all keys in this directory
  TChain *globChain = 0;
  TIter nextkey( current_sourcedir->GetListOfKeys() );
  TKey *key, *oldkey=0;
  while ( (key = (TKey*)nextkey())) {
 
    //keep only the highest cycle number for each key
    if (oldkey && !strcmp(oldkey->GetName(),key->GetName())) continue;
 
    // read object from first source file
    first_source->cd( path );
    TObject *obj = key->ReadObj();
 
    if ( obj->IsA()->InheritsFrom( TH1::Class() ) ) {
      // descendant of TH1 -> merge it
 
      //      cout << "Merging histogram " << obj->GetName() << endl;
      TH1 *h1 = (TH1*)obj;
 
      // loop over all source files and add the content of the
      // correspondant histogram to the one pointed to by "h1"
      TFile *nextsource = (TFile*)sourcelist->After( first_source );
      while ( nextsource ) {
 
	// make sure we are at the correct directory level by cd'ing to path
	nextsource->cd( path );
	TKey *key2 = (TKey*)gDirectory->GetListOfKeys()->FindObject(h1->GetName());
	if (key2) {
	  TH1 *h2 = (TH1*)key2->ReadObj();
	  h1->Add( h2 );
	  delete h2;
	}
 
	nextsource = (TFile*)sourcelist->After( nextsource );
      }
    }
    else if ( obj->IsA()->InheritsFrom( TTree::Class() ) ) {
 
      // loop over all source files create a chain of Trees "globChain"
      const char* obj_name= obj->GetName();
 
      globChain = new TChain(obj_name);
      globChain->SetMaxTreeSize(200000000000LL); // set size to ~100 GB
      globChain->Add(first_source->GetName());
      TFile *nextsource = (TFile*)sourcelist->After( first_source );
      //      const char* file_name = nextsource->GetName();
      // cout << "file name  " << file_name << endl;
      while ( nextsource ) {
 
	globChain->Add(nextsource->GetName());
	nextsource = (TFile*)sourcelist->After( nextsource );
      }
 
    } else if ( obj->IsA()->InheritsFrom( TDirectory::Class() ) ) {
      // it's a subdirectory
 
      cout << "Found subdirectory " << obj->GetName() << endl;
 
      // create a new subdir of same name and title in the target file
      target->cd();
      TDirectory *newdir = target->mkdir( obj->GetName(), obj->GetTitle() );
 
      // newdir is now the starting point of another round of merging
      // newdir still knows its depth within the target file via
      // GetPath(), so we can still figure out where we are in the recursion
      MergeRootfile( newdir, sourcelist );
 
    } else {
 
      // object is of no type that we know or can handle
      cout << "Unknown object type, name: "
	   << obj->GetName() << " title: " << obj->GetTitle() << endl;
    }
 
    // now write the merged histogram (which is "in" obj) to the target file
    // note that this will just store obj in the current directory level,
    // which is not persistent until the complete directory itself is stored
    // by "target->Write()" below
    if ( obj ) {
      target->cd();
 
      //!!if the object is a tree, it is stored in globChain...
      if(obj->IsA()->InheritsFrom( TTree::Class() ))
	globChain->Merge(target->GetFile(),0,"keep");
      else
	obj->Write( key->GetName() );
    }
 
  } // while ( ( TKey *key = (TKey*)nextkey() ) )
 
  // save modifications to target file
  target->SaveSelf(kTRUE);
  TH1::AddDirectory(status);
}
