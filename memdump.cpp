// memdump.cpp

#include <wx/wx.h>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include "cpu.h"
#include "events.h"
#include "memdump.h"
#include "memory.h"

wxBEGIN_EVENT_TABLE(MemDumpWindow, wxVListBox)
    EVT_LISTBOX_DCLICK(wxID_ANY, MemDumpWindow::OnDoubleClick)
wxEND_EVENT_TABLE()

MemDumpWindow::MemDumpWindow(wxWindow *parent, CPU *cpu_,
            unsigned height_,
            unsigned first_, unsigned last_) :
    wxVListBox(parent, wxID_ANY),
    cpu(cpu_),
    start(0),
    current(0),
    height(height_),
    first(first_),
    last(last_)
{
    SetFont(wxFont(
            10,
            wxFONTFAMILY_TELETYPE,
            wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL));

    SetItemCount((last - first + 15) / 16);
    Update();
}

void
MemDumpWindow::SetAddress(std::uint16_t addr)
{
    Update();
    SetSelection((addr-first)/16);
}

void
MemDumpWindow::Update(void)
{
    auto size = GetTextExtent("$0000: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................").x;
    SetMinSize(wxSize(size+20, height));
    Refresh();
}

void
MemDumpWindow::OnDoubleClick(wxCommandEvent& event)
{
    // Address and existing data at selected line
    auto str = GetMemLine(event.GetInt());
    auto colon = str.find(':');
    auto addr_str = str.substr(1, colon-1);
    auto addr = strtoul(addr_str.c_str(), NULL, 16);
    auto bytes = str.substr(colon+2, 47);

    wxTextEntryDialog dlg(this, "New bytes at $" + addr_str,
            "Modify memory", bytes);
    if (dlg.ShowModal() == wxID_OK) {
        auto newBytesStr = dlg.GetValue();
        const char *newBytes = newBytesStr.utf8_str();
        bool changed = false;
        const char *p = newBytes;

        while (*p != 0) {
            // Skip whitespace
            while (std::isspace(*p)) {
                ++p;
            }
            if (*p == 0) {
                break;
            }

            // Convert byte to hex
            char *end;
            auto byte = std::strtoul(p, &end, 16);

            // Stop on invalid conversion
            if (*end != 0 && !std::isspace(*end)) {
                break;
            }

            // Write the byte to memory
            cpu->GetMemory()->Load8(addr++, byte);
            changed = true;

            // Advance to the next byte
            p = end;
        }

        if (changed) {
            wxCommandEvent event(wxEVT_UPDATE_ALL, GetId());
            event.SetEventObject(this);
            ProcessWindowEvent(event);
        }
    }
}

void
MemDumpWindow::OnDrawItem(wxDC & dc, const wxRect & rect, size_t n) const
{
    auto line = GetMemLine(n);
    dc.DrawText(line, rect.GetLeft(), rect.GetTop() + 3);
}

std::string
MemDumpWindow::GetMemLine(size_t n) const
{
    auto addr = n*16 + first;

    char str[10], ascii[17];
    std::snprintf(str, sizeof(str), "$%04" PRIXMAX ":", (std::uintmax_t)addr);
    std::string line = str;
    for (unsigned j = 0; j < 16; ++j) {
        auto byte = cpu->GetMemory()->Peek8(addr+j);
        std::snprintf(str, sizeof(str), " %02X", byte);
        line += str;
        ascii[j] = (0x20 <= byte && byte <= 0x7E) ? byte : '.';
    }
    ascii[16] = '\0';
    line += "  ";
    line += ascii;

    return line;
}

wxCoord
MemDumpWindow::OnMeasureItem(size_t n) const
{
    return GetTextExtent("X").y + 6;
}
