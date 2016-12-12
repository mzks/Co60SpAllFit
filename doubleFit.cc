// -------------------------------- //
// doubleFit.cc - root macro
// to Draw and fit pocket mca data
//
// Usage:$root -b doubleFit.cc
//  in directory including mca file 
//
// Author: Mizukoshi Keita
// 2016. Dec, 12 
// -------------------------------- //

using namespace std;
int ps(TString filename);

int doubleFit(){

	// FOOL file search
	// set for your own files
	// For file like "live_data1204_50on.mca" 
	// Fix for your data
	const string HEADER = "live_data1204_";
	const string FOOTER1 = "on.mca";
	const string FOOTER2 = "off.mca";
	
	int result = 0;
	stringstream ss;

	for(int i=0;i<100;i++){
		//stringstream init
		ss.str("");
		ss.clear(stringstream::goodbit);
		ss << HEADER << i << FOOTER1;
		cout << ss.str() << endl;
		if(ps(ss.str())==0) result++;
	}
			                                             
	for(int i=0;i<100;i++){
		//stringstream init
		ss.str("");
		ss.clear(stringstream::goodbit);
		ss << HEADER << i << FOOTER2;
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

	gStyle->SetOptFit();
	gStyle->SetOptFit(1111);

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
	Int_t maxpeaks = 6;
	TSpectrum *spectrum = new TSpectrum(maxpeaks);	
	spectrum->Search(hist1,3,"new");
	//TH1* back = spectrum->Background(hist1,20,"Compton");
	//TCanvas* c2 = new TCanvas;
	//back->Draw("same");

	Double_t *xpeaks = spectrum->GetPositionX();
	Double_t *ypeaks = spectrum->GetPositionY();

	double peakX1333 = 0;
	double peakX1173 = 0;
	double peakY1333;
	double peakY1173;
	for(int i=0;i<sizeof(xpeaks);i++){
		if(xpeaks[i] > peakX1333 && xpeaks[i] < 2000.0){
			//peakX1173 = peakX1333;
			//peakY1173 = peakY1333;
			peakX1333 = xpeaks[i];
			peakY1333 = ypeaks[i];
		}
	}
	double disX;
	double disP;
	for(int i=0;i<sizeof(xpeaks);i++){
		disX =(peakX1333 *1173/1333 - xpeaks[i])*(peakX1333 *1173/1333 - xpeaks[i]);
		disP =(peakX1333 *1173/1333 - peakX1173)*(peakX1333 *1173/1333 - peakX1173);
		if( disX < disP  ){
			peakX1173 = xpeaks[i];
			peakY1173 = ypeaks[i];
		}
	}
	//cout << "Point1333: " << peakX1333 << ":" << peakY1333 << endl;
	//cout << "Point1173: " << peakX1173 << ":" << peakY1173 << endl;
	double MaxRangeFit = 1.1 * peakX1333;
	double MinRangeFit = 0.9 * peakX1173;

	//Insert Fit model here.
	//Double gaussian
	TF1* doubleGauss = new TF1("doubleGauss"," [0]*exp(-0.5*((x-[1])/[2])**2)+ [3]*exp(-0.5*((x-([1]/1.3325*1.1732))/[4])**2)",MinRangeFit,MaxRangeFit);//name,func,min,max

	//Set 5 parameters
	doubleGauss->SetParName(0,"const1333");
	doubleGauss->SetParameter(0,peakY1333);
	doubleGauss->SetParLimits(0,peakY1333*0.9,peakY1333*1.1);

	doubleGauss->SetParName(1,"Mean1333");
	doubleGauss->SetParameter(1,peakX1333);
	doubleGauss->SetParLimits(1,peakX1333*0.99,peakX1333*1.01);

	doubleGauss->SetParName(2,"sigma1333");
	doubleGauss->SetParameter(2,10.0);
	doubleGauss->SetParLimits(2,1.0,60.0);

	doubleGauss->SetParName(3,"Const1173");
	doubleGauss->SetParameter(3,peakY1173);
	doubleGauss->SetParLimits(3,peakY1173*0.9,peakY1173*1.1);

	doubleGauss->SetParName(4,"sigma1173");
	doubleGauss->SetParameter(4,10.0);
	doubleGauss->SetParLimits(4,1.0,60.0);;

	hist1->Fit("doubleGauss","","",MinRangeFit,MaxRangeFit);


	// Output Fit parameter and error
	ofstream ofs;
	ofs.open("fit.txt",ios::app);
	ofs << filename  <<" " << doubleGauss->GetParameter(1)  << " " << doubleGauss->GetParError(1) << endl;
	ofs << peakX1333 << endl;
	ofs.close();

	// Save as pdf
	filename = filename + ".pdf";
	c1->SaveAs(filename);
	delete hist1;
	return 0;
}
