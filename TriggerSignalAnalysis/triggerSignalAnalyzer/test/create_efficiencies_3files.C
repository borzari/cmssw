#include "tdrstyle.C"
#include "CMS_lumi.C"

void create_efficiencies_3files()
{
    setTDRStyle();

    gStyle->SetOptStat(0);

    writeExtraText = true;
    extraText = "Simulation";
    lumi_sqrtS = "Signal MC, 13.6 TeV";

    std::string inputFile1 = "Plots_TriggerEffs_10cm/10_hlt_efficiencies.root";
    std::string inputFile2 = "Plots_TriggerEffs_100cm/100_hlt_efficiencies.root";
    std::string inputFile3 = "Plots_TriggerEffs_1000cm/1000_hlt_efficiencies.root";
    TFile *f1 = TFile::Open(inputFile1.c_str());
    TFile *f2 = TFile::Open(inputFile2.c_str());
    TFile *f3 = TFile::Open(inputFile3.c_str());

    TH1 *PFMET110_file1;
    TH1 *PFMET110_file2;
    TH1 *PFMET110_file3;
    f1->GetObject("overallEff;1", PFMET110_file1);
    f2->GetObject("overallEff;1", PFMET110_file2);
    f3->GetObject("overallEff;1", PFMET110_file3);
    
    float ymin = 0.0;
    float ymax = 1.0;
    float x0 = 0.1891;
    float x1 = 0.4395;
    float y0 = 0.8192;
    float y1 = 0.9214;
    std::string legend_file1 = "AMSB - 700 GeV, 10cm";
    std::string legend_file2 = "AMSB - 700 GeV, 100cm";
    std::string legend_file3 = "AMSB - 700 GeV, 1000cm";

    TCanvas *c1 = new TCanvas("c1", "c1", 700, 700);

    std::string outFile = "10vs100vs1000_hlt_effs_comp.root";
    TFile *file = new TFile(outFile.c_str(), "RECREATE");

    c1->cd();
    PFMET110_file1->GetYaxis()->SetTitle("Trigger Efficiency");
    PFMET110_file1->GetXaxis()->SetTitle("100");
    PFMET110_file1->GetYaxis()->SetTitleSize(0.04);
    PFMET110_file1->GetXaxis()->SetTitleSize(0.04);
    PFMET110_file1->GetYaxis()->SetLabelSize(0.04);
    PFMET110_file1->GetXaxis()->SetLabelSize(0.04);
    PFMET110_file1->SetMinimum(ymin);
    PFMET110_file1->SetMaximum(ymax);
    PFMET110_file2->SetMarkerColor(1);
    PFMET110_file1->SetLineColor(1);
    PFMET110_file1->SetLineWidth(2);
    PFMET110_file1->SetLineStyle(1);

    PFMET110_file2->SetMarkerColor(4);
    PFMET110_file2->SetLineColor(4);
    PFMET110_file2->SetLineWidth(2);
    PFMET110_file2->SetLineStyle(2);
    
    PFMET110_file3->SetMarkerColor(2);
    PFMET110_file3->SetLineColor(2);
    PFMET110_file3->SetLineWidth(2);
    PFMET110_file3->SetLineStyle(3);

    auto legend1 = new TLegend(x0, y0, x1, y1);
    legend1->AddEntry(PFMET110_file1, legend_file1.c_str(), "PL");
    legend1->AddEntry(PFMET110_file2, legend_file2.c_str(), "PL");
    legend1->AddEntry(PFMET110_file3, legend_file3.c_str(), "PL");
    legend1->SetLineWidth(0);
    legend1->SetTextSize(0.02559);
    legend1->SetBorderSize(0);
    legend1->SetTextFont(42);
    legend1->SetHeader("HLT_MET105_IsoTrk50_v*");
    PFMET110_file1->Draw();
    PFMET110_file2->Draw("SAME");
    PFMET110_file3->Draw("SAME");
    legend1->Draw();
    CMS_lumi(c1, 0, 0);
    c1->Update();
    c1->Write();
    std::string c1Name = "10vs100vs1000_HLT_MET105_IsoTrk50_v.pdf";
    c1->Print(c1Name.c_str());

    file->Close();
}