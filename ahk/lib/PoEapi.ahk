;
; poeapi.ahk, 9/10/2020 8:27 PM
;

if (FileExist("..\poeapi.dll")) {
    FileMove ..\poeapi.dll, poeapi.dll, true
}

if (Not DllCall("LoadLibrary", "str", "poeapi.dll", "ptr")) {
    Msgbox, % "Load poeapi.dll failed!"
}

#Include, %A_ScriptDir%\lib\ahkpp.ahk
#Include, %A_ScriptDir%\lib\InventoryPanel.ahk
#Include, %A_ScriptDir%\lib\PoETask.ahk

; PoEapi windows messages
global WM_POEAPI_LOG       := 0x9000
global WM_PLAYER_CHANGED   := 0x9001
global WM_PLAYER_LIFE      := 0x9002
global WM_PLAYER_MANA      := 0x9003
global WM_PLAYER_ES        := 0x9004
global WM_PLAYER_DIED      := 0x9005
global WM_USE_SKILL        := 0x9006
global WM_MOVE             := 0x9007
global WM_BUFF_ADDED       := 0x9008
global WM_BUFF_REMOVED     := 0x9009
global WM_AREA_CHANGED     := 0x900a
global WM_MONSTER_CHANGED  := 0x900b
global WM_MINION_CHANGED   := 0x900c
global WM_KILL_COUNTER     := 0x900d
global WM_DELVE_CHEST      := 0x900e
global WM_PICKUP           := 0x900f
global WM_FLASK_CHANGED    := 0x9010

; Register PoEapi classes
ahkpp_register_class(PoETask)
ahkpp_register_class(PoEObject)
ahkpp_register_class(Element)
ahkpp_register_class(Inventory)
ahkpp_register_class(Stash)

class PoEObject extends AhkObj {
    
    __read(address, size) {
        return DllCall("poeapi\poeapi_read", "Ptr", address, "Int", size, "Ptr")
    }

    getByte(address) {
        dataPtr := DllCall("poeapi\poeapi_read", "Ptr", address, "Char", 1, "Ptr")
        return NumGet(dataPtr + 0, "Char")
    }

    getShort(address) {
        dataPtr := DllCall("poeapi\poeapi_read", "Ptr", address, "Short", 2, "Ptr")
        return NumGet(dataPtr + 0, "Short")
    }

    getInt(address) {
        dataPtr := DllCall("poeapi\poeapi_read", "Ptr", address, "Int", 4, "Ptr")
        return NumGet(dataPtr + 0, "Int")
    }

    getFloat(address) {
        dataPtr := DllCall("poeapi\poeapi_read", "Ptr", address, "Int", 4, "Ptr")
        return NumGet(dataPtr + 0, "Float")
    }

    getPtr(address) {
        dataPtr := DllCall("poeapi\poeapi_read", "Ptr", address, "Int", 8, "Ptr")
        return NumGet(dataPtr + 0, "Ptr")
    }

    getString(address, len) {
        dataPtr := this.__read(address, (len + 1) * 2)
        return StrGet(dataPtr + 0)
    }

    getAString(address, len) {
        dataPtr := this.__read(address, len + 1)
        return StrGet(dataPtr + 0, "utf-8")
    }

    readString(address, len = 0) {
        len := len > 0 ? len : this.getInt(address + 0x10)
        address := this.getPtr(address)
        dataPtr := this.__read(address, (len + 1) * 2)
        return StrGet(dataPtr + 0)
    }
}

class Element extends PoEObject {

    getChild(params*) {
        element := this
        for i, n in params {
            element.getChilds()
            element := element.childs[n]
        }

        return element
    }

    getPos() {
        r := this.getRect()
        x := NumGet(r + 0x0, "Int")
        y := NumGet(r + 0x4, "Int")
        w := NumGet(r + 0x8, "Int")
        h := NumGet(r + 0xc, "Int")

        return new Rect(x, y, w, h)
    }

    draw(label = "", color = "") {
        if (Not color) {
            Random, r, 0, 255
            Random, g, 0, 255
            Random, b, 0, 255
            color := b << 16 | g << 8 | r
        }

        r := this.getPos()
        if (r.w < 0 || r.h < 0)
            return

        ptask.c.drawRect(r.l, r.t, r.w, r.h, color)
        if (label)
            ptask.c.drawText(r.l, r.t, r.l + 80, r.t + 20, label, color)

        this.getChilds()
        for i, e in this.Childs {
            if (e.isVisible()) {
                r := e.getPos()
                if (r.w != 317 && r.h != 317)
                    e.draw(label ? label "." i : i)
            }
        }
    }
}

class Inventory extends InventoryPanel {

    __init() {
        this.element := this.getChild(4, 20)
        this.inventory := ptask.inventories[1]
        this.rows := this.inventory.rows
        this.cols := this.inventory.cols
        this.rect := this.element.getPos()
    }

    open() {
        if (this.isOpened())
            return true

        SendInput, %InventoryKey%
        loop, 50 {
            if (this.isOpened())
                return true
            Sleep, 20
        }

        return false
    }

    openPortal() {
        isLBttonPressed := GetKeyState("LButton")
        isMoving := ptask.player.isMoving()

        if (Not this.isOpened()) {
            SendInput {f}
            Sleep, 100
            closeInventory := true
        }

        MouseGetPos, tempX, tempY
        if (isLBttonPressed)
            SendInput {LButton up}

        aItem := this.findItem("Portal")
        if (Not aItem) {
            debug("!!! Out of ""Portal Scroll"".")
            return
        }

        this.use(aItem)
        if (closeInventory)
            SendInput {f}

        if (isMoving)
            MouseMove, tempX, tempY, 0
        else
            MouseMove, ptask.width / 2, ptask.height / 2

        if (isLBttonPressed)
            SendInput {LButton down}
    }

    use(aItem, targetItem = "", n = 1) {
        if (Not aItem || n < 1 || n > 20)
            return aItem

        if (n > 1)
            SendInput {Shift down}

        this.moveTo(aItem.Index)
        Click, Right

        if (targetItem) {
            this.moveTo(targetItem.Index)
            loop, % n {
                Click, Left
                Sleep, 100
            }
            SendInput {Shift up}

            return this.getItemByIndex(targetItem.Index)
        }
    }
}

class Stash extends Element {

    __init() {
        this.element := this.getChild(3, 1, 2, 2)
        this.r := this.getPos()
    }

    drawItems() {
        ptask.c.drawGrid(this.r.l, this.r.t, this.r.w, this.r.h, 12, 12)
    }

    draw() {
        this.element.draw()
    }
}
