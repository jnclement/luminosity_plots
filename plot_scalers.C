#include "TH2D.h"
#include <iostream>
#include <string>
#include <map>
using namespace std;

void plot_scalers(const char * infile = "trees/scalers.root") {

    TFile * f = TFile::Open(infile);
    TTree * t = (TTree*)f->Get("scalers");
    gStyle->SetOptStat(0);
    gStyle->SetPadTickX(1);
    gStyle->SetPadTickY(1);
    int runnum = 0;
    int events = 0;
    int duration = 0;
    int scaledown[64] = { 0 };
    long long raw[64] = { 0 };
    long long live[64] = { 0 };
    long long scaled[64] = { 0 };
    int starttime = 0;
    t->SetBranchAddress("starttime",&starttime);
    t->SetBranchAddress("runnumber",&runnum);
    t->SetBranchAddress("events",&events);
    t->SetBranchAddress("duration",&duration);
    t->SetBranchAddress("scaledowns",&scaledown);
    t->SetBranchAddress("raw",&raw);
    t->SetBranchAddress("live",&live);
    t->SetBranchAddress("scaled",&scaled);
    TH1D * h1_rateovertime = new TH1D("rate","",2832409,1714000000,1716832409);
    h1_rateovertime->GetYaxis()->SetTitle("Rate [Hz]");
    h1_rateovertime->GetXaxis()->SetTimeDisplay(1);
    h1_rateovertime->GetXaxis()->SetTimeFormat("%m\/%d");
    h1_rateovertime->GetXaxis()->SetTitle("Date [month/day of 2024]");
    TH1D * h1_imbots = new TH1D("imbots","",2832409,1714000000,1716832409);
    TH1D * h1_imbotr = new TH1D("imbotr","",2832409,1714000000,1716832409);
    h1_imbotr->GetYaxis()->SetTitle("Int. Live MB Trigs with Phot Trig Scaler = 0 [Counts]");
    h1_imbots->GetYaxis()->SetTitle("Int. Live Min Bias Triggers [Counts]");
    h1_imbotr->GetXaxis()->SetTimeDisplay(1);
    h1_imbotr->GetXaxis()->SetTitle("Date [month/day of 2024]");
    h1_imbots->GetXaxis()->SetTimeDisplay(1);
    h1_imbotr->GetXaxis()->SetTimeFormat("%m\/%d");
    h1_imbots->GetXaxis()->SetTimeFormat("%m\/%d");
    h1_imbots->GetXaxis()->SetTitle("Date [month/day of 2024]");
    TH1D * h1_scaledownlivescaled = new TH1D("h_scaledownlivescaled","(scaledown+1)/(live/scaled);counts;",100,0,3);
    TH1D * h1_duration = new TH1D("h1_duration",";duration [s];counts",100,0,5400);
    TH1D * h1_rates[32];
    TH1D * h1_livetime[32];
    for (int i = 0; i < 32; i++) {
        h1_rates[i] = new TH1D(Form("h1_rate_%i",i),";rate;counts",100,0,0);
        h1_livetime[i] = new TH1D(Form("h1_livetime_%i",i),";livetime;counts",101,0,1.01);
    }
    TH2D * h2_rates = new TH2D("h2_rates",";triggernum;rate",32,-0.5,31.5,100,0,50000); 
    long long eventsn = 0;
    long long alltrigs = 0;
    for ( int e = 0; e < t->GetEntries(); e++) {
        t->GetEntry(e);
        if (events < 500 || duration < 300) continue;
	eventsn += live[10];
	h1_imbots->Fill(starttime,eventsn);
	
	
	if(raw[3]/duration > 3000) cout << "error: " << starttime << " " << raw[3] << " " << duration << " " << raw[3]/duration << endl;
	if(raw[3]/duration <= 3000) h1_rateovertime->Fill(starttime,static_cast<double>(raw[3])/duration);
        h1_duration->Fill(duration);
        for (int i = 0; i < 32; i++) {
            if (scaledown[i] != -1 && scaled[i] != 0) h1_scaledownlivescaled->Fill(static_cast<float>(scaledown[i]+1)/(static_cast<float>(live[i])/scaled[i]));
            if (raw[i] != 0) h1_livetime[i]->Fill(static_cast<float>(live[i])/raw[i]);
            h1_rates[i]->Fill(static_cast<float>(raw[i])/duration);
            h2_rates->Fill(i,static_cast<float>(raw[i])/duration);
        }
	if(scaled[10] == 0) continue;
	if(scaled[24] == 0) continue;
	if(raw[28]/duration > 10000 || live[24]/scaled[24] > 1.5) continue;
	alltrigs += live[10];
	h1_imbotr->Fill(starttime,alltrigs);
    }
    TCanvas* c1 = new TCanvas("","",2000,500);
    //c1->SetLogy();
    gPad->SetTicks(1,1);
    h1_rateovertime->SetMarkerStyle(2);
    h1_imbotr->SetMarkerStyle(2);
    h1_imbots->SetMarkerStyle(2);
    h1_rateovertime->Draw("P HIST");
    c1->SaveAs("trees/rateovertime.pdf");
    h1_imbotr->Draw("P HIST");
    c1->SaveAs("trees/imbotr.pdf");
    h1_imbots->Draw("P HIST");
    c1->SaveAs("trees/imbots.pdf");
    cout << eventsn << endl;
}
