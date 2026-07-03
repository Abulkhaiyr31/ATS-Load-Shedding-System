import serial
import sqlite3
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime
import threading

# ── Настройки ──────────────────────────────────────────
PORT = 'COM4'
BAUD = 115200
DB   = 'ats_log.db'

# ── База данных ─────────────────────────────────────────
conn = sqlite3.connect(DB, check_same_thread=False)
c = conn.cursor()
c.execute('''CREATE TABLE IF NOT EXISTS logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ts INTEGER, source TEXT,
    vGrid REAL, vBat REAL,
    iLoad REAL, loads TEXT,
    time TEXT)''')
conn.commit()

# ── Данные для графика ──────────────────────────────────
times, vGrids, vBats, iLoads = [], [], [], []

def read_serial():
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        print(f"Подключено к {PORT}")
        while True:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line.startswith('#') or line.startswith('ts') or line == '' or ',' not in line:
                continue
            parts = line.split(',')
            if len(parts) != 6:
                continue
            try:
                ts, source, vGrid, vBat, iLoad, loads = parts
                now = datetime.now().strftime('%H:%M:%S')
                c.execute('INSERT INTO logs VALUES (NULL,?,?,?,?,?,?,?)',
                          (ts, source, float(vGrid), float(vBat), float(iLoad), loads, now))
                conn.commit()
                times.append(now)
                vGrids.append(float(vGrid))
                vBats.append(float(vBat))
                iLoads.append(float(iLoad))
                print(f"{now} | {source} | Grid:{vGrid}V | Bat:{vBat}V | I:{iLoad}A | {loads}")
                if len(times) > 60:
                    times.pop(0); vGrids.pop(0)
                    vBats.pop(0); iLoads.pop(0)
            except:
                continue
    except Exception as e:
        print(f"Ошибка: {e}")

# ── График ──────────────────────────────────────────────
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))
fig.suptitle('ATS Dashboard — Real Time Monitor', fontsize=14)

def animate(i):
    if len(times) < 2:
        return
    ax1.clear()
    ax2.clear()
    ax1.plot(times[-20:], vGrids[-20:], 'b-o', label='Grid V', markersize=3)
    ax1.plot(times[-20:], vBats[-20:],  'g-o', label='Battery V', markersize=3)
    ax1.axhline(y=9.5,  color='r', linestyle='--', label='Trip 9.5V')
    ax1.axhline(y=10.5, color='orange', linestyle='--', label='Recover 10.5V')
    ax1.set_ylabel('Voltage (V)')
    ax1.set_ylim(8, 13)
    ax1.legend(loc='upper right')
    ax1.tick_params(axis='x', rotation=45)
    ax2.plot(times[-20:], iLoads[-20:], 'r-o', label='Load Current A', markersize=3)
    ax2.set_ylabel('Current (A)')
    ax2.set_xlabel('Time')
    ax2.legend(loc='upper right')
    ax2.tick_params(axis='x', rotation=45)
    plt.tight_layout()

threading.Thread(target=read_serial, daemon=True).start()
ani = animation.FuncAnimation(fig, animate, interval=1000)
plt.show()