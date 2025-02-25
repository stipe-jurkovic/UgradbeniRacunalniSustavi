# Sustav za Evidenciju Prisustva

Ovaj projekt je **Bluetooth sustav za evidenciju prisustva** koji koristi **ESP32 uređaje** za skeniranje Bluetooth uređaja i **Flask web aplikaciju** za upravljanje studentima i evidencijom dolazaka.

---

## 📂 Struktura Projekta

### 🖥️ ESP32 Kod
- `central.cpp` – Glavni uređaj koji upravlja skeniranjem Bluetooth uređaja, komunicira s web poslužiteljem i vodi evidenciju prisustva.
- `scanner.cpp` – Skenira Bluetooth uređaje i šalje njihove MAC adrese glavnom uređaju.

### 🌐 Web Aplikacija (Server) 
- `app.py` – Flask poslužitelj koji omogućuje registraciju studenata, razreda i evidenciju dolazaka.
- `index.html` – Početna stranica za upravljanje studentima i dodjelu razredima.
- `class.html` – Stranica s evidencijom prisustva za određeni razred.

---
