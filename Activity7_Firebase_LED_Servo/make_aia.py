"""Builds Activity7.aia - the MIT App Inventor project for Activity 7.
Run:  python3 make_aia.py   -> creates Activity7.aia next to this script.

Every button sends "TAG:VALUE;" to the ESP32 over Bluetooth (when connected)
AND saves the value to Firebase with the Web component and the REST API
(PUT <url><tag>.json), so Bluetooth, online and the database stay in sync.
The app reads the whole database every 1.5 s and shows the live state. This works with every database region, unlike the
FirebaseDB component (black screen with *.firebasedatabase.app URLs)."""
import json, os, random, zipfile

PKG = "appinventor/ai_student/Activity7"
here = os.path.dirname(os.path.abspath(__file__))

FIREBASE_URL = "https://activity7-8fac0-default-rtdb.asia-southeast1.firebasedatabase.app/"

# (button name, text, firebase tag, value) - laid out 2 per row like the whiteboard
BUTTONS = [
    ("btnLed1On", "LED1 ON", "LED1", 1), ("btnLed1Off", "LED1 OFF", "LED1", 0),
    ("btnLed2On", "LED2 ON", "LED2", 1), ("btnLed2Off", "LED2 OFF", "LED2", 0),
    ("btnLed3On", "LED3 ON", "LED3", 1), ("btnLed3Off", "LED3 OFF", "LED3", 0),
    ("btnLed4On", "LED4 ON", "LED4", 1), ("btnLed4Off", "LED4 OFF", "LED4", 0),
    ("btnAllOn", "ALL LED ON", "ALL_LED", 1), ("btnAllOff", "ALL LED OFF", "ALL_LED", 0),
    ("btnServo90", "SERVO 90", "SERVO", 90), ("btnServo180", "SERVO 180", "SERVO", 180),
    ("btnServo0", "SERVO 0", "SERVO", 0),
]

uid = lambda: str(random.randint(10**8, 2 * 10**9))

cells = []
for i, (name, text_, _, _) in enumerate(BUTTONS):
    cells.append({"$Name": name, "$Type": "Button", "$Version": "7",
                  "Column": str(i % 2), "Row": str(i // 2),
                  "Text": text_, "Width": "-1050", "Uuid": uid()})

form = {
    "authURL": ["ai2.appinventor.mit.edu"], "YaVersion": "208", "Source": "Form",
    "Properties": {
        "$Name": "Screen1", "$Type": "Form", "$Version": "27",
        "AppName": "Activity7", "Title": "Activity 7 - ESP32 Control",
        "AlignHorizontal": "3", "Sizing": "Responsive", "Uuid": "0",
        "$Components": [
            {"$Name": "lblTitle", "$Type": "Label", "$Version": "5",
             "Text": "ESP32 LED + Servo (Firebase)", "FontSize": "20",
             "FontBold": "True", "Uuid": uid()},
            {"$Name": "HorizontalArrangement1", "$Type": "HorizontalArrangement", "$Version": "3",
             "AlignHorizontal": "3", "Width": "-2", "Uuid": uid(), "$Components": [
                {"$Name": "lpBluetooth", "$Type": "ListPicker", "$Version": "6",
                 "Text": "SCAN BLUETOOTH", "Title": "Choose ESP32_Activity7", "Uuid": uid()},
                {"$Name": "btnDisconnect", "$Type": "Button", "$Version": "7",
                 "Text": "DISCONNECT", "Uuid": uid()},
            ]},
            {"$Name": "lblBluetooth", "$Type": "Label", "$Version": "5",
             "Text": "Bluetooth: Not connected", "FontBold": "True", "TextColor": "&HFFFF0000",
             "Uuid": uid()},
            {"$Name": "TableArrangement1", "$Type": "TableArrangement", "$Version": "1",
             "Columns": "2", "Rows": str((len(BUTTONS) + 1) // 2),
             "Width": "-2", "Uuid": uid(), "$Components": cells},
            {"$Name": "lblStatus", "$Type": "Label", "$Version": "5",
             "Text": "Ready", "Uuid": uid()},
            {"$Name": "lblState", "$Type": "Label", "$Version": "5",
             "Text": "Loading database...", "FontSize": "16", "HasMargins": "True", "Uuid": uid()},
            {"$Name": "Web1", "$Type": "Web", "$Version": "4", "Uuid": uid()},
            {"$Name": "Web2", "$Type": "Web", "$Version": "4", "Uuid": uid()},
            {"$Name": "Clock1", "$Type": "Clock", "$Version": "4", "TimerInterval": "1500", "Uuid": uid()},
            {"$Name": "BluetoothClient1", "$Type": "BluetoothClient", "$Version": "4", "Uuid": uid()},
        ],
    },
}
scm = "#|\n$JSON\n" + json.dumps(form) + "\n|#\n"

# ---------- block helpers ----------
def text(s): return f'<block type="text" id="{uid()}"><field name="TEXT">{s}</field></block>'
def num(n): return f'<block type="math_number" id="{uid()}"><field name="NUM">{n}</field></block>'
def gget(name): return f'<block type="lexical_variable_get" id="{uid()}"><field name="VAR">global {name}</field></block>'
def pget(name): return f'<block type="lexical_variable_get" id="{uid()}"><field name="VAR">{name}</field></block>'
def eget(name):
    return (f'<block type="lexical_variable_get" id="{uid()}"><mutation><eventparam name="{name}"></eventparam></mutation>'
            f'<field name="VAR">{name}</field></block>')
def join(*parts):
    vals = "".join(f'<value name="ADD{i}">{p}</value>' for i, p in enumerate(parts))
    return f'<block type="text_join" id="{uid()}"><mutation items="{len(parts)}"></mutation>{vals}</block>'
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
def global_decl(name, value, y):
    return (f'<block type="global_declaration" id="{uid()}" x="20" y="{y}"><field name="NAME">{name}</field>'
            f'<value name="VALUE">{value}</value></block>\n')

# initialize global FIREBASE_URL / SECRET (SECRET empty = no auth, fine for test-mode rules)
blocks = global_decl("FIREBASE_URL", text(FIREBASE_URL), 20)
blocks += global_decl("SECRET", text(""), 70)

# to sendValue tag value:
#   set Web1.Url to FIREBASE_URL + tag + ".json" (+ "?auth=" + SECRET when SECRET is set)
#   call Web1.PutText value
def auth_url(path_expr):
    """FIREBASE_URL + path + ".json" (+ "?auth=" + SECRET when SECRET is set)"""
    return (f'<block type="controls_choose" id="{uid()}">'
            f'<value name="TEST"><block type="text_isEmpty" id="{uid()}"><value name="VALUE">{gget("SECRET")}</value></block></value>'
            f'<value name="THENRETURN">{join(gget("FIREBASE_URL"), path_expr, text(".json"))}</value>'
            f'<value name="ELSERETURN">{join(gget("FIREBASE_URL"), path_expr, text(".json?auth="), gget("SECRET"))}</value>'
            f'</block>')

def proc(name, args, body, y):
    muts = "".join(f'<arg name="{a}"></arg>' for a in args)
    fields = "".join(f'<field name="VAR{i}">{a}</field>' for i, a in enumerate(args))
    return (f'<block type="procedures_defnoreturn" id="{uid()}" x="20" y="{y}">'
            f'<mutation>{muts}</mutation><field name="NAME">{name}</field>{fields}'
            f'<statement name="STACK">{body}</statement></block>\n')

def call_proc(name, args, values, nxt=""):
    muts = "".join(f'<arg name="{a}"></arg>' for a in args)
    vals = "".join(f'<value name="ARG{i}">{v}</value>' for i, v in enumerate(values))
    nxt = f"<next>{nxt}</next>" if nxt else ""
    return (f'<block type="procedures_callnoreturn" id="{uid()}">'
            f'<mutation name="{name}">{muts}</mutation><field name="PROCNAME">{name}</field>{vals}{nxt}</block>')

def if_only(test, then, nxt=""):
    nxt = f"<next>{nxt}</next>" if nxt else ""
    return (f'<block type="controls_if" id="{uid()}"><value name="IF0">{test}</value>'
            f'<statement name="DO0">{then}</statement>{nxt}</block>')

def choose(test, a, b):
    return (f'<block type="controls_choose" id="{uid()}"><value name="TEST">{test}</value>'
            f'<value name="THENRETURN">{a}</value><value name="ELSERETURN">{b}</value></block>')

def compare(op, a, b):
    return (f'<block type="logic_compare" id="{uid()}"><field name="OP">{op}</field>'
            f'<value name="A">{a}</value><value name="B">{b}</value></block>')

def lookup(key):
    return (f'<block type="lists_lookup_in_pairs" id="{uid()}">'
            f'<value name="KEY">{text(key)}</value><value name="LIST">{gget("state")}</value>'
            f'<value name="NOTFOUND">{text("-")}</value></block>')

bt_connected = get_prop("BluetoothClient", "BluetoothClient1", "IsConnected")
blocks += global_decl("state", '<block type="lists_create_with" id="' + uid() + '"><mutation items="0"></mutation></block>', 100)

# to saveOnline tag value: PUT value to Firebase (Web1)
blocks += proc("saveOnline", ["tag", "value"],
               set_prop("Web", "Web1", "Url", auth_url(pget("tag")),
                        call("Web", "Web1", "PutText", pget("value"))), 150)

# to sendValue tag value:
#   if Bluetooth connected -> send "TAG:VALUE;\n" to the ESP32 (";" and newline so
#                             older ESP32 code that waits for a newline works too)
#   always                 -> save to Firebase (so database, Bluetooth and online stay in sync)
blocks += proc("sendValue", ["tag", "value"],
               if_only(bt_connected,
                       call("BluetoothClient", "BluetoothClient1", "SendText",
                            join(pget("tag"), text(":"), pget("value"), text(";\\n"))),
                       call_proc("saveOnline", ["tag", "value"], [pget("tag"), pget("value")],
                                 set_prop("Label", "lblStatus", "Text",
                                          join(text("Sending "), pget("tag"), text(" = "), pget("value"),
                                               choose(bt_connected, text(" (Bluetooth + Online)"), text(" (Online)")))))),
               260)

# --- Bluetooth: scan (paired devices), connect, disconnect ---
blocks += event("ListPicker", "lpBluetooth", "BeforePicking",
                set_prop("ListPicker", "lpBluetooth", "Elements",
                         get_prop("BluetoothClient", "BluetoothClient1", "AddressesAndNames")), 20, 480)
blocks += event("ListPicker", "lpBluetooth", "AfterPicking",
                if_else(call("BluetoothClient", "BluetoothClient1", "Connect",
                             get_prop("ListPicker", "lpBluetooth", "Selection")),
                        set_prop("Label", "lblBluetooth", "Text", text("Bluetooth: Connected"),
                                 set_prop("Label", "lblBluetooth", "TextColor",
                                          '<block type="color_green" id="' + uid() + '"><field name="COLOR">#00ff00</field></block>')),
                        set_prop("Label", "lblBluetooth", "Text", text("Bluetooth: Connection failed"))), 20, 560)
blocks += event("Button", "btnDisconnect", "Click",
                call("BluetoothClient", "BluetoothClient1", "Disconnect",
                     nxt=set_prop("Label", "lblBluetooth", "Text", text("Bluetooth: Not connected"),
                                  set_prop("Label", "lblBluetooth", "TextColor",
                                           '<block type="color_red" id="' + uid() + '"><field name="COLOR">#ff0000</field></block>'))),
                20, 720)

# when Web1.GotText: confirm the save, or show the error from Firebase
blocks += event("Web", "Web1", "GotText",
                if_else(compare("EQ", eget("responseCode"), num(200)),
                        set_prop("Label", "lblStatus", "Text", join(text("Saved online: "), eget("responseContent"))),
                        set_prop("Label", "lblStatus", "Text",
                                 join(text("Online error "), eget("responseCode"), text(": "), eget("responseContent")))),
                20, 800)

# --- Live sync: every 1.5 s read the whole database and show it ---
blocks += event("Clock", "Clock1", "Timer",
                set_prop("Web", "Web2", "Url", auth_url(text("")), call("Web", "Web2", "Get")), 20, 900)
show_state = set_prop("Label", "lblState", "Text", join(
    text("LED1: "), lookup("LED1"), text("   LED2: "), lookup("LED2"),
    text("   LED3: "), lookup("LED3"), text("   LED4: "), lookup("LED4"),
    text("\\nALL_LED: "), lookup("ALL_LED"), text("   SERVO: "), lookup("SERVO")))
blocks += event("Web", "Web2", "GotText",
                if_only('<block type="logic_operation" id="' + uid() + '"><field name="OP">AND</field>'
                        f'<value name="A">{compare("EQ", eget("responseCode"), num(200))}</value>'
                        f'<value name="B">{compare("NEQ", eget("responseContent"), text("null"))}</value></block>',
                        f'<block type="lexical_variable_set" id="{uid()}"><field name="VAR">global state</field>'
                        f'<value name="VALUE">{call("Web", "Web2", "JsonTextDecode", eget("responseContent"))}</value>'
                        f'<next>{show_state}</next></block>'),
                20, 1000)

# when <button>.Click: call sendValue "<tag>" <value>
# ALL LED buttons also save LED1..LED4 online, so the database matches at once.
for i, (name, _, tag, value) in enumerate(BUTTONS):
    body = call_proc("sendValue", ["tag", "value"], [text(tag), num(value)])
    if tag == "ALL_LED":
        chain = ""
        for n in (4, 3, 2, 1):
            chain = call_proc("saveOnline", ["tag", "value"], [text(f"LED{n}"), num(value)], chain)
        body = call_proc("sendValue", ["tag", "value"], [text(tag), num(value)], chain)
    blocks += event("Button", name, "Click", body, 700, 20 + i * 110)

bky = ('<xml xmlns="http://www.w3.org/1999/xhtml">\n' + blocks
       + '<yacodeblocks ya-version="208" language-version="33"></yacodeblocks>\n</xml>\n')

props = """main=appinventor.ai_student.Activity7.Screen1
name=Activity7
assets=../assets
source=../src
build=../build
versioncode=1
versionname=1.0
useslocation=False
aname=Activity7
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

out = os.path.join(here, "Activity7.aia")
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("youngandroidproject/project.properties", props)
    z.writestr(f"src/{PKG}/Screen1.scm", scm)
    z.writestr(f"src/{PKG}/Screen1.bky", bky)
print("wrote", out)
