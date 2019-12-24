#include <CtrlLib/CtrlLib.h>
#include <Controls4U/Controls4U.h>
#include <ScatterCtrl/ScatterCtrl.h>
#include <GLCanvas/GLCanvas.h>
#include <RasterPlayer/RasterPlayer.h>
#include <CtrlScroll/CtrlScroll.h>

#include <BEMRosetta/BEMRosetta_cl/BEMRosetta.h>

using namespace Upp;

#define IMAGECLASS Img2
#define IMAGEFILE <BEMRosetta/BEMRosetta/main.iml>
#include <Draw/iml.h>

#include "main.h"
#include "clip.brc"

void MainBEM::Init() {
	CtrlLayout(*this);
	
	CtrlLayout(menuOpen);
	menuOpen.file.WhenChange = THISBACK(OnLoad);
	menuOpen.file.BrowseRightWidth(40).UseOpenFolder().BrowseOpenFolderWidth(10);
	menuOpen.butLoad.WhenAction = [&] {menuOpen.file.DoGo();};
	
	ArrayModel_Init(menuOpen.arrayModel).MultiSelect();
	
	menuOpen.butRemove.Disable();	
	menuOpen.butRemove <<= THISBACK(OnRemove);
	menuOpen.butRemoveSelected.Disable();	
	menuOpen.butRemoveSelected <<= THISBACK1(OnRemoveSelected, false);
	menuOpen.butJoin.Disable();	
	menuOpen.butJoin <<= THISBACK(OnJoin);
	menuOpen.butSymmetrize.Disable();	
	menuOpen.butSymmetrize <<= THISBACK(OnSymmetrize);
	menuOpen.butA0.Disable();	
	menuOpen.butA0 <<= THISBACK(OnA0);
	menuOpen.butAinf.Disable();	
	menuOpen.butAinf <<= THISBACK(OnAinf);
	menuOpen.butDescription.Disable();
	menuOpen.butDescription <<= THISBACK(OnDescription);
	
	CtrlLayout(menuConvert);
	menuConvert.file.WhenChange = THISBACK(OnConvert);
	menuConvert.file.BrowseRightWidth(40).UseOpenFolder(true).BrowseOpenFolderWidth(10);
	menuConvert.butLoad.WhenAction = [&] {OnConvert();};

	ArrayModel_Init(menuConvert.arrayModel);
	
	menuConvert.arrayModel.WhenCursor = THISBACK(OnMenuConvertArraySel);
	menuConvert.opt.WhenAction = [&] {OnOpt();};
	
	OnOpt();
	
	CtrlLayout(menuPlot);
	menuPlot.butZoomToFit.WhenAction = [&] {GetSelScatter().ZoomToFit(true, true);};
	menuPlot.autoFit.WhenAction 	 = [&] {
		LoadSelTab(Bem());
		menuPlot.fromY0.Enable(~menuPlot.autoFit);
	};
	menuPlot.fromY0.WhenAction 	 	 = [&] {LoadSelTab(Bem());};
	menuPlot.opwT.WhenAction 	 	 = [&] {LoadSelTab(Bem());};
	menuPlot.showPoints.WhenAction 	 = [&] {LoadSelTab(Bem());};
	menuPlot.showNdim.WhenAction 	 = [&] {LoadSelTab(Bem());};
	
	OnOpt();
	
	menuFOAMM.Init(*this, Bem(), mainSetupFOAMM);
	
	OnOpt();
		
	menuTab.Add(menuOpen.SizePos(), 	t_("Load"));
	menuTab.Add(menuConvert.SizePos(), 	t_("Save as")).Disable();
	menuTab.Add(menuPlot.SizePos(), 	t_("Plot")).Disable();
	menuTab.Add(menuFOAMM.SizePos(), 	t_("FOAMM State Space")).Disable();
	
	menuTab.WhenSet = [&] {
		LOGTAB(menuTab);
		bool setupfoamm = false;
		if (menuTab.IsAt(menuFOAMM)) {
			setupfoamm = true;
			if (!FileExists(Bem().foammPath))
				Status(t_("FOAMM not found. Please set FOAMM path in Options"), 10000);	
		} else if (menuTab.IsAt(menuPlot)) 
			setupfoamm = true;
		
		if (!setupfoamm) 
			mainTab.Set(0);
		
		if (menuTab.IsAt(menuFOAMM)) 
			mainTab.Set(mainSetupFOAMM); 
	};

	mainTab.WhenSet = [&] {
		LOGTAB(mainTab);
		Vector<int> ids = ArrayModel_IdsHydro(menuOpen.arrayModel);
		bool plot = true, convertProcess = true, ismenuFOAMM = false;
		menuPlot.opwT.Enable();
		if (ids.IsEmpty())
			plot = convertProcess = false;
		else if (mainTab.IsAt(mainStiffness)) {
			plot = false;
			mainStiffness.Load(Bem().hydros, ids);
		} else if (mainTab.IsAt(mainA))
			mainA.Load(Bem(), ids);
		else if (mainTab.IsAt(mainB))
			mainB.Load(Bem(), ids);
		else if (mainTab.IsAt(mainForceSC))
			mainForceSC.Load(Bem(), ids);
		else if (mainTab.IsAt(mainForceFK))
			mainForceFK.Load(Bem(), ids);
		else if (mainTab.IsAt(mainForceEX))
			mainForceEX.Load(Bem(), ids);
		else if (mainTab.IsAt(mainRAO))
			mainRAO.Load(Bem(), ids);
		else if (mainTab.IsAt(mainStateSpace)) {
			mainStateSpace.Load(Bem(), ids);
			ismenuFOAMM = true;
		} else if (mainTab.IsAt(mainSetupFOAMM)) {
			menuPlot.opwT.Disable();
			menuTab.Set(menuFOAMM);
			ismenuFOAMM = true;
		} else if (menuTab.IsAt(menuFOAMM)) 
			;
		else 
			plot = false;
		
		TabCtrl::Item& tabMenuPlot = menuTab.GetItem(menuTab.Find(menuPlot));
		tabMenuPlot.Enable(plot);
		TabCtrl::Item& tabMenuConvert = menuTab.GetItem(menuTab.Find(menuConvert));
		tabMenuConvert.Enable(convertProcess);
		if (plot) 
			tabMenuPlot.Text(t_("Plot"));
		else 
			tabMenuPlot.Text("");

		if (convertProcess) 
			tabMenuConvert.Text(t_("Save as"));
		else 
			tabMenuConvert.Text("");
		
		TabCtrl::Item& tabMenuFOAMM = menuTab.GetItem(menuTab.Find(menuFOAMM));
		tabMenuFOAMM.Enable(convertProcess);
		if (convertProcess) 
			tabMenuFOAMM.Text(t_("FOAMM State Space"));
		else 
			tabMenuFOAMM.Text("");
		
		if (!ismenuFOAMM && plot && (menuTab.IsAt(menuFOAMM) || menuTab.IsAt(mainSetupFOAMM))) 
			menuTab.Set(menuPlot);
	};
	mainTab.WhenSet();
	
	menuTab.WhenSet = [&] {
	LOGTAB(menuTab);
		if (menuTab.IsAt(menuConvert)) 
			menuConvert.arrayModel.WhenCursor();
	};
	
	mainSummary.Init();
	mainTab.Add(mainSummary.SizePos(), t_("Summary"));
	
	mainArrange.Init();
	mainTab.Add(mainArrange.SizePos(), t_("Arrange DOF")).Disable();
	
	mainStiffness.Init();
	mainTab.Add(mainStiffness.SizePos(), t_("K")).Disable();
	
	mainA.Init(DATA_A);
	mainTab.Add(mainA.SizePos(), t_("A")).Disable();
	
	mainB.Init(DATA_B);
	mainTab.Add(mainB.SizePos(), t_("B")).Disable();

	mainForceEX.Init(DATA_FORCE_EX);
	mainTab.Add(mainForceEX.SizePos(), t_("Fex")).Disable();
	
	mainForceSC.Init(DATA_FORCE_SC);
	mainTab.Add(mainForceSC.SizePos(), t_("Fsc")).Disable();
	
	mainForceFK.Init(DATA_FORCE_FK);
	mainTab.Add(mainForceFK.SizePos(), t_("Ffk")).Disable();
	
	mainRAO.Init(DATA_RAO);
	mainTab.Add(mainRAO.SizePos(), t_("RAO")).Disable();

	mainSetupFOAMM.Init();
	mainTab.Add(mainSetupFOAMM.SizePos(), t_("Setup FOAMM")).Disable();

	mainStateSpace.Init();
	mainTab.Add(mainStateSpace.SizePos(), t_("State Space")).Disable();
}

void MainBEM::OnMenuConvertArraySel() {
	int id = ArrayModel_IdHydro(menuConvert.arrayModel);
	if (id < 0)
		return;
	
	String file = ~menuConvert.file;
	String folder = GetFileFolder(file);
	String ext = GetFileExt(file);
	String fileName = GetFileTitle(ArrayModel_GetFileName(menuConvert.arrayModel));
	file = AppendFileName(folder, fileName + ext);
	menuConvert.file <<= file;
}

void MainBEM::InitSerialize(bool ret) {
	if (!ret || IsNull(menuPlot.autoFit)) 
		menuPlot.autoFit = true;
	
	if (!ret || IsNull(menuPlot.fromY0)) 
		menuPlot.fromY0 = false;
	
	if (!ret || IsNull(menuPlot.opwT)) 
		menuPlot.opwT = 0;

	if (!ret || IsNull(menuPlot.showPoints)) 
		menuPlot.showPoints = true;
	
	if (!ret || IsNull(menuPlot.showNdim)) 
		menuPlot.showNdim = false;

	if (!ret || IsNull(menuConvert.opt)) 
		menuConvert.opt = 0;
}

void MainBEM::LoadSelTab(BEMData &bem) {
	Vector<int> ids = ArrayModel_IdsHydro(menuOpen.arrayModel);
	int id = mainTab.Get();
	if (id == mainTab.Find(mainStateSpace))
		mainStateSpace.Load(bem, ids);
	else if (id == mainTab.Find(mainStiffness))
		mainStiffness.Load(bem.hydros, ids);
	else if (id == mainTab.Find(mainSummary) || id == mainTab.Find(mainArrange))
		;
	else if (id == mainTab.Find(mainSetupFOAMM))
		mainSetupFOAMM.Load();
	else 
		GetSelABForce().Load(bem, ids);
}

MainABForce &MainBEM::GetSelABForce() {
	int id = mainTab.Get();
	Ctrl *ctrl = mainTab.GetItem(id).GetSlave();
	if (!ctrl)
		throw Exc(t_("Object not found in GetSelABForce()"));
	if (typeid(MainABForce) != typeid(*ctrl))
		throw Exc(t_("Unexpected type in GetSelABForce()"));
	return *(static_cast<MainABForce*>(ctrl));
}

MainStateSpace &MainBEM::GetSelStateSpace() {
	int id = mainTab.Get();
	Ctrl *ctrl = mainTab.GetItem(id).GetSlave();
	if (!ctrl)
		throw Exc(t_("Object not found in GetSelStateSpace()"));
	if (typeid(MainStateSpace) != typeid(*ctrl))
		throw Exc(t_("Unexpected type in GetSelStateSpace()"));
	return *(static_cast<MainStateSpace*>(ctrl));
}

ScatterCtrl &MainBEM::GetSelScatter() {
	int id = mainTab.Get();
	if (id == mainTab.Find(mainStateSpace)) {
		TabCtrl &tab = GetSelStateSpace().tab;
		Ctrl *ctrl = tab.GetItem(tab.Get()).GetSlave();
		if (!ctrl)
			throw Exc(t_("Object not found in GetSelScatter(1)"));
		if (typeid(MainStateSpacePlot) != typeid(*ctrl))
			throw Exc(t_("Unexpected type in GetSelScatter(1)"));		
		MainStateSpacePlot *mainStateSpacePlot = static_cast<MainStateSpacePlot*>(ctrl);
		return mainStateSpacePlot->mainPlot.scatt;
	} else {
		TabCtrl &tab = GetSelABForce().tab;
		Ctrl *ctrl = tab.GetItem(tab.Get()).GetSlave();
		if (!ctrl)
			throw Exc(t_("Object not found in GetSelScatter(2)"));
		if (typeid(MainPlot) != typeid(*ctrl))
			throw Exc(t_("Unexpected type in GetSelScatter(2)"));		
		MainPlot *mainPlot = static_cast<MainPlot*>(ctrl);
		return mainPlot->scatt;
	}
}

void MainBEM::OnOpt() {
	menuOpen.file.ClearTypes();

	menuOpen.file.Type(Format(t_("All supported BEM files (%s)"), Bem().bemFilesExt), Bem().bemFilesAst);
	menuOpen.file.AllFilesType();
	String extOpen = ToLower(GetFileExt(menuOpen.file.GetData().ToString()));
	if (extOpen.IsEmpty())
		menuOpen.file.ActiveType(0);
	else if (Bem().bemFilesExt.Find(extOpen) >= 0)
		menuOpen.file.ActiveType(0);
	else
		menuOpen.file.ActiveType(1);
	
	menuConvert.file.ClearTypes();
	switch (menuConvert.opt) {
	case 0:	menuConvert.file <<= ForceExtSafe(~menuConvert.file, ".1"); 	
			menuConvert.file.Type(t_("Wamit .1.3.hst file"), "*.1 *.3 *.hst");
			break;
	case 1:	menuConvert.file <<= ForceExtSafe(~menuConvert.file, ".dat"); 
			menuConvert.file.Type(t_("FAST HydroDyn file"), "*.dat");
			break;
	case 2:	menuConvert.file <<= ForceExtSafe(~menuConvert.file, ".bem"); 
			menuConvert.file.Type(t_("BEMRosetta file"), "*.bem");
			break;
	default:menuConvert.file.Type(t_("All converted files"), "*.1 *.3 *.hst *.dat *.bem");
			break;
	}
	String extConv = ToLower(GetFileExt(menuConvert.file.GetData().ToString()));
	if (extConv.IsEmpty())
		extConv = "-";
	if (String(".1 .3 .hst").Find(extConv) >= 0)
		menuConvert.file.ActiveType(0);
	else if (String(".dat").Find(extConv) >= 0)
		menuConvert.file.ActiveType(1);
	else
		menuConvert.file.ActiveType(2);
	
	/*menuFOAMM.file.ClearTypes();
	menuFOAMM.file.Type(t_("Maynooth COER FOAMM file *.mat"), "*.mat");
	menuFOAMM.file.AllFilesType();
	String extFOAMM = ToLower(GetFileExt(menuFOAMM.file.GetData().ToString()));
	if (extFOAMM.IsEmpty())
		menuFOAMM.file.ActiveType(0);
	else if (extFOAMM == ".mat")
		menuFOAMM.file.ActiveType(0);
	else
		menuFOAMM.file.ActiveType(1);*/	
}

bool MainBEM::OnLoad() {
	String file = ~menuOpen.file;
	return OnLoadFile(file);
}

bool MainBEM::OnLoadFile(String file) {
	GuiLock __;
	try {
		WaitCursor wait;
		Progress progress(t_("Loading BEM files..."), 100); 
		
		for (int i = 0; i < Bem().hydros.GetCount(); ++i) {
			if (Bem().hydros[i].hd().file == file) {
				if (!PromptYesNo(t_("Model is already loaded") + S("&") + t_("Do you wish to open it anyway?")))
					return false;
				break;
			}
		}
		
		Bem().Load(file, [&](String str, int _pos) {
			progress.SetText(str); 
			progress.SetPos(_pos); 
			return !progress.Canceled();
		}, false);
		
		int id = Bem().hydros.GetCount()-1;
		HydroClass &data = Bem().hydros[id];
		
		data.hd().Report();
		mainSummary.Report(data.hd(), id);
		if (data.hd().Nf < 0)
			return false;
		
		ArrayModel_Add(menuOpen.arrayModel, data.hd().GetCodeStr(), data.hd().name, data.hd().file, data.hd().GetId());
		int numrow = menuOpen.arrayModel.GetCount();
		menuOpen.butRemove.Enable(numrow > 0);
		menuOpen.butRemoveSelected.Enable(numrow > 0);
		menuOpen.butJoin.Enable(numrow > 1);
		menuOpen.butSymmetrize.Enable(numrow == 1);
		menuOpen.butA0.Enable(numrow == 1);
		menuOpen.butAinf.Enable(numrow == 1);
		menuOpen.butDescription.Enable(numrow == 1);
		
		ArrayModel_Add(menuConvert.arrayModel, data.hd().GetCodeStr(), data.hd().name, data.hd().file, data.hd().GetId());
		if (ArrayModel_IdHydro(menuConvert.arrayModel) < 0)
			menuConvert.arrayModel.SetCursor(0);
		ArrayModel_Add(menuFOAMM.arrayModel, data.hd().GetCodeStr(), data.hd().name, data.hd().file, data.hd().GetId());
		if (ArrayModel_IdHydro(menuFOAMM.arrayModel) < 0)
			menuFOAMM.arrayModel.SetCursor(0);

		Vector<int> ids = ArrayModel_IdsHydro(menuOpen.arrayModel);
		
		mainArrange.Load(Bem().hydros, ids);	
		mainTab.GetItem(mainTab.Find(mainArrange)).Enable(true);	
		mainTab.GetItem(mainTab.Find(mainStiffness)).Enable(mainStiffness.Load(Bem().hydros, ids));
		mainTab.GetItem(mainTab.Find(mainA)).Enable(mainA.Load(Bem(), ids));	
		mainTab.GetItem(mainTab.Find(mainB)).Enable(mainB.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainForceSC)).Enable(mainForceSC.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainForceFK)).Enable(mainForceFK.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainForceEX)).Enable(mainForceEX.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainRAO)).Enable(mainRAO.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainSetupFOAMM)).Enable(true);
		if (data.hd().IsLoadedStateSpace())
			mainTab.GetItem(mainTab.Find(mainStateSpace)).Enable(true);
		
		mainTab.WhenSet();
	} catch (Exc e) {
		Exclamation(DeQtfLf(e));
		return false;
	}
	return true;
}

void MainBEM::OnRemove() {
	OnRemoveSelected(true);
}

void MainBEM::OnRemoveSelected(bool all) {
	bool selected = false;
	for (int row = menuOpen.arrayModel.GetCount()-1; row >= 0; --row) {
		if (all || menuOpen.arrayModel.IsSelected(row)) {
			int id = ArrayModel_IdHydro(menuOpen.arrayModel, row);
			Bem().hydros.Remove(id);
			mainArrange.Remove(row);
			menuOpen.arrayModel.Remove(row);
			menuConvert.arrayModel.Remove(row);
			menuFOAMM.arrayModel.Remove(row);
			selected = true;
		}
	}
	if (!selected) {
		Exclamation(t_("No model selected"));
		return;
	}
 	mainSummary.Clear();
	for (int i = 0; i < Bem().hydros.GetCount(); ++i)
		mainSummary.Report(Bem().hydros[i].hd(), i);
	
	int numrow = menuOpen.arrayModel.GetCount();
	menuOpen.butRemove.Enable(numrow > 0);
	menuOpen.butRemoveSelected.Enable(numrow > 0);
	menuOpen.butJoin.Enable(numrow > 1);
	menuOpen.butSymmetrize.Enable(numrow == 1);
	menuOpen.butA0.Enable(numrow == 1);
	menuOpen.butAinf.Enable(numrow == 1);
	menuOpen.butDescription.Enable(numrow == 1);
	
	Vector<int> ids = ArrayModel_IdsHydro(menuOpen.arrayModel);
	
	mainArrange.Load(Bem().hydros, ids);	
	mainTab.GetItem(mainTab.Find(mainArrange)).Enable(ids.GetCount() > 0);	
	mainTab.GetItem(mainTab.Find(mainStiffness)).Enable(mainStiffness.Load(Bem().hydros, ids));
	mainTab.GetItem(mainTab.Find(mainA)).Enable(mainA.Load(Bem(), ids));	
	mainTab.GetItem(mainTab.Find(mainB)).Enable(mainB.Load(Bem(), ids));
	mainTab.GetItem(mainTab.Find(mainForceSC)).Enable(mainForceSC.Load(Bem(), ids));
	mainTab.GetItem(mainTab.Find(mainForceFK)).Enable(mainForceFK.Load(Bem(), ids));
	mainTab.GetItem(mainTab.Find(mainForceEX)).Enable(mainForceEX.Load(Bem(), ids));
	mainTab.GetItem(mainTab.Find(mainRAO)).Enable(mainRAO.Load(Bem(), ids));
	mainTab.GetItem(mainTab.Find(mainSetupFOAMM)).Enable(true);
	//if (data.hd().IsLoadedStateSpace())
	//	mainTab.GetItem(mainTab.Find(mainStateSpace)).Enable(mainStateSpace.Load(Bem(), ids));
	
	mainTab.WhenSet();
}

void MainBEM::OnJoin() {
	Vector<int> idsjoin;
	bool selected = false;
	for (int row = menuOpen.arrayModel.GetCount()-1; row >= 0; --row) {
		if (menuOpen.arrayModel.IsSelected(row)) {
			int id = ArrayModel_IdHydro(menuOpen.arrayModel, row);
			idsjoin << id;
			selected = true;
		}
	}
	if (!selected) {
		Exclamation(t_("No model selected"));
		return;
	}
	try {
		WaitCursor wait;
		Progress progress(t_("Joining selected BEM files..."), 100); 
		
		Bem().Join(idsjoin, [&](String str, int _pos) {
			progress.SetText(str); 
			progress.SetPos(_pos); 
			return !progress.Canceled();
		});
		
		mainSummary.Clear();
		mainArrange.Clear();
		
		ArrayModel_IdsHydroDel(menuOpen.arrayModel, idsjoin);
		ArrayModel_IdsHydroDel(menuConvert.arrayModel, idsjoin);
		ArrayModel_IdsHydroDel(menuFOAMM.arrayModel, idsjoin);
	
		Vector<int> ids = ArrayModel_IdsHydro(menuOpen.arrayModel);
	
		for (int id = 0; id < Bem().hydros.GetCount(); ++id) {
			const Hydro &data = Bem().hydros[id].hd();
			mainSummary.Report(data, id);
			mainArrange.Load(Bem().hydros, ids);
		}
			
		if (Bem().hydros.GetCount() > 0) {
			menuOpen.arrayModel.SetCursor(0);
			menuConvert.arrayModel.SetCursor(0);
			menuFOAMM.arrayModel.SetCursor(0);
		}

		mainArrange.Load(Bem().hydros, ids);	
		mainTab.GetItem(mainTab.Find(mainArrange)).Enable(ids.GetCount() > 0);		
		mainTab.GetItem(mainTab.Find(mainStiffness)).Enable(mainStiffness.Load(Bem().hydros, ids));
		mainTab.GetItem(mainTab.Find(mainA)).Enable(mainA.Load(Bem(), ids));	
		mainTab.GetItem(mainTab.Find(mainB)).Enable(mainB.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainForceSC)).Enable(mainForceSC.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainForceFK)).Enable(mainForceFK.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainForceEX)).Enable(mainForceEX.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainRAO)).Enable(mainRAO.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainSetupFOAMM)).Enable(true);
	} catch (Exc e) {
		Exclamation(DeQtfLf(e));
	}
		
	int numrow = menuOpen.arrayModel.GetCount();
	menuOpen.butJoin.Enable(numrow > 1);
	menuOpen.butSymmetrize.Enable(numrow == 1);
	menuOpen.butA0.Enable(numrow == 1);
	menuOpen.butAinf.Enable(numrow == 1);
	menuOpen.butDescription.Enable(numrow == 1);
}

void MainBEM::OnSymmetrize() {
	int id = -1;
	for (int row = 0; row < menuOpen.arrayModel.GetCount(); ++row) {
		if (menuOpen.arrayModel.IsSelected(row)) {
			id = ArrayModel_IdHydro(menuOpen.arrayModel, row);
			break;
		}
	}
	if (id < 0) {
		Exclamation(t_("No model selected"));
		return;
	}
	try {
		WaitCursor wait;
		Progress progress(t_("Symmetrizing forces and RAOs in selected BEM file..."), 100); 
		
		Bem().Symmetrize(id);
		
		mainSummary.Clear();
		for (int i = 0; i < Bem().hydros.GetCount(); ++i)
			mainSummary.Report(Bem().hydros[i].hd(), i);
		
		Vector<int> ids = ArrayModel_IdsHydro(menuOpen.arrayModel);
		
		mainTab.GetItem(mainTab.Find(mainForceSC)).Enable(mainForceSC.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainForceFK)).Enable(mainForceFK.Load(Bem(), ids));
		mainTab.GetItem(mainTab.Find(mainForceEX)).Enable(mainForceEX.Load(Bem(), ids));
	} catch (Exc e) {
		Exclamation(DeQtfLf(e));
	}
}

void MainBEM::OnA0() {
	int id = -1;
	for (int row = 0; row < menuOpen.arrayModel.GetCount(); ++row) {
		if (menuOpen.arrayModel.IsSelected(row)) {
			id = ArrayModel_IdHydro(menuOpen.arrayModel, row);
			break;
		}
	}
	if (id < 0) {
		Exclamation(t_("No model selected"));
		return;
	}
	try {
		WaitCursor wait;
		Progress progress(t_("Calculating A0 in selected BEM file..."), 100); 
		
		Bem().A0(id);
		
		mainSummary.Clear();
		for (int i = 0; i < Bem().hydros.GetCount(); ++i)
			mainSummary.Report(Bem().hydros[i].hd(), i);
		
		Vector<int> ids = ArrayModel_IdsHydro(menuOpen.arrayModel);
		
		mainTab.GetItem(mainTab.Find(mainA)).Enable(mainA.Load(Bem(), ids));	
	} catch (Exc e) {
		Exclamation(DeQtfLf(e));
	}
}

void MainBEM::OnAinf() {
	int id = -1;
	for (int row = 0; row < menuOpen.arrayModel.GetCount(); ++row) {
		if (menuOpen.arrayModel.IsSelected(row)) {
			id = ArrayModel_IdHydro(menuOpen.arrayModel, row);
			break;
		}
	}
	if (id < 0) {
		Exclamation(t_("No model selected"));
		return;
	}
	try {
		WaitCursor wait;
		Progress progress(t_("Calculating A0 in selected BEM file..."), 100); 
		
		Bem().Ainf(id);
		
		mainSummary.Clear();
		for (int i = 0; i < Bem().hydros.GetCount(); ++i)
			mainSummary.Report(Bem().hydros[i].hd(), i);
		
		Vector<int> ids = ArrayModel_IdsHydro(menuOpen.arrayModel);
		
		mainTab.GetItem(mainTab.Find(mainA)).Enable(mainA.Load(Bem(), ids));	
	} catch (Exc e) {
		Exclamation(DeQtfLf(e));
	}
}

void MainBEM::OnDescription() {
	int id = -1;
	for (int row = 0; row < menuOpen.arrayModel.GetCount(); ++row) {
		if (menuOpen.arrayModel.IsSelected(row)) {
			id = ArrayModel_IdHydro(menuOpen.arrayModel, row);
			break;
		}
	}
	if (id < 0) {
		Exclamation(t_("No model selected"));
		return;
	}
	
	WithDescription<TopWindow> w;
	CtrlLayout(w);
	w.Title(t_("Enter model description"));
	w.butOK.WhenAction = [&] {w.Close();};
	w.description <<= Bem().hydros[id].hd().description;
	
	w.Execute();
	
	Bem().hydros[id].hd().description = ~w.description;
	
	mainSummary.Clear();
	for (int i = 0; i < Bem().hydros.GetCount(); ++i)
		mainSummary.Report(Bem().hydros[i].hd(), i);
}

bool MainBEM::OnConvert() {
	String file = ~menuConvert.file;
	
	try {
		WaitCursor wait;
		
		int id = ArrayModel_IdHydro(menuConvert.arrayModel);
		if (id < 0) {
			Exclamation(t_("Please select a model to export"));
			return false;
		}
		
		Hydro::BEM_SOFT type;	
		switch (menuConvert.opt) {
		case 0:	type = Hydro::WAMIT_1_3;	break;
		case 1:	type = Hydro::FAST_WAMIT;	break;
		case 2:	type = Hydro::BEMROSETTA;	break;
		case 3:	type = Hydro::UNKNOWN;		break;
		default: throw Exc(t_("Unknown type in OnConvert()"));
		}
		Bem().hydros[id].hd().SaveAs(file, type);	
	} catch (Exc e) {
		Exclamation(DeQtfLf(e));
		return false;
	}
	return true;
}

void MainBEM::Jsonize(JsonIO &json) {
	json
		("menuOpen_file", menuOpen.file)
		("menuConvert_file", menuConvert.file)
		("menuConvert_opt", menuConvert.opt)
		("menuPlot_autoFit", menuPlot.autoFit)
		("menuPlot_fromY0", menuPlot.fromY0)
		("menuPlot_opwT", menuPlot.opwT)
		("menuPlot_showPoints", menuPlot.showPoints)
		("menuPlot_showNdim", menuPlot.showNdim)
		//("menuFOAMM_file", menuFOAMM.file)
	;
}

String MainBEM::BEMFile(String fileFolder) const {
	if (DirectoryExists(fileFolder)) {
		int bestipos = INT_MAX;
		FindFile ff(AppendFileName(fileFolder, "*.*"));
		while (ff) {
			if (ff.IsFile()) {
				int ipos = Bem().bemFilesExt.Find(GetFileExt(ff.GetName()));
 				if (ipos >= 0 && ipos < bestipos) {
					fileFolder = ff.GetPath();
					bestipos = ipos;	// It takes the file with most probable extension
				}
			}
			ff.Next();
		}
	}
	return fileFolder;
}

void MainBEM::DragAndDrop(Point , PasteClip& d) {
	GuiLock __;
	if (IsDragAndDropSource())
		return;
	if (AcceptFiles(d)) {
		Vector<String> files = GetFiles(d);
		bool followWithErrors = false;
		for (int i = 0; i < files.GetCount(); ++i) {
			String file = BEMFile(files[i]);
			menuOpen.file <<= file;
			Status(Format(t_("Loading '%s'"), file));	
			if (!OnLoad() && !followWithErrors && files.GetCount() - i > 1) {
				if (!PromptYesNo(Format(t_("Do you wish to load the pending %d files?"), files.GetCount() - i - 1)))
					return;
				followWithErrors = true;
			}
			ProcessEvents();
		}
	}
}

bool MainBEM::Key(dword key, int ) {
	GuiLock __;
	if (key == K_CTRL_V) {
		Vector<String> files = GetFiles(Ctrl::Clipboard());
		bool followWithErrors = false;
		for (int i = 0; i < files.GetCount(); ++i) {
			String file = BEMFile(files[i]);
			menuOpen.file <<= file;
			Status(Format(t_("Loading '%s'"), file));
			if (!OnLoad() && !followWithErrors && files.GetCount() - i > 1) {
				if (!PromptYesNo(Format(t_("Do you wish to load the pending %d files?"), files.GetCount() - i - 1)))
					return true;
				followWithErrors = true;
			}
			ProcessEvents();
		}
		return true;
	}
	return false;
}

void MainSummary::Init() {
	CtrlLayout(*this);
	array.SetLineCy(EditField::GetStdHeight()).MultiSelect();
	array.WhenBar = [&](Bar &menu) {ArrayCtrlWhenBar(menu, array);};
}

void MainSummary::Clear() {
	array.Reset();
	array.SetLineCy(EditField::GetStdHeight()).MultiSelect();;
}

void MainSummaryCoeff::Report(const Hydro &data, int id) {
	if (array.GetColumnCount() == 0)
		array.AddColumn("Param");
	if (id >= array.GetColumnCount()-1)
		array.AddColumn(Format("#%d %s", id+1, data.name));
	int row = 0;
	int col = id + 1;
	
	array.Set(row, 0, t_("File"));				array.Set(row++, col, data.file);
	array.Set(row, 0, t_("Name"));				array.Set(row++, col, data.name);
	array.Set(row, 0, t_("Description"));		array.Set(row++, col, data.description);
	array.Set(row, 0, t_("Soft"));				array.Set(row++, col, data.GetCodeStr());
	array.Set(row, 0, t_("g [m/s2]"));			array.Set(row++, col, data.S_g());
	array.Set(row, 0, t_("rho [kg/m3]"));		array.Set(row++, col, data.S_rho());
	array.Set(row, 0, t_("h (water depth) [m]"));array.Set(row++,col, data.S_h());
	array.Set(row, 0, t_("length scale [m]"));	array.Set(row++, col, data.S_len());
	
	array.Set(row, 0, t_("#frequencies"));		array.Set(row++, col, Nvl(data.Nf, 0)); 
	if (!data.w.IsEmpty()) {
		array.Set(row, 0, t_("freq_0 [rad/s]"));	array.Set(row++, col, data.w[0]);
	} else {
		array.Set(row, 0, t_("freq_0 [rad/s]"));	array.Set(row++, col, "-");
	}
	if (data.w.GetCount() > 1) {
		array.Set(row, 0, t_("freq_end [rad/s]"));	array.Set(row++, col, data.w[data.w.GetCount()-1]);
		if (data.GetIrregularFreq() < 0) { 
			array.Set(row, 0, t_("freq_delta [rad/s]"));array.Set(row++, col, data.w[1] - data.w[0]);
		} else {
			String strHead;
			for (int i = 0; i < data.w.GetCount(); ++i) {
				if (i > 0)
					strHead << ", ";
				strHead << data.w[i];
			}
			array.Set(row, 0, t_("freq_delta [rad/s]"));
			array.Set(row++, col, Format(t_("Non constant delta (%s)"), strHead));
		}
	} else {
		array.Set(row, 0, t_("freq_end [rad/s]"));	array.Set(row++, col, "-");
		array.Set(row, 0, t_("freq_delta [rad/s]"));array.Set(row++, col, "-");
	}
	
	array.Set(row, 0, t_("#headings"));			array.Set(row++, col, Nvl(data.Nh, 0));
	if (!data.head.IsEmpty()) {
		array.Set(row, 0, t_("head_0 [º]"));	array.Set(row++, col, data.head[0]);
	} else {
		array.Set(row, 0, t_("head_0 [º]"));	array.Set(row++, col, "-");
	}
	if (data.head.GetCount() > 1) {
		array.Set(row, 0, t_("head_end [º]"));	array.Set(row++, col, data.head[data.head.GetCount()-1]);
		if (data.GetIrregularHead() < 0) { 
			array.Set(row, 0, t_("head_delta [º]"));array.Set(row++, col, data.head[1] - data.head[0]);
		} else {
			String strHead;
			for (int i = 0; i < data.head.GetCount(); ++i) {
				if (i > 0)
					strHead << ", ";
				strHead << data.head[i];
			}
			array.Set(row, 0, t_("head_delta [º]"));
			array.Set(row++, col, Format(t_("Non constant delta (%s)"), strHead));
		}
	} else {
		array.Set(row, 0, t_("head_end [º]"));	array.Set(row++, col, "-");
		array.Set(row, 0, t_("head_delta [º]"));array.Set(row++, col, "-");
	}
	
	array.Set(row, 0, t_("A0 available"));		array.Set(row++, col, data.IsLoadedAw0()   ? t_("Yes") : t_("No"));
	array.Set(row, 0, t_("Ainf available"));	array.Set(row++, col, data.IsLoadedAwinf() ? t_("Yes") : t_("No"));
	array.Set(row, 0, t_("A available"));		array.Set(row++, col, data.IsLoadedA() 	   ? t_("Yes") : t_("No"));
	array.Set(row, 0, t_("B available"));		array.Set(row++, col, data.IsLoadedB() 	   ? t_("Yes") : t_("No"));
	array.Set(row, 0, t_("K available"));		array.Set(row++, col, data.IsLoadedC() 	   ? t_("Yes") : t_("No"));
	array.Set(row, 0, t_("Fex available"));		array.Set(row++, col, data.IsLoadedFex()   ? t_("Yes") : t_("No"));
	array.Set(row, 0, t_("Fsc available"));		array.Set(row++, col, data.IsLoadedFsc()   ? t_("Yes") : t_("No"));
	array.Set(row, 0, t_("Ffk available"));		array.Set(row++, col, data.IsLoadedFfk()   ? t_("Yes") : t_("No"));
	array.Set(row, 0, t_("RAO available"));		array.Set(row++, col, data.IsLoadedRAO()   ? t_("Yes") : t_("No"));
	
	array.Set(row, 0, t_("#bodies"));			array.Set(row++, col, data.Nb);
	for (int ib = 0; ib < data.Nb; ++ib) {
		String sib = Format("#%d", ib+1);
		if (data.names.GetCount() > ib) {
			sib += " " + data.names[ib];
			array.Set(row, 0, sib + " " + t_("Name"));		array.Set(row++, col, data.names[ib]);
		} else {
			array.Set(row, 0, sib + " " + t_("Name"));		array.Set(row++, col, "-");
		}
		array.Set(row, 0, sib + " " + t_("#dof"));
		if (data.dof.GetCount() > ib) 
			array.Set(row++, col, data.dof[ib]);
		else 
			array.Set(row++, col, "-");
		
		array.Set(row, 0, sib + " " + t_("Vsub [m3]"));
		if (data.Vo.size() > ib) 
			array.Set(row++, col, FormatDouble(data.Vo[ib], 3, FD_EXP));
		else 
			array.Set(row++, col, "-");
		
		array.Set(row, 0, sib + " " + t_("Cg [m]"));
		if (data.cg.size() > 3*ib) 
			array.Set(row++, col, Format(t_("%s, %s, %s"),
									FormatDouble(data.cg(0, ib), 3, FD_EXP),
									FormatDouble(data.cg(1, ib), 3, FD_EXP),
									FormatDouble(data.cg(2, ib), 3, FD_EXP)));
		else
			array.Set(row++, col, "-");

		array.Set(row, 0, sib + " " + t_("Cb [m]"));
		if (data.cg.size() > 3*ib) 
			array.Set(row++, col, Format(t_("%s, %s, %s"),
									FormatDouble(data.cb(0, ib), 3, FD_EXP),
									FormatDouble(data.cb(1, ib), 3, FD_EXP),
									FormatDouble(data.cb(2, ib), 3, FD_EXP)));
		else
			array.Set(row++, col, "-");
		
		if (data.C.GetCount() > ib) {
			for (int i = 0; i < 6; ++i) {
				for (int j = 0; j < 6; ++j) {
					if (!Hydro::C_units(i, j).IsEmpty()) {
						array.Set(row, 0, sib + " " + Format(t_("K(%d,%d) [%s]"), i+1, j+1, Hydro::C_units(i, j)));	array.Set(row++, col, FormatDouble(data.C_dim(ib, i, j), 3, FD_EXP));		
					}
				}
			}
		} else {
			for (int i = 0; i < 6; ++i) {
				for (int j = 0; j < 6; ++j) {
					if (!Hydro::C_units(i, j).IsEmpty()) {
						array.Set(row, 0, sib + " " + Format(t_("K(%d,%d) [%s]"), i+1, j+1, Hydro::C_units(i, j)));	array.Set(row++, col, "-");		
					}
				}
			}
		}
	}	
}

void MainOutput::Init() {
	CtrlLayout(*this);
	cout.SetReadOnly();
	Print(t_("BEMRosetta\nHydrodynamic coefficients viewer and converter for Boundary Element Method solver formats"));
}

void MainOutput::Print(String str) {
	cout.Append(str);
	cout.ScrollEnd();
}

void MainArrange::Init() {
	CtrlLayout(*this);
}

void MainArrange::Clear() {
	tab.Reset();
	arrangeDOF.Clear();
}

void MainArrange::Load(Upp::Array<HydroClass> &hydros, const Vector<int> &ids) {
	MainArrange::Clear();
	for (int i = 0; i < ids.GetCount(); ++i) {
		int icase = ids[i];
		ArrangeDOF &arr = arrangeDOF.Add();
		arr.Init(hydros[icase].hd());
		tab.Add(arr.SizePos(), Format("[%s] %s", hydros[icase].hd().GetCodeStr(), hydros[icase].hd().name));
	}
}

void MainArrange::Remove(int c) {
	tab.Remove(c);
	arrangeDOF.Remove(c);
}

void MainSetupFOAMM::Init() {
	CtrlLayout(*this);
	
	arrayCases.SetLineCy(EditField::GetStdHeight());
	arrayCases.AddColumn(t_("     Sel"), 30);
	arrayCases.AddColumn(t_("Body"), 20);
	arrayCases.AddColumn(t_("Row"), 20);	
	arrayCases.AddColumn(t_("Column"), 20);
	arrayCases.AddColumn(t_("From (rad/s)"), 40);
	arrayCases.AddColumn(t_("To (rad/s)"), 40);
	arrayCases.AddColumn(t_("Frequencies (rad/s)"), 60);
	arrayCases.WhenCursor = [&] {WhenSelArrayCases();};
	
	selectAll.WhenAction = [&] {
			for (int i = 0; i < arrayCases.GetCount(); ++i)
				arrayCases.Set(i, 0, ~selectAll);
		};
	setToAll.WhenAction = [&] {
			int id = arrayCases.GetCursor();
			if (id < 0)
				return;
			for (int i = 0; i < arrayCases.GetCount(); ++i) {
				if (id != i) {
					arrayCases.Set(i, 4, arrayCases.Get(id, 4));
					arrayCases.Set(i, 5, arrayCases.Get(id, 5));
					arrayCases.Set(i, 6, arrayCases.Get(id, 6));
				}
			}
		};
	
	fromFreq.WhenAction = THISBACK(WhenArrayCases);
	fromFreq.SetRectangle(rectFrom, THISBACK(WhenFocus));
	toFreq.WhenAction   = THISBACK(WhenArrayCases);
	toFreq.SetRectangle(rectTo, THISBACK(WhenFocus));
	selector.Init(THISBACK(WhenArrayCases), THISBACK(WhenFocus));

	rectPlots.Add(plots.SizePos());	
	
	plots.Init(true);
	
	plots.scatt.WhenPainter = THISBACK1(OnPainter, &plots.scatt);
	plots.scatt.WhenMouseClick = THISBACK1(OnMouse, &plots.scatt);
	plots.scatP.WhenPainter = THISBACK1(OnPainter, &plots.scatP);
	plots.scatP.WhenMouseClick = THISBACK1(OnMouse, &plots.scatP);
}

void MenuFOAMM::Init(MainBEM &mainBEM, BEMData &bem, MainSetupFOAMM &setup) {
	CtrlLayout(*this);
	
	butLoad.WhenAction 	= [&] {
		if (OnFOAMM(setup)) {
			int id = mainBEM.mainTab.Find(mainBEM.mainStateSpace);
			mainBEM.mainTab.GetItem(id).Enable(true);
			int idPlot = mainBEM.menuTab.Find(mainBEM.menuPlot);
			mainBEM.menuTab.GetItem(idPlot).Enable(true);
			mainBEM.mainTab.Set(0);
			mainBEM.mainTab.Set(id);
		}
	};
	
	ArrayModel_Init(arrayModel);	
	arrayModel.WhenCursor = [&] {	
		int id = ArrayModel_IdHydro(arrayModel);
		if (id < 0)
			return;
		setup.WhenSelArrayModel(id, bem);
	};
	
	foammLogo.Set(Img2::FOAMM());
	foammWorking.LoadBuffer(String(animatedStar, animatedStar_length));
	foammWorking.Hide();
	status.Hide();
	progress.Hide();
	butCancel.Hide();
	butCancel.WhenAction = [&] {isCancelled = true;};
}

void MainSetupFOAMM::WhenFocus(StaticRectangle *rect) {
	rectFrom.Hide();
	rectTo.Hide();
	selector.HideCtrls();
	if (rect)
		rect->Show();
	rectActual = rect;
	plots.RefreshScatter();
}

void MainSetupFOAMM::OnPainter(Painter &w, ScatterCtrl *pscat) {
	ScatterCtrl &scat = *pscat;
	int plotW = scat.GetPlotWidth(), plotH = scat.GetPlotHeight();
	
	for (int i = 0; i < selector.GetCount(); ++i) {
		if (!IsNull(selector.Get(i))) {
			double xFreq = scat.GetPosX(selector.Get(i));
			if (selector.IsSelected(i))
				DrawLineOpa(w, xFreq, 0, xFreq, plotH, 1, 1, 2, LtCyan(), "2 2");
			else
				DrawLineOpa(w, xFreq, 0, xFreq, plotH, 1, 1, 2, LtBlue(), "2 2");
		}
	}

	if (!IsNull(fromFreq)) {
		double xFrom = scat.GetPosX(~fromFreq);
		FillRectangleOpa(w, 0, 0, xFrom, plotH, 0.5, Null, LtBlue());
	}
	if (!IsNull(toFreq)) {
		double xTo = scat.GetPosX(~toFreq);
		FillRectangleOpa(w, xTo, 0, plotW, plotH, 0.5, Null, LtBlue());
	}
}

void MainSetupFOAMM::OnMouse(Point p, dword, ScatterCtrl::MouseAction action, ScatterCtrl *pscat) {
	ScatterCtrl &scat = *pscat;
	if (action != ScatterCtrl::LEFT_DOWN && action != ScatterCtrl::LEFT_MOVE)
		return; 
		
	double freq = scat.GetRealPosX(p.x);

	if (!scat.IsEmpty()) {
		DataSource &data = scat.GetDataSource(0);
		freq = data.x(data.ClosestX(freq));
	}
	
	if (rectActual == &rectFrom) {	
		fromFreq <<= freq;
		fromFreq.WhenAction();
	} else if (rectActual == &rectTo) {	
		toFreq <<= freq;
		toFreq.WhenAction();
	} else {
		int id = selector.IsRect(rectActual);
		if (id >= 0) 
			selector.Set(id, freq);
	}
}

void MainSetupFOAMM::WhenSelArrayModel(int _id, BEMData &bem) {
	arrayCases.Clear();
	options.Clear();
	
	id = _id;
	
	ASSERT(id < Bem().hydros.GetCount());
	
	const Hydro &hydro = Bem().hydros[id].hd();
	
	for (int ib = 0; ib < hydro.Nb; ++ib) {
		for (int idf = 0; idf < 6; ++idf) {
			for (int jdf = 0; jdf < 6; ++jdf) {
				if (!bem.onlyDiagonal || idf == jdf) {
					int _idf = ib*6 + idf;
					int _jdf = ib*6 + jdf;
	
					if (hydro.IsLoadedA() && hydro.IsLoadedB() && !IsNull(hydro.A[0](_idf, _jdf)) && !IsNull(hydro.B[0](_idf, _jdf))) {
						arrayCases.Add(false, ib+1, Hydro::StrDOF_base(idf), Hydro::StrDOF_base(jdf));
						int row = arrayCases.GetCount()-1;
						arrayCases.SetCtrl(row, 0, options.Add());
						options.Top() << [=] {options[row].SetFocus();};
					}
				}
			}
		}
	}
	if (arrayCases.GetCount() > 0)
		arrayCases.SetCursor(0);
}

void MainSetupFOAMM::WhenSelArrayCases() {
	try {
		int row = arrayCases.GetCursor();
		if (row < 0)
			return;
	
		bool opChoose = arrayCases.Get(row, 0);
		fromFreq <<= arrayCases.Get(row, 4);
		toFreq   <<= arrayCases.Get(row, 5);

		selector.Clear();
			
		String freqs = arrayCases.Get(row, 6);
		Vector<String> afreqs = Split(freqs, ';');
		for (int i = 0; i < afreqs.GetCount(); ++i)
			selector.AddField(ScanDouble(afreqs[i]));
		
		if (opChoose)
			Status(Check(~fromFreq, ~toFreq, ~freqs));
		
		const Hydro &hydro = Bem().hydros[id].hd();
		
		int ib = int(arrayCases.Get(row, 1)) - 1;
		int idf = Hydro::DOFStr(arrayCases.Get(row, 2));
		int jdf = Hydro::DOFStr(arrayCases.Get(row, 3));
		
		plots.Init(idf + 6*ib, jdf + 6*ib, DATA_STS);
		MainBEM &mbm = GetDefinedParent<MainBEM>(this);
		plots.Load(hydro, mbm);
	} catch (Exc e) {
		Exclamation(DeQtfLf(e));
		return;
	}
}

void MainSetupFOAMM::WhenArrayCases() {
	int row = arrayCases.GetCursor();
	if (row < 0)
		return;
	
	arrayCases.Set(row, 4, ~fromFreq);
	arrayCases.Set(row, 5, ~toFreq);

	Vector<double> freqs;
	for (int i = 0; i < selector.GetCount(); ++i) {
		double freq = selector.Get(i);
		if (!IsNull(freq)) {
			if (!plots.scatt.IsEmpty()) { 
				DataSource &data = plots.scatt.GetDataSource(0);
				freq = data.x(data.ClosestX(freq));
			}
		}
		freqs << freq;
	}
	
	Sort(freqs);
	
	String sfreqs;
	for (int i = 0; i < freqs.GetCount(); ++i) {
		if (!sfreqs.IsEmpty())
			sfreqs << ";";
		sfreqs << freqs[i];
	}
	
	arrayCases.Set(row, 6, sfreqs);
	
	Status(Check(~fromFreq, ~toFreq, sfreqs));
	
	plots.RefreshScatter();
}

String MainSetupFOAMM::Check(double fromFreq, double toFreq, String freqs) {
	if (IsNull(fromFreq))
		return t_("From: frequency is empty");

	if (IsNull(toFreq))
		return t_("To: frequency is empty");

	if (toFreq <= fromFreq) 
		return t_("From: frequency has to be lower than To: frequency");
			
	Vector<String> afreqs = Split(freqs, ';');
	if (afreqs.IsEmpty()) 
		return t_("No frequency has been selected");
	
	Vector<double> unique;
	for (int i = 0; i < afreqs.GetCount(); ++i) {
		double freq = ScanDouble(afreqs[i]);
		if (freq < fromFreq || freq > toFreq) 
			return t_("Selected frequencies have to be between lower and higher limits");
		FindAddRatio(unique, freq, 0.001);
	}
	if (unique.GetCount() != afreqs.GetCount()) 
		return t_("Some selected frequencies are repeated");
	
	return String("");
}

bool MainSetupFOAMM::Get(Vector<int> &ibs, Vector<int> &idfs, Vector<int> &jdfs,
		Vector<double> &froms, Vector<double> &tos, Vector<Vector<double>> &freqs) {
	for (int row = 0; row < arrayCases.GetCount(); ++row) {
		bool proc = arrayCases.Get(row, 0);
		if (proc) {
			int ib = int(arrayCases.Get(row, 1))-1;
			ibs << ib;
			String sidf = arrayCases.Get(row, 2);
			idfs << Hydro::DOFStr(sidf);
			String sjdf = arrayCases.Get(row, 3);
			jdfs << Hydro::DOFStr(sjdf);
			double from = arrayCases.Get(row, 4);
			double to = arrayCases.Get(row, 5);
			String strfreqs = arrayCases.Get(row, 6);
			String err = Check(from, to, strfreqs);
			if (!err.IsEmpty()) {
				Exclamation(Format(t_("Problem in body %d (%s, %s): %s"), ib+1, sidf, sjdf, err));
				return false;		
			}
			froms << from;
			tos << to;
			Vector<double> &f = freqs.Add();
			Vector<String> fs = Split(strfreqs, ';');
			for (int i = 0; i < fs.GetCount(); ++i)
				f << ScanDouble(fs[i]);
		}
	}
	if (ibs.IsEmpty()) {
		Exclamation(t_("No case has been selected"));
		return false;			
	}
	return true;
}

void MainSetupFOAMM::Clear() {
	plots.Clear();
}

void MenuFOAMM::Clear() {
	arrayModel.Clear();
}
		
bool MenuFOAMM::OnFOAMM(MainSetupFOAMM &setup) {
	Vector<int> ibs, idfs, jdfs;
	Vector<double> froms, tos;
	Vector<Vector<double>> freqs;
	String ret;
	
	try {
		int id = ArrayModel_IdHydro(arrayModel);
		if (id < 0)
			return false;
		
		if (!setup.Get(ibs, idfs, jdfs, froms, tos, freqs))
			return false;
		
		foammWorking.Show();
		foammWorking.Play();
		status.Show();
		progress.Show();
		butCancel.Show();
		butLoad.Disable();
		WaitCursor wait;
		isCancelled = false;
		status.SetText(t_("Starts processing"));
		Foamm &foamm = static_cast<Foamm&>(Bem().hydros[id]);
		foamm.Get(ibs, idfs, jdfs, froms, tos, freqs,
			[&](String str, int pos)->bool {
				if (!str.IsEmpty())
					status.SetText(str);	
				if (IsNull(pos))
					; 
				else
					progress.Set(pos, 100);
				ProcessEvents(); 
				return isCancelled;
			}, 
			[&](String str) {
				if (!str.IsEmpty()) {
					str.Replace("\r", "");
					str.Replace("\n\n", "\n");
					Exclamation(t_("FOAMM message:&") + DeQtfLf(str));
				}
				ProcessEvents(); 
			});
	} catch (Exc e) {
		ret = DeQtfLf(e);
	}
	foammWorking.Hide();
	foammWorking.Stop();
	status.Hide();
	progress.Hide();
	butCancel.Hide();
	butLoad.Enable();
	if (!ret.IsEmpty()) {
		Exclamation(ret);
		return false;
	}
	return true;
}

	
