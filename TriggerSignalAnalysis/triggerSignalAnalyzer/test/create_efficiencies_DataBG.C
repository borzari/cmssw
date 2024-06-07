#include "tdrstyle.C"
#include "CMS_lumi.C"

using namespace std;

double errorCalc(Int_t x, Int_t y){
    double dx = sqrt(x);
    double dy = sqrt(y);
    return (1/(double) y)*(dx + (((double) x/(double) y)*dy));
}

void create_efficiencies_DataBG(std::string process, std::string outputDir)
{

    setTDRStyle();
    gStyle->SetOptStat(0);
    writeExtraText = true;
    extraText = "Work in Progress";
    // lumi_sqrtS = "61.9 fb^{-1}, 13.6 TeV";
    lumi_sqrtS = "0.102 fb^{-1}, 13.6 TeV";
    // std::string inputFile = "hist_merged_TrigAnalysis_" + process + ".root";
    std::string inputFile = "outputFile" + process + ".root";
    TFile *f = TFile::Open(inputFile.c_str());
    TH1 *den5;
    TH1 *den52;
    f->GetObject("triggerDataBGAnalyzer/pfmet105", den5);
    f->GetObject("triggerDataBGAnalyzer/120pfmet105", den52);
    TH1 *num5;
    TH1 *num52;
    f->GetObject("triggerDataBGAnalyzer/hltpfmet105", num5);
    f->GetObject("triggerDataBGAnalyzer/120hltpfmet105", num52);
    
    int marker = 20;
    const char *ylabel = "Trigger Efficiency";
    const char *xlabelnomu = "PF E_{T}^{miss, no mu}";
    const char *xlabelyemu = "PF E_{T}^{miss}";
    float ymin = 0.0;
    float ymax = 1.2;
    float xmin = 0.0;
    float xmax = 1000.0;
    TLine *l = new TLine(xmin, 1.0, xmax, 1.0);
    l->SetLineStyle(7);
    l->SetLineWidth(2);
    float x0 = 0.1891;
    float x1 = 0.3395;
    float y0 = 0.8592;
    float y1 = 0.9214;

    TCanvas *c5 = new TCanvas("c5", "c5", 700, 700);
    TCanvas *c52 = new TCanvas("c52", "c52", 700, 700);
    
    std::string outFile = outputDir + "/" + process+ "_hlt_efficiencies.root";
    TFile *file = new TFile(outFile.c_str(), "RECREATE");

    TGraphAsymmErrors *pEff5 = new TGraphAsymmErrors("pfmet105");
    pEff5->SetTitle("HLT_MET105_IsoTrk50_v");
    pEff5->Divide(num5, den5);
    pEff5->SetTitle("");
    pEff5->SetMarkerStyle(marker);
    pEff5->GetHistogram()->SetMinimum(ymin);
    pEff5->GetHistogram()->SetMaximum(ymax);
    pEff5->GetYaxis()->SetTitle(ylabel);
    pEff5->GetXaxis()->SetTitle(xlabelnomu);
    pEff5->GetYaxis()->SetTitleSize(0.04);
    pEff5->GetXaxis()->SetTitleSize(0.04);
    pEff5->GetYaxis()->SetLabelSize(0.04);
    pEff5->GetXaxis()->SetLabelSize(0.04);
    pEff5->GetXaxis()->SetLimits(xmin, xmax);
    auto legend5 = new TLegend(x0, y0, x1, y1);
    legend5->AddEntry(pEff5, "HLT_MET105_IsoTrk50_v*", "");
    legend5->AddEntry(pEff5, "Muon 2022F - pat::Muon", "p");
    legend5->SetLineWidth(0);
    legend5->SetTextSize(0.02559);
    legend5->SetBorderSize(0);
    legend5->SetTextFont(42);
    c5->cd();
    pEff5->Draw("ap");
    legend5->Draw();
    l->Draw();
    CMS_lumi(c5, 0, 0);
    c5->Write();
    pEff5->Write();
    std::string outMET105 = outputDir + "/" + process+ "_HLT_MET105_IsoTrk50_v.pdf";
    c5->Print(outMET105.c_str());

    TGraphAsymmErrors *pEff52 = new TGraphAsymmErrors("120pfmet105");
    pEff52->SetTitle("HLT_MET105_IsoTrk50_v");
    pEff52->Divide(num52, den52);
    pEff52->SetTitle("");
    pEff52->SetMarkerStyle(marker);
    pEff52->GetHistogram()->SetMinimum(ymin);
    pEff52->GetHistogram()->SetMaximum(ymax);
    pEff52->GetYaxis()->SetTitle(ylabel);
    pEff52->GetXaxis()->SetTitle(xlabelnomu);
    pEff52->GetYaxis()->SetTitleSize(0.04);
    pEff52->GetXaxis()->SetTitleSize(0.04);
    pEff52->GetYaxis()->SetLabelSize(0.04);
    pEff52->GetXaxis()->SetLabelSize(0.04);
    pEff52->GetXaxis()->SetLimits(xmin, xmax);
    c52->cd();
    pEff52->Draw("ap");
    legend5->Draw();
    l->Draw();
    CMS_lumi(c52, 0, 0);
    c52->Write();
    pEff52->Write();
    std::string outMET1052 = outputDir + "/" + process+ "_120METCut_HLT_MET105_IsoTrk50_v.pdf";
    c52->Print(outMET1052.c_str());

    f->Close();
    file->Close();
}