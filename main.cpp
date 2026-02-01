// main.cpp

#include <wx/wx.h>
#include <algorithm>
#include <vector>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include "cpu.h"
#include "cpu6502.h"
#include "disasm.h"
#include "events.h"
#include "load.h"
#include "memdump.h"
#include "memory.h"
#include "registers.h"

#include "open.xpm"
#include "into.xpm"
#include "over.xpm"
#include "return.xpm"
#include "goto.xpm"
#include "mgoto.xpm"
 
class CPUSimApp : public wxApp
{
public:
    bool OnInit() override;
};
 
wxIMPLEMENT_APP(CPUSimApp);
 
class CPUSimFrame : public wxFrame
{
public:
    CPUSimFrame();
 
private:
    void OnLoad(wxCommandEvent& event);
    void OnCodeGoto(wxCommandEvent& event);
    void OnMemGoto(wxCommandEvent& event);
    void OnStepInto(wxCommandEvent& event);
    void OnStepOver(wxCommandEvent& event);
    void OnReturn(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnUpdateAll(wxEvent& event);

    void UpdateAll(void);

    Memory *memory;
    CPU *cpu;
    RegisterWindow *registers;
    DisassemblyWindow *disassembly;
    std::vector<MemDumpWindow *> zones;
    MemDumpWindow *memoryWin;

    wxDECLARE_EVENT_TABLE();
};
 
enum
{
    ID_Load = 1,
    ID_StepInto = 2,
    ID_StepOver = 3,
    ID_Return = 4,
    ID_CodeGoto = 5,
    ID_MemGoto = 6
};

static void setBold(wxWindow *window);
 
wxDEFINE_EVENT(wxEVT_UPDATE_ALL, wxEvent);

wxBEGIN_EVENT_TABLE(CPUSimFrame, wxFrame)
    EVT_CUSTOM(wxEVT_UPDATE_ALL, wxID_ANY, CPUSimFrame::OnUpdateAll)
wxEND_EVENT_TABLE()

bool CPUSimApp::OnInit()
{
    CPUSimFrame *frame = new CPUSimFrame();
    frame->Show(true);
    return true;
}
 
CPUSimFrame::CPUSimFrame() :
    wxFrame(nullptr, wxID_ANY, "CPU Simulator"),
    memory(new LittleEndianMemory(65536)),
    cpu(new CPU6502(memory)),
    registers(nullptr),
    disassembly(nullptr),
    memoryWin(nullptr)
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(ID_Load, "&Load...\tCtrl-L",
                     "Load a binary image into memory");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu *menuView = new wxMenu;
    menuView->Append(ID_CodeGoto, "View &code address");
    menuView->Append(ID_MemGoto, "View &memory address");

    wxMenu *menuRun = new wxMenu;
    menuRun->Append(ID_StepInto, "Step &into subroutine");
    menuRun->Append(ID_StepOver, "Step &over subroutine");
    menuRun->Append(ID_Return, "&Return from subroutine");
 
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
 
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuView, "&View");
    menuBar->Append(menuRun, "&Run");
    menuBar->Append(menuHelp, "&Help");
 
    SetMenuBar( menuBar );
 
    auto toolbar = CreateToolBar();
    // TODO: add tools here
    toolbar->AddTool(ID_Load, "Load",
                     wxBitmapBundle(open_xpm),
                     "Load image into memory");
    toolbar->AddSeparator();
    toolbar->AddTool(ID_CodeGoto, "Go to code",
                     wxBitmapBundle(goto_xpm),
                     "View code address");
    toolbar->AddTool(ID_MemGoto, "Go to memory",
                     wxBitmapBundle(mgoto_xpm),
                     "View memory address");
    toolbar->AddSeparator();
    toolbar->AddTool(ID_StepInto, "Step Into",
                     wxBitmapBundle(into_xpm),
                     "Step into subroutine");
    toolbar->AddTool(ID_StepOver, "Step Over",
                     wxBitmapBundle(over_xpm),
                     "Step over subroutine");
    toolbar->AddTool(ID_Return, "Return",
                     wxBitmapBundle(return_xpm),
                     "Return from subroutine");
    toolbar->Realize();

    auto *sizer0 = new wxBoxSizer(wxHORIZONTAL);

    auto sizer1 = new wxBoxSizer(wxVERTICAL);
    auto label = new wxStaticText(this, wxID_ANY, "Registers");
    setBold(label);
    sizer1->Add(label, 0);
    registers = new RegisterWindow(this, cpu);
    sizer1->Add(registers, 1, wxEXPAND);
    sizer0->Add(sizer1, 0, wxEXPAND);

    auto sizer2 = new wxBoxSizer(wxVERTICAL);
    label = new wxStaticText(this, wxID_ANY, "Disassembly");
    setBold(label);
    sizer2->Add(label, 0);
    disassembly = new DisassemblyWindow(this, cpu);
    sizer2->Add(disassembly, 1, wxEXPAND);
    sizer0->Add(sizer2, 0, wxEXPAND);

    auto sizer3 = new wxBoxSizer(wxVERTICAL);
    auto zones_ = cpu->GetMemZones();
    for (auto z = zones_.begin(); z != zones_.end(); ++z) {
        label = new wxStaticText(this, wxID_ANY, z->name);
        setBold(label);
        sizer3->Add(label, 0);
        auto win = new MemDumpWindow(this, cpu, 200, z->start, z->start+z->size-1);
        sizer3->Add(win, 0, wxEXPAND);
        zones.push_back(win);
    }
    label = new wxStaticText(this, wxID_ANY, "Memory");
    setBold(label);
    sizer3->Add(label, 0);
    memoryWin = new MemDumpWindow(this, cpu, 300, 0x0000, 0xFFFF);
    sizer3->Add(memoryWin, 0, wxEXPAND);
    sizer0->Add(sizer3, 0);

    CreateStatusBar();
    SetStatusText("");

    SetSizerAndFit(sizer0);
 
    Bind(wxEVT_MENU, &CPUSimFrame::OnLoad, this, ID_Load);
    Bind(wxEVT_MENU, &CPUSimFrame::OnCodeGoto, this, ID_CodeGoto);
    Bind(wxEVT_MENU, &CPUSimFrame::OnMemGoto, this, ID_MemGoto);
    Bind(wxEVT_MENU, &CPUSimFrame::OnStepInto, this, ID_StepInto);
    Bind(wxEVT_MENU, &CPUSimFrame::OnStepOver, this, ID_StepOver);
    Bind(wxEVT_MENU, &CPUSimFrame::OnReturn, this, ID_Return);
    Bind(wxEVT_MENU, &CPUSimFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &CPUSimFrame::OnExit, this, wxID_EXIT);
}
 
void CPUSimFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}
 
void CPUSimFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("CPU Simulator",
                 "About CPU Simulator", wxOK | wxICON_INFORMATION);
}
 
void
CPUSimFrame::OnLoad(wxCommandEvent& event)
{
    auto start = Load(this, cpu->GetMemory());
    if (start >= 0) {
        disassembly->SetAddress(start);
        for (auto z = zones.begin(); z != zones.end(); ++z) {
            (*z)->Update();
        }
        memoryWin->SetAddress(start);
    }
}

void
CPUSimFrame::OnCodeGoto(wxCommandEvent& event)
{
    wxTextEntryDialog dlg(this, "View code address");
    char pc[20];
    std::snprintf(pc, sizeof(pc), "%04" PRIx64, cpu->GetPC());
    dlg.SetValue(pc);
    if (dlg.ShowModal() == wxID_OK) {
        auto addr = std::strtoul(dlg.GetValue(), NULL, 16);
        disassembly->SetAddress(addr);
    }
}

void
CPUSimFrame::OnMemGoto(wxCommandEvent& event)
{
    wxTextEntryDialog dlg(this, "View memory address");
    dlg.SetValue("0000");
    if (dlg.ShowModal() == wxID_OK) {
        auto addr = std::strtoul(dlg.GetValue(), NULL, 16);
        memoryWin->SetAddress(addr);
    }
}

void
CPUSimFrame::OnStepInto(wxCommandEvent& event)
{
    try {
        cpu->Step();
    }
    catch (CPUExcept const &err) {
        wxMessageBox(err.what(), "Error", wxOK | wxICON_ERROR);
    }
    UpdateAll();
    disassembly->SetAddress(cpu->GetPC());
}

void
CPUSimFrame::OnStepOver(wxCommandEvent& event)
{
    try {
        cpu->Next();
    }
    catch (CPUExcept const &err) {
        wxMessageBox(err.what(), "Error", wxOK | wxICON_ERROR);
    }
    UpdateAll();
    disassembly->SetAddress(cpu->GetPC());
}

void
CPUSimFrame::OnReturn(wxCommandEvent& event)
{
    try {
        cpu->ToReturn();
    }
    catch (CPUExcept const &err) {
        wxMessageBox(err.what(), "Error", wxOK | wxICON_ERROR);
    }
    UpdateAll();
    disassembly->SetAddress(cpu->GetPC());
}

void
CPUSimFrame::OnUpdateAll(wxEvent& event)
{
    UpdateAll();
}

void
CPUSimFrame::UpdateAll(void)
{
    registers->Update();
    disassembly->Update();
    for (auto z = zones.begin(); z != zones.end(); ++z) {
        (*z)->Update();
    }
    memoryWin->Update();
}

static void
setBold(wxWindow *window)
{
    auto font = window->GetFont();
    font.MakeBold();
    window->SetFont(font);
}
