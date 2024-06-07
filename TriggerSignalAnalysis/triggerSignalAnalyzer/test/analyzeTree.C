void analyzeTree(){

    TFile *f = new TFile("/afs/cern.ch/work/b/borzari/mergeHistos/hist_merged_passedmuon_TrigAnalysis_Muon2022F.root");
    TTree *t2 = (TTree*)f->Get("triggerDataBGAnalyzer/tree");

    TFile *out = new TFile("passedmuon_Muon2022F.root", "RECREATE");
    TCanvas *c1 = new TCanvas("c1", "c1", 700, 700);
    TCanvas *c2 = new TCanvas("c2", "c2", 700, 700);
    TCanvas *c3 = new TCanvas("c3", "c3", 700, 700);
    TCanvas *c4 = new TCanvas("c4", "c4", 700, 700);
    TCanvas *c5 = new TCanvas("c5", "c5", 700, 700);
    TCanvas *c6 = new TCanvas("c6", "c6", 700, 700);
    TCanvas *c7 = new TCanvas("c7", "c7", 700, 700);

    Int_t hlt_pass_hltMET105Filter;
    Int_t hlt_pass_hltTrk50Filter;
    Int_t hlt_pass_HLT_MET105_IsoTrk50;

    Int_t hlt_pass_HLT_MET120_IsoTrk50;
    Int_t hlt_pass_HLT_PFMET105_IsoTrk50;
    Int_t hlt_pass_HLT_PFMET120_PFMHT120_IDTight;
    Int_t hlt_pass_HLT_PFMET130_PFMHT130_IDTight;
    Int_t hlt_pass_HLT_PFMET140_PFMHT140_IDTight;
    Int_t hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60;
    Int_t hlt_pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF;
    Int_t hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF;
    Int_t hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF;
    Int_t hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF;
    Int_t hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight;
    Int_t hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight;
    Int_t hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight;
    Int_t hlt_pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60;

    // Int_t nMuon;
    Float_t met_ptNoMu;
    Float_t muon_pt;
    TBranch *bhlt_pass_hltMET105Filter = t2->GetBranch("hlt_pass_hltMET105Filter");
    TBranch *bhlt_pass_hltTrk50Filter = t2->GetBranch("hlt_pass_hltTrk50Filter");
    TBranch *bhlt_pass_HLT_MET105_IsoTrk50 = t2->GetBranch("hlt_pass_HLT_MET105_IsoTrk50");

    TBranch *bhlt_pass_HLT_MET120_IsoTrk50 = t2->GetBranch("hlt_pass_HLT_MET120_IsoTrk50");
    TBranch *bhlt_pass_HLT_PFMET105_IsoTrk50 = t2->GetBranch("hlt_pass_HLT_PFMET105_IsoTrk50");
    TBranch *bhlt_pass_HLT_PFMET120_PFMHT120_IDTight = t2->GetBranch("hlt_pass_HLT_PFMET120_PFMHT120_IDTight");
    TBranch *bhlt_pass_HLT_PFMET130_PFMHT130_IDTight = t2->GetBranch("hlt_pass_HLT_PFMET130_PFMHT130_IDTight");
    TBranch *bhlt_pass_HLT_PFMET140_PFMHT140_IDTight = t2->GetBranch("hlt_pass_HLT_PFMET140_PFMHT140_IDTight");
    TBranch *bhlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60 = t2->GetBranch("hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60");
    TBranch *bhlt_pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF = t2->GetBranch("hlt_pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF");
    TBranch *bhlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF = t2->GetBranch("hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF");
    TBranch *bhlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF = t2->GetBranch("hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF");
    TBranch *bhlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF = t2->GetBranch("hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF");
    TBranch *bhlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight = t2->GetBranch("hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight");
    TBranch *bhlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight = t2->GetBranch("hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight");
    TBranch *bhlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight = t2->GetBranch("hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight");
    TBranch *bhlt_pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60 = t2->GetBranch("hlt_pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60");

    // TBranch *bnMuon = t2->GetBranch("nMuon");
    TBranch *bmet_ptNoMu = t2->GetBranch("met_ptNoMu");
    TBranch *bmuon_pt = t2->GetBranch("muon_pt");
    bhlt_pass_hltMET105Filter->SetAddress(&hlt_pass_hltMET105Filter);
    bhlt_pass_hltTrk50Filter->SetAddress(&hlt_pass_hltTrk50Filter);
    bhlt_pass_HLT_MET105_IsoTrk50->SetAddress(&hlt_pass_HLT_MET105_IsoTrk50);

    bhlt_pass_HLT_MET120_IsoTrk50->SetAddress(&hlt_pass_HLT_MET120_IsoTrk50);
    bhlt_pass_HLT_PFMET105_IsoTrk50->SetAddress(&hlt_pass_HLT_PFMET105_IsoTrk50);
    bhlt_pass_HLT_PFMET120_PFMHT120_IDTight->SetAddress(&hlt_pass_HLT_PFMET120_PFMHT120_IDTight);
    bhlt_pass_HLT_PFMET130_PFMHT130_IDTight->SetAddress(&hlt_pass_HLT_PFMET130_PFMHT130_IDTight);
    bhlt_pass_HLT_PFMET140_PFMHT140_IDTight->SetAddress(&hlt_pass_HLT_PFMET140_PFMHT140_IDTight);
    bhlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60->SetAddress(&hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60);
    bhlt_pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF->SetAddress(&hlt_pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF);
    bhlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF->SetAddress(&hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF);
    bhlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF->SetAddress(&hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF);
    bhlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF->SetAddress(&hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF);
    bhlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight->SetAddress(&hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight);
    bhlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight->SetAddress(&hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight);
    bhlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight->SetAddress(&hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight);
    bhlt_pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60->SetAddress(&hlt_pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60);

    // bnMuon->SetAddress(&nMuon);
    bmet_ptNoMu->SetAddress(&met_ptNoMu);
    bmuon_pt->SetAddress(&muon_pt);  

    TH1F *hmuon_pt = new TH1F("hmuon_pt","Muon pt",100,0.0,1000.0);
    TH1F *hmet_pt = new TH1F("hmet_pt","MET pt",100,0.0,1000.0);
    TH1F *hmuon_pt_hlt = new TH1F("hmuon_pt_hlt","Muon pt",100,0.0,1000.0);
    TH1F *hmet_pt_hlt = new TH1F("hmet_pt_hlt","MET pt",100,0.0,1000.0);
    TH1F *hmet_pt_hltGrandOR = new TH1F("hmet_pt_hltGrandOR","MET pt",100,0.0,1000.0);
    TH1F *hmet_pt_hltGrandOR_except = new TH1F("hmet_pt_hltGrandOR_except","MET pt",100,0.0,1000.0);
    TH1F *hmuon_pt_hltFilterMet = new TH1F("hmuon_pt_hltFilterMet","Muon pt",100,0.0,1000.0);
    TH1F *hmet_pt_hltFilterMet = new TH1F("hmet_pt_hltFilterMet","MET pt",100,0.0,1000.0);
    TH1F *hmuon_pt_hltFilterTrk = new TH1F("hmuon_pt_hltFilterTrk","Muon pt",100,0.0,1000.0);
    Int_t nentries = (Int_t)t2->GetEntries();

    for (Int_t i=0;i<nentries;i++) {
        bhlt_pass_hltMET105Filter->GetEntry(i);
        bhlt_pass_hltTrk50Filter->GetEntry(i);
        bhlt_pass_HLT_MET105_IsoTrk50->GetEntry(i);

        bhlt_pass_HLT_MET120_IsoTrk50->GetEntry(i);
        bhlt_pass_HLT_PFMET105_IsoTrk50->GetEntry(i);
        bhlt_pass_HLT_PFMET120_PFMHT120_IDTight->GetEntry(i);
        bhlt_pass_HLT_PFMET130_PFMHT130_IDTight->GetEntry(i);
        bhlt_pass_HLT_PFMET140_PFMHT140_IDTight->GetEntry(i);
        bhlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60->GetEntry(i);
        bhlt_pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF->GetEntry(i);
        bhlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF->GetEntry(i);
        bhlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF->GetEntry(i);
        bhlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF->GetEntry(i);
        bhlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight->GetEntry(i);
        bhlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight->GetEntry(i);
        bhlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight->GetEntry(i);
        bhlt_pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60->GetEntry(i);

        // bnMuon->GetEntry(i);
        bmet_ptNoMu->GetEntry(i);
        bmuon_pt->GetEntry(i);

        hmuon_pt->Fill(muon_pt);
        hmet_pt->Fill(met_ptNoMu);

        if(hlt_pass_HLT_MET105_IsoTrk50 == 1 ||
           hlt_pass_HLT_MET120_IsoTrk50 == 1 || 
           hlt_pass_HLT_PFMET105_IsoTrk50 == 1 || 
           hlt_pass_HLT_PFMET120_PFMHT120_IDTight == 1 || 
           hlt_pass_HLT_PFMET130_PFMHT130_IDTight == 1 || 
           hlt_pass_HLT_PFMET140_PFMHT140_IDTight == 1 || 
           hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60 == 1 || 
           hlt_pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF == 1 || 
           hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF == 1 || 
           hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF == 1 || 
           hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF == 1 || 
           hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight == 1 || 
           hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight == 1 || 
           hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight == 1 || 
           hlt_pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60 == 1){hmet_pt_hltGrandOR->Fill(met_ptNoMu);}

        if(hlt_pass_HLT_MET105_IsoTrk50 == 1 ||
           hlt_pass_HLT_MET120_IsoTrk50 == 1 || 
           hlt_pass_HLT_PFMET105_IsoTrk50 == 1 || 
           hlt_pass_HLT_PFMET120_PFMHT120_IDTight == 1 || 
           hlt_pass_HLT_PFMET130_PFMHT130_IDTight == 1 || 
           hlt_pass_HLT_PFMET140_PFMHT140_IDTight == 1 || 
           hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60 == 1 || 
           hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF == 1 || 
           hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF == 1 || 
           hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF == 1 || 
           hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight == 1 || 
           hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight == 1 || 
           hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight == 1 || 
           hlt_pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60 == 1){hmet_pt_hltGrandOR_except->Fill(met_ptNoMu);}

        if(hlt_pass_HLT_MET105_IsoTrk50 == 1) {hmuon_pt_hlt->Fill(muon_pt); hmet_pt_hlt->Fill(met_ptNoMu);}
        if(hlt_pass_hltMET105Filter == 1) {
            hmuon_pt_hltFilterMet->Fill(muon_pt);
            hmet_pt_hltFilterMet->Fill(met_ptNoMu);
            if(hlt_pass_hltTrk50Filter == 1) hmuon_pt_hltFilterTrk->Fill(muon_pt);
        }
    }

    TH1 * h1 = (TH1*) hmuon_pt_hlt->Clone();
    h1->Divide(hmuon_pt);
    h1->SetTitle("Muon Pt - HLT_MET105_IsoTrk50");

    TH1 * h2 = (TH1*) hmuon_pt_hltFilterMet->Clone();
    h2->Divide(hmuon_pt);
    h2->SetTitle("Muon Pt - hltMET105");

    TH1 * h3 = (TH1*) hmuon_pt_hltFilterTrk->Clone();
    h3->Divide(hmuon_pt_hltFilterMet);
    h3->SetTitle("Muon Pt - hltTrk50Filter");

    TH1 * h4 = (TH1*) hmet_pt_hlt->Clone();
    h4->Divide(hmet_pt);
    h4->SetTitle("MET Pt - HLT_MET105_IsoTrk50");

    TH1 * h5 = (TH1*) hmet_pt_hltFilterMet->Clone();
    h5->Divide(hmet_pt);
    h5->SetTitle("MET Pt - hltMET105");

    TH1 * h6 = (TH1*) hmet_pt_hltGrandOR->Clone();
    h6->Divide(hmet_pt);
    h6->SetTitle("MET Pt - GrandOR");

    TH1 * h7 = (TH1*) hmet_pt_hltGrandOR_except->Clone();
    h7->Divide(hmet_pt);
    h7->SetTitle("MET Pt - GrandOR");

    c1->cd();
    h1->Draw();
    c1->Update();
    c1->Write();
    h1->Write();

    c2->cd();
    h2->Draw();
    c2->Update();
    c2->Write();
    h2->Write();

    c3->cd();
    h3->Draw();
    c3->Update();
    c3->Write();
    h3->Write();

    c4->cd();
    h4->Draw();
    c4->Update();
    c4->Write();
    h4->Write();

    c5->cd();
    h5->Draw();
    c5->Update();
    c5->Write();
    h5->Write();

    c6->cd();
    h6->Draw();
    c6->Update();
    c6->Write();
    h6->Write();

    c7->cd();
    h7->Draw();
    c7->Update();
    c7->Write();
    h7->Write();

    out->Close();

}