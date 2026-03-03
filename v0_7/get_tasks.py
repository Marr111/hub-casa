import urllib.request

# Calendario degli eventi principali
MAIN_URL = "https://calendar.google.com/calendar/ical/casettamatteo1%40gmail.com/public/basic.ics"
# Calendario dedicato ai Task (quello collegato allo script Google Apps Script)
TASK_URL = "https://calendar.google.com/calendar/ical/9ac14c3d4297d6ad64b570997cef84bfc32ef31aa598ec653d26d7edcac4b5a7%40group.calendar.google.com/public/basic.ics"

def get_ical(url):
    req = urllib.request.Request(url, headers={'Accept-Encoding': 'identity'})
    with urllib.request.urlopen(req) as response:
        return response.read().decode('utf-8', errors='ignore')

def parse_tasks_vtodo(data):
    """Cerca blocchi VTODO (vecchio formato)"""
    tasks = []
    in_task = False
    is_completed = False
    task_summary = "Task senza nome"

    for line in data.split('\n'):
        line = line.strip()
        if line == "BEGIN:VTODO":
            in_task = True
            is_completed = False
            task_summary = "Task senza nome"
        elif line == "END:VTODO":
            if in_task and not is_completed:
                tasks.append(task_summary)
            in_task = False
        elif in_task:
            if line.startswith("STATUS:COMPLETED"):
                is_completed = True
            elif line.startswith("SUMMARY:"):
                task_summary = line[8:].strip().replace("\\,", ",").replace("\\;", ";")
    return tasks

def parse_tasks_vevent_emoji(data):
    """Cerca VEVENT con emoji 🎯 (creati dallo script di Google Apps Script)"""
    tasks = []
    in_event = False
    summary = ""

    for line in data.split('\n'):
        line = line.strip()
        if line == "BEGIN:VEVENT":
            in_event = True
            summary = ""
        elif line == "END:VEVENT":
            if in_event and summary.startswith("🎯"):
                # Rimuovi l'emoji e il trattino/spazio davanti
                clean = summary.replace("🎯", "").strip()
                tasks.append(clean)
            in_event = False
        elif in_event and line.startswith("SUMMARY:"):
            summary = line[8:].strip().replace("\\,", ",")
    return tasks

print("=" * 50)
print(f"Scaricando da calendario TASK: {TASK_URL}\n")

try:
    data = get_ical(TASK_URL)

    vtodo_tasks = parse_tasks_vtodo(data)
    emoji_tasks = parse_tasks_vevent_emoji(data)

    if vtodo_tasks:
        print(f"Trovati {len(vtodo_tasks)} task in formato VTODO:")
        for t in vtodo_tasks:
            print(f"   👉 {t}")
    else:
        print("Nessun blocco VTODO trovato nel calendario task.")

    if emoji_tasks:
        print(f"\nTrovati {len(emoji_tasks)} task in formato VEVENT+emoji (script Google):")
        for t in emoji_tasks:
            print(f"   target {t}")
    else:
        print("Nessun VEVENT con emoji trovato nel calendario task.")

    if not vtodo_tasks and not emoji_tasks:
        print("\nCalendario Task vuoto o script Google Apps Script non eseguito.")
        print("Prime 40 righe del file iCal scaricato:")
        righe = [l.strip() for l in data.split('\n') if l.strip() and not l.strip().startswith('X-')]
        for l in righe[:40]:
            print(f"  {l}")

except Exception as e:
    print(f"Errore: {e}")

