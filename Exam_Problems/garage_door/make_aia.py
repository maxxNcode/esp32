"""Builds GarageDoor.aia - MIT App Inventor app for Problem 14 (Mobile-Controlled Garage Door).
Run:  python3 make_aia.py   -> creates GarageDoor.aia next to this script.

Connects to the HC-05 over Bluetooth Classic.
- OPEN DOOR sends "OPEN", CLOSE DOOR sends "CLOSE"
- Shows what the ESP32 sends: DOOR ... (door status), VEHICLE: YES/NO,
  BLOCKED - VEHICLE DETECTED (close refused)."""
import json, os, random, zipfile

NAME = "GarageDoor"
PKG = f"appinventor/ai_student/{NAME}"
here = os.path.dirname(os.path.abspath(__file__))
uid = lambda: str(random.randint(10**8, 2 * 10**9))

form = {
    "authURL": ["ai2.appinventor.mit.edu"], "YaVersion": "208", "Source": "Form",
    "Properties": {
        "$Name": "Screen1", "$Type": "Form", "$Version": "27",
        "AppName": NAME, "Title": "Garage Door",
        "AlignHorizontal": "3", "Sizing": "Responsive", "Uuid": "0",
        "$Components": [
            {"$Name": "lblTitle", "$Type": "Label", "$Version": "5",
             "Text": "Garage Door Control", "FontSize": "24", "FontBold": "True", "Uuid": uid()},
            {"$Name": "HorizontalArrangement1", "$Type": "HorizontalArrangement", "$Version": "3",
             "AlignHorizontal": "3", "Width": "-2", "Uuid": uid(), "$Components": [
                {"$Name": "lpBluetooth", "$Type": "ListPicker", "$Version": "6",
                 "Text": "SCAN BLUETOOTH", "Title": "Choose HC-05", "Uuid": uid()},
                {"$Name": "btnDisconnect", "$Type": "Button", "$Version": "7",
                 "Text": "DISCONNECT", "Uuid": uid()},
            ]},
            {"$Name": "lblBluetooth", "$Type": "Label", "$Version": "5",
             "Text": "Bluetooth: Not connected", "FontBold": "True",
             "TextColor": "&HFFFF0000", "Uuid": uid()},
            {"$Name": "lblDoor", "$Type": "Label", "$Version": "5",
             "Text": "DOOR ---", "FontSize": "34", "FontBold": "True", "Uuid": uid()},
            {"$Name": "lblVehicle", "$Type": "Label", "$Version": "5",
             "Text": "Vehicle: ---", "FontSize": "20", "FontBold": "True", "Uuid": uid()},
            {"$Name": "HorizontalArrangement2", "$Type": "HorizontalArrangement", "$Version": "3",
             "AlignHorizontal": "3", "Width": "-2", "Uuid": uid(), "$Components": [
                {"$Name": "btnOpen", "$Type": "Button", "$Version": "7",
                 "Text": "OPEN DOOR", "FontSize": "20", "FontBold": "True",
                 "Width": "-1045", "Height": "80", "BackgroundColor": "&HFF4CAF50",
                 "TextColor": "&HFFFFFFFF", "Uuid": uid()},
                {"$Name": "btnClose", "$Type": "Button", "$Version": "7",
                 "Text": "CLOSE DOOR", "FontSize": "20", "FontBold": "True",
                 "Width": "-1045", "Height": "80", "BackgroundColor": "&HFFF44336",
                 "TextColor": "&HFFFFFFFF", "Uuid": uid()},
            ]},
            {"$Name": "lblMessage", "$Type": "Label", "$Version": "5",
             "Text": "", "FontSize": "18", "FontBold": "True", "TextColor": "&HFFFF0000", "Uuid": uid()},
            {"$Name": "BluetoothClient1", "$Type": "BluetoothClient", "$Version": "4",
             "DelimiterByte": "10", "Uuid": uid()},
            {"$Name": "Clock1", "$Type": "Clock", "$Version": "4",
             "TimerInterval": "200", "Uuid": uid()},
        ],
    },
}
scm = "#|\n$JSON\n" + json.dumps(form) + "\n|#\n"

# ---------- block helpers ----------
def text(s): return f'<block type="text" id="{uid()}"><field name="TEXT">{s}</field></block>'
def num(n): return f'<block type="math_number" id="{uid()}"><field name="NUM">{n}</field></block>'
def gget(name): return f'<block type="lexical_variable_get" id="{uid()}"><field name="VAR">global {name}</field></block>'
def gset(name, value, nxt=""):
    nxt = f"<next>{nxt}</next>" if nxt else ""
    return (f'<block type="lexical_variable_set" id="{uid()}"><field name="VAR">global {name}</field>'
            f'<value name="VALUE">{value}</value>{nxt}</block>')
def set_prop(ctype, inst, prop, value, nxt=""):
    nxt = f"<next>{nxt}</next>" if nxt else ""
    return (f'<block type="component_set_get" id="{uid()}">'
            f'<mutation component_type="{ctype}" set_or_get="set" property_name="{prop}" is_generic="false" instance_name="{inst}"></mutation>'
            f'<field name="COMPONENT_SELECTOR">{inst}</field><field name="PROP">{prop}</field>'
            f'<value name="VALUE">{value}</value>{nxt}</block>')
def get_prop(ctype, inst, prop):
    return (f'<block type="component_set_get" id="{uid()}">'
            f'<mutation component_type="{ctype}" set_or_get="get" property_name="{prop}" is_generic="false" instance_name="{inst}"></mutation>'
            f'<field name="COMPONENT_SELECTOR">{inst}</field><field name="PROP">{prop}</field></block>')
def call(ctype, inst, method, *args, nxt=""):
    nxt = f"<next>{nxt}</next>" if nxt else ""
    vals = "".join(f'<value name="ARG{i}">{a}</value>' for i, a in enumerate(args))
    return (f'<block type="component_method" id="{uid()}">'
            f'<mutation component_type="{ctype}" method_name="{method}" is_generic="false" instance_name="{inst}"></mutation>'
            f'<field name="COMPONENT_SELECTOR">{inst}</field>{vals}{nxt}</block>')
def event(ctype, inst, name, body, x, y):
    return (f'<block type="component_event" id="{uid()}" x="{x}" y="{y}">'
            f'<mutation component_type="{ctype}" is_generic="false" instance_name="{inst}" event_name="{name}"></mutation>'
            f'<field name="COMPONENT_SELECTOR">{inst}</field><statement name="DO">{body}</statement></block>\n')
def if_else(test, then, other):
    return (f'<block type="controls_if" id="{uid()}"><mutation else="1"></mutation>'
            f'<value name="IF0">{test}</value><statement name="DO0">{then}</statement>'
            f'<statement name="ELSE">{other}</statement></block>')
def if_only(test, then):
    return (f'<block type="controls_if" id="{uid()}"><value name="IF0">{test}</value>'
            f'<statement name="DO0">{then}</statement></block>')
def color(name, hexv):
    return f'<block type="color_{name}" id="{uid()}"><field name="COLOR">{hexv}</field></block>'

bt = ("BluetoothClient", "BluetoothClient1")
connected = get_prop(*bt, "IsConnected")
def compare(op, a, b):
    return (f'<block type="logic_compare" id="{uid()}"><field name="OP">{op}</field>'
            f'<value name="A">{a}</value><value name="B">{b}</value></block>')

blocks = (f'<block type="global_declaration" id="{uid()}" x="20" y="20"><field name="NAME">line</field>'
          f'<value name="VALUE">{text("")}</value></block>\n')

# SCAN BLUETOOTH: list paired devices, connect to the chosen one
blocks += event("ListPicker", "lpBluetooth", "BeforePicking",
                set_prop("ListPicker", "lpBluetooth", "Elements", get_prop(*bt, "AddressesAndNames")), 20, 80)
blocks += event("ListPicker", "lpBluetooth", "AfterPicking",
                if_else(call(*bt, "Connect", get_prop("ListPicker", "lpBluetooth", "Selection")),
                        set_prop("Label", "lblBluetooth", "Text", text("Bluetooth: Connected"),
                                 set_prop("Label", "lblBluetooth", "TextColor", color("green", "#00ff00"))),
                        set_prop("Label", "lblBluetooth", "Text", text("Bluetooth: Connection failed"))),
                20, 160)
blocks += event("Button", "btnDisconnect", "Click",
                call(*bt, "Disconnect",
                     nxt=set_prop("Label", "lblBluetooth", "Text", text("Bluetooth: Not connected"),
                                  set_prop("Label", "lblBluetooth", "TextColor", color("red", "#ff0000")))),
                20, 320)

# OPEN DOOR / CLOSE DOOR: send the command
def send_button(btn, cmd_text, y):
    return event("Button", btn, "Click",
                 if_else(connected,
                         call(*bt, "SendText", text(cmd_text),
                              nxt=set_prop("Label", "lblMessage", "Text", text(f"Sent {cmd_text}"))),
                         set_prop("Label", "lblMessage", "Text", text("Connect to HC-05 first"))),
                 20, y)
blocks += send_button("btnOpen", "OPEN", 420)
blocks += send_button("btnClose", "CLOSE", 520)

# Every 200 ms: read one line from the ESP32 and show it
got_data = (f'<block type="logic_operation" id="{uid()}"><field name="OP">AND</field>'
            f'<value name="A">{connected}</value>'
            f'<value name="B"><block type="math_compare" id="{uid()}"><field name="OP">GT</field>'
            f'<value name="A">{get_prop(*bt, "BytesAvailableToReceive")}</value><value name="B">{num(0)}</value>'
            f'</block></value></block>')
show = (f'<block type="controls_if" id="{uid()}"><mutation elseif="2" else="1"></mutation>'
        f'<value name="IF0">{compare("EQ", gget("line"), text("VEHICLE: YES"))}</value>'
        f'<statement name="DO0">{set_prop("Label", "lblVehicle", "Text", text("Vehicle: DETECTED"), set_prop("Label", "lblVehicle", "TextColor", color("red", "#ff0000")))}</statement>'
        f'<value name="IF1">{compare("EQ", gget("line"), text("VEHICLE: NO"))}</value>'
        f'<statement name="DO1">{set_prop("Label", "lblVehicle", "Text", text("Vehicle: none"), set_prop("Label", "lblVehicle", "TextColor", color("green", "#00ff00")))}</statement>'
        f'<value name="IF2">{compare("EQ", gget("line"), text("BLOCKED - VEHICLE DETECTED"))}</value>'
        f'<statement name="DO2">{set_prop("Label", "lblMessage", "Text", text("Cannot close: vehicle detected!"))}</statement>'
        f'<statement name="ELSE">{set_prop("Label", "lblDoor", "Text", gget("line"))}</statement>'
        f'</block>')
line_set = (f'<block type="lexical_variable_set" id="{uid()}"><field name="VAR">global line</field>'
            f'<value name="VALUE"><block type="text_trim" id="{uid()}"><value name="TEXT">{call(*bt, "ReceiveText", num(-1))}</value></block></value>'
            f'<next>{show}</next></block>')
blocks += event("Clock", "Clock1", "Timer", if_only(got_data, line_set), 20, 640)

bky = ('<xml xmlns="http://www.w3.org/1999/xhtml">\n' + blocks
       + '<yacodeblocks ya-version="208" language-version="33"></yacodeblocks>\n</xml>\n')

props = f"""main=appinventor.ai_student.{NAME}.Screen1
name={NAME}
assets=../assets
source=../src
build=../build
versioncode=1
versionname=1.0
useslocation=False
aname={NAME}
sizing=Responsive
showlistsasjson=True
tutorialurl=
subsetjson=
actionbar=True
theme=Classic
color.primary=&HFF3F51B5
color.primary.dark=&HFF303F9F
color.accent=&HFFFF4081
"""

out = os.path.join(here, f"{NAME}.aia")
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("youngandroidproject/project.properties", props)
    z.writestr(f"src/{PKG}/Screen1.scm", scm)
    z.writestr(f"src/{PKG}/Screen1.bky", bky)
print("wrote", out)
