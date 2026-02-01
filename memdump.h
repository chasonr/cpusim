// memdump.h

#ifndef MEMDUMP_H
#define MEMDUMP_H

#include <wx/wx.h>
#include <wx/vlbox.h>
#include <string>
#include <cstdint>

class CPU;

class MemDumpWindow : public wxVListBox {
public:
    MemDumpWindow(wxWindow *parent, CPU *cpu_,
            unsigned height_,
            unsigned first_, unsigned last_);
    void Update(void);
    void SetAddress(std::uint16_t addr);

    virtual void OnDrawItem(wxDC & dc, const wxRect & rect, std::size_t n) const override;
    virtual wxCoord OnMeasureItem(std::size_t n) const override;

private:
    CPU *cpu;
    std::uint16_t start, current;
    unsigned height;
    unsigned first, last;

    void OnDoubleClick(wxCommandEvent& event);

    std::string GetMemLine(std::size_t n) const;

    wxDECLARE_EVENT_TABLE();
};

#endif
