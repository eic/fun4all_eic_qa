/*
> LoopEvalFR.C(int print = 1, int debug = 0, Double_t energyCutAggregate = 0.1, Double_t energyCut = 0.0, int MIP_theta_parametrisation = 1)
- Creates the analysis plots for the combined forward calorimeters FEMC+FHCAL (default parameters for pions)
- Processing - Eta Cuts, Manual Clustering (or elliptical cuts based on difference between generated and detected azimuth and polar angle), Recalibration, Tower energy cuts on FEMC alone and FEMC+FHCAL as well, and polar angle based energy cuts to eliminate MIPs    
- Arguments
  # print - saves plots as .png files if not 0
  # debug - prints debugging messages if not 0; Pipe the output to a file while using this
  # energyCutAggregate - specify the value for the tower energy cut on FEMC+FHCAL
  # energyCut - specify the value for the tower energy cut on individual towers
  # MIP_theta_parametrization - applies a polar angle dependent energy cut on individual towers of FEMC to eliminate MIPs, based on the parametric equation provided in the code
- Output file - energy_verification_EtaCut_CircularCut_FEMC_FHCAL.root, and the .png plots if generated
*/

/*
  Authors -  	Siddhant Rathi   (me190003061@iiti.ac.in)
   		Sagar Joshi      (ee190002054@iiti.ac.in)
           
	    
  version - 2

*/

#include <iostream>
#include <stdexcept>
#include <eicqa_modules/EvalRootTTree.h>
#include <eicqa_modules/EvalHit.h>
#include "TMath.h"
#include "TStyle.h"
#include <unistd.h>

R__LOAD_LIBRARY(libeicqa_modules.so)

void LoopEvalFR(int print = 1, int debug = 0, Double_t energyCutAggregate = 0.1, Double_t energyCut = 0.0, int MIP_theta_parametrisation = 1)
{

  Double_t EMC_cut = 0.0;
  //TF1 *mip_pmzn_energy_cut_ftheta = new TF1("mip_pmzn_energy_cut_ftheta", "(2.49951e-02) + (2.15109e-01)*x - (1.15673)*(x^2) + (2.75562)*(x^3) - (2.31809)*(x^4)"); 
  TF1 *mip_pmzn_energy_cut_ftheta = new TF1("mip_pmzn_energy_cut_ftheta", "0"); 

  if(MIP_theta_parametrisation == 1 && energyCut != 0){
    throw std::invalid_argument("We do not currently support theta-parametrized \nMIP cut on EMC simultaneously with individual tower cuts \non other detectors.:(;");
  } 

  //variable binning code
  int arraySizeBins = 0;           //size of the bins array
  Double_t arraySizeBinsLoop = 0;  //loop variable
  int lowThresholdBins = 2;        //number of bins below the threshold bin
  int highThresholdBins = 9;      //number of bins above the threshold bin
  Double_t maxEnergyBin = 30;      //highest energy
  Double_t lowEnergyBin = 3;       //threshold energy 

  //calculating the number of bins required
  while(arraySizeBinsLoop < maxEnergyBin){
    arraySizeBins += 1;
    if(arraySizeBinsLoop < lowEnergyBin){
      arraySizeBinsLoop += lowEnergyBin/lowThresholdBins;
    }
    else{
      arraySizeBinsLoop += (maxEnergyBin - lowEnergyBin)/highThresholdBins;
    }
  }

  Double_t binLimitArray[arraySizeBins + 1];

  //filing the array containing bin lower limits
  for(int i = 0; i <= arraySizeBins; i++){
    if(i <= lowThresholdBins){
      binLimitArray[i] = lowEnergyBin*i/lowThresholdBins;
    }
    else{
      binLimitArray[i] = lowEnergyBin  + ((maxEnergyBin - lowEnergyBin)*(i - lowThresholdBins)/highThresholdBins);
    }
  }

  //print array
  for(int i = 0; i < arraySizeBins; i++){
    cout<<"element "<<i<<" of the bin array is "<<binLimitArray[i]<<" \n";
  }
  
  //variable binning code end

  TString detector = "FEMC_FHCAL";
  TFile *f1 = new TFile("merged_Eval_FHCAL.root","READ"); 
  TFile *f2 = new TFile("merged_Eval_FEMC.root","READ"); 
  //TFile *f1 = new TFile("../../../simrankaur/Fun4All-Calorimeter-Macros_copy/final/Pion_fhcalcorrected/Eval_FHCAL.root","READ");
  //TFile *f2 = new TFile("../../../simrankaur/Fun4All-Calorimeter-Macros_copy/final/Pion_fhcalcorrected/Eval_FEMC.root","READ");
  //TFile *f1 = new TFile("../../July21/pi_30_2gev/merged_Eval_FHCAL.root","READ"); 
  //TFile *f2 = new TFile("../../July21/pi_30_2gev/merged_Eval_FEMC.root","READ"); 

  TTree* T1 = (TTree*)f1->Get("T");
  EvalRootTTree *evaltree1 = nullptr;
  
  TTree* T2 = (TTree*)f2->Get("T");
  EvalRootTTree *evaltree2 = nullptr;
  
  gStyle->SetCanvasPreferGL(kTRUE); 
  TCanvas *c = new TCanvas();
  c->SetTickx();
  c->SetTicky();

  // Modifying default plotting style  
  gStyle->SetOptTitle(0);
  gStyle->SetOptFit(102);
  gStyle->SetTitleXOffset(1);
  gStyle->SetTitleYOffset(1);
  gStyle->SetLabelSize(0.05);  
  gStyle->SetTitleXSize(0.05);  
  gStyle->SetTitleYSize(0.05);

  long double total_te = 0; 
  long double total_te_CircularCut = 0;
  long double total_ge = 0; 

  int nSlicesx = arraySizeBins; // Number of ge-axis slices taken for making sigma_e vs ge plot
  int nSlicesy = 350;
  Double_t eta_min, eta_max; // Eta range of detectors
  Double_t x_radius_FHCAL = 0.15; //0.15; Length of semi-minor axis of circular (elliptical) in FHCAL
  Double_t y_radius_FHCAL = 0.45; //0.45; Length of semi-major axis of circular (elliptical) in FHCAL
  Double_t x_radius_FEMC = 0.13; //0.13; Length of semi-minor axis of circular (elliptical) in FEMC
  Double_t y_radius_FEMC = 0.35; //0.35; Length of semi-major axis of circular (elliptical) in FEMC
  Double_t fit_min, fit_max; // Fit range of the first slice of [(te-ge)/ge vs ge] plot
  Double_t sigma_min, sigma_max; // Range of Y-axis in sigma_e vs ge plot
  Double_t mean_min, mean_max; // Range of Y-axis in mean_e vs ge plot
  Double_t chi2_min, chi2_max; // Range of Y-axis in chi2_e vs ge plot
  Double_t recalibration_factor; // Number divided from entries of [(te-ge)/ge vs ge] plot for recalibration
  Double_t recalibration_firstSlice = 20.746;
  Double_t recalibration_firstSlice_FHCAL = 0.2079;                             // Recalibration factor of first slice (needed to be done manually because of low statistics)
  Double_t recalibration_firstSlice_FEMC = 0.1498;
  Double_t te_minus_ge_by_ge_ge_min, te_minus_ge_by_ge_ge_max;   // Range of y-axis in [(te-ge)/ge vs ge] plot

  fit_min = 2;
  fit_max = 15;
  eta_min = 1.4;
  eta_max = 3.0;
  sigma_min = 0.0;
  sigma_max = 1.0;
  mean_min = -0.5;
  mean_max = 0.5;
  chi2_min = 0;
  chi2_max = 2.52;
  //  recalibration_factor = 0.7088;
  te_minus_ge_by_ge_ge_min = -0.99;
  te_minus_ge_by_ge_ge_max = 1.0;
   

  TString cut_text = " {1.4 < geta < 3.0} ";
  

  TH2D *te_minus_ge_by_ge_ge_EtaCut = new TH2D("te_minus_ge_by_ge_ge_EtaCut","#frac{#Delta e_{agg}}{truth e} vs truth e",200,0,30,200,-2,1);
  TH2D *te_minus_ge_by_ge_ge_EtaCut_CircularCut = new TH2D("te_minus_ge_by_ge_ge_EtaCut_CircularCut","#frac{#Delta e_{agg}}{truth e} vs truth e",200,0,30,200,-1.5,2);
  TH2D *te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated = new TH2D("te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated","#frac{#Delta e_{agg}}{truth e} vs truth e",200,0,30,200,-1.5,1.5);

  TH2D *te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_FEMC = new TH2D("te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_FEMC","#frac{#Delta e_{agg}}{truth e} vs truth e",nSlicesx,binLimitArray,nSlicesy,te_minus_ge_by_ge_ge_min,te_minus_ge_by_ge_ge_max);
  TH2D *te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_FHCAL = new TH2D("te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_FHCAL","#frac{#Delta e_{agg}}{truth e} vs truth e",nSlicesx,binLimitArray,nSlicesy,te_minus_ge_by_ge_ge_min,te_minus_ge_by_ge_ge_max);

  TH2D *te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp = new TH2D("te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp","#frac{#Delta e_{agg}}{truth e} vs truth e",nSlicesx,binLimitArray,nSlicesy,te_minus_ge_by_ge_ge_min,te_minus_ge_by_ge_ge_max); // histogram from which mean vs ge, sigma vs ge, and reduced_chi2 vs ge plots are derived

  TH2D *te_minus_ge_by_ge_ge_EtaCut_temp = new TH2D("te_minus_ge_by_ge_ge_EtaCut_temp","#frac{#Delta e_{agg}}{truth e} vs truth e",nSlicesx,binLimitArray,500,-0.99,1.3); // histogram from which mean vs ge, sigma vs ge, and reduced_chi2 vs ge plots are derived

  TH2D *te_by_ge_ge_EtaCut = new TH2D("te_by_ge_ge_EtaCut","te_{agg}/ge vs ge",200,0,30,200,-0.5,1.5);
  TH2D *te_by_ge_ge_EtaCut_CircularCut = new TH2D("te_by_ge_ge_EtaCut_CircularCut","te_{agg}/ge vs ge",200,0,30,200,-0.5,1.5);

  TH2D *te_by_ge_ge_EtaCut_CircularCut_FHCAL = new TH2D("te_by_ge_ge_EtaCut_CircularCut_FHCAL","te_{agg}/ge vs ge",200,0,30,200,-1,2);
  TH2D *te_by_ge_ge_EtaCut_CircularCut_FEMC = new TH2D("te_by_ge_ge_EtaCut_CircularCut_FEMC","te_{agg}/ge vs ge",200,0,30,200,-1,2);

  TH1D *te_aggregate_EtaCut_CircularCut_FEMC = new TH1D("te_aggregate_EtaCut_CircularCut_FEMC","",200,0,1);

  auto *mean_te_by_ge_ge_EtaCut_CircularCut = new TProfile("mean_te_by_ge_ge_EtaCut_CircularCut","Mean_{te/ge}",nSlicesx,binLimitArray,-0.5,35);
  auto *mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL = new TProfile("mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL","Mean_{te/ge}",nSlicesx,binLimitArray,-0.5,35);
  auto *mean_te_by_ge_ge_EtaCut_CircularCut_FEMC = new TProfile("mean_te_by_ge_ge_EtaCut_CircularCut_FEMC","Mean_{te/ge}",nSlicesx,binLimitArray,-0.5,35);

  T1->SetBranchAddress("DST#EvalTTree_FHCAL",&evaltree1);
  T2->SetBranchAddress("DST#EvalTTree_FEMC",&evaltree2);

  for(int i=0; i<T1->GetEntries(); i++) // We assume same no. of entries, since no cuts are applied
    {
      T1->GetEntry(i);
      T2->GetEntry(i);

      if(debug==1){
	std::cout<<"\n\n\n------------------------------------------\nParticle: "<<i<<"\n\n";
	std::cout<<"Initial Parameters "<<"\n";
      }

      Double_t geta1 = evaltree1->get_geta();

      if(debug==1){
	std::cout<<"geta: "<<geta1<<"\n";
      }

      if(geta1>=eta_min && geta1<=eta_max){

	if(debug==1){    
	  cout<<"\ngeta cut applied (1.3, 3.3)"<<"\n\n";
	}
	Double_t ge = evaltree1->get_ge();
	if(debug==1){
	  std::cout<<"ge: "<<ge<<"\n";
	}

	Double_t gphi = evaltree1->get_gphi();
	if(debug==1){
	  std::cout<<"gphi: "<<gphi<<"\n";
	}

	Double_t gtheta = evaltree1->get_gtheta();
	if(debug==1){
	  std::cout<<"gtheta: "<<gtheta<<"\n";
	}
 
	total_ge += ge;
	if(debug==1){
	  std::cout<<"total_ge till now = "<<total_ge<<"\n";
	}     

	Double_t te_aggregate = 0;
	Double_t te_aggregate_CircularCut = 0;    
	Double_t te_aggregate_FHCAL_CircularCut = 0;

	for (int j=0; j<evaltree1->get_ntowers(); j++){
	
	  if(debug==1){
	    std::cout<<"\nFHCAL Tower: "<<j<<"\n";
	  }

	  EvalTower *twr1 = evaltree1->get_tower(j);
	  if (twr1){ 
	  
	    if(debug==1){
	      cout<<"non-empty FHCAL tower\n";
	    }

	    Double_t tphi = twr1->get_tphi();
	    if(debug==1){
	      std::cout<<"tphi: "<<tphi<<"\n";
	    }

	    Double_t ttheta = twr1->get_ttheta();
	    if(debug==1){
	      std::cout<<"ttheta: "<<ttheta<<"\n";
	    }

	    Double_t dphi = tphi - gphi;
	    if(debug==1){
	      std::cout<<"tphi-gphi: "<<dphi<<"\n";
	    }

	    Double_t dtheta = ttheta - gtheta;
	    if(debug==1){
	      std::cout<<"ttheta-gtheta: "<<dtheta<<"\n";
	    }

	    Double_t te = twr1->get_te();
	    if(debug==1){
	      std::cout<<"te: "<<te<<"\n";
	    }

	    if(te > energyCut){
	      te_aggregate += te;
     
	      if(debug==1){
		std::cout<<"FHCAL: pow(dphi/y_radius,2)+pow(dtheta/x_radius,2) = "<<pow(dphi/y_radius_FHCAL,2)+pow(dtheta/x_radius_FHCAL,2)<<"\n";    
	      }

	      if (pow(dphi/y_radius_FHCAL,2)+pow(dtheta/x_radius_FHCAL,2)<=1){
		if(debug==1){
		  cout<<"FHCAL Tower included after circular cut\n";
		}	      
		te_aggregate_CircularCut += te;
		te_aggregate_FHCAL_CircularCut += te;
	      }

	      if(debug==1){
		cout<<"te_aggregate till now = "<<te_aggregate<<"\n";
		std::cout<<"te += "<<twr1->get_te()<<"\n";
		cout<<"te_aggregate_CircularCut till now = "<<te_aggregate_CircularCut<<"\n";
	      }
	    }
	  }
	}

	for (int j=0; j<evaltree2->get_ntowers(); j++){
	

	  if(debug==1){
	    std::cout<<"\nFEMC Tower: "<<j<<"\n";
	  }

	  EvalTower *twr2 = evaltree2->get_tower(j);
	  if (twr2){ 

	    if(debug==1){
	      cout<<"non-empty FEMC tower\n";
	    }

	    Double_t tphi = twr2->get_tphi();
	    if(debug==1){
	      std::cout<<"tphi: "<<tphi<<"\n";
	    }

	    Double_t ttheta = twr2->get_ttheta();
	    if(debug==1){
	      std::cout<<"ttheta: "<<ttheta<<"\n";
	    }

	    Double_t dphi = tphi - gphi;
	    if(debug==1){
	      std::cout<<"tphi-gphi: "<<dphi<<"\n";
	    }

	    Double_t dtheta = ttheta - gtheta;
	    if(debug==1){
	      std::cout<<"ttheta-gtheta: "<<dtheta<<"\n";
	    }

	    Double_t te = twr2->get_te();
	    if(debug==1){
	      std::cout<<"te: "<<te<<"\n";
	    }

	    if(MIP_theta_parametrisation == 1){
	      EMC_cut = mip_pmzn_energy_cut_ftheta->Eval(gtheta);
	    }

	    if(te > energyCut + EMC_cut){
	      te_aggregate += te;
     
	      if(debug==1){
		std::cout<<"FEMC: pow(dphi/y_radius,2)+pow(dtheta/x_radius,2): "<<pow(dphi/y_radius_FEMC,2)+pow(dtheta/x_radius_FEMC,2)<<"\n";    
	      }

	      if (pow(dphi/y_radius_FEMC,2)+pow(dtheta/x_radius_FEMC,2)<=1){
		if(debug==1){
		  cout<<"FEMC Tower included after circular cut\n";
		}
		te_aggregate_CircularCut += te;
	      }

	      if(debug==1){
		cout<<"te_aggregate till now = "<<te_aggregate<<"\n";
		cout<<"te_aggregate_CircularCut till now = "<<te_aggregate_CircularCut<<"\n";
		std::cout<<"te += "<<twr2->get_te()<<"\n";
	      }
	    }
	  }
	}
       
	total_te += te_aggregate;
	total_te_CircularCut += te_aggregate_CircularCut;

	if(debug==1){
	  cout<<"total_te till now = "<<total_te<<"\n";
	  cout<<"total_te_CircularCut till now = "<<total_te_CircularCut<<"\n\n";
	}

	if(te_aggregate_CircularCut > energyCutAggregate){
	  
	  if(te_aggregate_CircularCut < 0.1) {

	    cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;

	  }

	  te_minus_ge_by_ge_ge_EtaCut->Fill(ge, (te_aggregate-ge)/ge);

	  if(debug==1){
	    cout<<"(ge, (te_aggregate-ge)/ge): ("<<ge<<", "<<(te_aggregate-ge)/ge<<")\n";
	  }

	  te_minus_ge_by_ge_ge_EtaCut_CircularCut->Fill(ge, (te_aggregate_CircularCut-ge)/ge);
	  if(debug==1){
	    cout<<"(ge, (te_aggregate_CircularCut-ge)/ge): ("<<ge<<", "<<(te_aggregate_CircularCut-ge)/ge<<")\n";
	  }
         

	  /*te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated->Fill(ge, ((te_aggregate_CircularCut/recalibration_factor)-ge)/ge);
	    if(debug==1){
	    cout<<"(ge, ((te_aggregate_CircularCut/recalibration_factor)-ge)/ge: ("<<ge<<", "<<((te_aggregate_CircularCut/recalibration_factor)-ge)/ge<<")\n";
	    }
	    te_minus_ge_by_ge_ge_EtaCut_Recalibrated_temp->Fill(ge, ((te_aggregate_CircularCut/recalibration_factor)-ge)/ge);*/
   
	  te_by_ge_ge_EtaCut->Fill(ge, te_aggregate/ge);

	  if(debug==1){	
	    cout<<"(ge, te_aggregate/ge): ("<<ge<<", "<<te_aggregate/ge<<")\n";
	  }

	  te_by_ge_ge_EtaCut_CircularCut->Fill(ge, te_aggregate_CircularCut/ge);

	  te_aggregate_EtaCut_CircularCut_FEMC->Fill(te_aggregate_CircularCut - te_aggregate_FHCAL_CircularCut);

	  te_by_ge_ge_EtaCut_CircularCut_FHCAL->Fill(ge, te_aggregate_FHCAL_CircularCut/ge);
	  te_by_ge_ge_EtaCut_CircularCut_FEMC->Fill(ge, (te_aggregate_CircularCut - te_aggregate_FHCAL_CircularCut)/ge);

	  mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL->Fill(ge, te_aggregate_FHCAL_CircularCut/ge);
	  mean_te_by_ge_ge_EtaCut_CircularCut_FEMC->Fill(ge, (te_aggregate_CircularCut-te_aggregate_FHCAL_CircularCut)/ge);
     

	  te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_FHCAL->Fill(ge, (te_aggregate_FHCAL_CircularCut-ge)/ge);
	  te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_FEMC->Fill(ge, (te_aggregate_CircularCut - te_aggregate_FHCAL_CircularCut-ge)/ge);

     
	  if(debug==1){
	    cout<<"(ge, te_aggregate_CircularCut/ge): ("<<ge<<", "<<te_aggregate_CircularCut/ge<<")\n"; 
	    cout<<"(ge, te_aggregate_FHCAL_CircularCut/ge): ("<<ge<<", "<<te_aggregate_FHCAL_CircularCut/ge<<")\n"; 
	    cout<<"(ge, (te_aggregate_CircularCut-te_aggregate_FHCAL_CircularCut)/ge): ("<<ge<<", "<<(te_aggregate_CircularCut-te_aggregate_FHCAL_CircularCut)/ge<<")\n"; 
	  }     
	} 
      }  
    }

  if(debug==1){
    cout<<"\neta cut if ends"<<"\n";
  }



  TString arr[nSlicesx]; // Used for naming fitted slices used in sigma_e vs ge 
  for(int sno = 0; sno < nSlicesx; sno++){
    arr[sno] = TString::Itoa(sno + 1, 10);
  }

 if(debug==1){
   std::cout<<"\nGenerating sigma_e vs ge plots\n\n";
 }

 Double_t recalibrationArr1[nSlicesx];
 Double_t rf_integral_FHCAL = 0;
 Double_t weight_FHCAL = te_by_ge_ge_EtaCut_CircularCut_FHCAL->GetMean(2);

 //recalibrationArr1[0] = recalibration_firstSlice_FHCAL;
 //rf_integral_FHCAL += recalibrationArr1[0];
 //cout << "Recalibration factor for slice " << 1 << " of FHCAL is: " <<  recalibrationArr1[0] << endl;

 for(int binIter = 1; binIter <= nSlicesx; binIter++){
   recalibrationArr1[binIter-1] = mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL->GetBinContent(binIter);    
   rf_integral_FHCAL += recalibrationArr1[binIter-1];

   if(debug == 1){
     cout<<"rf_integral_FHCAL += "<<recalibrationArr1[binIter-1]<<"\n";
   }

   cout << "Recalibration factor for slice " << binIter << " of FHCAL is: " <<  recalibrationArr1[binIter-1] << endl;
 }

 if(debug == 1){  
   cout<<"rf_integral_FHCAL = "<<rf_integral_FHCAL<<"\n\n";
 }

 Double_t recalibrationArr2[nSlicesx];
 Double_t rf_integral_FEMC = 0;
 Double_t weight_FEMC = te_by_ge_ge_EtaCut_CircularCut_FEMC->GetMean(2);
 //recalibrationArr2[0] = recalibration_firstSlice_FEMC;
 //rf_integral_FEMC += recalibrationArr2[0];
 //cout << "Recalibration factor for slice " << 1 << " of FEMC is: " <<  recalibrationArr2[0] << endl;

 for(int binIter = 1; binIter <= nSlicesx; binIter++){
   recalibrationArr2[binIter-1] = mean_te_by_ge_ge_EtaCut_CircularCut_FEMC->GetBinContent(binIter);
   rf_integral_FEMC += recalibrationArr2[binIter-1];

   if(debug == 1){   
     cout<<"rf_integral_FEMC += "<<recalibrationArr2[binIter-1]<<"\n";
   }

   cout << "Recalibration factor for slice " << binIter << " of FEMC is: " <<  recalibrationArr2[binIter-1] << endl;
 }

 if(debug == 1){
   cout<<"rf_integral_FEMC = "<<rf_integral_FEMC<<"\n\n";
 }

 for(int i=0; i<T1->GetEntries(); i++){

   T1->GetEntry(i);
   T2->GetEntry(i);

   Double_t geta1 = evaltree1->get_geta();
   Double_t gphi = evaltree1->get_gphi();
   Double_t gtheta = evaltree1->get_gtheta();
   Double_t ge = evaltree1->get_ge();
   Double_t te_aggregate_CircularCut = 0;
   Double_t te_aggregate_FHCAL_CircularCut = 0;
   Double_t te_aggregate_CircularCut_normalised = 0;    

   int recalibration_factor = 0;

  
    if(ge > lowEnergyBin){
      Double_t eRangeBin = (maxEnergyBin - lowEnergyBin)/highThresholdBins;
      recalibration_factor = (lowThresholdBins - 1) + ceil(ge/eRangeBin)- 1;
    }
 
    else{
      recalibration_factor = ceil((ge/lowEnergyBin)*(Double_t)lowThresholdBins)- 1;  
    }

   if(geta1>=eta_min && geta1<=eta_max){
    	
      
     for (int j=0; j<evaltree1->get_ntowers(); j++){

       EvalTower *twr1 = evaltree1->get_tower(j);
       if (twr1){ 

	 Double_t tphi = twr1->get_tphi();
	 Double_t ttheta = twr1->get_ttheta();
	 Double_t dphi = tphi - gphi;
	 Double_t dtheta = ttheta - gtheta;	 
	 Double_t te = twr1->get_te();

	 if(te > energyCut){
    
	   if(debug==1){
	     std::cout<<"pow(dphi/y_radius_FHCAL,2)+pow(dtheta/x_radius_FHCAL,2): "<<pow(dphi/y_radius_FHCAL,2)+pow(dtheta/x_radius_FHCAL,2)<<"\n";    
	   }

	   if (pow(dphi/y_radius_FHCAL,2)+pow(dtheta/x_radius_FHCAL,2)<=1){
	     te_aggregate_CircularCut += te;
	     te_aggregate_FHCAL_CircularCut += te;
	     te_aggregate_CircularCut_normalised += te*weight_FHCAL/recalibrationArr1[recalibration_factor];
	   }
	 }
       }
     }

     for (int j=0; j<evaltree2->get_ntowers(); j++){

       EvalTower *twr2 = evaltree2->get_tower(j);
       if (twr2){ 

	 Double_t tphi = twr2->get_tphi();
	 Double_t ttheta = twr2->get_ttheta();
	 Double_t dphi = tphi - gphi;
	 Double_t dtheta = ttheta - gtheta;	 
	 Double_t te = twr2->get_te();

	 if(MIP_theta_parametrisation == 1){
	   EMC_cut = mip_pmzn_energy_cut_ftheta->Eval(gtheta);
	 }

	 if(te > energyCut + EMC_cut){
    
	   if(debug==1){
	     std::cout<<"pow(dphi/y_radius_FEMC,2)+pow(dtheta/x_radius_FEMC,2): "<<pow(dphi/y_radius_FEMC,2)+pow(dtheta/x_radius_FEMC,2)<<"\n";    
	   }

	   if (pow(dphi/y_radius_FEMC,2)+pow(dtheta/x_radius_FEMC,2)<=1){
	     te_aggregate_CircularCut += te;
	     te_aggregate_CircularCut_normalised += te*weight_FEMC/recalibrationArr2[recalibration_factor];
	   }
	 }
       }
     }
     

     if(te_aggregate_CircularCut > energyCutAggregate){
       mean_te_by_ge_ge_EtaCut_CircularCut->Fill(ge, te_aggregate_CircularCut_normalised/ge);
       if(debug == 1){
	 cout<<"(ge, te_aggregate_CircularCut_normalised/ge): ("<<ge<<", "<<te_aggregate_CircularCut_normalised/ge<<")\n";
       } 
     }  
   }    
 }


 Double_t recalibrationArr[nSlicesx];

 //recalibrationArr[0] = recalibration_firstSlice;
 //cout << "Recalibration factor for slice " << 1 << " is: " <<  recalibrationArr[0] << endl;

 for(int binIter = 1; binIter <= nSlicesx; binIter++){
   recalibrationArr[binIter-1] = mean_te_by_ge_ge_EtaCut_CircularCut->GetBinContent(binIter);
   cout << "Recalibration factor for slice " << binIter << " of is: " <<  recalibrationArr[binIter-1] << endl;
 }


 for(int i=0; i<T1->GetEntries(); i++){

   T1->GetEntry(i);
   T2->GetEntry(i);

   Double_t geta1 = evaltree1->get_geta();
   Double_t gphi = evaltree1->get_gphi();
   Double_t gtheta = evaltree1->get_gtheta();
   Double_t ge = evaltree1->get_ge();
   Double_t te_aggregate_CircularCut = 0.0;
   Double_t te_aggregate_FHCAL_CircularCut = 0.0;
   Double_t te_aggregate_CircularCut_normalised = 0.0;  

   int recalibration_factor = 0;

  
    if(ge > lowEnergyBin){
      Double_t eRangeBin = (maxEnergyBin - lowEnergyBin)/highThresholdBins;
      recalibration_factor = (lowThresholdBins - 1) + ceil(ge/eRangeBin)- 1;
    }
 
    else{
      recalibration_factor = ceil((ge/lowEnergyBin)*(Double_t)lowThresholdBins)- 1;  
    }


   if(geta1>=eta_min && geta1<=eta_max){
    	
      
     for (int j=0; j<evaltree1->get_ntowers(); j++){

       EvalTower *twr1 = evaltree1->get_tower(j);
       if (twr1){ 

	 Double_t tphi = twr1->get_tphi();
	 Double_t ttheta = twr1->get_ttheta();
	 Double_t dphi = tphi - gphi;
	 Double_t dtheta = ttheta - gtheta;	 
	 Double_t te = twr1->get_te();

	 if(te > energyCut){
    
	   if(debug==1){
	     std::cout<<"pow(dphi/y_radius_FHCAL,2)+pow(dtheta/x_radius_FHCAL,2): "<<pow(dphi/y_radius_FHCAL,2)+pow(dtheta/x_radius_FHCAL,2)<<"\n";    
	   }

	   if (pow(dphi/y_radius_FHCAL,2)+pow(dtheta/x_radius_FHCAL,2)<=1){
	     te_aggregate_CircularCut += te;
	     te_aggregate_FHCAL_CircularCut += te;
	     te_aggregate_CircularCut_normalised += te*weight_FHCAL/recalibrationArr1[recalibration_factor];
	   }
	 }
       }
     }

     for (int j=0; j<evaltree2->get_ntowers(); j++){

       EvalTower *twr2 = evaltree2->get_tower(j);
       if (twr2){ 

	 Double_t tphi = twr2->get_tphi();
	 Double_t ttheta = twr2->get_ttheta();
	 Double_t dphi = tphi - gphi;
	 Double_t dtheta = ttheta - gtheta;	 
	 Double_t te = twr2->get_te();
	 
	 if(MIP_theta_parametrisation == 1){
	   EMC_cut = mip_pmzn_energy_cut_ftheta->Eval(gtheta);
	 }

	 if(te > energyCut + EMC_cut){
    
	   if(debug==1){
	     std::cout<<"pow(dphi/y_radius_FEMC,2)+pow(dtheta/x_radius_FEMC,2): "<<pow(dphi/y_radius_FEMC,2)+pow(dtheta/x_radius_FEMC,2)<<"\n";    
	   }

	   if (pow(dphi/y_radius_FEMC,2)+pow(dtheta/x_radius_FEMC,2)<=1){
	     te_aggregate_CircularCut += te;
	     te_aggregate_CircularCut_normalised += te*weight_FEMC/recalibrationArr2[recalibration_factor];
	   }
	 }
       }
     }


     if(te_aggregate_CircularCut > energyCutAggregate){
       te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated->Fill(ge, ((te_aggregate_CircularCut_normalised/recalibrationArr[recalibration_factor])-ge)/ge);
       te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp->Fill(ge, ((te_aggregate_CircularCut_normalised/recalibrationArr[recalibration_factor])-ge)/ge);
	
       if(debug==1){
	 cout<<"(ge, ((te_aggregate_CircularCut_normalised/recalibration_factor)-ge)/ge: ("<<ge<<", "<<((te_aggregate_CircularCut_normalised/recalibrationArr[recalibration_factor])-ge)/ge<<")\n";
       }
     }
   }    
 }


 // Initialising fit functions 
 TF1 *fit = new TF1("fit", "gaus");   
 TF1 *fit1 = new TF1("fit1","gaus",fit_min,fit_max);
 TF1 *fExp = new TF1("fExp","0.1 + 0.5/sqrt(x)",0,30);
 TF1 *fTrue = new TF1("fTrue","[0] + [1]/sqrt(x)",0,30);


 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp->FitSlicesY(0, 1, -1, 0, "QN");
 TH2D *te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2 = (TH2D*)gDirectory->Get("te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2");
 TH2D *te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1 = (TH2D*)gDirectory->Get("te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1");
 TH2D *te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2 = (TH2D*)gDirectory->Get("te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2");



 TH1D* slices[nSlicesx];

 // Generating plots for individual slices
 for(int sno = 0; sno < nSlicesx; sno++){
   int plusOne = sno+1;
   TString sname = "slice " + arr[sno];
   slices[sno] = te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp->ProjectionY(sname, plusOne, plusOne);
 }

 if(debug==1){
   cout<<"\nHistogram Formatting\n\n";
 }

 te_minus_ge_by_ge_ge_EtaCut->GetXaxis()->SetTitle("Generated Energy (GeV)");
 te_minus_ge_by_ge_ge_EtaCut->GetXaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut->GetXaxis()->SetTitleSize(0.05);
 te_minus_ge_by_ge_ge_EtaCut->GetYaxis()->SetTitle("(te_{agg}-ge)/ge");
 te_minus_ge_by_ge_ge_EtaCut->GetYaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut->GetYaxis()->SetTitleSize(0.05);

 te_minus_ge_by_ge_ge_EtaCut_CircularCut->GetXaxis()->SetTitle("Generated Energy (GeV)");
 te_minus_ge_by_ge_ge_EtaCut_CircularCut->GetXaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut_CircularCut->GetXaxis()->SetTitleSize(0.05);
 te_minus_ge_by_ge_ge_EtaCut_CircularCut->GetYaxis()->SetTitle("(te_{agg}-ge)/ge");
 te_minus_ge_by_ge_ge_EtaCut_CircularCut->GetYaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut_CircularCut->GetYaxis()->SetTitleSize(0.05);

 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated->GetXaxis()->SetTitle("Generated Energy (GeV)");
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated->GetXaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated->GetXaxis()->SetTitleSize(0.05);
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated->GetYaxis()->SetTitle("(te_{agg}-ge)/ge");
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated->GetYaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated->GetYaxis()->SetTitleSize(0.05);

 te_by_ge_ge_EtaCut->GetXaxis()->SetTitle("Generated Energy (GeV)");
 te_by_ge_ge_EtaCut->GetXaxis()->SetLabelSize(0.05);  
 te_by_ge_ge_EtaCut->GetXaxis()->SetTitleSize(0.05);
 te_by_ge_ge_EtaCut->GetYaxis()->SetTitle("te_{agg}/ge");
 te_by_ge_ge_EtaCut->GetYaxis()->SetLabelSize(0.05);  
 te_by_ge_ge_EtaCut->GetYaxis()->SetTitleSize(0.05);

 te_aggregate_EtaCut_CircularCut_FEMC->GetXaxis()->SetTitle("te_{agg} (GeV)");
 te_aggregate_EtaCut_CircularCut_FEMC->GetXaxis()->SetLabelSize(0.05);  
 te_aggregate_EtaCut_CircularCut_FEMC->GetXaxis()->SetTitleSize(0.05);
 te_aggregate_EtaCut_CircularCut_FEMC->GetYaxis()->SetTitle("Counts");
 te_aggregate_EtaCut_CircularCut_FEMC->GetYaxis()->SetLabelSize(0.05);  
 te_aggregate_EtaCut_CircularCut_FEMC->GetYaxis()->SetTitleSize(0.05);

 te_by_ge_ge_EtaCut_CircularCut->GetXaxis()->SetTitle("Generated Energy (GeV)");
 te_by_ge_ge_EtaCut_CircularCut->GetXaxis()->SetLabelSize(0.05);  
 te_by_ge_ge_EtaCut_CircularCut->GetXaxis()->SetTitleSize(0.05);
 te_by_ge_ge_EtaCut_CircularCut->GetYaxis()->SetTitle("te_{agg}/ge");
 te_by_ge_ge_EtaCut_CircularCut->GetYaxis()->SetLabelSize(0.05);  
 te_by_ge_ge_EtaCut_CircularCut->GetYaxis()->SetTitleSize(0.05);

 mean_te_by_ge_ge_EtaCut_CircularCut->GetXaxis()->SetTitle("Generated Energy (GeV)");
 mean_te_by_ge_ge_EtaCut_CircularCut->GetXaxis()->SetLabelSize(0.05);  
 mean_te_by_ge_ge_EtaCut_CircularCut->GetXaxis()->SetTitleSize(0.05);
 mean_te_by_ge_ge_EtaCut_CircularCut->GetYaxis()->SetTitle("Mean of te_{agg}/ge");
 mean_te_by_ge_ge_EtaCut_CircularCut->GetYaxis()->SetLabelSize(0.05);  
 mean_te_by_ge_ge_EtaCut_CircularCut->GetYaxis()->SetTitleSize(0.05);

 mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL->GetXaxis()->SetTitle("Generated Energy (GeV)");
 mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL->GetXaxis()->SetLabelSize(0.05);  
 mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL->GetXaxis()->SetTitleSize(0.05);
 mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL->GetYaxis()->SetTitle("Mean of te_{agg}/ge (FHCAL)");
 mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL->GetYaxis()->SetLabelSize(0.05);  
 mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL->GetYaxis()->SetTitleSize(0.05);

 mean_te_by_ge_ge_EtaCut_CircularCut_FEMC->GetXaxis()->SetTitle("Generated Energy (GeV)");
 mean_te_by_ge_ge_EtaCut_CircularCut_FEMC->GetXaxis()->SetLabelSize(0.05);  
 mean_te_by_ge_ge_EtaCut_CircularCut_FEMC->GetXaxis()->SetTitleSize(0.05);
 mean_te_by_ge_ge_EtaCut_CircularCut_FEMC->GetYaxis()->SetTitle("Mean of te_{agg}/ge (FEMC)");
 mean_te_by_ge_ge_EtaCut_CircularCut_FEMC->GetYaxis()->SetLabelSize(0.05);  
 mean_te_by_ge_ge_EtaCut_CircularCut_FEMC->GetYaxis()->SetTitleSize(0.05);

 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->GetXaxis()->SetTitle("Generated Energy (GeV)");
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->GetXaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->GetXaxis()->SetTitleSize(0.05);
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->GetYaxis()->SetTitle("#sigma_{e_{agg}}");
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->GetYaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->GetYaxis()->SetTitleSize(0.05);
 //te_minus_ge_by_ge_ge_EtaCut_2->SetTitle("#sigma_{e_{agg}} vs true_e" + cut_text);

 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2->GetXaxis()->SetTitle("Generated Energy (GeV)");
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2->GetXaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2->GetXaxis()->SetTitleSize(0.05);
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2->GetYaxis()->SetTitle("Reduced_#chi^{2}_{e_{agg}}");
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2->GetYaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2->GetYaxis()->SetTitleSize(0.04);
 //te_minus_ge_by_ge_ge_EtaCut_chi2->SetTitle("#chi^{2}_{e_{agg}} vs true_e" + cut_text);

 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1->GetXaxis()->SetTitle("Generated Energy (GeV)");
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1->GetXaxis()->SetLabelSize(0.05);  
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1->GetXaxis()->SetTitleSize(0.05);
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1->GetYaxis()->SetTitle("Mean_{e_{agg}}");
 te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1->GetYaxis()->SetLabelSize(0.05);  
  te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1->GetYaxis()->SetTitleSize(0.05);
  //te_minus_ge_by_ge_ge_EtaCut_1->SetTitle("mean_{e_{agg}} vs true_e" + cut_text);
	
  for(int sno = 0; sno < nSlicesx; sno++){
    slices[sno]->GetXaxis()->SetTitle("#Delta e^{agg}/ ge");
    slices[sno]->GetXaxis()->SetLabelSize(0.05);  
    slices[sno]->GetXaxis()->SetTitleSize(0.05);
    slices[sno]->GetYaxis()->SetTitle("Counts");
    slices[sno]->GetYaxis()->SetLabelSize(0.05);  
    slices[sno]->GetYaxis()->SetTitleSize(0.05);
  }

  if(debug==1){
    std::cout<<"\nWrite Histograms to File\n";
  }

  TFile *f = new TFile("energy_verification_EtaCut_CircularCut_" + detector + ".root","RECREATE"); 
    
  f->GetList()->Add(te_by_ge_ge_EtaCut);
  f->GetList()->Add(te_by_ge_ge_EtaCut_CircularCut);
  f->GetList()->Add(te_minus_ge_by_ge_ge_EtaCut);
  f->GetList()->Add(te_minus_ge_by_ge_ge_EtaCut_CircularCut);
  f->GetList()->Add(mean_te_by_ge_ge_EtaCut_CircularCut);
  f->GetList()->Add(mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL);
  f->GetList()->Add(mean_te_by_ge_ge_EtaCut_CircularCut_FEMC);
  f->GetList()->Add(te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated);
  f->GetList()->Add(te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2);
  f->GetList()->Add(te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1);
  f->GetList()->Add(te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2);
  f->GetList()->Add(te_aggregate_EtaCut_CircularCut_FEMC);
  
  for(int sno = 0; sno < nSlicesx; sno++){
    f->GetList()->Add(slices[sno]);
  }
  f->Write();
	  
  gStyle -> SetOptStat(11);
  gStyle -> SetOptFit(112);
  
  /*  int sno = 0;
  TString plusOne = (TString)(sno + 1);
  TString nameF = detector + "_sigmaE_slice" + arr[sno] + "_EtaCut_CircularCut.png";
  slices[sno] -> Fit("fit1", "R+");
  slices[sno] -> Draw("hist same");
  c->Print(nameF);

  double_t mean = fit1->GetParameter(1);
  double_t mean_error = fit1->GetParError(1);
  double_t sigma = fit1->GetParameter(2);
  double_t sigma_error = fit1->GetParError(2); 
  double_t chi2 = (fit1->GetChisquare())/(fit1->GetNDF());
 
  TLine *Mean = new TLine(0,mean,30.0/nSlicesx,mean);
  TLine *Sigma = new TLine(0,sigma,30.0/nSlicesx,sigma);
  TLine *Chi2 = new TLine(0,chi2,30.0/nSlicesx,chi2);
  TLine *Periphery_Chi2 = new TLine(30.0/nSlicesx,0,30.0/nSlicesx,chi2);
  TLine *Sigma_error = new TLine(30.0/(2.0*nSlicesx),sigma-(sigma_error),30.0/(2.0*nSlicesx),sigma+(sigma_error));
  TLine *Mean_error = new TLine(30.0/(2.0*nSlicesx),mean-(mean_error),30.0/(2.0*nSlicesx),mean+(mean_error));
  TLine *Point = new TLine(30.0/(2.0*nSlicesx),sigma,30.0/(2.0*nSlicesx),sigma);*/

  if(print==1){
    if(debug==1){
      std::cout<<"\nSaving Histograms as .png\n";
    }

    gStyle -> SetOptStat(0);

    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1->SetAxisRange(mean_min,mean_max,"Y");
    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_1->Draw();
    /*Mean->SetLineColor(1);
    Mean->SetLineWidth(1); 
    Mean_error->SetLineColor(1);
    Mean_error->SetLineWidth(1);
    Mean->Draw("same");
    Mean_error->Draw("same");*/
    c->Print(detector + "_meanE_ge_EtaCut_CircularCut.png");

    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2->SetAxisRange(chi2_min,chi2_max,"Y");
    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_chi2->Draw();
    /*Chi2->SetLineColor(1);
    Chi2->SetLineWidth(1); 
    Periphery_Chi2->SetLineColor(1);
    Periphery_Chi2->SetLineWidth(1);
    Chi2->Draw("same");
    Periphery_Chi2->Draw("same");*/
    c->Print(detector + "_chi2E_ge_EtaCut_CircularCut.png");
 
    gStyle -> SetOptStat(0);
    gStyle -> SetOptFit(0);

    /* Sigma->SetLineColor(1);
    Sigma->SetLineWidth(1); 
    Sigma_error->SetLineColor(1);
    Sigma_error->SetLineWidth(1);
    Point->SetLineColor(3);
    Point->SetLineWidth(5);*/
    fExp->SetLineColor(4); //38
    fTrue->SetLineColor(2); //46
  
    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->SetMarkerStyle(kFullCircle);
    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->SetMarkerColor(46); //30
    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->SetMarkerSize(0.75);
    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->SetAxisRange(sigma_min,sigma_max,"Y");

    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->Fit("fTrue", "M+");
    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2->Draw("same");
    fExp->Draw("same");
    /*Sigma->Draw("same");
    Sigma_error->Draw("same");
    Point->Draw("same");*/
  
    TLegend* legend = new TLegend(1.75,1.75);
    legend->SetHeader("Legend", "C");
    //legend->AddEntry(te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated_temp_2, "#sigma_{e_{agg}} vs Generated Energy", "flep");
    //legend->AddEntry((TObject*)0,"","");
    legend->AddEntry(fTrue, "p_{0} + p_{1}/#sqrt{ge} (Fitted)", "l");
    legend->AddEntry((TObject*)0,"","");
    legend->AddEntry(fExp, "0.1 + 0.5/#sqrt{ge} (Requirement)", "l");
    legend->SetTextSize(0.033);
    legend->Draw();

    std::cout<<"reduced_chi2 of fit: "<<fTrue->GetChisquare()/fTrue->GetNDF()<<"\n";

    c->Print(detector + "_sigmaE_ge_EtaCut_CircularCut.png");
    usleep(5e6);

    gStyle -> SetOptStat(11);
    gStyle -> SetOptFit(112);

    for(int sno = 0; sno < nSlicesx; sno++){
      TString plusOne = (TString)(sno + 1);
      TString nameF = detector + "_sigmaE_slice" + arr[sno] + "_EtaCut_CircularCut.png";
      slices[sno] -> Fit("fit", "M+");
      slices[sno] -> Draw("hist same");
      c->Print(nameF);
    }

    te_aggregate_EtaCut_CircularCut_FEMC->Draw();
    c->Print("te_aggregate_EtaCut_CircularCut_FEMC.png");
    usleep(5e6);

    gStyle -> SetOptStat(1);

    te_by_ge_ge_EtaCut->Draw("colz");
    c->Print(detector + "_te_by_ge_ge_EtaCut.png");
    te_by_ge_ge_EtaCut_CircularCut->Draw("colz");
    c->Print(detector + "_te_by_ge_ge_EtaCut_CircularCut.png");

    te_minus_ge_by_ge_ge_EtaCut->Draw("colz");
    c->Print(detector + "_te_minus_ge_by_ge_ge_EtaCut.png");
    te_minus_ge_by_ge_ge_EtaCut_CircularCut->Draw("colz");
    c->Print(detector + "_te_minus_ge_by_ge_ge_EtaCut_CircularCut.png");
    te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated->Draw("colz");
    c->Print(detector + "_te_minus_ge_by_ge_ge_EtaCut_CircularCut_Recalibrated.png");

    gStyle -> SetOptStat(0);

    mean_te_by_ge_ge_EtaCut_CircularCut->Draw();
    c->Print(detector + "_mean_te_by_ge_ge_EtaCut_CircularCut.png");

    mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL->Draw();
    c->Print(detector + "_mean_te_by_ge_ge_EtaCut_CircularCut_FHCAL.png");

    mean_te_by_ge_ge_EtaCut_CircularCut_FEMC->Draw();
    c->Print(detector + "_mean_te_by_ge_ge_EtaCut_CircularCut_FEMC.png");

    gStyle -> SetOptStat(1);
    c->Close();

  }

  std::cout << "The total te is: " << total_te << endl;
  std::cout << "The total te_CircularCut is: " << total_te_CircularCut << endl;
  std::cout << "The total ge is: " << total_ge << endl; 
  std::cout<<"\n\nDone\n----------------------------------------------------------------------\n\n";
 
}
