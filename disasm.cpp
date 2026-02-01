// disasm.cpp

#include <wx/wx.h>
#include <vector>
#include <cstdio>
#include "cpu.h"
#include "disasm.h"
#include "events.h"
#include "memory.h"

wxBEGIN_EVENT_TABLE(DisassemblyWindow, wxVListBox)
    EVT_CHAR(DisassemblyWindow::OnChar)
    EVT_LISTBOX_DCLICK(wxID_ANY, DisassemblyWindow::OnDoubleClick)
wxEND_EVENT_TABLE()

static std::string ByteStr(CPU const *cpu, std::uint16_t addr, unsigned len);

DisassemblyWindow::DisassemblyWindow(wxWindow *parent, CPU *cpu_) :
    wxVListBox(parent, wxID_ANY),
    cpu(cpu_),
    start(0xA000),
    current(0xA000)
{
    SetFont(wxFont(
            10,
            wxFONTFAMILY_TELETYPE,
            wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL));

    SetMinSize(wxSize(
            GetTextExtent(std::string(40, ' ')).x,
            500));

    Update();
}

void
DisassemblyWindow::SetAddress(std::uint16_t addr)
{
    start = addr;
    current = addr;
    Update();

    unsigned rownum = 0;
    for (auto p = text.begin(); p != text.end(); ++p) {
        auto a = p->addr;
        if (a >= addr) {
            break;
        }
        ++rownum;
    }

    SetSelection(rownum);
}

void
DisassemblyWindow::Update(void)
{
    // Build new contents of window
    unsigned addr = 0;
    text.clear();
    while (addr < 0x10000) {
        char addr_str[20];
        std::string value_str;

        // Address of this row
        std::snprintf(addr_str, sizeof(addr_str), "$%04X  ", addr);

        // Avoid crossing the current address
        auto disasm = cpu->Disassemble(addr);
        if (addr < current && addr + disasm.num_bytes > current) {
            char subst_str[20];
            snprintf(subst_str, sizeof(subst_str), "??? $%02X",
                    cpu->GetMemory()->Peek8(addr));
            disasm.disasm = subst_str;
            disasm.num_bytes = 1;
        }
        value_str = ByteStr(cpu, addr, disasm.num_bytes);
        DisasmLine line;
        line.text = addr_str + value_str + "  " + disasm.disasm;
        line.addr = addr;
        line.count = disasm.num_bytes;
        text.push_back(line);

        addr += disasm.num_bytes;
    }

    SetItemCount(text.size());
}

// Return the bytes that compose the instruction
static std::string
ByteStr(CPU const *cpu, std::uint16_t addr, unsigned len)
{
    Memory const *memory = cpu->GetMemory();
    auto max_len = cpu->GetMaxLen();
    bool trunc = false;

    if (len > max_len) {
        trunc = true;
        len = max_len - 1;
    }

    char byte[10];
    std::snprintf(byte, sizeof(byte), "%02X", memory->Peek8(addr+0));
    std::string bytes = byte;
    for (unsigned i = 1; i < len; ++i) {
        std::snprintf(byte, sizeof(byte), " %02X", memory->Peek8(addr+i));
        bytes += byte;
    }

    if (trunc) {
        bytes += "...";
    } else {
        bytes += std::string((max_len-len)*3, ' ');
    }

    return bytes;
}

void
DisassemblyWindow::OnChar(wxKeyEvent& event)
{
    auto ch = event.GetKeyCode();
    switch (ch) {
    case 'b':
    case 'B':
        {
            auto n = GetSelection();
            if (n >= 0) {
                auto line = text[n];
                if (cpu->HasBreakpoint(line.addr, line.count)) {
                    // Clear any breakpoints within this line
                    for (unsigned i = 0; i < line.count; ++i) {
                        cpu->ClearBreakpoint(line.addr + i);
                    }
                } else {
                    // Set a breakpoint at this address
                    cpu->SetBreakpoint(line.addr);
                }
                Refresh();
            }
        }
        break;

    default:
        break;
    }
}

void
DisassemblyWindow::OnDoubleClick(wxCommandEvent& event)
{
    // Address at selected line
    auto addr = text[event.GetInt()].addr;
    wxString newInstr;

    // Enter instructions until cancelled
    while (true) {
        char addr_str[10];
        std::snprintf(addr_str, sizeof(addr_str), "%04X", (unsigned)addr);
        wxTextEntryDialog dlg(this, "New instruction at $" + std::string(addr_str),
                "Enter instruction", newInstr);
        if (dlg.ShowModal() == wxID_CANCEL) {
            break;
        }

        newInstr = dlg.GetValue();
        auto assem = cpu->Assemble(addr, newInstr.ToStdString());
        if (!assem.valid) {
            wxMessageBox("Instruction not recognized", "Error", wxOK | wxICON_ERROR);
            continue;
        }

        for (std::size_t i = 0; i < assem.bytes.size(); ++i) {
            cpu->GetMemory()->Load8(addr++, assem.bytes[i]);
        }

        wxCommandEvent event2(wxEVT_UPDATE_ALL, GetId());
        event2.SetEventObject(this);
        ProcessWindowEvent(event2);

        newInstr = "";
    }
}

void
DisassemblyWindow::OnDrawItem(wxDC & dc, const wxRect & rect, size_t n) const
{
    auto square = rect.GetHeight();

    auto line = text[n];
    dc.DrawText(line.text, rect.GetLeft() + square, rect.GetTop() + 3);
    if (cpu->HasBreakpoint(line.addr, line.count)) {
        // Draw the breakpoint marker
        auto pen = dc.GetPen();
        auto brush = dc.GetBrush();
        dc.SetPen(*wxRED_PEN);
        dc.SetBrush(*wxRED_BRUSH);
        dc.DrawCircle(rect.GetLeft()+square/2, rect.GetTop()+square/2, square/2-5);
        dc.SetPen(pen);
        dc.SetBrush(brush);
    }
}

wxCoord
DisassemblyWindow::OnMeasureItem(size_t n) const
{
    return GetTextExtent(text[n].text).y + 6;
}
