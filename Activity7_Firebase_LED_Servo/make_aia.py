"""Builds Activity7.aia - the MIT App Inventor project for Activity 7.
Run:  python3 make_aia.py   -> creates Activity7.aia next to this script."""
import json, os, random, zipfile

PKG = "appinventor/ai_student/Activity7"
here = os.path.dirname(os.path.abspath(__file__))

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
for i, (name, text, _, _) in enumerate(BUTTONS):
    cells.append({"$Name": name, "$Type": "Button", "$Version": "7",
                  "Column": str(i % 2), "Row": str(i // 2),
                  "Text": text, "Width": "-1050", "Uuid": uid()})

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
            {"$Name": "FirebaseDB1", "$Type": "FirebaseDB", "$Version": "3",
             "FirebaseURL": "https://YOUR-PROJECT-default-rtdb.firebaseio.com/",
             "FirebaseToken": "PASTE_DATABASE_SECRET_HERE",
             "ProjectBucket": "", "Uuid": uid()},
        ],
    },
}
scm = "#|\n$JSON\n" + json.dumps(form) + "\n|#\n"

def click_block(i, name, text, tag, value):
    return f'''  <block type="component_event" id="{uid()}" x="20" y="{20 + i * 150}">
    <mutation component_type="Button" is_generic="false" instance_name="{name}" event_name="Click"></mutation>
    <field name="COMPONENT_SELECTOR">{name}</field>
    <statement name="DO">
      <block type="component_method" id="{uid()}">
        <mutation component_type="FirebaseDB" method_name="StoreValue" is_generic="false" instance_name="FirebaseDB1"></mutation>
        <field name="COMPONENT_SELECTOR">FirebaseDB1</field>
        <value name="ARG0"><block type="text" id="{uid()}"><field name="TEXT">{tag}</field></block></value>
        <value name="ARG1"><block type="math_number" id="{uid()}"><field name="NUM">{value}</field></block></value>
        <next>
          <block type="component_set_get" id="{uid()}">
            <mutation component_type="Label" set_or_get="set" property_name="Text" is_generic="false" instance_name="lblStatus"></mutation>
            <field name="COMPONENT_SELECTOR">lblStatus</field>
            <field name="PROP">Text</field>
            <value name="VALUE"><block type="text" id="{uid()}"><field name="TEXT">Sent: {text}</field></block></value>
          </block>
        </next>
      </block>
    </statement>
  </block>
'''

bky = ('<xml xmlns="http://www.w3.org/1999/xhtml">\n'
       + "".join(click_block(i, *b) for i, b in enumerate(BUTTONS))
       + '  <yacodeblocks ya-version="208" language-version="33"></yacodeblocks>\n</xml>\n')

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
