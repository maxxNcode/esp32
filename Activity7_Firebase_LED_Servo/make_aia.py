"""Builds Activity7.aia - the MIT App Inventor project for Activity 7.
Run:  python3 make_aia.py   -> creates Activity7.aia next to this script.

The app writes to Firebase with the Web component and the Firebase REST API
(PUT <url><tag>.json). This works with every database region, unlike the
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
            {"$Name": "TableArrangement1", "$Type": "TableArrangement", "$Version": "1",
             "Columns": "2", "Rows": str((len(BUTTONS) + 1) // 2),
             "Width": "-2", "Uuid": uid(), "$Components": cells},
            {"$Name": "lblStatus", "$Type": "Label", "$Version": "5",
             "Text": "Ready", "Uuid": uid()},
            {"$Name": "Web1", "$Type": "Web", "$Version": "4", "Uuid": uid()},
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
def global_decl(name, value, y):
    return (f'<block type="global_declaration" id="{uid()}" x="20" y="{y}"><field name="NAME">{name}</field>'
            f'<value name="VALUE">{value}</value></block>\n')

# initialize global FIREBASE_URL / SECRET (SECRET empty = no auth, fine for test-mode rules)
blocks = global_decl("FIREBASE_URL", text(FIREBASE_URL), 20)
blocks += global_decl("SECRET", text(""), 70)

# to sendValue tag value:
#   set Web1.Url to FIREBASE_URL + tag + ".json" (+ "?auth=" + SECRET when SECRET is set)
#   call Web1.PutText value
url_expr = (f'<block type="controls_choose" id="{uid()}">'
            f'<value name="TEST"><block type="text_isEmpty" id="{uid()}"><value name="VALUE">{gget("SECRET")}</value></block></value>'
            f'<value name="THENRETURN">{join(gget("FIREBASE_URL"), pget("tag"), text(".json"))}</value>'
            f'<value name="ELSERETURN">{join(gget("FIREBASE_URL"), pget("tag"), text(".json?auth="), gget("SECRET"))}</value>'
            f'</block>')
put = (f'<block type="component_method" id="{uid()}">'
       f'<mutation component_type="Web" method_name="PutText" is_generic="false" instance_name="Web1"></mutation>'
       f'<field name="COMPONENT_SELECTOR">Web1</field>'
       f'<value name="ARG0">{pget("value")}</value>'
       f'<next>{set_prop("Label", "lblStatus", "Text", join(text("Sending "), pget("tag"), text(" = "), pget("value")))}</next>'
       f'</block>')
blocks += (f'<block type="procedures_defnoreturn" id="{uid()}" x="20" y="130">'
           f'<mutation><arg name="tag"></arg><arg name="value"></arg></mutation>'
           f'<field name="NAME">sendValue</field><field name="VAR0">tag</field><field name="VAR1">value</field>'
           f'<statement name="STACK">{set_prop("Web", "Web1", "Url", url_expr, put)}</statement>'
           f'</block>\n')

# when Web1.GotText: "Saved: x" on 200, otherwise the error from Firebase
blocks += (f'<block type="component_event" id="{uid()}" x="20" y="360">'
           f'<mutation component_type="Web" is_generic="false" instance_name="Web1" event_name="GotText"></mutation>'
           f'<field name="COMPONENT_SELECTOR">Web1</field>'
           f'<statement name="DO"><block type="controls_if" id="{uid()}"><mutation else="1"></mutation>'
           f'<value name="IF0"><block type="logic_compare" id="{uid()}"><field name="OP">EQ</field>'
           f'<value name="A">{eget("responseCode")}</value><value name="B">{num(200)}</value></block></value>'
           f'<statement name="DO0">{set_prop("Label", "lblStatus", "Text", join(text("Saved: "), eget("responseContent")))}</statement>'
           f'<statement name="ELSE">{set_prop("Label", "lblStatus", "Text", join(text("Error "), eget("responseCode"), text(": "), eget("responseContent")))}</statement>'
           f'</block></statement></block>\n')

# when <button>.Click: call sendValue "<tag>" <value>
for i, (name, _, tag, value) in enumerate(BUTTONS):
    blocks += (f'<block type="component_event" id="{uid()}" x="650" y="{20 + i * 90}">'
               f'<mutation component_type="Button" is_generic="false" instance_name="{name}" event_name="Click"></mutation>'
               f'<field name="COMPONENT_SELECTOR">{name}</field>'
               f'<statement name="DO"><block type="procedures_callnoreturn" id="{uid()}">'
               f'<mutation name="sendValue"><arg name="tag"></arg><arg name="value"></arg></mutation>'
               f'<field name="PROCNAME">sendValue</field>'
               f'<value name="ARG0">{text(tag)}</value><value name="ARG1">{num(value)}</value>'
               f'</block></statement></block>\n')

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
