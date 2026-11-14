#include <wx/wx.h>
#include <wx/valnum.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/timer.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <cstdint>

// ============================================================================
//  Tema: palette pastello ispirata alla fattoria (chiaro) + versione notte (scuro)
//  Ogni colore dipende da theme::Dark(): basta cambiare quel flag e ridisegnare.
// ============================================================================
namespace theme {
    inline bool& Dark() { static bool dark = false; return dark; }

    inline wxColour Pick(const wxColour& light, const wxColour& dark) { return Dark() ? dark : light; }

    inline wxColour Bg()        { return Pick(wxColour(239, 246, 251), wxColour( 22,  27,  38)); }
    inline wxColour Card()      { return Pick(wxColour(255, 255, 255), wxColour( 33,  40,  55)); }
    inline wxColour Ink()       { return Pick(wxColour( 58,  72,  94), wxColour(232, 238, 247)); }
    inline wxColour Muted()     { return Pick(wxColour(128, 146, 166), wxColour(140, 156, 180)); }
    inline wxColour Field()     { return Pick(wxColour(244, 249, 253), wxColour( 25,  31,  44)); }
    inline wxColour FieldEdge() { return Pick(wxColour(214, 228, 240), wxColour( 55,  65,  86)); }
    inline wxColour Focus()     { return {122, 184, 232}; }
    inline wxColour BtnFrom()   { return {255, 196, 164}; }
    inline wxColour BtnTo()     { return {255, 224, 150}; }
    inline wxColour BtnText()   { return { 96,  62,  40}; }
    inline wxColour Sage()      { return Pick(wxColour(216, 238, 222), wxColour( 36,  66,  54)); }
    inline wxColour SageText()  { return Pick(wxColour( 47,  98,  70), wxColour(182, 230, 204)); }
    inline wxColour Shadow()    { return Pick(wxColour( 90, 120, 160,  6), wxColour(  0,   0,   0, 12)); }

    inline wxColour Mix(const wxColour& a, const wxColour& b, double t) {
        auto m = [t](int x, int y) { return static_cast<unsigned char>(x + (y - x) * t + 0.5); };
        return {m(a.Red(), b.Red()), m(a.Green(), b.Green()), m(a.Blue(), b.Blue())};
    }

    inline wxFont Font(int pt, bool bold = false) {
        wxFont f = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
        f.SetPointSize(pt);
        f.SetWeight(bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
        return f;
    }
}

// ============================================================================
//  SoftPanel: Stile con wxGraphicsContext
// ============================================================================
class SoftPanel : public wxPanel {
public:
    SoftPanel(wxWindow *parent, wxColour (*fill)(), double radius, bool shadow)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE),
          fill_(fill), radius_(radius), shadow_(shadow) {
        SetBackgroundColour(fill_());            // i figli ereditano il colore della card
        SetBackgroundStyle(wxBG_STYLE_PAINT);    // disegniamo tutto noi
        Bind(wxEVT_PAINT, &SoftPanel::OnPaint, this);
    }

    void Restyle() { SetBackgroundColour(fill_()); Refresh(); }

    int ContentBorder() const { return (shadow_ ? kShadow : 0) + FromDIP(20); }

private:
    static constexpr int kShadow = 10;

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        const double w = GetClientSize().x;
        const double h = GetClientSize().y;
        const double in = shadow_ ? kShadow : 0;

        gc->SetPen(*wxTRANSPARENT_PEN);

        if (shadow_) {
            for (int i = 0; i < kShadow; ++i) {
                gc->SetBrush(wxBrush(theme::Shadow()));
                gc->DrawRoundedRectangle(i, i + 3, w - 2 * i, h - 2 * i - 3,
                                         radius_ + (kShadow - i) * 0.6);
            }
        }

        gc->SetBrush(wxBrush(fill_()));
        gc->DrawRoundedRectangle(in, in, w - 2 * in, h - 2 * in, radius_);
    }

    wxColour (*fill_)();
    double radius_;
    bool shadow_;
};

// ============================================================================
//  SoftInput: Stile per wxTextCtrl
// ============================================================================
class SoftInput : public wxPanel {
public:
    explicit SoftInput(wxWindow *parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize(-1, FromDIP(46)));
        Bind(wxEVT_PAINT, &SoftInput::OnPaint, this);
    }

    void Attach(wxTextCtrl *ctrl) {
        ctrl->SetFont(theme::Font(13));

        ctrl->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& e) { focused_ = true;  Refresh(); e.Skip(); });
        ctrl->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { focused_ = false; Refresh(); e.Skip(); });
        Bind(wxEVT_LEFT_DOWN, [ctrl](wxMouseEvent&) { ctrl->SetFocus(); });

        auto *s = new wxBoxSizer(wxHORIZONTAL);
        s->Add(ctrl, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(16));
        SetSizer(s);
    }

private:
    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        const double w = GetClientSize().x;
        const double h = GetClientSize().y;
        const double r = 14;

        if (focused_) {
            wxColour glow = theme::Focus();
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(wxColour(glow.Red(), glow.Green(), glow.Blue(), 60)));
            gc->DrawRoundedRectangle(0, 0, w, h, r + 2);
        }

        gc->SetPen(wxPen(focused_ ? theme::Focus() : theme::FieldEdge(), 1));
        gc->SetBrush(wxBrush(theme::Field()));
        gc->DrawRoundedRectangle(2, 2, w - 4, h - 4, r);
    }

    bool focused_ = false;
};

// ============================================================================
//  SoftButton: Stile bottone
// ============================================================================
class SoftButton : public wxPanel {
public:
    SoftButton(wxWindow *parent, wxWindowID id, const wxString& label)
        : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE),
          label_(label) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));
        SetMinSize(wxSize(-1, FromDIP(56)));

        timer_.SetOwner(this);
        Bind(wxEVT_TIMER, &SoftButton::OnTimer, this);
        Bind(wxEVT_PAINT, &SoftButton::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { hovering_ = true;  timer_.Start(15); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
            hovering_ = false; pressed_ = false; timer_.Start(15); Refresh();
        });
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { pressed_ = true; Refresh(); });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
            if (!pressed_) return;
            pressed_ = false;
            Refresh();
            auto *evt = new wxCommandEvent(wxEVT_BUTTON, GetId());
            evt->SetEventObject(this);
            wxQueueEvent(GetParent(), evt);
        });
    }

private:
    void OnTimer(wxTimerEvent&) {
        const double target = hovering_ ? 1.0 : 0.0;
        hover_ += (target - hover_) * 0.25; // ease-out
        if (std::fabs(target - hover_) < 0.01) {
            hover_ = target;
            timer_.Stop();
        }
        Refresh();
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        const double w = GetClientSize().x;
        const double h = GetClientSize().y;

        const double dy = pressed_ ? 1.5 : -hover_ * 1.5;   // si solleva in hover, si abbassa al click
        const double bx = 4, by = 2 + dy, bw = w - 8, bh = h - 10;
        const double r = bh / 2;                            // pillola

        const unsigned char shadowAlpha = theme::Dark() ? 7 : 10;
        gc->SetPen(*wxTRANSPARENT_PEN);
        for (int i = 0; i < 5; ++i) {
            const int ex = 4 - i;
            gc->SetBrush(wxBrush(wxColour(240, 170, 120, shadowAlpha)));
            gc->DrawRoundedRectangle(bx - ex, by + 4 - ex + 1, bw + 2 * ex, bh + 2 * ex, r + ex);
        }

        wxColour c1 = theme::Mix(theme::BtnFrom(), *wxWHITE, hover_ * 0.14);
        wxColour c2 = theme::Mix(theme::BtnTo(),   *wxWHITE, hover_ * 0.14);
        if (pressed_) {
            c1 = theme::Mix(c1, *wxBLACK, 0.06);
            c2 = theme::Mix(c2, *wxBLACK, 0.06);
        }
        gc->SetBrush(gc->CreateLinearGradientBrush(bx, by, bx + bw, by + bh, c1, c2));
        gc->DrawRoundedRectangle(bx, by, bw, bh, r);

        gc->SetFont(theme::Font(13, true), theme::BtnText());
        double tw = 0, th = 0;
        gc->GetTextExtent(label_, &tw, &th);
        gc->DrawText(label_, bx + (bw - tw) / 2, by + (bh - th) / 2);
    }

    wxString label_;
    wxTimer timer_;
    double hover_ = 0.0;
    bool hovering_ = false;
    bool pressed_ = false;
};

// ============================================================================
//  SoftToggle: Bottone cambio tema
// ============================================================================
class SoftToggle : public wxPanel {
public:
    SoftToggle(wxWindow *parent, wxWindowID id)
        : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));
        SetMinSize(FromDIP(wxSize(44, 44)));
        SetToolTip("Cambia tema");

        timer_.SetOwner(this);
        Bind(wxEVT_TIMER, &SoftToggle::OnTimer, this);
        Bind(wxEVT_PAINT, &SoftToggle::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { hovering_ = true;  timer_.Start(15); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
            hovering_ = false; pressed_ = false; timer_.Start(15); Refresh();
        });
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { pressed_ = true; Refresh(); });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
            if (!pressed_) return;
            pressed_ = false;
            Refresh();
            auto *evt = new wxCommandEvent(wxEVT_BUTTON, GetId());
            evt->SetEventObject(this);
            wxQueueEvent(GetParent(), evt);
        });
    }

private:
    void OnTimer(wxTimerEvent&) {
        const double target = hovering_ ? 1.0 : 0.0;
        hover_ += (target - hover_) * 0.25;
        if (std::fabs(target - hover_) < 0.01) {
            hover_ = target;
            timer_.Stop();
        }
        Refresh();
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        const double w = GetClientSize().x;
        const double h = GetClientSize().y;
        const double k = std::min(w, h) / 44.0;
        const double cx = w / 2, cy = h / 2 + (pressed_ ? 0.5 : 0);
        const double R = std::min(w, h) / 2 - 4 * k;

        gc->SetPen(*wxTRANSPARENT_PEN);
        for (int i = 0; i < 3; ++i) {
            const double ex = (3 - i) * k;
            gc->SetBrush(wxBrush(theme::Shadow()));
            gc->DrawEllipse(cx - R - ex, cy - R - ex + 2 * k, 2 * (R + ex), 2 * (R + ex));
        }

        const wxColour fill = theme::Mix(theme::Card(), theme::BtnTo(), hover_ * 0.35);
        gc->SetPen(wxPen(theme::FieldEdge(), 1));
        gc->SetBrush(wxBrush(fill));
        gc->DrawEllipse(cx - R, cy - R, 2 * R, 2 * R);

        if (theme::Dark()) {
            const wxColour sun(255, 196, 80);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(sun));
            gc->DrawEllipse(cx - 3.6 * k, cy - 3.6 * k, 7.2 * k, 7.2 * k);

            gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(sun).Width(1.8 * k).Cap(wxCAP_ROUND)));
            for (int i = 0; i < 8; ++i) {
                const double a = i * 3.14159265358979 / 4;
                gc->StrokeLine(cx + std::cos(a) * 6.6 * k, cy + std::sin(a) * 6.6 * k,
                               cx + std::cos(a) * 9.2 * k, cy + std::sin(a) * 9.2 * k);
            }
        } else {
            const wxColour moon(112, 126, 168);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(moon));
            const double mx = cx + 1.0 * k, my = cy - 0.9 * k;
            const double rm = 7.0 * k;
            const double rc = 5.7 * k;
            gc->DrawEllipse(mx - rm, my - rm, 2 * rm, 2 * rm);
            gc->SetBrush(wxBrush(fill));
            gc->DrawEllipse(mx + 4.1 * k - rc, my - 3.4 * k - rc, 2 * rc, 2 * rc);
        }
    }

    wxTimer timer_;
    double hover_ = 0.0;
    bool hovering_ = false;
    bool pressed_ = false;
};

// ============================================================================
//  App
// ============================================================================
class MyApp : public wxApp {
public:
    bool OnInit() override;
};

class MyFrame : public wxFrame {
public:
    MyFrame();

private:
    uint32_t costoFinale = 0;

    wxStaticText *prezzInizTxt = nullptr;
    wxStaticText *prezzFinTxt = nullptr;
    wxStaticText *quantitaTxt = nullptr;

    std::string costoTotTesto = "Il costo totale è ancora da calcolare!";
    wxStaticText *costoTotTxt = nullptr;

    wxTextCtrl *prezzInizTxtFld;
    wxTextCtrl *prezzFinlTxtFld;
    wxTextCtrl *quantitaTxtFld;

    wxPanel *rootPanel_ = nullptr;
    wxStaticText *title_ = nullptr;
    wxStaticText *subtitle_ = nullptr;
    SoftPanel *card_ = nullptr;
    SoftPanel *resultCard_ = nullptr;

    static uint32_t calcolaPrezzo(uint16_t prezzoIniziale, uint16_t prezzoFinale, uint16_t quantita) {
        uint32_t costoTot = 0;

        for(int i = 0; i < quantita; ++i) costoTot += prezzoIniziale + ((prezzoFinale-prezzoIniziale) * i);

        return costoTot;
    }

    void OnButtonClick(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    void OnThemeToggle(wxCommandEvent& event);
    void ApplyTheme();

    enum {
        ID_AddGame = 1,
        ID_Button = 2,
        ID_Theme = 3
    };
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit() {
    auto *frame = new MyFrame();
    frame->Show(true);
    return true;
}

MyFrame::MyFrame() : wxFrame(nullptr, wxID_ANY, "Calcolatore costi di Hay Day") {
    theme::Dark() = wxSystemSettings::GetAppearance().IsDark();

    wxMenu *menuEdit = new wxMenu;
    menuEdit->Append(ID_AddGame, "&Calcola il costo finale\tCtrl-G");

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuEdit, "&Calcola");
    SetMenuBar(menuBar);

    // Sfondo
    wxPanel *panel = new wxPanel(this, wxID_ANY);
    rootPanel_ = panel;
    panel->SetBackgroundColour(theme::Bg());

    auto *root = new wxBoxSizer(wxVERTICAL);

    // Intestazione
    title_ = new wxStaticText(panel, wxID_ANY, "Calcolatore Hay Day");
    title_->SetFont(theme::Font(22, true));

    subtitle_ = new wxStaticText(panel, wxID_ANY, "Quanto ti costa davvero comprare tutto?");
    subtitle_->SetFont(theme::Font(12));

    auto *titles = new wxBoxSizer(wxVERTICAL);
    titles->Add(title_, 0);
    titles->AddSpacer(FromDIP(4));
    titles->Add(subtitle_, 0);

    auto *themeBtn = new SoftToggle(panel, ID_Theme);

    auto *head = new wxBoxSizer(wxHORIZONTAL);
    head->Add(titles, 1, wxALIGN_CENTER_VERTICAL);
    head->Add(themeBtn, 0, wxALIGN_CENTER_VERTICAL);

    root->AddSpacer(FromDIP(24));
    root->Add(head, 0, wxEXPAND | wxLEFT, FromDIP(28));
    root->AddSpacer(FromDIP(10));

    // Card dei campi
    auto *card = new SoftPanel(panel, &theme::Card, 22, true);
    card_ = card;
    auto *fields = new wxBoxSizer(wxVERTICAL);

    uint16_t defInt = 0;
    wxIntegerValidator<uint16_t> intVal(&defInt);

    auto addField = [&](wxStaticText*& lbl, const wxString& text, wxTextCtrl*& fld, bool last) {
        lbl = new wxStaticText(card, wxID_ANY, text);
        lbl->SetFont(theme::Font(11, true));

        auto *box = new SoftInput(card);
        fld = new wxTextCtrl(box, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, intVal);
        box->Attach(fld);

        fields->Add(lbl, 0, wxLEFT, FromDIP(4));
        fields->AddSpacer(FromDIP(6));
        fields->Add(box, 0, wxEXPAND);
        if (!last) fields->AddSpacer(FromDIP(16));
    };

    addField(prezzInizTxt, "Costo dell'oggetto iniziale",   prezzInizTxtFld, false);
    addField(prezzFinTxt,  "Costo dell'oggetto dopo",        prezzFinlTxtFld, false);
    addField(quantitaTxt,  "Quanti oggetti vuoi acquistare", quantitaTxtFld,  true);

    auto *cardOuter = new wxBoxSizer(wxVERTICAL);
    cardOuter->Add(fields, 1, wxEXPAND | wxALL, card->ContentBorder());
    card->SetSizer(cardOuter);

    root->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(14)); // 14 + 10 d'ombra = 24 visivi
    root->AddSpacer(FromDIP(8));

    // Bottone
    auto *button = new SoftButton(panel, ID_Button, "Calcola");
    root->Add(button, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
    root->AddSpacer(FromDIP(12));

    // Risultato
    resultCard_ = new SoftPanel(panel, &theme::Sage, 18, false);
    costoTotTxt = new wxStaticText(resultCard_, wxID_ANY, this->costoTotTesto,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
    costoTotTxt->SetFont(theme::Font(12, true));

    auto *resultSizer = new wxBoxSizer(wxVERTICAL);
    resultSizer->Add(costoTotTxt, 0, wxEXPAND | wxALL, FromDIP(20));
    resultCard_->SetSizer(resultSizer);

    root->Add(resultCard_, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
    root->AddSpacer(FromDIP(24));

    root->SetMinSize(FromDIP(wxSize(440, 0)));
    panel->SetSizer(root);

    auto *frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(panel, 1, wxEXPAND);
    SetSizer(frameSizer);
    frameSizer->SetSizeHints(this);
    Centre();

    ApplyTheme();

    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_BUTTON, &MyFrame::OnButtonClick, this, ID_Button);
    Bind(wxEVT_BUTTON, &MyFrame::OnThemeToggle, this, ID_Theme);
}

// Applica colori
void MyFrame::ApplyTheme() {
    rootPanel_->SetBackgroundColour(theme::Bg());
    card_->Restyle();
    resultCard_->Restyle();

    title_->SetBackgroundColour(theme::Bg());
    title_->SetForegroundColour(theme::Ink());
    subtitle_->SetBackgroundColour(theme::Bg());
    subtitle_->SetForegroundColour(theme::Muted());

    for (wxStaticText *lbl : {prezzInizTxt, prezzFinTxt, quantitaTxt}) {
        lbl->SetBackgroundColour(theme::Card());
        lbl->SetForegroundColour(theme::Muted());
    }

    for (wxTextCtrl *fld : {prezzInizTxtFld, prezzFinlTxtFld, quantitaTxtFld}) {
        fld->SetBackgroundColour(theme::Field());
        fld->SetForegroundColour(theme::Ink());
    }

    costoTotTxt->SetBackgroundColour(theme::Sage());
    costoTotTxt->SetForegroundColour(theme::SageText());

    std::function<void(wxWindow*)> refreshAll = [&](wxWindow *w) {
        w->Refresh();
        for (wxWindow *c : w->GetChildren()) refreshAll(c);
    };
    refreshAll(rootPanel_);
}

void MyFrame::OnThemeToggle(wxCommandEvent& event) {
    theme::Dark() = !theme::Dark();
    ApplyTheme();
}

void MyFrame::OnExit(wxCommandEvent& event) {
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& event) {
    wxMessageBox("Calcola il costo dei prezzi degli oggetti in Hay Day!", "About MyApp", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnButtonClick(wxCommandEvent& event) {
    unsigned long tempVal = 0;
    uint16_t inizio;
    if (prezzInizTxtFld->GetValue().ToULong(&tempVal) && tempVal <= UINT16_MAX) {
        inizio = static_cast<uint16_t>(tempVal);
    } else {
        inizio = 0;
    }

    uint16_t fine;
    if (prezzFinlTxtFld->GetValue().ToULong(&tempVal) && tempVal <= UINT16_MAX) {
        fine = static_cast<uint16_t>(tempVal);
    } else {
        fine = 0;
    }

    uint16_t quantita;
    if (quantitaTxtFld->GetValue().ToULong(&tempVal) && tempVal <= UINT16_MAX) {
        quantita = static_cast<uint16_t>(tempVal);
    } else {
        quantita = 0;
    }

    this->costoFinale = calcolaPrezzo(inizio, fine, quantita);

    costoTotTxt->SetLabel("Il costo totale è: " + std::to_string(this->costoFinale) + "!");

    wxMessageBox("Costo finale calcolato!", "Calcolo completato", wxOK | wxICON_AUTH_NEEDED);
}