#!/usr/bin/env python3
"""Farmer quick-reference card, one per language (English, Hindi, Telugu, Kannada).

Command codes come from firmware/lib/core/include/seri/commands.h; stage and language names are printed
by the firmware itself (tools/card_strings.cpp), so the card always matches the device. The short labels
below are new text: they need review by a native speaker of each language before printing (bench item).
Writes docs/handoff/farmer-card.html and farmer-card.pdf (A5 pages, via headless Chromium).
"""
import json
import os
import subprocess
import tempfile

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
FONTS = "/usr/share/fonts/truetype/noto/"

L = {  # en, hi, te, kn
    "title": ["SMS codes", "SMS कोड", "SMS కోడ్‌లు", "SMS ಕೋಡ್‌ಗಳು"],
    "send": ["Send an SMS to the device's number", "डिवाइस के नंबर पर SMS भेजें", "పరికరం నంబర్‌కు SMS పంపండి", "ಸಾಧನದ ಸಂಖ್ಯೆಗೆ SMS ಕಳುಹಿಸಿ"],
    "status": ["Send me the readings now", "अभी तापमान और नमी भेजो", "ఇప్పుడు ఉష్ణోగ్రత, తేమ పంపండి", "ಈಗ ತಾಪಮಾನ, ತೇವಾಂಶ ಕಳುಹಿಸಿ"],
    "stage": ["Change stage", "अवस्था बदलें", "దశ మార్చండి", "ಹಂತ ಬದಲಿಸಿ"],
    "lang": ["My SMS language", "मेरी SMS भाषा", "నా SMS భాష", "ನನ್ನ SMS ಭಾಷೆ"],
    "buzzer": ["Buzzer off / on", "बजर बंद / चालू", "బజర్ ఆపు / ఆన్", "ಬಜರ್ ಆಫ್ / ಆನ್"],
    "add": ["Add a phone number (owner only)", "फोन नंबर जोड़ें (केवल मालिक)", "ఫోన్ నంబర్ చేర్చండి (యజమాని మాత్రమే)", "ಫೋನ್ ಸಂಖ್ಯೆ ಸೇರಿಸಿ (ಮಾಲೀಕರು ಮಾತ್ರ)"],
    "remove": ["Remove number 1 or 2 (owner only)", "नंबर 1 या 2 हटाएँ (केवल मालिक)", "నంబర్ 1 లేదా 2 తొలగించండి (యజమాని మాత్రమే)", "ಸಂಖ್ಯೆ 1 ಅಥವಾ 2 ತೆಗೆದುಹಾಕಿ (ಮಾಲೀಕರು ಮಾತ್ರ)"],
    "owner": ["Become the owner (PIN on the label)", "मालिक बनें (PIN लेबल पर)", "యజమాని అవ్వండి (PIN లేబుల్‌పై)", "ಮಾಲೀಕರಾಗಿ (PIN ಲೇಬಲ್‌ನಲ್ಲಿ)"],
    "green": ["Green light: all OK", "हरी बत्ती: सब ठीक", "ఆకుపచ్చ దీపం: అంతా బాగుంది", "ಹಸಿರು ದೀಪ: ಎಲ್ಲಾ ಸರಿ"],
    "red": ["Red light: problem, see the screen or SMS", "लाल बत्ती: समस्या, स्क्रीन या SMS देखें",
            "ఎరుపు దీపం: సమస్య, స్క్రీన్ లేదా SMS చూడండి", "ಕೆಂಪು ದೀಪ: ಸಮಸ್ಯೆ, ಪರದೆ ಅಥವಾ SMS ನೋಡಿ"],
    "beep": ["Beeping: press the middle button to stop", "बीप: रोकने के लिए बीच का बटन दबाएँ",
             "బీప్: ఆపడానికి మధ్య బటన్ నొక్కండి", "ಬೀಪ್: ನಿಲ್ಲಿಸಲು ಮಧ್ಯದ ಬಟನ್ ಒತ್ತಿ"],
}
FONT = ["Noto Sans", "Noto Sans Devanagari", "Noto Sans Telugu", "Noto Sans Kannada"]


def firmware_strings():
    exe = os.path.join(tempfile.mkdtemp(), "card_strings")
    inc = os.path.join(ROOT, "firmware", "lib", "core")
    srcs = [os.path.join(inc, "src", f) for f in sorted(os.listdir(os.path.join(inc, "src"))) if f.endswith(".cpp")]
    subprocess.run(["g++", "-std=gnu++17", "-I" + os.path.join(inc, "include"), os.path.join(ROOT, "tools", "card_strings.cpp")]
                   + srcs + ["-o", exe], check=True)
    return json.loads(subprocess.run([exe], capture_output=True, text=True, check=True).stdout)


def card(i, fw):
    t = lambda k: L[k][i]
    stages = "<br>".join(f"<b>11 {n + 1}</b> = {fw['stages'][n][i]}" for n in range(len(fw["stages"])))
    langs = "<br>".join(f"<b>12 {n + 1}</b> = {fw['languages'][n]}" for n in range(len(fw["languages"])))
    rows = [("20", t("status")), ("11 &#9633;", t("stage") + "<div class=sub>" + stages + "</div>"),
            ("12 &#9633;", t("lang") + "<div class=sub>" + langs + "</div>"), ("41 0 / 41 1", t("buzzer")),
            ("21 &#9633;&#9633;&#9633;", t("add")), ("22 1 / 22 2", t("remove")), ("10 PIN", t("owner"))]
    body = "".join(f"<tr><td class=code>{c}</td><td>{d}</td></tr>" for c, d in rows)
    return (f"<section style=\"font-family:'{FONT[i]}','Noto Sans'\"><h1>{t('title')}</h1><p class=send>{t('send')}</p>"
            f"<table>{body}</table><p class=led><span class=g>&#9679;</span> {t('green')}<br>"
            f"<span class=r>&#9679;</span> {t('red')}<br>&#128276; {t('beep')}</p></section>")


def main():
    fw = firmware_strings()
    faces = "".join(f"@font-face{{font-family:'{n}';src:url('file://{FONTS}{f}-Regular.ttf')}}"
                    f"@font-face{{font-family:'{n}';font-weight:bold;src:url('file://{FONTS}{f}-Bold.ttf')}}"
                    for n, f in zip(FONT, ["NotoSans", "NotoSansDevanagari", "NotoSansTelugu", "NotoSansKannada"]))
    css = (faces + "@page{size:A5;margin:8mm}body{margin:0}section{page-break-after:always;font-size:12pt}"
           "h1{font-size:16pt;margin:0 0 2mm}p.send{margin:0 0 2mm}table{border-collapse:collapse;width:100%}"
           "td{border-top:1px solid #999;padding:1.2mm 1mm;vertical-align:top}td.code{font-family:'Noto Sans';"
           "font-weight:bold;font-size:12pt;white-space:nowrap;width:30mm}.sub{font-size:9.5pt;color:#333}"
           ".led{margin-top:2mm}.g{color:#090}.r{color:#c00}")
    html = f"<!doctype html><html><head><meta charset=utf-8><title>Seri-tool farmer card</title><style>{css}</style></head><body>" \
           + "".join(card(i, fw) for i in range(4)) + "</body></html>"
    out = os.path.join(ROOT, "docs", "handoff")
    os.makedirs(out, exist_ok=True)
    hp = os.path.abspath(os.path.join(out, "farmer-card.html"))
    open(hp, "w", encoding="utf-8").write(html)
    js = ("const {chromium}=require('playwright');(async()=>{const b=await chromium.launch();const p=await b.newPage();"
          f"await p.goto('file://{hp}');await p.waitForTimeout(500);await p.pdf({{path:'{hp[:-5]}.pdf',format:'A5',printBackground:true}});"
          f"await p.setViewportSize({{width:420,height:600}});await p.screenshot({{path:'{hp[:-5]}-preview.png',fullPage:true}});await b.close();}})();")
    subprocess.run(["node", "-e", js], check=True)
    print("farmer card: docs/handoff/farmer-card.{html,pdf}")


if __name__ == "__main__":
    main()
