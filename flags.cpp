// flags.cpp

#include <wx/wx.h>
#include <algorithm>
#include "flags.h"

class wxPoint;
class wxSize;

FlagsDialog::FlagsDialog(
        wxWindow *parent, wxWindowID id, const wxString &title,
        const wxPoint &pos, const wxSize &size, long style,
        const wxString &name) :
        wxDialog(parent, id, title, pos, size, style, name)
{
}

void
FlagsDialog::SetFlags(CPU *cpu, const std::string& flags)
{
    m_flags = cpu->GetFlags();

    auto sizer0 = new wxBoxSizer(wxVERTICAL);

    m_checkboxes.clear();
    for (auto i = m_flags.begin(); i != m_flags.end(); ++i) {
        auto button = new wxCheckBox(this, wxID_ANY, i->name);
        sizer0->Add(button, 0);
        button->SetValue(flags.find(i->letter) != std::string::npos);
        m_checkboxes.push_back(button);
    }

    auto sizer1 = CreateButtonSizer(wxOK | wxCANCEL);
    sizer0->Add(sizer1, 0);

    SetSizerAndFit(sizer0);
}

std::string
FlagsDialog::GetFlags(void) const
{
    std::string flags;

    for (unsigned i = 0; i < m_flags.size(); ++i) {
        auto flag = m_flags.at(i);
        auto button = m_checkboxes.at(i);

        flags += button->GetValue() ? flag.letter : '-';
    }

    return flags;
}
