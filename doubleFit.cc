// -------------------------------- //
// doubleFit.C - root macro
// to Draw and fit pocket mca data
//
// Usage:
//
// Author: Mizukoshi Keita
// 2016. Dec, 12 
// -------------------------------- //

using namespace std;
int ps(TString filename);

int doubleFit(){
	const string HEADER = "live_data1204_";
	const string FOOTER = "on.mca";
	//const string FOOTER = "off.mca";
	int result = 0;
	stringstream ss;

	for(int i=0;i<100;i++){
		ss.str("");
		ss.clear(stringstream::goodbit);
		ss << HEADER << i << FOOTER;
		cout << ss.str() << endl;
		if(ps(ss.str())==0) result++;
	}
	cout << "RESULT:" << result << endl;
	return 0;

}

int ps(TString filename)
{
	const Int_t MCA_CHANNEL = 2048;
	const int gomiLineHead = 12;
	const Double_t HIST_MIN = 0.0;
	const Double_t HIST_MAX = 2048;

	ifstream ifs(filename);
	// for calibration
	// y = a*x + b
	Double_t a = 1.0;
	Double_t b = 0.0;

	TH1D *hist1 = new TH1D("h",filename , MCA_CHANNEL, HIST_MIN, HIST_MAX);


	Double_t x;
	Int_t y;
	Double_t j=0;
	string buffer;
	int readLine = 0;

	//Fill
	while(getline(ifs,buffer)){
		if((readLine >= gomiLineHead ) && (readLine < (MCA_CHANNEL+gomiLineHead))){
			try{
				y = stoi(buffer);
			}catch(const std::invalid_argument& e) {
				puts(e.what()); // "stoi: no conversion"
				puts("stoi failed (Maybe gomiLine(l.12)Error)");
				return 1;
			}

			//for calibration
			x=a*j+b;

			// Fill
			while(y>0){
				hist1->Fill(x);
				y--;
			}
			j++;
		}
		readLine++;
	}
	if(hist1->GetEntries() == 0){
		return -2;
	}

	hist1->Rebin(4);
	TCanvas *c1 = new TCanvas;
	// Statics box option
	// draw only entries
	gStyle->SetOptStat("e");
	hist1->SetXTitle("MCA channel");
	hist1->SetYTitle("count");
	hist1->Draw();

	// Peak Search
	Int_t maxpeaks = 5;
	TSpectrum *spectrum = new TSpectrum(maxpeaks);	
	spectrum->Search(hist1,1,"new");
	//TH1* back = spectrum->Background(hist1,20,"Compton");
	//TCanvas* c2 = new TCanvas;
	//back->Draw("same");

	Double_t *xpeaks = spectrum->GetPositionX();
	Double_t *ypeaks = spectrum->GetPositionY();

	//for(int i=0;i<sizeof(xpeaks);i++){
	//	cout << filename << " "  << xpeaks[i] << " " << ypeaks[i] << endl;
	//}

	// Save as pdf
	filename = filename + ".pdf";
	c1->SaveAs(filename);
	delete hist1;
	return 0;
}
