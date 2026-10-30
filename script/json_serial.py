"""
Envia um programa ladder (JSON) para o ESP32 no formato <START>json<END>
e mostra tudo que o ESP32 responder.

Uso:
    python script/json_serial.py script/json/t01_contatos_no_nc.json
    python script/json_serial.py programa.json -p COM8

Opções para testes de robustez:
    --sem-end   não envia <END> (deve gerar timeout de recepção)
    --lixo N    envia N bytes de lixo antes do <START>

Obs.: abrir a porta serial reinicia o ESP32 (DTR/RTS), por isso o script
espera 2 s antes de enviar. Feche o monitor serial antes de rodar.
"""
import argparse
import json
import time

import serial

BAUD = 115200


def listen(ser, seconds):
    end = time.time() + seconds
    while time.time() < end:
        line = ser.readline().decode(errors="ignore").rstrip()
        if line:
            print(line)


def main():
    ap = argparse.ArgumentParser(description="Envia programa ladder ao ESP32")
    ap.add_argument("arquivo")
    ap.add_argument("-p", "--porta", default="COM8")
    ap.add_argument("--sem-end", action="store_true")
    ap.add_argument("--lixo", type=int, default=0)
    args = ap.parse_args()

    with open(args.arquivo, encoding="utf-8") as f:
        text = f.read()
    try:
        text = json.dumps(json.loads(text), separators=(",", ":"))  # valida e minifica
    except json.JSONDecodeError:
        print("(aviso) JSON invalido no PC - enviando mesmo assim")
        text = text.strip()

    message = "x" * args.lixo + "<START>" + text + ("" if args.sem_end else "<END>")

    with serial.Serial(args.porta, BAUD, timeout=0.2) as ser:
        print("--- boot ---")
        listen(ser, 2)
        print("--- enviando ---")
        ser.write(message.encode())
        listen(ser, 5 if args.sem_end else 2)


if __name__ == "__main__":
    main()
