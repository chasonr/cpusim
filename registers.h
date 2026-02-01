// registers.h

#ifndef REGISTERS_H
#define REGISTERS_H

#include <wx/wx.h>

class CPU;

class RegisterWindow : public wxListBox {
public:
    RegisterWindow(wxWindow *parent, CPU *cpu_);
    void Update(void);

private:
    CPU *cpu;

    void OnDoubleClick(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

#endif
