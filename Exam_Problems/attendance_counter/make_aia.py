"""Builds AttendanceCounter.aia - MIT App Inventor app for the attendance counter.
Run:  python3 make_aia.py   -> creates AttendanceCounter.aia next to this script.

The app connects to the HC-05 over Bluetooth, sends "C" when GET COUNT is
pressed, and shows the Arduino's reply ("Attendance count: N") as a big number."""
import json, os, random, zipfile

NAME = "AttendanceCounter"
PKG = f"appinventor/ai_student/{NAME}"
here = os.path.dirname(os.path.abspath(__file__))
uid = lambda: str(random.randint(10**8, 2 * 10**9))

form = {
    "authURL": ["ai2.appinventor.mit.edu"], "YaVersion": "208", "Source": "Form",
    "Properties": {
        "$Name": "Screen1", "$Type": "Form", "$Version": "27",
        "AppName": NAME, "Title": "Attendance Counter",
        "AlignHorizontal": "3", "Sizing": "Responsive", "Uuid": "0",
        "$Components": [
            {"$Name": "lblTitle", "$Type": "Label", "$Version": "5",
             "Text": "Attendance Counter", "FontSize": "24", "FontBold": "True", "Uuid": uid()},
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
            {"$Name": "btnGetCount", "$Type": "Button", "$Version": "7",
             "Text": "GET ATTENDANCE COUNT", "FontSize": "18", "FontBold": "True",
             "Width": "-1080", "Height": "70", "BackgroundColor": "&HFF2196F3",
             "TextColor": "&HFFFFFFFF", "Uuid": uid()},
            {"$Name": "lblCountTitle", "$Type": "Label", "$Version": "5",
             "Text": "Present:", "FontSize": "18", "Uuid": uid()},
            {"$Name": "lblCount", "$Type": "Label", "$Version": "5",
             "Text": "-", "FontSize": "72", "FontBold": "True", "TextColor": "&HFF4CAF50",
             "Uuid": uid()},
            {"$Name": "lblReply", "$Type": "Label", "$Version": "5",
             "Text": "Connect to HC-05, then tap GET ATTENDANCE COUNT", "Uuid": uid()},
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

blocks = (f'<block type="global_declaration" id="{uid()}" x="20" y="20"><field name="NAME">reply</field>'
          f'<value name="VALUE">{text("")}</value></block>\n')

# SCAN: list paired devices, connect to the chosen one
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

# GET COUNT: send "C" to the Arduino
blocks += event("Button", "btnGetCount", "Click",
                if_else(connected,
                        call(*bt, "SendText", text("C"),
                             nxt=set_prop("Label", "lblReply", "Text", text("Requesting count..."))),
                        set_prop("Label", "lblReply", "Text", text("Connect to HC-05 first (SCAN BLUETOOTH)"))),
                20, 420)

# Every 200 ms: if a reply arrived, read one line and show the number
got_data = (f'<block type="logic_operation" id="{uid()}"><field name="OP">AND</field>'
            f'<value name="A">{connected}</value>'
            f'<value name="B"><block type="math_compare" id="{uid()}"><field name="OP">GT</field>'
            f'<value name="A">{get_prop(*bt, "BytesAvailableToReceive")}</value><value name="B">{num(0)}</value>'
            f'</block></value></block>')
number = (f'<block type="text_trim" id="{uid()}"><value name="TEXT">'
          f'<block type="text_replace_all" id="{uid()}"><value name="TEXT">{gget("reply")}</value>'
          f'<value name="SEGMENT">{text("Attendance count:")}</value>'
          f'<value name="REPLACEMENT">{text("")}</value></block></value></block>')
blocks += event("Clock", "Clock1", "Timer",
                if_only(got_data,
                        gset("reply", call(*bt, "ReceiveText", num(-1)),
                             set_prop("Label", "lblReply", "Text", gget("reply"),
                                      set_prop("Label", "lblCount", "Text", number)))),
                20, 560)

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
