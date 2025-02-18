#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>
#include <TFile.h>
#include <TCanvas.h>
#include <TRint.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TMath.h>
#include <arpa/inet.h>

using namespace std;

int main(int argc, char *argv[]){
  if(argc!=3){
    cerr << " USAGE> DAC_Analysis [data dir.] [Vth val.]" << endl;
    cerr << "  Vth: give a decimal number from 0 and 16383 " << endl;
    exit(1);
  }
  string dirname = argv[1];
  int Vth = atoi(argv[2]) & 0x3fff;

  TRint app("app", &argc, argv);
  gStyle->SetOptStat(0);

  TH2F *DAC_image = new TH2F("DAC_image", "DAC Survey",
			     130, -1.5, 128.5, 64, -0.5, 63.5);

  for(int fi=0; fi<64; fi++){
    char filename[100];
    sprintf(filename, "%s/DAC_%02d_%04x.srv", dirname.c_str(), fi, Vth);
    cout << filename << '\r' << flush;

    double event_num=0;
    double total[128]={0};
    ifstream data_file(filename);
    if(!data_file){
      cerr << " Can't find " << filename << endl;
      exit(1);
    }

    while(data_file){
      char data;
      data_file.read(&data, 1);
      if(data_file.eof())
    	break;
      
      if((data & 0xff) == 0xeb){
    	data_file.read(&data, 1);
	//	printf("0x%x\n", data & 0xff);
    	if((data & 0xff) == 0x90){
	  data_file.read(&data, 1);
	  if((data & 0xff) == 0x19){
	    data_file.read(&data, 1);
	    if((data & 0xff) == 0x64){

	      //	      printf("\nheader eve=%f\n", event_num);
	      double count[128]={0};

	      int i_data;
	      data_file.read((char*)&i_data, sizeof(int)); // trigger counter
	      data_file.read((char*)&i_data, sizeof(int)); // clock counter
	      data_file.read((char*)&i_data, sizeof(int)); // Input Ch2 counter

	      data_file.read((char*)&i_data, sizeof(int));
	      //	      printf("after header, data=0x%08x\n", i_data);

	      if(htonl(i_data) != 0x75504943){                 // if not footer
		data_file.read((char*)&i_data, sizeof(int));
		for(int i=0; i<2*511; i++)
		  data_file.read((char*)&i_data, sizeof(int));

		data_file.read((char*)&i_data, sizeof(int));
		//		printf("after FADC, data=0x%08x\n", htonl(i_data));
		
		// Hit data
		for(;;){
		  data_file.read((char*)&i_data, sizeof(int));
		  //		  printf("Hit header, data=0x%08x\n", i_data);
		  if(htonl(i_data) == 0x75504943 || data_file.eof())  break;

		  // search the hit headers (added by Furuno on 2022/08/22)
		  // This search was needed to run on maikodaq2 PC
		  if( (((htonl(i_data)) >> 16)&0xffff) ==  0x8000){
		    //		    printf("Hit header, eve=%f\n", event_num);
		    
		    for(int i=0; i<4; i++){
		      data_file.read((char*)&i_data, sizeof(int));
		      i_data = htonl(i_data);
		      
		      for(int strip=0; strip<32; strip++){
			if((i_data >> strip) & 0x1)
			  count[32*(3-i) + strip] += 1;
		      }
		    }
		  }
		}
		
		for(int strp=0; strp<128; strp++)
		  total[strp] += (double)count[strp]/1024.;
		event_num++;
	      }
	    }
	  }
	}
      }
    }
    data_file.close();
    
    if(event_num){
      for(int st=0; st<128; st++){
	DAC_image->Fill(st, fi, (double)total[st]/event_num);
      }
    }
  }

  TFile *RootFile = new TFile("DAC.root", "recreate");
  DAC_image->Write();

  
  ofstream DacOut("base_correct.dac", ios::out);
  TH1F *proj = new TH1F("proj", "", 64, -0.5, 63.5);
  TF1 *erf = new TF1("erf", "[0] * TMath::Erf((x-[1])/[2])+[3]");
  TCanvas *ViewWin = new TCanvas("ViewWin", "", 0, 0, 800, 600);
  ViewWin->SetGridx();
  ViewWin->SetGridy();
  ViewWin->Draw();
  for(int strip=0; strip<128; strip++){
    for(int dac=0; dac<64; dac++){
      proj->SetBinContent(dac+1,
			  DAC_image->GetBinContent(strip+2, dac+1));

    proj->SetBinError(dac+1, 0.01); 
    }

    char histname[50], name[50];
    sprintf(histname, "Ch %03d", strip);
    sprintf(name, "h_%d", strip);    
    proj->SetTitle(histname);
    proj->SetName(name);    
    proj->Write();
    
    //double pol = 0.5 - proj->GetBinContent(1);
    //    double pol = proj->GetBinContent(1);
    double avr = proj->GetMean();

    //erf->SetParameters(pol, avr, 5);    
    erf->SetParameters(1.0, avr, 2, 0);
    erf->SetParLimits(1, 5.0, 63.0);
    erf->SetParLimits(2, 0.0, 10.0);
    proj->Fit("erf", "", "", 0, 64);

    proj->Draw();
    ViewWin->Update();

    DacOut << strip << '\t' 
	   << TMath::FloorNint(proj->GetFunction("erf")->GetParameter(1) + 0.5)
	   << endl;
    sprintf(histname, "Ch_%03d.png", strip);
    ViewWin->Print(histname);

    sleep(1);
  }

  RootFile->Close();
}
