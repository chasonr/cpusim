// disasm.h

#ifndef DISASM_H
#define DISASM_H

#include <wx/wx.h>
#include <wx/vlbox.h>
#include <string>
#include <vector>
#include <cstdint>

class CPU;

class DisassemblyWindow : public wxVListBox {
public:
    DisassemblyWindow(wxWindow *parent, CPU *cpu_);
    void Update(void);
    void SetAddress(std::uint16_t addr);

    virtual void OnDrawItem(wxDC & dc, const wxRect & rect, std::size_t n) const override;
    virtual wxCoord OnMeasureItem(std::size_t n) const override;

private:
    CPU *cpu;
    std::uint16_t start, current;

    struct DisasmLine {
        std::string text;
        std::uint16_t addr;
        unsigned count;
    };
    std::vector<DisasmLine> text;

    void OnDoubleClick(wxCommandEvent& event);
    void OnChar(wxKeyEvent& event);

    wxDECLARE_EVENT_TABLE();
};

#endif
