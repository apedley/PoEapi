;
; Logger.ahk, 8/30/2020 1:33 PM
;

class Logger {

    level := 0

    __new(title, filename = "", level = 0) {
        static __mshtml

        Gui, LogWin:New, +HwndHwnd +LastFound, % title
        Gui, LogWin:Margin, 5
        Gui, Font, 10, Courier New
        Gui, LogWin:Add, ActiveX, Border VScroll r35 w690 v__mshtml, about:
        poe.activate()
        Gui, LogWin:Show, Hide x10 y50 w700 h460

        clearMethod := ObjBindMethod(this, "clear")
        Menu, Tray, NoStandard
        Menu, Tray, Add, Clear Log History, %clearMethod%
        Menu, Tray, Add
        Menu, Tray, Standard

        this.Hwnd := Hwnd
        this.filename := filename
        this.logFile := FileOpen(filename, "a", "utf-8")
        this.level := level
        this.__delayedHideTimer := ObjBindMethod(this, "__delayedHide")

        this.doc := __mshtml.Document
        this.doc.write("<pre style=""font-size:15px; line-height:1.2"">")
        this.__log("<pre>")
        this.logId := 0
    }

    clear() {
        this.logFile.Close()
        this.logFile := FileOpen(this.filename, "w", "utf-8")
    }

    log(aText, id = "", level = 0) {
        if (id == "") {
            ; Increment logId by 1
            this.logId += 1
            id := "#" this.logId
        }

        FormatTime, t,, MM/dd hh:mm:ss
        aText := Format("<span id=""{}""><span style=""color:dimgray"">{}</span> {}</span>`n"
                       , id, t, aText)

        ; Always save to log file
        this.__log(aText)

        if (level >= this.level) {
            this.doc.write(aText)
            this.doc.parentWindow.scrollBy(0, 20)
            this.show(false)

            return id
        }
    }

    rlog(id, aText, append = false, level = 0) {
        element := this.doc.getElementById(id)
        if (Not element)
            return this.log(aText, id, level)

        FormatTime, t,, MM/dd hh:mm:ss
        if (append)
            element.innerHtml .= Format("<br><span style=""color:black"">{}</span> {}`n", t, aText)
        else
            element.innerHtml := Format("<span style=""color:black"">{}</span> {}`n", t, aText)
    }

    show(active = true) {
        if (active && WinActive("ahk_class AutoHotkeyGUI") == this.Hwnd) {
            this.hide()
            return
        }

        Gui, LogWin:Show, % !active ? "NoActivate" :
    }

    hide() {
        Gui, LogWin:Hide
    }

    __log(aText) {
        this.logFile.Write(aText)
        DllCall("FlushFileBuffers", "uint", this.logFile.__Handle)
    }
}

debug(string, args*) {
    logger.log(Format("[D] <i>" string "</i>", args*), 0)
}

rdebug(id, string, args*) {
    logger.rlog(id, Format("[D] <i>" string "</i>", args*), 0)
}

syslog(string, args*) {
    logger.log(Format("[I] " string, args*),, 1)
}

rsyslog(id, string, args*) {
    logger.rlog(id, Format("[I] " string, args*), true, 1)
}

error(string, args*) {
    logger.log(Format("[E]<span style=""color: red""> !!! " string "</span>", args*), 2)
    SendInput {F8}
}
