"""
Teste de corrida: alterna dois programas N vezes na MESMA sessão serial
enquanto o PLC está rodando, e confere se o ESP32 nunca reiniciou.

Uso:
    python script/stress_upload.py script/payloads/t12_pisca.json script/payloads/t06_ton.json 50 -p COM8
"""
import argparse
import json
import time

import serial

BAUD = 115200


def load(path):
    with open(path, encoding="utf-8") as f:
        return "<START>" + json.dumps(json.load(f), separators=(",", ":")) + "<END>"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("vezes", type=int, nargs="?", default=50)
    ap.add_argument("-p", "--porta", default="COM8")
    args = ap.parse_args()

    msgs = [load(args.a), load(args.b)]
    log = ""
    with serial.Serial(args.porta, BAUD, timeout=0.1) as ser:
        time.sleep(2)
        ser.reset_input_buffer()          # descarta o log de boot
        for i in range(args.vezes):
            ser.write(msgs[i % 2].encode())
            end = time.time() + 0.8       # espera o blink do upload
            while time.time() < end:
                log += ser.read(4096).decode(errors="ignore")

    ok = log.count("Programa validado!")
    reboot = any(k in log for k in ("Guru Meditation", "Backtrace", "rst:", "Rebooting"))
    print(f"uploads enviados: {args.vezes} | confirmados: {ok} | reinicio detectado: {reboot}")
    print("PASSOU" if ok == args.vezes and not reboot else "FALHOU")


if __name__ == "__main__":
    main()
