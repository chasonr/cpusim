// registers.cpp

#include <wx/wx.h>
#include <wx/textdlg.h>
#include <string>
#include <vector>
#include "cpu.h"
#include "flags.h"
#include "registers.h"

wxBEGIN_EVENT_TABLE(RegisterWindow, wxListBox)
    EVT_LISTBOX_DCLICK(wxID_ANY, RegisterWindow::OnDoubleClick)
wxEND_EVENT_TABLE()

RegisterWindow::RegisterWindow(wxWindow *parent, CPU *cpu_) :
    wxListBox(parent, wxID_ANY),
    cpu(cpu_)
{
    SetFont(wxFont(
            10,
            wxFONTFAMILY_TELETYPE,
            wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL));

    Update();
}

void
RegisterWindow::Update(void)
{
    auto regs = cpu->GetRegisterList();
    auto row = GetTextExtent("X").y;
    int width = 0;
    unsigned linenum = 0;
    for (auto p = regs.begin(); p != regs.end(); ++p) {
        auto reg = *p;
        auto value = cpu->GetRegister(*p);
        auto line = reg + wxString(10 - reg.size(), ' ') + value;

        if (linenum < GetCount()) {
            SetString(linenum, line);
        } else {
            InsertItems(1, &line, GetCount());
        }
        ++linenum;

        auto w1 = GetTextExtent(line).x;
        if (w1 > width) {
            width = w1;
        }
    }

    SetMinSize(wxSize(width + 20,
            (row+7) * ((regs.size() > 10 ? 10 : regs.size()) + 1)));
}

void
RegisterWindow::OnDoubleClick(wxCommandEvent& event)
{
    // Identify the register to be altered and its current value
    auto str = event.GetString();
    auto space = str.find(' ');
    auto reg = str.substr(0, space);
    while (str[space] == ' ') {
        ++space;
    }
    auto value = str.substr(space);

    if (reg == "FLAGS") {
        auto dlg = FlagsDialog(this, wxID_ANY, "New value for FLAGS");
        dlg.SetFlags(cpu, value.ToStdString());
        if (dlg.ShowModal() == wxID_OK) {
            cpu->SetRegister(reg.ToStdString(), dlg.GetFlags());
            Update();
        }
    } else {
        wxTextEntryDialog dlg(this, "New value for " + reg,
                "Modify register", value);
        if (dlg.ShowModal() == wxID_OK) {
            auto value2 = dlg.GetValue();
            cpu->SetRegister(reg.ToStdString(), value2.ToStdString());
            Update();
        }
    }
}
