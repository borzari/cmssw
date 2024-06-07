#include "tdrstyle.C"
#include "CMS_lumi.C"

void create_efficiencies_DataBG_4files()
{
    setTDRStyle();

    gStyle->SetOptStat(0);

    writeExtraText = true;
    extraText = "Work in Progress";
    lumi_sqrtS = "61.9 fb^{-1}, 13.6 TeV";

    std::string inputFile1 = "Plots_TriggerEffs_Muon2022/Muon2022_hlt_efficiencies.root";
    std::string inputFile2 = "Plots_TriggerEffs_Muon2023/Muon2023_hlt_efficiencies.root";
    std::string inputFile3 = "Plots_TriggerEffs_WToLNu2022All/WToLNu2022All_hlt_efficiencies.root";
    std::string inputFile4 = "Plots_TriggerEffs_WToLNu2023All/WToLNu2023All_hlt_efficiencies.root";
    TFile *f1 = TFile::Open(inputFile1.c_str());
    TFile *f2 = TFile::Open(inputFile2.c_str());
    TFile *f3 = TFile::Open(inputFile3.c_str());
    TFile *f4 = TFile::Open(inputFile4.c_str());

    TGraph *PFMET110_file1;
    TGraph *PFMET110_file2;
    TGraph *PFMET110_file3;
    TGraph *PFMET110_file4;
    f1->GetObject("Graph;1", PFMET110_file1);
    f2->GetObject("Graph;1", PFMET110_file2);
    f3->GetObject("Graph;1", PFMET110_file3);
    f4->GetObject("Graph;1", PFMET110_file4);
    
    float ymin = 0.0;
    float ymax = 1.0;
    float x0 = 0.1891;
    float x1 = 0.4395;
    float y0 = 0.7992;
    float y1 = 0.9214;
    std::string legend_file1 = "Data 2022";
    std::string legend_file2 = "Data 2023";
    std::string legend_file3 = "W->l#nu 2022";
    std::string legend_file4 = "W->l#nu 2023";

    TCanvas *c1 = new TCanvas("c1", "c1", 700, 700);

    std::string outFile = "DatavsBG_hlt_effs_comp.root";
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
    PFMET110_file1->SetMarkerColor(1);
    PFMET110_file1->SetLineColor(1);
    PFMET110_file1->SetMarkerStyle(20);

    PFMET110_file2->SetMarkerColor(2);
    PFMET110_file2->SetLineColor(2);
    PFMET110_file2->SetMarkerStyle(20);
    
    PFMET110_file3->SetMarkerColor(3);
    PFMET110_file3->SetLineColor(3);
    PFMET110_file3->SetMarkerStyle(20);

    PFMET110_file4->SetMarkerColor(4);
    PFMET110_file4->SetLineColor(4);
    PFMET110_file4->SetMarkerStyle(20);

    auto legend1 = new TLegend(x0, y0, x1, y1);
    legend1->AddEntry(PFMET110_file1, legend_file1.c_str(), "P");
    legend1->AddEntry(PFMET110_file2, legend_file2.c_str(), "P");
    legend1->AddEntry(PFMET110_file3, legend_file3.c_str(), "P");
    legend1->AddEntry(PFMET110_file4, legend_file4.c_str(), "P");
    legend1->SetLineWidth(0);
    legend1->SetTextSize(0.02559);
    legend1->SetBorderSize(0);
    legend1->SetTextFont(42);
    legend1->SetHeader("HLT_MET105_IsoTrk50_v*");
    PFMET110_file1->Draw("AP");
    PFMET110_file2->Draw("P,SAME");
    PFMET110_file3->Draw("P,SAME");
    PFMET110_file4->Draw("P,SAME");
    legend1->Draw();
    CMS_lumi(c1, 0, 0);
    c1->Update();
    c1->Write();
    std::string c1Name = "DatavsBG_HLT_MET105_IsoTrk50_v.pdf";
    c1->Print(c1Name.c_str());

    file->Close();
}